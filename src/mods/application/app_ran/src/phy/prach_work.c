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
#define OSET_LOG2_DOMAIN   "prach-work"

typedef struct{
	uint32_t		   cc_idx;
	srsran_cell_t	   cell;
	srsran_prach_cfg_t prach_cfg;
	srsran_prach_t	   prach;
	sf_buffer*		   current_buffer;
	float			   max_prach_offset_us;
	uint32_t		   nof_sf;
	uint32_t		   sf_cnt;
	uint32_t		   nof_workers;
}prach_worker_manager_t;

static prach_worker_manager_t prach_work_manager = {
    .max_prach_offset_us = 50,
};

prach_worker_manager_t *prach_work_manager_self(void)
{
    return &prach_work_manager;
}


int prach_worker_init(uint32_t		   cc_idx,
                             const srsran_cell_t *cell_,
                             const srsran_prach_cfg_t  *prach_cfg_,
                             uint32_t nof_workers_)
{
  prach_work_manager.cc_idx   = cc_idx;
  prach_work_manager.prach_cfg   = *prach_cfg_;
  prach_work_manager.cell        = *cell_;
  prach_work_manager.nof_workers = nof_workers_;

  if (srsran_prach_init(&prach_work_manager.prach, srsran_symbol_sz(prach_work_manager.cell.nof_prb))) {
    return OSET_ERROR;
  }

  if (srsran_prach_set_cfg(&prach_work_manager.prach, &prach_work_manager.prach_cfg, prach_work_manager.cell.nof_prb)) {
    oset_error("Error initiating PRACH");
    return OSET_ERROR;
  }

  srsran_prach_set_detect_factor(&prach_work_manager.prach, 60);

  prach_work_manager.nof_sf = (uint32_t)ceilf(prach_work_manager.prach.T_tot * 1000);

  if (prach_work_manager.nof_workers > 0) {
	  if (OSET_ERROR == task_thread_create(TASK_PRACH, NULL)) {
		oset_error("Create task for gNB PRACH failed");
		return OSET_ERROR;
	  }
  }

  prach_work_manager.sf_cnt = 0;

  return OSET_OK;
}

