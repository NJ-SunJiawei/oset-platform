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


void cc_worker_init(cc_worker *cc_w, cell_config_manager *params)
{
	cc_w->cfg = params;

	// Pre-allocate HARQs in common pool of softbuffers
	harq_softbuffer_pool_init(&g_harq_buffer_pool[params->cc], params->carrier.nof_prb, 4 * MAX_HARQ, 0);

	// idx0 for BWP-common
	for (uint32_t bwp_id = 0; bwp_id < 1; ++bwp_id) {
		bwp_manager_init(&cc_w->bwps[bwp_id], params->bwps[bwp_id]);
	}
}


