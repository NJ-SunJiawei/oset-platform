/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_WORKER_H_
#define SCHED_WORKER_H_

#include "mac/sched_cfg.h"
#include "mac/sched_ue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	// const params
    cell_config_manager     *cfg;
	// cc-specific resources
	cvector_vector_t(bwp_manager) bwps; //bounded_vector<bwp_manager, SCHED_NR_MAX_BWP_PER_CELL>
	// {slot,cc} specific variables
	OSET_POOL(slot_ue_pool, slot_ue);
	oset_hash_t     *slot_ues;//slot_ue
	cvector_vector_t(slot_ue *) slot_ue_list; 
	slot_point     last_tx_sl;
}cc_worker;

void cc_worker_destoy(cc_worker *cc_w);
void cc_worker_init(cc_worker *cc_w, cell_config_manager *params);
void cc_worker_dl_rach_info(cc_worker *cc_w, rar_info_t *rar_info);
dl_res_t* cc_worker_run_slot(cc_worker *cc_w, slot_point tx_sl, oset_list_t *ue_db);
ul_sched_t* cc_worker_get_ul_sched(cc_worker *cc_w, slot_point rx_sl);


#ifdef __cplusplus
}
#endif

#endif
