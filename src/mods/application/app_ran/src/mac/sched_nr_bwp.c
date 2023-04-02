/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_bwp.h"
	
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-bwp"

///////////////////////////////ra_sched///////////////////////////////////////////
static void ra_sched_init(ra_sched *ra, bwp_params_t *bwp_cfg_) :
{
	  ra->bwp_cfg = bwp_cfg_;
}


///////////////////////////////bwp_manager///////////////////////////////////////
void bwp_manager_init(bwp_manager *bwp_m, bwp_params_t *bwp_cfg)
{
	bwp_m->cfg = bwp_cfg;
	ra_sched_init(&bwp_m->ra, bwp_cfg);
	si_sched_init(&bwp_m->si, bwp_cfg);
	bwp_res_grid_init(&bwp_m->grid, bwp_cfg);
}


