/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef PRACH_WORK_H_
#define PRACH_WORK_H_

#include "oset-core.h"
#include "srsran/srsran.h"

#define sf_buffer_sz  128 * 1024

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	cf_t     samples[sf_buffer_sz];
	uint32_t nof_samples;
	uint32_t tti;
}sf_buffer;

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

int prach_worker_init(uint32_t		   cc_idx,
                             const srsran_cell_t *cell_,
                             const srsran_prach_cfg_t  *prach_cfg_,
                             uint32_t nof_workers_);
void *gnb_prach_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
