/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "prach_work.h"

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

prach_worker_manager_t *prach_work_manager_self(void)
{
    return prach_work_manager;
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

  prach_work_manager[cc_idx].nof_sf = (uint32_t)ceilf(prach_work_manager[cc_idx].prach.T_tot * 1000);

  if (prach_work_manager[cc_idx].nof_workers > 0) {
	  if (OSET_ERROR == task_thread_create(TASK_PRACH, NULL)) {
		oset_error("Create task for gNB PRACH failed");
		return OSET_ERROR;
	  }
  }

  prach_work_manager[cc_idx].sf_cnt = 0;

  return OSET_OK;
}

void prach_worker_stop(uint32_t cc_idx)
{
    oset_pool_final(&pool_buffer[cc_idx]);
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
	   prach_work_manager[cc_idx].sf_cnt++;
	   if (prach_work_manager[cc_idx].sf_cnt == nof_sf) {
		 prach_work_manager[cc_idx].sf_cnt = 0;
		 if (prach_work_manager[cc_idx].nof_workers == 0) {
		   run_tti(current_buffer);
		   current_buffer->reset();
		   buffer_pool.deallocate(current_buffer);
		 } else {
		   pending_buffers.push(current_buffer);
		 }
	   }
	 }
	 return 0;

}


 void *gnb_prach_task(oset_threadplus_t *thread, void *data)
 {
	 msg_def_t *received_msg = NULL;
	 uint32_t length = 0;
	 task_map_t *task = task_map_self(TASK_PRACH);
	 int rv = 0;
	 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Starting PHY prach thread");
 
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
		  //func
		  received_msg = NULL;
		  length = 0;
	 }
 }



