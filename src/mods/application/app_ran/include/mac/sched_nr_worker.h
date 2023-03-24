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
	bwp_manager bwps[SCHED_NR_MAX_BWP_PER_CELL]; //bounded_vector<bwp_manager, SCHED_NR_MAX_BWP_PER_CELL>
}cc_worker;

void cc_worker_init(cc_worker *cc_w, cell_config_manager *params);

#ifdef __cplusplus
}
#endif

#endif
