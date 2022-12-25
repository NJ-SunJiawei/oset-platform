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

#define sf_buffer_sz  128 * 1024

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	cf_t     samples[sf_buffer_sz];
	uint32_t nof_samples;
	uint32_t tti;
}sf_buffer;

int prach_worker_init(uint32_t		   cc_idx,
                             const srsran_cell_t *cell_,
                             const srsran_prach_cfg_t  *prach_cfg_,
                             uint32_t nof_workers_);

#ifdef __cplusplus
}
#endif

#endif
