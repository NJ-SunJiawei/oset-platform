/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "phy/prach_worker.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-prach"

static prach_worker_manager_t prach_work_manager[SRSRAN_MAX_CARRIERS] = {
	{.max_prach_offset_us = 50,},
	{.max_prach_offset_us = 50,},
	{.max_prach_offset_us = 50,},
	{.max_prach_offset_us = 50,},
	{.max_prach_offset_us = 50,},
};

static OSET_POOL(pool_buffer[SRSRAN_MAX_CARRIERS], sf_buffer);

prach_worker_manager_t *prach_worker_manager_self(uint32_t		   cc_idx)
{
    return &prach_work_manager[cc_idx];
}


int prach_worker_init(uint32_t		   cc_idx,
                             const srsran_cell_t *cell_,
                             const srsran_prach_cfg_t  *prach_cfg_,
                             uint32_t nof_workers_)
{
	oset_pool_init(&pool_buffer[cc_idx], 8);

	prach_work_manager[cc_idx].prach_cfg   = *prach_cfg_;
	prach_work_manager[cc_idx].cell        = *cell_;
	prach_work_manager[cc_idx].nof_workers = nof_workers_;

	if (srsran_prach_init(&prach_work_manager[cc_idx].prach, srsran_symbol_sz(prach_work_manager[cc_idx].cell.nof_prb))) {
		return OSET_ERROR;
	}

	if (srsran_prach_set_cfg(&prach_work_manager[cc_idx].prach, &prach_work_manager[cc_idx].prach_cfg, prach_work_manager[cc_idx].cell.nof_prb)) {
		oset_error("Error initiating PRACH");
		return OSET_ERROR;
	}

	srsran_prach_set_detect_factor(&prach_work_manager[cc_idx].prach, 60);

	//(Tcp+Tseq)*Ts~subframe number prach占用子帧的数量
	prach_work_manager[cc_idx].nof_sf = (uint32_t)ceilf(prach_work_manager[cc_idx].prach.T_tot * 1000);//s->ms->sf

	if (prach_work_manager[cc_idx].nof_workers > 0) {
		if (OSET_ERROR == task_thread_create(TASK_PRACH, NULL)) {
			oset_error("Create task for gNB PRACH failed");
			return OSET_ERROR;
		}
	}

	prach_work_manager[cc_idx].sf_cnt = 0;

#if defined(ENABLE_GUI) and ENABLE_PRACH_GUI
	//todo
#endif // defined(ENABLE_GUI) and ENABLE_PRACH_GUI

	return OSET_OK;
}

void prach_worker_stop(uint32_t cc_idx)
{
    //????todo
	if (if (prach_work_manager[cc_idx].nof_workers > 0)) {
		oset_threadplus_destroy(task_map_self(TASK_PRACH)->thread, 1);
	}

    oset_pool_final(&pool_buffer[cc_idx]);
}

static void prach_worker_rach_detected(uint32_t tti, uint32_t enb_cc_idx, uint32_t preamble_idx, uint32_t time_adv)
{
	rach_info_t rach_info = {0};
	rach_info.enb_cc_idx	= enb_cc_idx;
	rach_info.slot_index	= tti;
	rach_info.preamble		= preamble_idx;
	rach_info.time_adv		= time_adv;

	msg_def_t *msg_ptr = NULL;
	msg_ptr = task_alloc_msg (TASK_PRACH, RACH_MAC_DETECTED_INFO);
	RQUE_MSG_TTI(msg_ptr) = tti;
	RACH_MAC_DETECTED_INFO(msg_ptr) = rach_info;
	task_send_msg(TASK_MAC, msg_ptr);
}


int prach_worker_run_tti(uint32_t cc_idx, sf_buffer* b)
{
  uint32_t prach_nof_det = 0;
  if (srsran_prach_tti_opportunity(&prach_work_manager[cc_idx].prach, b->tti, -1)) {
    // Detect possible PRACHs
    if (srsran_prach_detect_offset(&prach_work_manager[cc_idx].prach,
                                   prach_work_manager[cc_idx].prach_cfg.freq_offset,
                                   &b->samples[prach_work_manager[cc_idx].prach.N_cp],
                                   prach_work_manager[cc_idx].nof_sf * SRSRAN_SF_LEN_PRB(cell.nof_prb) - prach_work_manager[cc_idx].prach.N_cp,
                                   prach_work_manager[cc_idx].prach_indices,
                                   prach_work_manager[cc_idx].prach_offsets,
                                   prach_work_manager[cc_idx].prach_p2avg,
                                   &prach_nof_det)) {
      oset_error("Error detecting PRACH");
      return SRSRAN_ERROR;
    }

    if (prach_nof_det) {
      for (uint32_t i = 0; i < prach_nof_det; i++) {
        oset_info("PRACH: cc=%d, %d/%d, preamble=%d, offset=%.1f us, peak2avg=%.1f, max_offset=%.1f us",
                    cc_idx,
                    i,
                    prach_nof_det,
                    prach_work_manager[cc_idx].prach_indices[i],//preamble
                    prach_work_manager[cc_idx].prach_offsets[i] * 1e6,
                    prach_work_manager[cc_idx].prach_p2avg[i],
                    prach_work_manager[cc_idx].max_prach_offset_us);

        if (prach_work_manager[cc_idx].prach_offsets[i] * 1e6 < prach_work_manager[cc_idx].max_prach_offset_us) {
          // Convert time offset to Time Alignment command
          uint32_t n_ta = (uint32_t)(prach_work_manager[cc_idx].prach_offsets[i] / (16 * SRSRAN_LTE_TS));
		  
          prach_worker_rach_detected(b->tti, cc_idx, prach_work_manager[cc_idx].prach_indices[i], n_ta); //mac access
          //mac_rach_detected(b->tti, cc_idx, prach_work_manager[cc_idx].prach_indices[i], n_ta);
#if defined(ENABLE_GUI) and ENABLE_PRACH_GUI
        //todo
#endif // defined(ENABLE_GUI) and ENABLE_PRACH_GUI
        }
      }
    }
  }
  return 0;
}


int prach_new_tti(uint32_t cc_idx, uint32_t tti_rx, cf_t* buffer_rx)
{
	 // Save buffer only if it's a PRACH TTI
	 if (srsran_prach_tti_opportunity(&prach_work_manager[cc_idx].prach, tti_rx, -1) || prach_work_manager[cc_idx].sf_cnt) {
	   if (prach_work_manager[cc_idx].sf_cnt == 0) {
		 oset_pool_alloc(&pool_buffer, &prach_work_manager[cc_idx].current_buffer);
		 if (!prach_work_manager[cc_idx].current_buffer) {
		   oset_warn("PRACH skipping tti=%d due to lack of available buffers", tti_rx);
		   return 0;
		 }
	   }
	   if (!prach_work_manager[cc_idx].current_buffer) {
		 oset_error("PRACH: Expected available current_buffer");
		 return -1;
	   }

       prach_work_manager[cc_idx].current_buffer.cc_id = cc_idx;

	   //sf_buffer_sz/(15*2048) ~ max 4 sf
	   if (prach_work_manager[cc_idx].current_buffer->nof_samples + SRSRAN_SF_LEN_PRB(prach_work_manager[cc_idx].cell.nof_prb) < sf_buffer_sz) {
		 memcpy(&prach_work_manager[cc_idx].current_buffer->samples[prach_work_manager[cc_idx].sf_cnt * SRSRAN_SF_LEN_PRB(prach_work_manager[cc_idx].cell.nof_prb)],
				buffer_rx,
				sizeof(cf_t) * SRSRAN_SF_LEN_PRB(prach_work_manager[cc_idx].cell.nof_prb));
		 prach_work_manager[cc_idx].current_buffer->nof_samples += SRSRAN_SF_LEN_PRB(prach_work_manager[cc_idx].cell.nof_prb);
		 if (prach_work_manager[cc_idx].sf_cnt == 0) {
		   prach_work_manager[cc_idx].current_buffer->tti = tti_rx;
		 }
	   } else {
		 oset_error("PRACH: Not enough space in current_buffer");
		 return -1;
	   }
	   //15khz, 1slot = 1sf
	   prach_work_manager[cc_idx].sf_cnt++;
	   if (prach_work_manager[cc_idx].sf_cnt == prach_work_manager[cc_idx].nof_sf) {
		 prach_work_manager[cc_idx].sf_cnt = 0;
		 if (prach_work_manager[cc_idx].nof_workers == 0) {
		   prach_worker_run_tti(cc_idx, prach_work_manager[cc_idx].current_buffer);
		   oset_pool_free(&pool_buffer, prach_work_manager[cc_idx].current_buffer);
		 } else {
		   oset_ring_queue_put(task_map_self(TASK_PRACH)->msg_queue,\
		   	                   &prach_work_manager[cc_idx].current_buffer,\
		   	                   &sizeof(sf_buffer));
		 }
	   }
	 }
	 return 0;

}


 void *gnb_prach_task(oset_threadplus_t *thread, void *data)
 {
	 sf_buffer    *current_buffer = NULL;
	 uint32_t length = 0;
	 int rv = 0;
	 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Starting PHY prach thread");
 
	  for ( ;; ){
		  rv = oset_ring_queue_try_get(task_map_self(TASK_PRACH)->msg_queue, &current_buffer, &length);
		  if(rv != OSET_OK)
		  {
			 if (rv == OSET_DONE)
				 break;
		  
			 if (rv == OSET_RETRY){
				 continue;
			 }
		  }
		  prach_worker_run_tti(current_buffer->cc_id, current_buffer);
		  oset_pool_free(&pool_buffer, current_buffer);
		  current_buffer = NULL;
		  length = 0;
	 }
 }



