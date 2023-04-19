/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-worker"

static harq_softbuffer_pool    g_harq_buffer_pool[SCHED_NR_MAX_CARRIERS];

harq_softbuffer_pool *harq_buffer_pool_self(uint32_t cc)
{
	return &g_harq_buffer_pool[cc];
}

void cc_worker_destoy(cc_worker *cc_w)
{
	// just idx0 for BWP-common
	bwp_manager *bwp = NULL;
	cvector_for_each_in(bwp, cc_w->bwps){
		bwp_manager_destory(bwp);
	}		
	//release harqbuffer
	harq_softbuffer_pool_destory(harq_buffer_pool_self(cc_w->cfg->cc), cc_w->cfg->carrier.nof_prb, 4 * MAX_HARQ, 0);

}

void cc_worker_init(cc_worker *cc_w, cell_config_manager *params)
{
	ASSERT_IF_NOT(cvector_size(params->bwps) > SCHED_NR_MAX_BWP_PER_CELL, "cc_worker_init error: MAX_BWP_PER_CELL > 2")

	cc_w->cfg = params;
	// Pre-allocate HARQs in common pool of softbuffers
	harq_softbuffer_pool_init(&g_harq_buffer_pool[params->cc], params->carrier.nof_prb, 4 * MAX_HARQ, 0);

	cvector_reserve(cc_w->bwps[bwp_id], SCHED_NR_MAX_BWP_PER_CELL);
	// idx0 for BWP-common
	for (uint32_t bwp_id = 0; bwp_id < cvector_size(params->bwps); ++bwp_id) {
		bwp_manager bwp = {0};
		bwp_manager_init(&bwp, &params->bwps[bwp_id]);
		cvector_push_back(cc_w->bwps[bwp_id], bwp)
	}
}

void cc_worker_dl_rach_info(cc_worker *cc_w, rar_info_t *rar_info)
{
	// idx0 for BWP-common
	bwp_manager *bwp = NULL;
	cvector_for_each_in(bwp, cc_w->bwps){
		ra_sched_dl_rach_info(&bwp->ra, rar_info);
	}
}

