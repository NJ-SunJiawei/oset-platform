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
#include "phy/slot_worker.h"
#include "mac/mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-slot-worker"

#define SLOT_WORK_POOL_SIZE  (TX_ENB_DELAY*2)
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
	//const char* filename = "nr_baseband.dat";
	//oset_debug("Opening %s to dump baseband\n", filename);
	//f = fopen(filename, "w");
#endif

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

static bool slot_worker_work_dl(slot_worker_t *slot_w)
{
	// The Scheduler interface needs to be called synchronously, wait for the sync to be available

	if(NULL == slot_w) return false;

	srsran_gnb_dl_t *gnb_dl = &slot_w->gnb_dl;
	if (NULL == gnb_dl) return false;

	//todo need lock?
	//oset_apr_mutex_lock(mac_manager_self()->sched.mutex);
	// Retrieve Scheduling for the current processing DL slot
	dl_sched_t* dl_sched_ptr = mac_get_dl_sched(&slot_w->dl_slot_cfg);

	//oset_apr_mutex_unlock(mac_manager_self()->sched.mutex);

	// Abort DL processing if the scheduling returned an invalid pointer
	if (NULL == dl_sched_ptr) return false;

	srsran_slot_cfg_t *dl_slot_cfg = &slot_w->dl_slot_cfg;
	if (NULL == dl_slot_cfg) return false;

	if (srsran_gnb_dl_base_zero(gnb_dl) < SRSRAN_SUCCESS) {
		oset_error("[%5lu] Error zeroing RE grid", slot_w->context.sf_idx);
		return false;
	}

	// Encode PDCCH for DL transmissions
	for (pdcch_dl_t& pdcch : dl_sched_ptr->pdcch_dl) {
		// Set PDCCH configuration, including DCI dedicated
		if (srsran_gnb_dl_set_pdcch_config(gnb_dl, &pdcch_cfg, &pdcch.dci_cfg) < SRSRAN_SUCCESS) {//配置dci coreset和namespace
			oset_error("[%5lu] PDCCH: Error setting DL configuration", slot_w->context.sf_idx);
			return false;
		}

		// Put PDCCH message
		if (srsran_gnb_dl_pdcch_put_dl(gnb_dl, dl_slot_cfg, &pdcch.dci) < SRSRAN_SUCCESS) {//编码dci
			oset_error("[%5lu] PDCCH: Error putting DL message", slot_w->context.sf_idx);
			return false;
		}

		// Log PDCCH information
		if (oset_runtime()->hard_log_level >= OSET_LOG2_INFO) {
			char str[512] = {0};
			srsran_gnb_dl_pdcch_dl_info(gnb_dl, &pdcch.dci, str, 512);
			oset_info("[%5lu] PDCCH: cc=%d %s tti_tx=%d", slot_w->context.sf_idx, slot_w->cell_index, str, dl_slot_cfg->idx);
		}
	}

	// Encode PDCCH for UL transmissions
	for (const stack_interface_phy_nr::pdcch_ul_t& pdcch : dl_sched_ptr->pdcch_ul) {
		// Set PDCCH configuration, including DCI dedicated
		if (srsran_gnb_dl_set_pdcch_config(gnb_dl, &pdcch_cfg, &pdcch.dci_cfg) < SRSRAN_SUCCESS) {
			oset_error("[%5lu] PDCCH: Error setting DL configuration", slot_w->context.sf_idx);
			return false;
		}

		// Put PDCCH message
		if (srsran_gnb_dl_pdcch_put_ul(gnb_dl, dl_slot_cfg, &pdcch.dci) < SRSRAN_SUCCESS) {
			oset_error("[%5lu] PDCCH: Error putting DL message", slot_w->context.sf_idx);
			return false;
		}

		// Log PDCCH information
		if (oset_runtime()->hard_log_level >= OSET_LOG2_INFO) {
			char str[512] = {0};
			srsran_gnb_dl_pdcch_ul_info(gnb_dl, &pdcch.dci, str, 512);
			oset_info("[%5lu] PDCCH: cc=%d %s tti_tx=%d", slot_w->context.sf_idx, slot_w->cell_index, str, dl_slot_cfg->idx);
		}
	}

	// Encode PDSCH
	for (pdsch_t& pdsch : dl_sched_ptr->pdsch) {
		// convert MAC to PHY buffer data structures
		//一个码字（codeword）是对在一个TTI上发送的一个TB进行CRC插入、码块分割并为每个码块插入CRC、信道编码、速率匹配之后，得到的数据码流
		//每个码字与一个TB相对应，因此一个UE在一个TTI至多发送2个码字。码字可以看作是带出错保护的TB
		uint8_t* data[SRSRAN_MAX_TB] = {};
		for (uint32_t i = 0; i < SRSRAN_MAX_TB; ++i) {
			if (pdsch.data[i] != nullptr) {
				//存放gnb下行组装好mac数据
				data[i] = pdsch.data[i]->msg;
			}
		}

		// Put PDSCH message//重传也在该模块
		if (srsran_gnb_dl_pdsch_put(gnb_dl, dl_slot_cfg, &pdsch.sch, data) < SRSRAN_SUCCESS) {
			oset_error("[%5lu] PDSCH: Error putting DL message", slot_w->context.sf_idx);
			return false;
		}

		// Log PDSCH information
		if (oset_runtime()->hard_log_level >= OSET_LOG2_INFO) {
			std::array<char, 512> str = {};
			srsran_gnb_dl_pdsch_info(gnb_dl, &pdsch.sch, str.data(), (uint32_t)str.size());

			if (oset_runtime()->hard_log_level >= OSET_LOG2_DEBUG) {
				std::array<char, 1024> str_extra = {};
				srsran_sch_cfg_nr_info(&pdsch.sch, str_extra.data(), (uint32_t)str_extra.size());
				oset_info("[%5lu] PDSCH: cc=%d %s tti_tx=%d\n%s", slot_w->context.sf_idx, slot_w->cell_index, str.data(), dl_slot_cfg->idx, str_extra.data());
			} else {
				oset_info("[%5lu] PDSCH: cc=%d %s tti_tx=%d", slot_w->context.sf_idx, slot_w->cell_index, str.data(), dl_slot_cfg->idx);
			}
		}
	}

	// Put NZP-CSI-RS
	for (const srsran_csi_rs_nzp_resource_t& nzp_csi_rs : dl_sched_ptr->nzp_csi_rs) {
		if (srsran_gnb_dl_nzp_csi_rs_put(gnb_dl, dl_slot_cfg, &nzp_csi_rs) < SRSRAN_SUCCESS) {
			oset_error("[%5lu] NZP-CSI-RS: Error putting signal", slot_w->context.sf_idx);
			return false;
		}
	}

	// Generate baseband signal
	srsran_gnb_dl_gen_signal(gnb_dl);

	// Add SSB to the baseband signal
	for (ssb_t& ssb : dl_sched_ptr->ssb) {
		if (srsran_gnb_dl_add_ssb(gnb_dl, &ssb.pbch_msg, dl_slot_cfg->idx) < SRSRAN_SUCCESS) {
			oset_error("[%5lu] SSB: Error putting signal", slot_w->context.sf_idx);
			return false;
		}
	}

	return true;
}


void* slot_worker_process(oset_threadplus_t *thread, void *data)
{
	slot_worker_t *slot_w = (slot_worker_t *)data;
	if(NULL == slot_w) return NULL;

	// Inform Scheduler about new slot
	// mac_slot_indication(&slot_w->dl_slot_cfg);//do nothing

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
	if (! work_ul()) {//phy up
	// Wait and release synchronization
		worker_end(context, false, tx_rf_buffer);
		return NULL;
	}

	// Process downlink
	if (!slot_worker_work_dl(slot_w)) {
		worker_end(context, false, tx_rf_buffer);
		return NULL;
	}

	worker_end(context, true, tx_rf_buffer);

#ifdef DEBUG_WRITE_FILE
	if (num_slots++ < slots_to_dump) {
	oset_info("[%5lu] Writing slot %d", slot_w->context.sf_idx, slot_w->dl_slot_cfg.idx);
	fwrite(tx_rf_buffer.get(0), tx_rf_buffer.get_nof_samples() * sizeof(cf_t), 1, f);
	} else if (num_slots == slots_to_dump) {
	oset_info("[%5lu] Baseband signaled dump finished. Please close app", slot_w->context.sf_idx);
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

