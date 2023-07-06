/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "phy/phy.h"
#include "mac/mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-slotWorker"

#define DEBUG_WRITE_FILE

#define SLOT_WORK_POOL_SIZE  (TX_ENB_DELAY + 1)
oset_stl_queue_def(slot_worker_t, slot_worker) slot_work_list = NULL;

slot_worker_t* slot_worker_alloc(void)
{
	slot_worker_t *slot_w = oset_stl_queue_del_first(&slot_work_list);
}

void slot_worker_free(slot_worker_t *slot_w)
{
	int i = 0;

	slot_w->context = {0};
	slot_w->ul_slot_cfg = {0};
	slot_w->dl_slot_cfg = {0};
	
	for (i = 0; i < phy_manager_self()->slot_args.nof_rx_ports; i++) {
	  srsran_vec_cf_zero(slot_w->rx_buffer[i], slot_w->sf_len);
	}
	for (i = 0; i < phy_manager_self()->slot_args.nof_tx_ports; i++) {
	  srsran_vec_cf_zero(slot_w->tx_buffer[i], slot_w->sf_len);
	}
	oset_stl_queue_add_last(&slot_work_list, slot_w);
}

bool slot_worker_init(slot_worker_args_t *args)
{
	oset_stl_queue_init(&slot_work_list);

	for(int i = 0; i < SLOT_WORK_POOL_SIZE; i++){
		slot_worker_t slot_work = {0};

		// Calculate subframe length
		slot_work.sf_len = (uint32_t)(args->srate_hz / 1000.0);//1/(15000*2048)~~~15symbol*2048FFT
		
		// Copy common configurations
		slot_work.cell_index = args->cell_index;
		slot_work.rf_port    = args->rf_port;
		
		// Allocate Tx buffers
		slot_work.tx_buffer = (cf_t **)oset_malloc(args->nof_tx_ports*sizeof(cf_t *));
		for (uint32_t i = 0; i < args->nof_tx_ports; i++) {
		  slot_work.tx_buffer[i] = srsran_vec_cf_malloc(slot_work.sf_len);
		  if (slot_work.tx_buffer[i] == NULL) {
			oset_error("Error allocating Tx buffer");
			return false;
		  }
		}
		
		// Allocate Rx buffers
		slot_work.rx_buffer = (cf_t **)oset_malloc(args->nof_rx_ports*sizeof(cf_t *));
		for (uint32_t i = 0; i < args->nof_rx_ports; i++) {
		 slot_work.rx_buffer[i] = srsran_vec_cf_malloc(slot_work.sf_len);
		  if (slot_work.rx_buffer[i] == NULL) {
			oset_error("Error allocating Rx buffer");
			return false;
		  }
		}
		
		// Prepare DL arguments
		srsran_gnb_dl_args_t dl_args = {0};
		dl_args.pdsch.measure_time	 = true;
		dl_args.pdsch.max_layers	 = args->nof_tx_ports;
		dl_args.pdsch.max_prb		 = args->nof_max_prb;
		dl_args.nof_tx_antennas 	 = args->nof_tx_ports;
		dl_args.nof_max_prb 		 = args->nof_max_prb;
		dl_args.srate_hz             = args->srate_hz;
		
		// Initialise DL
		if (srsran_gnb_dl_init(&slot_work.gnb_dl, slot_work.tx_buffer, &dl_args) < SRSRAN_SUCCESS) {
			oset_error("Error gNb DL init");
			return false;
		}
		
		// Prepare UL arguments
		srsran_gnb_ul_args_t ul_args	 = {0};
		ul_args.pusch.measure_time	     = true;
		ul_args.pusch.measure_evm	     = true;
		ul_args.pusch.max_layers		 = args->nof_rx_ports;
		ul_args.pusch.sch.max_nof_iter	 = args->pusch_max_its;
		ul_args.pusch.max_prb			 = args->nof_max_prb;
		ul_args.nof_max_prb 			 = args->nof_max_prb;
		ul_args.pusch_min_snr_dB         = args->pusch_min_snr_dB;
		
		// Initialise UL
		if (srsran_gnb_ul_init(&slot_work.gnb_ul, slot_work.rx_buffer[0], &ul_args) < SRSRAN_SUCCESS) {
			oset_error("Error gNb DL init");
			return false;
		}
		oset_stl_queue_add_last(&slot_work_list, slot_work);
	}

#ifdef DEBUG_WRITE_FILE
	const char* filename = "/tmp/nr_baseband.dat";
	oset_debug("Opening %s to dump baseband", filename);
	f = fopen(filename, "w");
#endif

	return true;
}

bool slot_worker_set_common_cfg(const srsran_carrier_nr_t *carrier,
                                 const srsran_pdcch_cfg_nr_t *pdcch_cfg_,
                                 const srsran_ssb_cfg_t      *ssb_cfg_)
{
	slot_worker_t *slot_w = NULL;
	oset_stl_queue_foreach(&slot_work_list, slot_w){
		// Set gNb DL carrier
		if (srsran_gnb_dl_set_carrier(&slot_w->gnb_dl, carrier) < SRSRAN_SUCCESS) {
			oset_error("Error setting DL carrier");
			return false;
		}

		// Configure SSB
		if (srsran_gnb_dl_set_ssb_config(&slot_w->gnb_dl, ssb_cfg_) < SRSRAN_SUCCESS) {
			oset_error("Error setting SSB");
			return false;
		}

		// Set gNb UL carrier
		if (srsran_gnb_ul_set_carrier(&slot_w->gnb_ul, carrier) < SRSRAN_SUCCESS) {
			oset_error("Error setting UL carrier (pci=%d, nof_prb=%d, max_mimo_layers=%d)",
			             carrier.pci,
			             carrier.nof_prb,
			             carrier.max_mimo_layers);
			return false;
		}

		slot_w->pdcch_cfg = *pdcch_cfg_;//get from rrc

		// Update subframe length
		slot_w->sf_len = SRSRAN_SF_LEN_PRB_NR(carrier->nof_prb);
	}

	return true;
}


void slot_worker_destory(void)
{
	int i = 0;
	slot_worker_t *slot_w = NULL;

	oset_threadpool_destory(phy_manager_self()->th_pools);

	oset_stl_queue_foreach(&slot_work_list, slot_w){
		for (i = 0; i < (phy_manager_self()->slot_args.nof_rx_ports; i++){
			free(slot_w->rx_buffer[i]);
		}
		oset_free(slot_w->rx_buffer);
		
		for (i = 0; i < (phy_manager_self()->slot_args.nof_tx_ports; i++){
			free(slot_w->tx_buffer[i]);
		}
		oset_free(slot_w->tx_buffer);
		
		srsran_gnb_dl_free(&slot_w->gnb_dl);
		srsran_gnb_ul_free(&slot_w->gnb_ul);
	}
    oset_stl_queue_term(&slot_work_list);
}


cf_t* get_buffer_rx(slot_worker_t *slot_w, uint32_t antenna_idx)
{
	//std::lock_guard<std::mutex> lock(mutex);
	if (antenna_idx >= (uint32_t)phy_manager_self()->slot_args.nof_rx_ports) {
		return NULL;
	}
	return slot_w->rx_buffer[antenna_idx];
}

cf_t* get_buffer_tx(slot_worker_t *slot_w, uint32_t antenna_idx)
{
	//std::lock_guard<std::mutex> lock(mutex);
	if (antenna_idx >= (uint32_t)phy_manager_self()->slot_args.nof_tx_ports) {
		return NULL;
	}
	return slot_w->tx_buffer[antenna_idx];
}

uint32_t get_buffer_len(slot_worker_t *slot_w)
{
	return slot_w->sf_len;
}

static bool slot_worker_work_ul(slot_worker_t *slot_w, uint32_t cc_idx)
{
	srsran_gnb_ul_t *gnb_ul = &slot_w->gnb_ul;
	if (NULL == gnb_ul) return false;

	//获取ul_slot_cfg时刻上行res预调度资源
	ul_sched_t* ul_sched = API_mac_phy_get_ul_sched(&slot_w->ul_slot_cfg, cc_idx);
	if (NULL == ul_sched) {
		oset_error("[%5u] Error retrieving UL scheduling", slot_w->context.sf_idx);
		return false;
	}

	if (cvector_empty(ul_sched->pucch) && cvector_empty(ul_sched->pusch)) {
		// early exit if nothing has been scheduled
		return true;
	}

	// Demodulate解调
	if (srsran_gnb_ul_fft(gnb_ul) < SRSRAN_SUCCESS) {
		oset_error("[%5u] Error in demodulation", slot_w->context.sf_idx);
		return false;
	}

	// For each PUCCH
	pucch_t *pucch = NULL;
	// 不同ue的uci
	cvector_for_each_in(pucch, ul_sched->pucch){
		// pucch decode list
		cvector_vector_t(pucch_info_t) pucch_info_list = NULL;
		cvector_reserve(pucch_info_list, cvector_size(pucch->candidates));

		// For each candidate decode PUCCH
		for (uint32_t i = 0; i < (uint32_t)cvector_size(pucch->candidates); i++) {
			pucch_info_t pucch_node = {0};
			pucch_node.uci_data.cfg = pucch->candidates[i].uci_cfg;

			// Decode PUCCH
			if (srsran_gnb_ul_get_pucch(gnb_ul,
			                          &slot_w->ul_slot_cfg,
			                          &pucch->pucch_cfg,
			                          &pucch->candidates[i].resource,
			                          &pucch_node.uci_data.cfg,
			                          &pucch_node.uci_data.value,
			                          &pucch_node.csi) < SRSRAN_SUCCESS) {
				oset_error("[%5u] Error getting PUCCH", slot_w->context.sf_idx);
				cvector_free(pucch_info_list);
				return false;
			}
			cvector_push_back(pucch_info_list, pucch_node);
		}

		// Find most suitable PUCCH candidate
		uint32_t best_candidate = 0;
		for (uint32_t i = 1; i < (uint32_t)cvector_size(pucch_info_list); i++) {
			// Select candidate if exceeds the previous best candidate SNR
			if (pucch_info_list[i].csi.snr_dB > pucch_info_list[best_candidate].csi.snr_dB) {
				best_candidate = i;
			}
		}

		// Inform stack
		if (API_mac_phy_pucch_info(&pucch_info_list[best_candidate], slot_w->cell_index) < OSET_OK) {
			cvector_free(pucch_info_list);
			return false;
		}

		// Log PUCCH decoding
		char str[512] = {0};
		srsran_gnb_ul_pucch_info(gnb_ul,
		                       &pucch->candidates[0].resource,
		                       &pucch_info_list[best_candidate].uci_data,
		                       &pucch_info_list[best_candidate].csi,
		                       str,
		                       (uint32_t)sizeof(str));

		oset_info("[%5u] PUCCH: %s", slot_w->context.sf_idx, str);
		cvector_free(pucch_info_list);
	}

	// For each PUSCH
	byte_buffer_t *pdu = byte_buffer_init();
	pusch_t **pusch_node = NULL;
	cvector_for_each_in(pusch_node, ul_sched->pusch){
		pusch_t *pusch = *pusch_node;
		// Prepare PUSCH
		pusch_info_t pusch_info = {0};
		pusch_info.uci_cfg = pusch->sch.uci;
		pusch_info.pid     = pusch->pid;
		pusch_info.rnti    = pusch->sch.grant.rnti;
		pusch_info.pdu     = byte_buffer_clear(pdu);
		if (NULL == pusch_info.pdu) {
		  oset_error("[%5u] Couldn't allocate PDU", slot_w->context.sf_idx);
		  return false;
		}
		pusch_info.pdu->N_bytes             = pusch->sch.grant.tb[0].tbs / 8;
		pusch_info.pusch_data.tb[0].payload = pusch_info.pdu->msg; // 存放上行ue mac源数据buffer

		// Decode PUSCH to mac pdu
		if (srsran_gnb_ul_get_pusch(gnb_ul, &slot_w->ul_slot_cfg, &pusch->sch, &pusch->sch.grant, &pusch_info.pusch_data) < SRSRAN_SUCCESS) {
		  oset_error("[%5u] Error getting PUSCH", slot_w->context.sf_idx);
		  return false;
		}

		// Extract DMRS information
		pusch_info.csi = gnb_ul->dmrs.csi;

		// Inform stack
		if (API_mac_phy_pusch_info(&slot_w->ul_slot_cfg, pusch_info, slot_w->cell_index) < OSET_OK) {
		  oset_error("[%5u] Error pushing PUSCH information to stack", slot_w->context.sf_idx);
		  return false;
		}

		// Log PUSCH decoding
		char str[512] = {0};
		srsran_gnb_ul_pusch_info(gnb_ul, &pusch->sch, &pusch_info.pusch_data, str, (uint32_t)sizeof(str));
		if (oset_runtime()->hard_log_level >= OSET_LOG2_DEBUG) {
			char str_extra[1024] = {0};
			srsran_sch_cfg_nr_info(&pusch.sch, str_extra (uint32_t)sizeof(str_extra));
			oset_debug("[%5u] PUSCH: %s %s", slot_w->context.sf_idx, str, str_extra);
		} else {
			oset_info("[%5u] PUSCH: %s", slot_w->context.sf_idx, str);
		}
	}
	oset_free(pdu);

	return true;
}


static bool slot_worker_work_dl(slot_worker_t *slot_w, uint32_t cc_idx)
{
	// The Scheduler interface needs to be called synchronously, wait for the sync to be available

	if(NULL == slot_w) return false;

	srsran_gnb_dl_t *gnb_dl = &slot_w->gnb_dl;
	if (NULL == gnb_dl) return false;

	//todo need lock?
	//oset_apr_mutex_lock(mac_manager_self()->sched.mutex);
	// Retrieve Scheduling for the current processing DL slot
	dl_sched_t* dl_sched_ptr = API_mac_phy_get_dl_sched(&slot_w->dl_slot_cfg, cc_idx);

	//oset_apr_mutex_unlock(mac_manager_self()->sched.mutex);

	// Abort DL processing if the scheduling returned an invalid pointer
	if (NULL == dl_sched_ptr) return false;

	const srsran_slot_cfg_t *dl_slot_cfg = &slot_w->dl_slot_cfg;
	if (NULL == dl_slot_cfg) return false;

	// 清空天线端口缓冲 fft_cfg.in_buffer
	if (srsran_gnb_dl_base_zero(gnb_dl) < SRSRAN_SUCCESS) {
		oset_error("[%5u] Error zeroing RE grid", slot_w->context.sf_idx);
		return false;
	}

	// Encode PDCCH for DL transmissions
	pdcch_dl_t **pdcch_dl_node = NULL;
	cvector_for_each_in(pdcch_dl_node, dl_sched_ptr->pdcch_dl){
		pdcch_dl_t *pdcch = *pdcch_dl_node;
		// Set PDCCH configuration, including DCI dedicated
		if (srsran_gnb_dl_set_pdcch_config(gnb_dl, &slot_w->pdcch_cfg, &pdcch->dci_cfg) < SRSRAN_SUCCESS) {
			oset_error("[%5u] PDCCH: Error setting DL configuration", slot_w->context.sf_idx);
			return false;
		}

		// Put PDCCH message
		if (srsran_gnb_dl_pdcch_put_dl(gnb_dl, dl_slot_cfg, &pdcch->dci) < SRSRAN_SUCCESS) {
			oset_error("[%5u] PDCCH: Error putting DL message", slot_w->context.sf_idx);
			return false;
		}

		// Log PDCCH information
		char str[512] = {0};
		srsran_gnb_dl_pdcch_dl_info(gnb_dl, &pdcch.dci, str, sizeof(str));
		oset_info("[%5u] PDCCH: cc=%d %s tti_tx=%d", slot_w->context.sf_idx, slot_w->cell_index, str, dl_slot_cfg->idx);
	}

	// Encode PDCCH for UL transmissions
	pdcch_ul_t **pdcch_ul_node = NULL;
	cvector_for_each_in(pdcch_ul_node, dl_sched_ptr->pdcch_ul){
		pdcch_ul_t *pdcch = *pdcch_ul_node;
		// Set PDCCH configuration, including DCI dedicated
		if (srsran_gnb_dl_set_pdcch_config(gnb_dl,  &slot_w->pdcch_cfg, &pdcch->dci_cfg) < SRSRAN_SUCCESS) {
			oset_error("[%5u] PDCCH: Error setting DL configuration", slot_w->context.sf_idx);
			return false;
		}

		// Put PDCCH message
		if (srsran_gnb_dl_pdcch_put_ul(gnb_dl, dl_slot_cfg, &pdcch->dci) < SRSRAN_SUCCESS) {
			oset_error("[%5u] PDCCH: Error putting DL message", slot_w->context.sf_idx);
			return false;
		}

		// Log PDCCH information
		char str[512] = {0};
		srsran_gnb_dl_pdcch_ul_info(gnb_dl, &pdcch.dci, str, sizeof(str));
		oset_info("[%5u] PDCCH: cc=%d %s tti_tx=%d", slot_w->context.sf_idx, slot_w->cell_index, str, dl_slot_cfg->idx);
	}

	// Encode PDSCH
	pdsch_t **pdsch_node = NULL;
	cvector_for_each_in(pdsch_node, dl_sched_ptr->pdsch){
		pdsch_t *pdsch = *pdsch_node;
		// convert MAC to PHY buffer data structures
		//一个码字（codeword）是对在一个TTI上发送的一个TB进行CRC插入、码块分割并为每个码块插入CRC、信道编码、速率匹配之后，得到的数据码流
		//每个码字与一个TB相对应，因此一个UE在一个TTI至多发送2个码字。码字可以看作是带出错保护的TB
		uint8_t* data[SRSRAN_MAX_TB] = {0};
		for (uint32_t i = 0; i < SRSRAN_MAX_TB; ++i) {
			if (pdsch->data[i] != nullptr) {
				//存放gnb下行组装好mac数据
				data[i] = pdsch->data[i]->msg;
			}
		}

		// Put PDSCH message
		if (srsran_gnb_dl_pdsch_put(gnb_dl, dl_slot_cfg, &pdsch->sch, data) < SRSRAN_SUCCESS) {
			oset_error("[%5u] PDSCH: Error putting DL message", slot_w->context.sf_idx);
			return false;
		}

		// Log PDSCH information
		char str[512] = {0};
		srsran_gnb_dl_pdsch_info(gnb_dl, &pdsch->sch, str, sizeof(str));

		if (oset_runtime()->hard_log_level >= OSET_LOG2_DEBUG) {
			char str_extra[1024] = {0};
			srsran_sch_cfg_nr_info(&pdsch->sch, str_extra, sizeof(str_extra));
			oset_debug("[%5u] PDSCH: cc=%d %s tti_tx=%d\n%s", slot_w->context.sf_idx, slot_w->cell_index, str, dl_slot_cfg->idx, str_extra);
		} else {
			oset_info("[%5u] PDSCH: cc=%d %s tti_tx=%d", slot_w->context.sf_idx, slot_w->cell_index, str, dl_slot_cfg->idx);
		}
	}

	// Put NZP-CSI-RS
	srsran_csi_rs_nzp_resource_t *nzp_csi_rs = NULL;
	cvector_for_each_in(nzp_csi_rs, dl_sched_ptr->nzp_csi_rs){
		if (srsran_gnb_dl_nzp_csi_rs_put(gnb_dl, dl_slot_cfg, nzp_csi_rs) < SRSRAN_SUCCESS) {
			oset_error("[%5u] NZP-CSI-RS: Error putting signal", slot_w->context.sf_idx);
			return false;
		}
	}

	// Generate baseband signal
	srsran_gnb_dl_gen_signal(gnb_dl);

	// Add SSB to the baseband signal
	ssb_t *ssb = NULL;
	cvector_for_each_in(ssb, dl_sched_ptr->ssb){
		if (srsran_gnb_dl_add_ssb(gnb_dl, &ssb->pbch_msg, dl_slot_cfg->idx) < SRSRAN_SUCCESS) {
			oset_error("[%5u] SSB: Error putting signal", slot_w->context.sf_idx);
			return false;
		}
	}

	return true;
}


void* slot_worker_process(oset_threadplus_t *thread, void *data)
{
	slot_worker_t *slot_w = (slot_worker_t *)data;
	if(NULL == slot_w) return NULL;

	// Get Transmission buffer
	uint32_t  nof_ant = phy_manager_self()->slot_args.nof_tx_ports;//get_nof_ports()

	rf_buffer_t tx_rf_buffer = {0};
	set_nof_samples(&tx_rf_buffer, slot_w->sf_len);//1slot(15khz)

	//bind tx_buffer and tx_rf_buffer
	uint32_t rf_port = get_rf_port(slot_w->cell_index);
	//uint32_t rf_port = slot_w->rf_port;//one cell
	for (uint32_t a = 0; a < nof_ant; a++) {
		tx_rf_buffer.sample_buffer[rf_port * nof_ant + a] = get_buffer_tx(a);
	}

	// Ensure sequence
	// Process uplink
	if (!slot_worker_work_ul(slot_w, slot_w->cell_index)) {
	// Wait and release synchronization
		worker_end(context, false, tx_rf_buffer);
		return NULL;
	}

	// Process downlink
	if (!slot_worker_work_dl(slot_w, slot_w->cell_index)) {
		worker_end(context, false, tx_rf_buffer);
		return NULL;
	}

	worker_end(context, true, tx_rf_buffer);

#ifdef DEBUG_WRITE_FILE
	if (num_slots++ < slots_to_dump) {
		oset_info("[%5u] Writing slot %d", slot_w->context.sf_idx, slot_w->dl_slot_cfg.idx);
		fwrite(tx_rf_buffer.get(0), tx_rf_buffer.get_nof_samples() * sizeof(cf_t), 1, f);
	} else if (num_slots == slots_to_dump) {
		oset_info("[%5u] Baseband signaled dump finished. Please close app", slot_w->context.sf_idx);
		fclose(f);
	}
#endif

   //todo put into  work end
	slot_worker_free(slot_w);

	oset_apr_mutex_lock(phy_manager_self()->mutex);
	oset_apr_thread_cond_broadcast(phy_manager_self()->cond);
	oset_apr_mutex_unlock(phy_manager_self()->mutex);

	return NULL;
}

