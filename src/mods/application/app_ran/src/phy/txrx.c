/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "channel_2c.h"
#include "radio.h"
#include "phy_util.h"
#include "txrx.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-txrx"

void txrx_stop(void)
{
	oset_apr_mutex_lock(phy_manager_self()->mutex);
	oset_apr_thread_cond_broadcast(phy_manager_self()->cond);
	oset_apr_mutex_unlock(phy_manager_self()->mutex);
	oset_threadplus_destroy(task_map_self(TASK_RXTX)->thread);
}

void txrx_init(void)
{
	phy_common* worker_com = &phy_manager_self()->workers_common;

  // Instantiate UL channel emulator
  if (worker_com->params.ul_channel_args.enable) {
    gnb_manager_self()->ul_channel = channel_create(worker_com->params.ul_channel_args));
  }

  if (OSET_ERROR == task_thread_create(TASK_RXTX, NULL)) {
	oset_error("Create task for gNB rxtx failed");
	return OSET_ERROR;
  }

  return OSET_OK;
}


void txrx_task_init(void)
{
	float samp_rate = srsran_sampling_freq_hz(get_nof_prb(0));
	// Calculate the number of FFT bins required for the specified system bandwidth. 
	//It derives system bandwidth from nof_prb and then figure out FFT size based on the derived BW.

	// Configure radio
	set_rx_srate(samp_rate);
	set_tx_srate(samp_rate);

	// Set Tx/Rx frequencies
	for (uint32_t cc_idx = 0; cc_idx < get_nof_carriers(); cc_idx++) {
	  double   tx_freq_hz = get_dl_freq_hz(cc_idx);
	  double   rx_freq_hz = get_ul_freq_hz(cc_idx);
	  uint32_t rf_port	  = get_rf_port(cc_idx);
	  oset_info(
		    "Setting frequency: DL=%.1f Mhz, DL_SSB=%.2f Mhz (SSB-ARFCN=%d), UL=%.1f MHz for cc_idx=%d nof_prb=%d",
		    tx_freq_hz / 1e6f,
		    get_ssb_freq_hz(cc_idx) / 1e6f,
		    freq_to_nr_arfcn_2c(gnb_manager_self()->band_helper, get_ssb_freq_hz(cc_idx)),
		    rx_freq_hz / 1e6f,
		    cc_idx,
		    get_nof_prb(cc_idx));
	
	  set_tx_freq(rf_port, tx_freq_hz);
	  set_rx_freq(rf_port, rx_freq_hz);
	}

	// Set channel emulator sampling rate
	if (gnb_manager_self()->ul_channel) {
	    channel_set_srate(gnb_manager_self()->ul_channel ,(uint32_t)samp_rate);
	}
}


void *gnb_txrx_task(oset_threadplus_t *thread, void *data)
{
    msg_def_t *received_msg = NULL;
	uint32_t length = 0;
    task_map_t *task = task_map_self(TASK_TXRX);
    int rv = 0;
	rf_buffer_t    buffer	 = {0};
	rf_timestamp_t timestamp = {0};
	uint32_t	   sf_len	 = SRSRAN_SF_LEN_PRB(get_nof_prb(0));//15khz   5G 1slot

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Starting PHY txrx thread");

	txrx_task_init();
	oset_info("Starting RX/TX thread nof_prb=%d, sf_len=%d", get_nof_prb(0), sf_len);

    phy_manager_self()->tti = TTI_SUB(0, FDD_HARQ_DELAY_UL_MS + 1);

	// Main loop
    while(gnb_manager_self()->running){
		////1ms调度一次 	  (系统帧号1024循环	    1帧=10子帧=10ms)      for 5G  SCS=15khz 1slot=1ms=14+1symbols
		phy_manager_self()->tti = TTI_ADD(phy_manager_self()->tti, 1);
	    
		if ((NULL == phy_manager_self()->slot_worker.th_pools) || (get_nof_carriers_nr() <= 0)) {
			oset_error("%s run error",__OSET_FUNC__);
			break;
		}

		oset_debug("%s:threadpool current task %d",__OSET_FUNC__, oset_threadpool_tasks_count(phy_manager_self()->slot_worker.th_pools));
		if(oset_threadpool_tasks_count(phy_manager_self()->slot_worker.th_pools) >= 10)
		{
			oset_apr_mutex_lock(phy_manager_self()->mutex);
			oset_apr_thread_cond_wait(phy_manager_self()->cond, phy_manager_self()->mutex);
			oset_apr_mutex_unlock(phy_manager_self()->mutex);
		}

		// Multiple cell buffer mapping
		{
		  uint32_t cc = 0;
		  for (uint32_t cc_nr = 0; cc_nr < get_nof_carriers_nr(); cc_nr++, cc++) {
			uint32_t rf_port = get_rf_port(cc);
		
			for (uint32_t p = 0; p < get_nof_ports(cc); p++) {
			  // WARNING:
			  // - The number of ports for all cells must be the same
			  // - Only one NR cell is currently supported
				buffer.set(rf_port, p, worker_com->get_nof_ports(0), get_buffer_rx(p));
			}
		  }
		}


	}
}

