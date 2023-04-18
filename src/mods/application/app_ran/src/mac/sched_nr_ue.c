/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_ue.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-ue"

//////////////////////////////////////////pdu_builder////////////////////////////////////////////

static void pdu_builder_init(pdu_builder        *pdu_builders, uint32_t cc_, ue_buffer_manager *parent_)
{
	pdu_builders->cc = cc_;
	pdu_builders->parent = parent_;
}


//////////////////////////////////////////ue_carrier////////////////////////////////////////////
static void ue_carrier_destory(ue_carrier *carrier)
{
	cvector_free(carrier->bwp_cfg.cce_positions_list);
	oset_free(carrier);
}


static ue_carrier* ue_carrier_init(sched_nr_ue       *u, uint32_t cc)
{
	osset_assert(u);
	ue_cfg_manager		*uecfg_ = &u->ue_cfg;
	cell_config_manager *cell_params_ = &u->sched_cfg->cells[cc];

	ue_carrier *carrier = oset_malloc(sizeof(ue_carrier));
	osset_assert(carrier);
	carrier->rnti = u->rnti;
	carrier->cc = cc;
	ue_carrier_params_init(&carrier->bwp_cfg, u->rnti, &cell_params_->bwps[0], uecfg_);
	carrier->cell_params = &u->sched_cfg->cells[cc];
	pdu_builder_init(&carrier->pdu_builders, cc, &u->buffers);
	carrier->common_ctxt = &u->common_ctxt;
	return carrier;
}


//////////////////////////////////////////sched_nr_ue////////////////////////////////////////////
static void free_rach_ue_cfg_helper(sched_nr_ue_cfg_t *uecfg)
{
	cvector_free(uecfg->carriers);
	cvector_free(uecfg->lc_ch_to_add);
	cvector_free(uecfg->lc_ch_to_rem);
	oset_free(uecfg);
}

static sched_nr_ue_cfg_t* get_rach_ue_cfg_helper(uint32_t cc, sched_params_t *sched_params)
{
	sched_nr_ue_cfg_t *uecfg = oset_malloc(sizeof(sched_nr_ue_cfg_t));
	uecfg->maxharq_tx = 4;
	//cvector_reserve(uecfg->carriers, 1);
	sched_nr_ue_cc_cfg_t carrier = {0};
	carrier.active = true;
	carrier.cc     = cc;
	cvector_push_back(uecfg->carriers, carrier)
	uecfg->phy_cfg            = sched_params->cells[cc].default_ue_phy_cfg;
	return uecfg;
}

static void sched_nr_ue_set_cfg(sched_nr_ue *u, sched_nr_ue_cfg_t *cfg)
{
	ue_cfg_manager_apply_config_request(&u->ue_cfg, cfg);

	sched_nr_ue_cc_cfg_t *ue_cc_cfg = NULL;
	cvector_for_each_in(ue_cc_cfg, cfg->carriers){
		if (ue_cc_cfg->active) {
		  if (u->carriers[ue_cc_cfg->cc] == nullptr) {
		  	u->carriers[ue_cc_cfg->cc] = ue_carrier_init(u, ue_cc_cfg->cc);
		  } else {
			  //rrc传递下来明确coreset和searchspace资源，非coreset0和searchspcae0
		    ue_carrier  *carrier = u->carriers[ue_cc_cfg->cc];
			ue_carrier_params_init(&carrier->bwp_cfg, carrier->rnti, &carrier->cell_params->bwps[0], &u->ue_cfg);
		  }
		}
	}

	//没使用令牌桶，只是初始化相关逻辑信道
	base_ue_buffer_manager_config_lcids(&u->buffers.base_ue ,u->ue_cfg.ue_bearers);
}

void sched_nr_ue_remove(sched_nr_ue *u)
{
	//todo
	sched_nr_ue_cc_cfg_t *ue_cc_cfg = NULL;
	cvector_for_each_in(ue_cc_cfg, u->ue_cfg.carriers){
		ue_carrier_destory(u->carriers[ue_cc_cfg->cc]);
	}
	cvector_free(u->ue_cfg.carriers);

	oset_pool_free(&mac_manager_self()->sched.ue_pool, u);
}


sched_nr_ue *sched_nr_ue_add(uint16_t rnti_, uint32_t cc, sched_params_t *sched_cfg_)
{
	sched_nr_ue *u = NULL; 

	// create user object outside of sched main thread
	oset_pool_alloc(&mac_manager_self()->sched.ue_pool, &u);
    if (u == NULL) {
        oset_error("Could not allocate sched_nr_ue context from pool");
        return NULL;
    }

	sched_nr_ue_cfg_t *uecfg = get_rach_ue_cfg_helper(cc, sched_cfg_);

	u->rnti = rnti_;
	u->sched_cfg = sched_cfg_;
	base_ue_buffer_manager_init(&u->buffers.base_ue, rnti_);
	ue_cfg_manager_init(&u->ue_cfg, cc);
	sched_nr_ue_set_cfg(u, uecfg);//create carriers[cc]

	free_rach_ue_cfg_helper(uecfg);
	return u;
}

