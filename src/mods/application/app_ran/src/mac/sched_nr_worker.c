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


void cc_worker_init(cc_worker *cc_w, cell_config_manager *params)
{
	cc_w->cfg = params;

	// idx0 for BWP-common
	for (uint32_t bwp_id = 0; bwp_id < 1; ++bwp_id) {
		bwp_manager_init(&cc_w->bwps[bwp_id], params->bwps[bwp_id]);
	}
	
	// Pre-allocate HARQs in common pool of softbuffers 在softbuffers的公共池中预分配HARQ
	harq_softbuffer_pool::get_instance().init_pool(cfg.nof_prb());

}


