/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "rrc/rrc.h"
#include "lib/mac/sched_nr_util.h"
#include "mac/mac.h"


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-mac"

#define WRITE_MIB_SIB_PCAP
#define WRITE_RAR_PCAP

static mac_manager_t mac_manager = {0};

mac_manager_t *mac_manager_self(void)
{
	return &mac_manager;
}

static void mac_manager_init(void)
{
	mac_manager.app_pool = gnb_manager_self()->app_pool;
	oset_apr_mutex_init(&mac_manager.mutex, OSET_MUTEX_NESTED, mac_manager.app_pool);
	oset_apr_thread_cond_create(&mac_manager.cond, mac_manager.app_pool);
	oset_apr_thread_rwlock_create(&mac_manager.rwmutex, mac_manager.app_pool);
}

static void mac_manager_destory(void)
{
	oset_apr_mutex_destroy(mac_manager.mutex);
	oset_apr_thread_cond_destroy(mac_manager.cond);
	oset_apr_thread_rwlock_destroy(mac_manager.rwmutex);

	mac_manager.app_pool = NULL; /*app_pool release by openset process*/
}

static int mac_init(void)
{
	mac_manager_init();
	mac_manager.bcch_bch_payload = byte_buffer_init();
	mac_manager.rar_pdu_buffer = byte_buffer_init();
	sched_nr_init(&mac_manager.sched);

	mac_manager.started = true;
	mac_manager.args = &gnb_manager_self()->args.nr_stack.mac_nr;

	oset_apr_mutex_init(&mac_manager.sched.mutex, OSET_MUTEX_NESTED, mac_manager.app_pool);

	if (mac_manager.args->pcap.enable) {
		oset_apr_mutex_init(&mac_manager.pcap.base.mutex, OSET_MUTEX_NESTED, mac_manager.app_pool);
		mac_manager.pcap.base.queue = oset_ring_queue_create(MAX_MAC_PCAP_QUE_SIZE);
	  	mac_manager.pcap.base.buf = oset_ring_buf_create(MAX_MAC_PCAP_QUE_SIZE, sizeof(pcap_pdu_t));
		mac_pcap_open(&mac_manager.pcap, mac_manager.args->pcap.filename, 0);
	}

	oset_list_init(&mac_manager.mac_ue_list);
	mac_manager.ue_db = oset_hash_make();

	//todo
	return OSET_OK;
}

static int mac_destory(void)
{
	cvector_free(mac_manager.bcch_dlsch_payload);
	cvector_free(mac_manager.detected_rachs);
	cvector_free(mac_manager.cell_config);

	//todo

	mac_remove_ue_all();
	oset_list_empty(&mac_manager.mac_ue_list);
	oset_hash_destroy(mac_manager.ue_db);

	oset_apr_mutex_destroy(mac_manager.sched.mutex);

	if (mac_manager.args->pcap.enable) {
	  	mac_pcap_close(&mac_manager.pcap);
		oset_ring_buf_destroy(mac_manager.pcap.base.buf);
		oset_ring_queue_destroy(mac_manager.pcap.base.queue);
		oset_apr_mutex_destroy(mac_manager.pcap.base.mutex);
	}
	mac_manager.started = false;

    sched_nr_destory(&mac_manager.sched);
	oset_free(mac_manager.bcch_bch_payload);
	oset_free(mac_manager.rar_pdu_buffer);
	mac_manager_destory();
	return OSET_OK;
}
static bool is_rnti_valid(uint16_t rnti)
{
	if (!mac_manager.started) {
		oset_error("RACH ignored as eNB is being shutdown");
		return false;
	}
	if (oset_list_count(&mac_manager.mac_ue_list) > SRSENB_MAX_UES) {
		oset_error("Maximum number of connected UEs %zd connected to the eNB. Ignoring PRACH", SRSENB_MAX_UES);
		return false;
	}
	if (ue_nr_find_by_rnti(rnti)) {
		oset_error("Failed to allocate rnti=0x%x. Attempting a different rnti", rnti);
		return false;
	}
	return true;
}

static uint16_t mac_alloc_ue(uint32_t enb_cc_idx)
{
	ue_nr*   inserted_ue = NULL;
	uint16_t rnti        = SRSRAN_INVALID_RNTI;

	do {
		// Assign new RNTI
		//oset_apr_mutex_lock(mac_manager.mutex);
		rnti = (uint16_t)FIRST_RNTI + (mac_manager.ue_counter++) % 60000);
		//oset_apr_mutex_unlock(mac_manager.mutex);

		// Pre-check if rnti is valid
		{
		  //srsran::rwlock_read_guard read_lock(rwmutex);
		  if (! is_rnti_valid(rnti)) {
		    continue;
		  }
		}

		// Allocate and initialize UE object
		//srsran::rwlock_write_guard rw_lock(rwmutex);
		inserted_ue = ue_nr_add(rnti);
		if(NULL == inserted_ue){
		    oset_error("Failed to allocate rnti=0x%x. Attempting a different rnti.", rnti);
		}

	} while (inserted_ue == NULL);

	return rnti;
}

void mac_remove_ue(uint16_t rnti)
{
	//srsran::rwlock_write_guard lock(rwmutex);
	if (is_rnti_valid(rnti)) {
	  sched_nr_ue_rem(rnti);
	  ue_nr_remove(ue_nr_find_by_rnti(rnti));
	} else {
	  oset_error("User rnti=0x%x not found", rnti);
	}
}

void mac_remove_ue_all(void)
{
	ue_nr *ue = NULL, *next_ue = NULL;
	oset_list_for_each_safe(&mac_manager.mac_ue_list, next_ue, ue)
		mac_remove_ue(ue);
}

static void get_metrics_nolock(mac_metrics_t *metrics)
{
	//srsran::rwlock_read_guard lock(rwmutex);
	ue_nr *u = NULL, *next_u = NULL;
	oset_list_for_each_safe(&mac_manager.mac_ue_list, next_u, u){
		mac_ue_metrics_t u_metric = {0};
		ue_nr_metrics_read(&u_metric);
		cvector_push_back(metrics->ues, u_metric)
	}

	for (uint8_t cc = 0; cc < cvector_size(mac_manager.cell_config); ++cc) {
		mac_cc_info_t cc_info = {0};
		cc_info.cc_rach_counter = mac_manager.detected_rachs[cc];
		cc_info.pci             = mac_manager.cell_config[cc].pci;
		cvector_push_back(metrics->cc_info, cc_info)
	}
}

/// Called from metrics thread.
/// Note: This can contend for the same mutexes as the ones used by L1/L2 workers.
///       However, get_metrics is called infrequently enough to cause major halts in the L1/L2
void mac_get_metrics(mac_metrics_t *metrics)
{
	// TODO: We should comment on the logic we follow to get the metrics. Some of them are retrieved from MAC, some
	// others from the scheduler.
	get_metrics_nolock(metrics);
	sched_nr_get_metrics(&mac_manager.sched.metrics_handler, metrics);
}

static byte_buffer_t* mac_assemble_rar(cvector_vector_t(msg3_grant_t) grants, uint32_t tbs_len)
{
	mac_rar_pdu_nr rar_pdu = {0};

	//uint32_t pdsch_tbs = 10;   // FIXME: how big is the PDSCH?

	// msg2 prb = 4 ===> 4*(12*(14-2)*2*379/1024*1) = 426bit/8 = 53byte
	uint32_t pdsch_tbs = tbs_len;
	mac_rar_pdu_nr_init_tx(&rar_pdu, mac_manager.rar_pdu_buffer, pdsch_tbs);

	msg3_grant_t *rar_grant = NULL;
	cvector_for_each_in(rar_grant, grants){
		mac_rar_subpdu_nr rar_subpdu = {0};

		// set values directly coming from scheduler
		rar_subpdu->type = (rar_subh_type_t)RAPID;
		rar_subpdu->payload_length = MAC_RAR_NBYTES;
		rar_subpdu->header_length = 1;
		rar_subpdu->E_bit = true; // not last
		rar_subpdu->rapid = rar_grant->data.preamble_idx;
		rar_subpdu->ta = rar_grant->data.ta_cmd;
		rar_subpdu->temp_crnti = rar_grant->data.temp_crnti;

		// convert Msg3 grant to raw UL grant
		// msg3上行授权调度， ul_grant包含在mac rar中(27 bit)
		srsran_dci_nr_t     dci     = {0};// 入参 rar不需要此入参
		srsran_dci_msg_nr_t dci_msg = {0};// 出参
		if (srsran_dci_nr_ul_pack(&dci, &rar_grant->msg3_dci, &dci_msg) != SRSRAN_SUCCESS) {
			oset_error("Couldn't pack Msg3 UL grant");
			return NULL;
		}

		char str[512] = {0};
		srsran_dci_ul_nr_to_str(&dci, &rar_grant->msg3_dci, str, sizeof(str));
		oset_debug("Setting RAR Grant %s", str);

		// copy only the required bits
		memcpy(rar_subpdu->ul_grant, dci_msg->payload, SRSRAN_RAR_UL_GRANT_NBITS);

		cvector_push_back(rar_pdu->subpdus, rar_subpdu);
	}

	if (mac_rar_pdu_nr_pack(&rar_pdu) != OSET_OK) {
		oset_error("Couldn't assemble RAR PDU");
		return NULL;
	}

	mac_rar_pdu_nr_to_string(&rar_pdu);

	cvector_free(rar_pdu->subpdus);

	return mac_manager.rar_pdu_buffer;
}

static ul_sched_t* mac_get_ul_sched(srsran_slot_cfg_t* slot_cfg, uint32_t cc_idx)
{
	slot_point pusch_slot = {0};
	slot_point_init(&pusch_slot, NUMEROLOGY_IDX, slot_cfg->idx);

	ul_sched_t* ul_sched = sched_nr_get_ul_sched(&mac_manager.sched, pusch_slot, cc_idx);

	// srsran::rwlock_read_guard rw_lock(rwmutex);
	pusch_t **sch_node = NULL;
	cvector_for_each_in(sch_node, ul_sched->pusch){
		pusch_t *pusch = *sch_node;
		ue_nr *mac_ue = ue_nr_find_by_rnti(pusch->sch.grant.rnti);
		oset_assert(mac_ue);
		ue_nr_metrics_ul_mcs(mac_ue, pusch.sch.grant.tb[0].mcs);
	}
	return ul_sched;
}


static dl_sched_t* mac_get_dl_sched(srsran_slot_cfg_t *slot_cfg, uint32_t cc_idx)
{
	slot_point pdsch_slot = {0};
	slot_point_init(&pdsch_slot, NUMEROLOGY_IDX, slot_cfg->idx);

	// Initiate new slot and sync UE internal states
	sched_nr_slot_indication(&mac_manager.sched, pdsch_slot);

	// Run DL Scheduler for CC
	// todp cell=0
	dl_res_t* dl_res = sched_nr_get_dl_sched(&mac_manager.sched, pdsch_slot, cc_idx);
		if (NULL == dl_res) {
		return NULL;
	}

	ssb_t *ssb = NULL;
	cvector_for_each_in(ssb, dl_res->phy.ssb){
#ifdef WRITE_MIB_SIB_PCAP
		if (mac_manager.pcap.pcap_file != NULL) {
			mac_pcap_write_dl_bch_nr(&mac_manager.pcap, 
									  ssb->pbch_msg.payload,
									  sizeof(ssb->pbch_msg.payload),
									  NO_RNTI,
									  0,
									  slot_cfg->idx);
		}
#endif
	}

	// Generate MAC DL PDUs
	uint32_t rar_count = 0, si_count = 0, data_count = 0;
	//srsran::rwlock_read_guard rw_lock(rwmutex);
	pdsch_t **sch_node = NULL;
	cvector_for_each_in(sch_node, dl_res->phy.pdsch){
		pdsch_t *pdsch = *sch_node;
		if (pdsch->sch.grant.rnti_type == srsran_rnti_type_c) {
		  uint16_t rnti = pdsch->sch.grant.rnti;
		  if (!is_rnti_valid(rnti)) {
		    continue;
		  }
		  for (int i = 0; i < SRSRAN_MAX_TB; ++i) {
		  	//RLC PDU
		  	//pdsch->data[0] = slot_u->h_dl->pdu;
		  	byte_buffer_t *tb_data = pdsch->data[i];
		    if (tb_data != NULL && tb_data->N_bytes == 0) {
			  // TODO: exclude rlc retx from packing
			  ue_nr *mac_ue = ue_nr_find_by_rnti(rnti);
			  oset_assert(mac_ue);
		      // lcid sdu
		      dl_pdu_t *pdu = dl_res->data[data_count++];
		      ue_nr_generate_pdu(mac_ue, tb_data, pdsch->sch.grant.tb[i].tbs / 8, pdu->subpdus);
		      if (mac_manager.pcap.pcap_file != NULL) {
		      	mac_pcap_write_dl_crnti_nr(&mac_manager.pcap,
									      	tb_data->msg,
									      	tb_data->N_bytes,
									      	rnti,
									      	pdsch->sch.grant.tb[i].pid,
									      	slot_cfg->idx);
		      }
		      ue_nr_metrics_dl_mcs(mac_ue, pdsch->sch.grant.tb[i].mcs);
		    }
		  }
		} else if (pdsch.sch.grant.rnti_type == srsran_rnti_type_ra) {
		  rar_t *rar = dl_res->rar[rar_count++];
		  // for RARs we could actually move the byte_buffer to the PHY, as there are no retx

		  // 如果UE收到了一个BI子头，则会保存一个backoff值，该值等于该subheader的BI值(其为一个backoff的索引值)所对应索引所对应的值；否则UE会将backoff值设为0。
		  // BI(Backoff Indicator)指定了UE重发preamble前需要等待的时间范围(取值范围见38.321.的7.2节)。如果UE在RAR时间窗内没有接收到RAR，或接收到的RAR中没有一个preamble与自己的相符合，
		  // 则认为此次RAR接收失败，此时UE需要等待一段时间后，再次发起随机接入请求。等待的时间为在0至BI值指定的等待时间区间内随机选取一个值。
		  // BI的取值从侧面反映了小区的负载情况，如果接入的UE多，则该值可以设置得大些；如果接入的UE少，该值就可以设置得小一些，这由基站实现所决定。

		  // |MAC subPDU 1(BI only)|MAC subPDU 2(RAPID only)|MAC subPDU 3(RAPID + RAR)|
		  // |E|T|R|R|     BI      |E|T|     RAPID          |E|T|   RAPID|  MAC   RAR |

		  // |E|T|R|R|	 BI  | OCT1   MAC sub header
		  
		  // |E|T	   PAPID | OCT1   MAC sub header
		  // |R|R|R|   TA	 | OCT1
		  // | TA	|UL Grant| OCT2
		  // |	  UL Grant	 | OCT3
		  // |	  UL Grant	 | OCT4
		  // |	  UL Grant	 | OCT5
		  // |	  TC-RNTI	 | OCT6
		  // |	  TC-RNTI	 | OCT7
		  pdsch.data[0] = mac_assemble_rar(rar->grants, pdsch->sch.grant.tb[0].tbs / 8);
#ifdef WRITE_RAR_PCAP
			if (mac_manager.pcap.pcap_file != NULL) {
			  	mac_pcap_write_dl_ra_rnti_nr(&mac_manager.pcap,
			  								mac_manager.rar_pdu_buffer.msg,
			  								mac_manager.rar_pdu_buffer.N_bytes
			  								pdsch->sch.grant.rnti,
			  								pdsch->sch.grant.tb[0].pid,
			  								slot_cfg->idx);
			}
#endif
		} else if (pdsch->sch.grant.rnti_type == srsran_rnti_type_si) {
		  uint32_t sib_idx = dl_res->sib_idxs[si_count++];
		  pdsch.data[0]    = mac_manager.bcch_dlsch_payload[sib_idx].payload;
#ifdef WRITE_MIB_SIB_PCAP
		  if (mac_manager.pcap.pcap_file != NULL) {
		    mac_pcap_write_dl_si_rnti_nr(&mac_manager.pcap, 
										  mac_manager.bcch_dlsch_payload[sib_idx].payload->msg,
			                              mac_manager.bcch_dlsch_payload[sib_idx].payload->N_bytes,
			                              SI_RNTI,
			                              0,
			                              slot_cfg->idx);
		  }
#endif
		}
	}

	ue_nr *ue = NULL, *next_ue = NULL;
	oset_list_for_each_safe(&mac_manager.mac_ue_list, next_ue, ue){
		ue_nr_metrics_cnt(ue);
	}

	return &dl_res->phy;
}

static bool mac_handle_uci_data(uint32_t cc_idx, uint16_t rnti, srsran_uci_cfg_nr_t *cfg_, srsran_uci_value_nr_t *value)
{
  // Process HARQ-ACK
  for (uint32_t i = 0; i < cfg_->ack.count; i++) {
    const srsran_harq_ack_bit_t* ack_bit = &cfg_->ack.bits[i];
    bool                         is_ok   = (value->ack[i] == 1) && value->valid;
  	sched_nr_dl_ack_info(&mac_manager.sched, rnti, cc_idx, ack_bit->pid, 0, is_ok);
    //srsran::rwlock_read_guard rw_lock(rwmutex);
	//ue_nr *mac_ue = ue_nr_find_by_rnti(rnti);
	//if(mac_ue){
		/*TODO get size of packet from scheduler somehow*/
	//	 ue_nr_metrics_tx(mac_ue, is_ok, 0);
	//}
  }

  // Process SR
  if (value->valid && value->sr > 0) {
    sched_nr_ul_sr_info(&mac_manager.sched, cfg_->pucch.rnti);
  }

  // Process CQI
  for (uint32_t i = 0; i < cfg_->nof_csi; i++) {
    // Skip if invalid or not supported CSI report
    if (!value->valid || cfg_->csi[i].cfg.quantity != SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI ||
        cfg_->csi[i].cfg.freq_cfg != SRSRAN_CSI_REPORT_FREQ_WIDEBAND || value->csi[i].wideband_cri_ri_pmi_cqi.cqi == 0) {
      continue;
    }

    // 1. Pass CQI report to scheduler
    sched_nr_dl_cqi_info(&mac_manager.sched, rnti, cc_idx, value->csi[i].wideband_cri_ri_pmi_cqi.cqi);

    // 2. Save CQI report for metrics stats
    // srsran::rwlock_read_guard rw_lock(rwmutex);
    ue_nr *mac_ue = ue_nr_find_by_rnti(rnti);
    if(mac_ue && value.valid){
		ue_nr_metrics_dl_cqi(mac_ue, cfg_, value->csi[i].wideband_cri_ri_pmi_cqi.cqi);
	}
  }

  return true;
}


static int mac_pucch_info(pucch_info_t *pucch_info, uint32_t cc_idx)
{
	if (!mac_handle_uci_data(cc_idx, pucch_info->uci_data.cfg.pucch.rnti, &pucch_info->uci_data.cfg, &pucch_info->uci_data.value)) {
		oset_error("Error handling UCI data from PUCCH reception");
		return OSET_ERROR;
	}

	// process PUCCH SNR
	// srsran::rwlock_read_guard rw_lock(rwmutex);
	ue_nr *mac_ue = ue_nr_find_by_rnti(pucch_info->uci_data.cfg.pucch.rnti);
	if(mac_ue){
		ue_nr_metrics_pucch_sinr(mac_ue, pucch_info->csi.snr_dB);
	}
	return OSET_OK;
}

static void mac_handle_pdu_rx(uint32_t tti, uint16_t rnti, byte_buffer_t *pdu)
{
	msg_def_t *msg_ptr = task_alloc_msg(TASK_PHY, PUSCH_MAC_PDU_INFO);
	oset_assert(msg_ptr);

	RQUE_MSG_TTI(msg_ptr) = tti;
	pusch_mac_pdu_info_t  *pusch_pdu_info = &PUSCH_MAC_PDU_INFO(msg_ptr);
	pusch_pdu_info->rnti = rnti;
	pusch_pdu_info->pdu = oset_memdup(pdu, sizeof(byte_buffer_t));
	task_send_msg(TASK_MAC, msg_ptr);
}

static int mac_pusch_info(srsran_slot_cfg_t *slot_cfg, pusch_info_t *pusch_info, uint32_t cc_idx)
{
	uint16_t rnti      = pusch_info->rnti;
	uint32_t nof_bytes = pusch_info->pdu->N_bytes;

	// Handle UCI data
	if (!mac_handle_uci_data(cc_idx, rnti, &pusch_info->uci_cfg, &pusch_info->pusch_data.uci)) {
		oset_error("Error handling UCI data from PUSCH reception");
		return OSET_ERROR;
	}

	sched_nr_ul_crc_info(&mac_manager.sched, rnti, cc_idx, pusch_info->pid, pusch_info->pusch_data.tb[0].crc);

	// process only PDUs with CRC=OK
	if (pusch_info->pusch_data.tb[0].crc) {
		if (mac_manager.pcap.pcap_file) {
			mac_pcap_write_ul_crnti_nr(&mac_manager.pcap,
										pusch_info->pdu->msg,
										pusch_info->pdu->N_bytes,
										pusch_info->rnti,
										pusch_info->pid,
										slot_cfg->idx);
		}

		// Decode and send PDU to upper layers
		mac_handle_pdu_rx(rnti, pusch_info->pdu);
	}
	// srsran::rwlock_read_guard rw_lock(rwmutex);
	ue_nr *mac_ue = ue_nr_find_by_rnti(rnti);
	if(mac_ue){
	  ue_nr_metrics_rx(mac_ue, pusch_info->pusch_data.tb[0].crc, nof_bytes); // 统计误码率
	  ue_nr_metrics_pusch_sinr(mac_ue, pusch_info->csi.snr_dB); // 统计信噪比
	}
  return OSET_OK;
}

static void mac_handle_rach_info(rach_info_t *rach_info)
{
	uint16_t rnti = FIRST_RNTI;

	rnti = mac_alloc_ue(rach_info->enb_cc_idx);//alloc rnti
	{
		//srsran::rwlock_write_guard lock(rwmutex);
		//oset_apr_mutex_lock(mac_manager.mutex);
		++mac_manager.detected_rachs[rach_info->enb_cc_idx];
		//oset_apr_mutex_unlock(mac_manager.mutex);
	}
	// Trigger scheduler RACH
	rar_info_t rar_info = {0};
	rar_info.msg3_size    = 7;//???todo
	rar_info.cc			  = rach_info->enb_cc_idx;
	rar_info.preamble_idx = rach_info->preamble;
	rar_info.temp_crnti	  = rnti;
	rar_info.ta_cmd		  = rach_info->time_adv;
	slot_point_init(&rar_info.prach_slot, NUMEROLOGY_IDX, rach_info->slot_index);

	sched_nr_dl_rach_info(&mac_manager.sched, &rar_info);
	API_rrc_mac_add_user(rnti, rach_info->enb_cc_idx);//todo

	oset_info("[%5u] RACH:slot=%d, cc=%d, preamble=%d, offset=%d, temp_crnti=0x%x",
				rach_info->slot_index,
				rach_info->slot_index,
				rach_info->enb_cc_idx,
				rach_info->preamble,
				rach_info->time_adv,
				rnti);
}

//????prach_worker_rach_detected()
/*void mac_rach_detected(uint32_t tti, uint32_t enb_cc_idx, uint32_t preamble_idx, uint32_t time_adv)
{
	rach_info_t rach_info = {0};
	rach_info.enb_cc_idx	= enb_cc_idx;
	rach_info.slot_index	= tti;
	rach_info.preamble		= preamble_idx;
	rach_info.time_adv		= time_adv;
	mac_handle_rach_info(&rach_info);
}*/

static int mac_process_ce_subpdu(uint16_t rnti, mac_sch_subpdu_nr *subpdu)
{
  // Handle MAC lcid
  switch (subpdu->lcid) {
	case CCCH_SIZE_48:
	case CCCH_SIZE_64: {
	  mac_sch_subpdu_nr *ccch_subpdu = const_cast<srsran::mac_sch_subpdu_nr&>(subpdu);
	  rlc->write_pdu(rnti, 0, ccch_subpdu.get_sdu(), ccch_subpdu.get_sdu_length());
	  // store content for ConRes CE and schedule CE accordingly
	  mac.store_msg3(rnti,
					 srsran::make_byte_buffer(ccch_subpdu.get_sdu(), ccch_subpdu.get_sdu_length(), __FUNCTION__));
	  sched->dl_mac_ce(rnti, srsran::mac_sch_subpdu_nr::CON_RES_ID);//void sched_nr::dl_mac_ce(uint16_t rnti, uint32_t ce_lcid)
	} break;
	case CRNTI: {
	  // 当 UE 处于 RRC_CONNECTED 态但上行不同步时，UE 有自己的 C-RNTI，在随机接入过程的
      // Msg3 中，UE 会通过 C-RNTI MAC control element 将自己的 C-RNTI 告诉 eNodeB，eNodeB 在步骤
      // 四中使用这个 C-RNTI 来解决冲突。

	  // 上行Out of sync
	  // UE在MAC层如何判断上行同步/失步（详见36.321的5.2节）：
	  // eNB会通过RRC信令给UE配置一个timer（在MAC层，称为timeAlignmentTimer），UE使用该timier在MAC层确定上行是否同步。
	  // 需要注意的是：该timer有Cell-specific级别和UE-specific级别之分。eNodeB通过SystemInformationBlockType2的timeAlignmentTimerCommon字段来配置的Cell-specific级别的timer；eNodeB通过MAC-MainConfig的timeAlignmentTimerDedicated字段来配置UE-specific级别的timer。
	  // 如果UE配置了UE-specific的timer，则UE使用该timer值，否则UE使用Cell-specific的timer值。
	  // 当UE收到Timing Advance Command（来自RAR或Timing Advance Command MAC controlelement），UE会启动或重启该timer。如果该timer超时，则认为上行失步，UE会清空HARQ buffer，通知RRC层释放PUCCH/SRS，并清空任何配置的DL assignment和UL grant。
	  // 当该timer在运行时，UE认为上行是同步的；而当该timer没有运行，即上行失步时，UE在上行只能发送preamble。
	  uint16_t ce_crnti  = subpdu.get_c_rnti();
	  if (ce_crnti == SRSRAN_INVALID_RNTI) {
		oset_error("Malformed C-RNTI CE detected. C-RNTI can't be 0x0.", subpdu->lcid);
		return OSET_ERROR;
	  }
	  uint16_t prev_rnti = rnti;
	  rnti				 = ce_crnti;
	  rrc->update_user(prev_rnti, rnti);//int rrc_nr::update_user(uint16_t new_rnti, uint16_t old_rnti)
	  sched->ul_sr_info(rnti); // provide UL grant regardless of other BSR content for UE to complete RA
	} break;
	case SHORT_BSR:
	case SHORT_TRUNC_BSR: {
	  lcg_bsr_t sbsr = subpdu.get_sbsr();
	  uint32_t buffer_size_bytes = buff_size_field_to_bytes(sbsr.buffer_size, SHORT_BSR);
	  // Assume all LCGs are 0 if reported SBSR is 0
	  if (buffer_size_bytes == 0) {
		for (uint32_t j = 0; j <= SCHED_NR_MAX_LC_GROUP; j++) {
		  sched->ul_bsr(rnti, j, 0);
		}
	  } else {
		sched->ul_bsr(rnti, sbsr.lcg_id, buffer_size_bytes);//void sched_nr::ul_bsr(uint16_t rnti, uint32_t lcg_id, uint32_t bsr)
	  }
	} break;
	case LONG_BSR:
	case LONG_TRUNC_BSR: {
	  srsran::mac_sch_subpdu_nr::lbsr_t lbsr = subpdu.get_lbsr();
	  for (auto& lb : lbsr.list) {
		sched->ul_bsr(rnti, lb.lcg_id, buff_size_field_to_bytes(lb.buffer_size, LONG_BSR));
	  }
	} break;
	case SE_PHR:
	  // SE_PHR not implemented
	  break;
	case PADDING:
	  break;
	default:
	  oset_error("Unhandled subPDU with LCID=%d", subpdu->lcid);
  }

  return OSET_OK;
}


static int mac_handle_pusch_pdu_info(pusch_mac_pdu_info_t *pdu_info)
{
	uint16_t rnti = pdu_info->rnti;
	byte_buffer_t *pdu = pdu_info->pdu;
	oset_assert(pdu);

	ue_nr *mac_ue = ue_nr_find_by_rnti(rnti);
	oset_assert(mac_ue);

	mac_sch_pdu_nr_init_rx(&mac_ue->mac_pdu_ul, true);

	// mac pdu decode
	if (mac_sch_pdu_nr_unpack(&mac_ue->mac_pdu_ul, pdu->msg, pdu->N_bytes) != OSET_OK) {
		return OSET_ERROR;
	}

	mac_sch_pdu_nr_to_string(&mac_ue->mac_pdu_ul, rnti);

	// 下行(dl_sch)MAC PDU要求组装subPDU的时候，CEs优先于SDU，SDU优先于padding
	// |Sub header 1|MAC CE 1|Sub header 2|MAC CE 2|Sub header 3|MAC SDU|Padding|

	// 上行(ul_sch)MAC PDU要求组装subPDU的时候，SDU优先于CEs，CEs优先于padding
	// |Sub header 1|MAC SDU|Sub header 2|MAC CE 1|Sub header 3|MAC MAC CE 2|Padding|

	// Process MAC CRNTI CE first, if it exists
	uint32_t crnti_ce_pos = cvector_size(&mac_ue->mac_pdu_ul.subpdus);
	for (uint32_t n = cvector_size(&mac_ue->mac_pdu_ul.subpdus); n > 0; --n) {
		mac_sch_subpdu_nr *subpdu = &mac_ue->mac_pdu_ul.subpdus[n-1];
		if (subpdu->lcid == CRNTI) {
		  if (mac_process_ce_subpdu(rnti, subpdu) != OSET_OK) {
			return OSET_ERROR;
		  }
		  // 记录mac ce的边界
		  crnti_ce_pos = n - 1;
		}
	}

	// Process SDUs and remaining MAC CEs
	for (uint32_t n = 0; n < pdu_ul.get_num_subpdus(); ++n) {
		srsran::mac_sch_subpdu_nr& subpdu = pdu_ul.get_subpdu(n);
		if (subpdu.is_sdu()) {
			rrc->set_activity_user(rnti);//void rrc_nr::set_activity_user(uint16_t rnti)
			rlc->write_pdu(rnti, subpdu.get_lcid(), subpdu.get_sdu(), subpdu.get_sdu_length());//void rlc::write_pdu(uint16_t rnti, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes)
		} else if (n != crnti_ce_pos) {
			if (mac_process_ce_subpdu(rnti, subpdu) != OSET_OK) {
				return OSET_ERROR;
			}
		}
	}

	return OSET_OK;
}


static void gnb_mac_task_handle(msg_def_t *msg_p, uint32_t msg_l)
{
	oset_assert(msg_p);
	oset_assert(msg_l > 0);
	switch (RQUE_MSG_ID(msg_p))
	{	
		case RACH_MAC_DETECTED_INFO:
			/*rach info handle*/
			mac_handle_rach_info(&RACH_MAC_DETECTED_INFO(msg_p));
			break;

		case PUSCH_MAC_PDU_INFO:
			/*pusch info handle*/
			mac_handle_pusch_pdu_info(&PUSCH_MAC_PDU_INFO(msg_p));
			oset_free(PUSCH_MAC_PDU_INFO(msg_p).pdu);
			break;		
		default:
			oset_error("Received unknown message: %d:%s",  RQUE_MSG_ID(msg_p), RQUE_MSG_NAME(msg_p));
			break;
	}
}

void *gnb_mac_task(oset_threadplus_t *thread, void *data)
{
	msg_def_t *received_msg = NULL;
	uint32_t length = 0;
	task_map_t *task = task_map_self(TASK_MAC);
	int rv = 0;

	mac_init();
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Starting MAC layer thread");

	 for ( ;; ){
		 rv = oset_ring_queue_try_get(task->msg_queue, &received_msg, &length);
		 if(rv != OSET_OK)
		 {
			if (rv == OSET_DONE)
				break;
		 
			if (rv == OSET_RETRY){
				continue;
			}
		 }
		 gnb_mac_task_handle(received_msg, length);
		 task_free_msg(RQUE_MSG_ORIGIN_ID(received_msg), received_msg);
		 received_msg = NULL;
		 length = 0;
	}
	mac_destory();
}

int API_mac_rrc_cell_cfg(cvector_vector_t(sched_nr_cell_cfg_t) sched_cells)
{
	cvector_copy(sched_cells, mac_manager.cell_config);
	sched_nr_config(&mac_manager.sched, &mac_manager.args->sched_cfg, mac_manager.cell_config);
	cvector_reserve(mac_manager.detected_rachs, cvector_size(mac_manager.cell_config));

	//cell 0
	sched_nr_cell_cfg_t *sched_nr_cell_cfg = &mac_manager.cell_config[0];
	oset_assert(sched_nr_cell_cfg);
	// read SIBs from RRC (SIB1 for now only)
	for (uint32_t i = 0; i < cvector_size(sched_nr_cell_cfg->sibs); i++) {
		sib_info_t sib  = {0};
		sib.index       = i;
		sib.periodicity = 160; // TODO: read period_rf from config
		if (API_rrc_mac_read_pdu_bcch_dlsch(sib.index, sib.payload) != OSET_OK) {
		  oset_error("Couldn't read SIB %d from RRC", sib.index);
		}

		oset_info("Including SIB %d into SI scheduling", sib.index + 1);
		cvector_push_back(mac_manager.bcch_dlsch_payload, sib);
	}
	
	//mac_manager.rx = new mac_nr_rx; ???callback func
	//mac_manager.default_ue_phy_cfg = get_common_ue_phy_cfg(sched_nr_cell_cfg);

	return OSET_OK;
}

//mac_ue_cfg
int API_mac_rrc_api_ue_cfg(uint16_t rnti, sched_nr_ue_cfg_t *ue_cfg)
{
	sched_nr_ue_cfg(&mac_manager.sched, rnti, ue_cfg);
	return OSET_OK;
}

int API_mac_rrc_remove_ue(uint16_t rnti)
{
	mac_remove_ue(rnti);
	return OSET_OK;
}

int API_mac_rlc_buffer_state(uint16_t rnti, uint32_t lc_id, uint32_t tx_queue, uint32_t retx_queue)
{
	sched_nr_dl_buffer_state(&mac_manager.sched, rnti, lc_id, tx_queue, retx_queue);
	return OSET_OK;
}

ul_sched_t* API_mac_phy_get_ul_sched(srsran_slot_cfg_t* slot_cfg, uint32_t cc_idx)
{
	return mac_get_ul_sched(slot_cfg, cc_idx);
}

dl_sched_t* API_mac_phy_get_dl_sched(srsran_slot_cfg_t *slot_cfg, uint32_t cc_idx)
{
	return mac_get_dl_sched(slot_cfg, cc_idx);
}

int API_mac_phy_pucch_info(pucch_info_t *pucch_info, uint32_t cc_idx)
{
	return mac_pucch_info(pucch_info, cc_idx);
}

int API_mac_phy_pusch_info(srsran_slot_cfg_t *slot_cfg, pusch_info_t *pusch_info, uint32_t cc_idx)
{
	return mac_pusch_info(slot_cfg, pusch_info, cc_idx);
}

