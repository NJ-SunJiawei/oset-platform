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

int mac_slot_indication(srsran_slot_cfg_t *slot_cfg)
{
	//todo
	return 0;
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

dl_sched_t* mac_get_dl_sched(srsran_slot_cfg_t *slot_cfg)
{
	slot_point pdsch_slot = {0};
	slot_point_init(&pdsch_slot, NUMEROLOGY_IDX, slot_cfg->idx);

	// Initiate new slot and sync UE internal states
	sched_nr_slot_indication(&mac_manager.sched, pdsch_slot);

	// Run DL Scheduler for CC
	// todp cell=0
	dl_res_t* dl_res = sched_nr_get_dl_sched(&mac_manager.sched, pdsch_slot, 0);
		if (NULL == dl_res) {
		return NULL;
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
		      	uint32_t pid = 0; // TODO: get PID from PDCCH struct?
		      	mac_pcap_write_dl_crnti_nr(&mac_manager.pcap, tb_data->msg, tb_data->N_bytes, rnti, pid, slot_cfg->idx);
		      }
		      ue_nr_metrics_dl_mcs(mac_ue, pdsch->sch.grant.tb[i].mcs);
		    }
		  }
		} else if (pdsch.sch.grant.rnti_type == srsran_rnti_type_ra) {
		  rar_t *rar = dl_res->rar[rar_count++];
		  // for RARs we could actually move the byte_buffer to the PHY, as there are no retx
		  pdsch.data[0] = assemble_rar(rar->grants);
		} else if (pdsch.sch.grant.rnti_type == srsran_rnti_type_si) {
		  uint32_t sib_idx = dl_res->sib_idxs[si_count++];
		  pdsch.data[0]    = bcch_dlsch_payload[sib_idx].payload.get();//bcch_dlsch_payload.push_back(std::move(sib))
#ifdef WRITE_SIB_PCAP
		  if (pcap != NULL) {
		    pcap->write_dl_si_rnti_nr(bcch_dlsch_payload[sib_idx].payload->msg,
		                              bcch_dlsch_payload[sib_idx].payload->N_bytes,
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

int API_mac_rlc_buffer_state(uint16_t rnti, uint32_t lc_id, uint32_t tx_queue, uint32_t retx_queue)
{
	sched_nr_dl_buffer_state(&mac_manager.sched, rnti, lc_id, tx_queue, retx_queue);
	return OSET_OK;
}

int API_mac_rrc_remove_ue(uint16_t rnti)
{
	mac_remove_ue(rnti);
	return OSET_OK;
}

