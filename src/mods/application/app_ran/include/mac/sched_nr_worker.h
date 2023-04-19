/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_WORKER_H_
#define SCHED_NR_WORKER_H_

#include "mac/sched_nr_cfg.h"
#include "mac/sched_nr_bwp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	// const params
    cell_config_manager     *cfg;
	// cc-specific resources
	cvector_vector_t(bwp_manager) bwps; //bounded_vector<bwp_manager, SCHED_NR_MAX_BWP_PER_CELL>
}cc_worker;

harq_softbuffer_pool *harq_buffer_pool_self(uint32_t cc);
void cc_worker_destoy(cc_worker *cc_w);
void cc_worker_init(cc_worker *cc_w, cell_config_manager *params);
void cc_worker_dl_rach_info(cc_worker *cc_w, rar_info_t *rar_info);

#ifdef __cplusplus
}
#endif

#endif
