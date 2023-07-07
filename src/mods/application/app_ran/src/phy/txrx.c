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
#include "phy/prach_worker.h"
#include "phy/phy_util.h"
#include "phy/phy.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-txrx"

#define PRIORITY_LEVEL_LOW  1
#define PRIORITY_LEVEL_MID  2
#define PRIORITY_LEVEL_HIGH 3

// Main system TTI counter
static uint32_t realtime_tti = 0;

static void set_slot_worker_context(slot_worker_t *slot_w, worker_context_t *w_ctx)
{
	slot_w->ul_slot_cfg.idx = w_ctx->sf_idx;
	slot_w->dl_slot_cfg.idx = TTI_ADD(w_ctx->sf_idx, TX_ENB_DELAY);//phy process max delay
	slot_w->context = *w_ctx;
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
	oset_threadplus_destroy(task_map_self(TASK_RXTX)->thread, 1);
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

// 20M的LTE带宽(12*100prb = 1200子载波采样点)，经过快速傅里叶逆变换IFFT，生产的时域信号，只需要2048个采样点，采样带宽为15K * 2048
// IFFT将时域转换到频域，所以时域上有2048个采样点
// srate_hz = 15k*2048=30.72MHz
// Ts       = 1/(15000HZ*2048)秒
// 4G  1 slot=0.5ms=15360*Ts=((160+2048)+(144+2048)*6)*Ts  

//default one cell  index = 0
void *gnb_txrx_task(oset_threadplus_t *thread, void *data)
{
    msg_def_t *received_msg = NULL;
	uint32_t length = 0;
    task_map_t *task = task_map_self(TASK_TXRX);
    int rv = 0;
	rf_buffer_t    buffer	 = {0};
	rf_timestamp_t timestamp = {0};
	//15khz 1subframe = 1slot = 15khz*2048*0.1ms
	uint32_t	   sf_len	 = SRSRAN_SF_LEN_PRB(get_nof_prb(0));
	worker_context_t context = {0};
	slot_worker_t  *slot_w   = NULL;

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Starting PHY txrx thread");

	txrx_task_init();
	oset_info("Starting RX/TX thread nof_prb=%d, sf_len=%d", get_nof_prb(0), sf_len);

    realtime_tti = TTI_SUB(0, FDD_HARQ_DELAY_UL_MS + 1);

	// Main loop
    while(gnb_manager_self()->running){
		////1ms调度一次 	  (系统帧号1024循环	    1帧=10子帧=10ms)      for 5G  SCS=15khz 1slot=1ms=14+1symbols
		realtime_tti = TTI_ADD(realtime_tti, 1);
	    
		if ((NULL == phy_manager_self()->th_pools) || (get_nof_carriers_nr() <= 0)) {
			oset_error("[%5u] phy run error", realtime_tti);
			break;
		}

		size_t task = oset_threadpool_tasks_count(phy_manager_self()->th_pools);
		if(task > TX_ENB_DELAY)
		{
			//当进入该分支,当队列任务>TX_ENB_DELAY时,阻塞接收, 可以控制任务队列长度,这样会导致处理时间变长
			//若不进入该分支,若上层处理过慢,会导致任务队列很长
			oset_debug("[%5u] phy threadpool task %u > TX_ENB_DELAY, blocking!!!", realtime_tti, task);
			oset_apr_mutex_lock(phy_manager_self()->mutex);
			oset_apr_thread_cond_wait(phy_manager_self()->cond, phy_manager_self()->mutex);
			oset_apr_mutex_unlock(phy_manager_self()->mutex);
		}

		slot_w = slot_worker_alloc();
		oset_assert(slot_w);

		// Multiple cell buffer mapping
		{
		  for (uint32_t cc_nr = 0; cc_nr < get_nof_carriers_nr(); cc_nr++) {
			uint32_t rf_port = get_rf_port(cc_nr);

			for (uint32_t p = 0; p < get_nof_ports(cc_nr); p++) {
			    // WARNING:
			    // - The number of ports for all cells must be the same
			    // - Only one NR cell is currently supported
			    //channel_idx = logical_ch * nof_antennas + port_idx
			    //phys_antenna_idx = i * rf_manager.nof_antennas + j;
				buffer.sample_buffer[rf_port * get_nof_ports(cc_nr) + p] = get_buffer_rx(slot_w, p);//Logical Port=Physical Port Mapping
			}
		  }
		}

        buffer.nof_samples = sf_len;
		rx_now(&buffer, &timestamp)

		if (gnb_manager_self()->ul_channel) {
	    	channel_run(gnb_manager_self()->ul_channel, buffer.sample_buffer, buffer.sample_buffer, sf_len, timestamp.timestamps[0]);
		}

		// Compute TX time: Any transmission happens in TTI+4 thus advance 4 ms the reception time
		timestamp_add(timestamp.timestamps, TX_ENB_DELAY * 1e-3);

		oset_debug("[%5u] Setting TTI=%d, tx_time=%ld:%f to slot worker pool %p",
			  realtime_tti,
			  realtime_tti,
			  timestamp.timestamps[0].full_secs,
			  timestamp.timestamps[0].frac_secs,
			  phy_manager_self()->th_pools);

			  
	    // Set NR worker context and start
        memset(&context, 0,sizeof(worker_context_t));
		context.sf_idx     = realtime_tti;
		context.tx_time    = timestamp;
		set_slot_worker_context(slot_w, &context);
		
		// Feed PRACH detection before start processing 
		prach_new_tti(slot_w->cell_index, slot_w->context.sf_idx, get_buffer_rx(slot_w, 0));

		// Start actual worker
		// Process one at a time and hang it on the linked list in sequence
		// 当前线程池为单线程，可以保证处理slot顺序。
		// 若线程池为多线程，加锁会导致乱序，但是不加锁并发是否会导致数据混乱？？？
		rv = oset_threadpool_push(phy_manager_self()->th_pools,
									slot_worker_process,
									slot_w,
									PRIORITY_LEVEL_HIGH,
									"slot_handle");
		oset_assert(OSET_OK == rv);

		if (realtime_tti % SRSRAN_NSLOTS_PER_SF_NR(srsran_subcarrier_spacing_15kHz) == 0)
		{
			uint32_t f_idx    = SRSRAN_SLOT_NR_DIV(srsran_subcarrier_spacing_15kHz, realtime_tti);
			uint32_t slot_idx = SRSRAN_SLOT_NR_MOD(srsran_subcarrier_spacing_15kHz, realtime_tti);//15khz, slot_id==sf_id
			uint32_t sf_idx   = slot_idx / SRSRAN_NSLOTS_PER_FRAME_NR(srsran_subcarrier_spacing_15kHz);
			gnb_time_tick(f_idx, sf_idx);
		}
	}
}

