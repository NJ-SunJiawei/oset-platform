/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "rf/channel_2c.h"
#include "rf/radio.h"
#include "phy/txrx.h"
#include "phy/prach_work.h"
#include "phy/phy_util.h"
#include "phy/phy.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-txrx"


static void set_context(worker_context_t *w_ctx)
{
	phy_manager_self()->slot_worker.ul_slot_cfg.idx = w_ctx->sf_idx;
	phy_manager_self()->slot_worker.dl_slot_cfg.idx = TTI_ADD(w_ctx->sf_idx, FDD_HARQ_DELAY_UL_MS);
    //???w_ctx->sf_idx change
	phy_manager_self()->slot_worker.context = w_ctx;
}


/*************time util*****************/

/**
 * Add a given amount of seconds to all the timestamps
 * @param secs number of seconds
 */
void timestamp_add(srsran_timestamp_t timestamps[],double secs)
{
  for (int i = 0; i < SRSRAN_MAX_CHANNELS; ++i) {
	srsran_timestamp_add(&timestamps[i], 0, secs);
  }
}

/**
 * Subtract a given amount of seconds to all the timestamps
 * @param secs number of seconds
 */
void timestamp_sub(srsran_timestamp_t timestamps[], double secs)
{
  for (int i = 0; i < SRSRAN_MAX_CHANNELS; ++i) {
	srsran_timestamp_sub(&timestamps[i], 0, secs);
  }
}

/*************txrx*****************/

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

  if (OSET_ERROR == task_thread_create(TASK_TXRX, NULL)) {
	oset_error("Create task for gNB txrx failed");
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
	worker_context_t context = {0};

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Starting PHY txrx thread");

	txrx_task_init();
	oset_info("Starting RX/TX thread nof_prb=%d, sf_len=%d", get_nof_prb(0), sf_len);

    phy_manager_self()->tti = TTI_SUB(0, FDD_HARQ_DELAY_UL_MS + 1);

	// Main loop
    while(gnb_manager_self()->running){
		////1ms调度一次 	  (系统帧号1024循环	    1帧=10子帧=10ms)      for 5G  SCS=15khz 1slot=1ms=14+1symbols
		phy_manager_self()->tti = TTI_ADD(phy_manager_self()->tti, 1);
	    
		if ((NULL == phy_manager_self()->slot_worker.th_pools) || (get_nof_carriers_nr() <= 0)) {
			oset_error("%s run error",OSET_FILE_LINE);
			break;
		}

		oset_debug("%s:threadpool current task %d",__OSET_FUNC__, oset_threadpool_tasks_count(phy_manager_self()->slot_worker.th_pools));
		if(oset_threadpool_tasks_count(phy_manager_self()->slot_worker.th_pools) > 10)
		{
			oset_apr_mutex_lock(phy_manager_self()->mutex);
			oset_apr_thread_cond_wait(phy_manager_self()->cond, phy_manager_self()->mutex);
			oset_apr_mutex_unlock(phy_manager_self()->mutex);
		}

		// Multiple cell buffer mapping
		{
		  for (uint32_t cc_nr = 0; cc_nr < get_nof_carriers_nr(); cc_nr++) {
			uint32_t rf_port = get_rf_port(cc_nr);
		
			for (uint32_t p = 0; p < get_nof_ports(cc_nr); p++) {
			    // WARNING:
			    // - The number of ports for all cells must be the same
			    // - Only one NR cell is currently supported
			    //channel_idx = logical_ch * nof_antennas + port_idx
				buffer.sample_buffer[rf_port * get_nof_ports(0) + p] = get_buffer_rx(p);//Logical Port=Physical Port Mapping
			}
		  }
		}

        buffer.nof_samples = sf_len;
		rx_now(&buffer, &timestamp)

		if (gnb_manager_self()->ul_channel) {
	    channel_run(gnb_manager_self()->ul_channel, buffer.sample_buffer, buffer.sample_buffer, sf_len, timestamp.timestamps[0]);
		}

		// Compute TX time: Any transmission happens in TTI+4 thus advance 4 ms the reception time
		timestamp_add(timestamp.timestamps, FDD_HARQ_DELAY_UL_MS * 1e-3);

		oset_debug("Setting TTI=%d, tx_time=%ld:%f to slot worker pool %p",
			  phy_manager_self()->tti,
			  timestamp.timestamps[0].full_secs,
			  timestamp.timestamps[0].frac_secs,
			  phy_manager_self()->slot_worker.th_pools);

			  
	    // Set NR worker context and start
        memset(&context, 0,sizeof(worker_context_t));
		context.sf_idx     = phy_manager_self()->tti;
		context.worker_ptr = phy_manager_self()->slot_worker.th_pools;
		context.last	   = true;
		context.tx_time    = timestamp;
		set_context(&context);
		
		// Feed PRACH detection before start processing 
		prach_new_tti(0, context.sf_idx, get_buffer_rx(0));


	}
}

