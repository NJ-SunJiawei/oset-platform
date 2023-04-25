/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/ue_cfg_manager.h"
//#include "lib/rrc/rrc_util.h"
#include "rrc/rrc.h"//tochange

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-uecfg"

void ue_cfg_manager_init(ue_cfg_manager *ue_cfg, uint32_t enb_cc_idx)
{
	ue_cfg->maxharq_tx = 4;
	//cvector_reserve(ue_cfg->carriers, 1);//SCHED_NR_MAX_CARRIERS
	sched_nr_ue_cc_cfg_t carrier = {0};
	carrier.active = true;
	carrier.cc     = enb_cc_idx;
	cvector_push_back(ue_cfg->carriers, carrier)
	ue_cfg->ue_bearers[0].direction     = (direction_t)BOTH;
}


int ue_cfg_manager_apply_config_request(ue_cfg_manager *ue_cfg, sched_nr_ue_cfg_t *cfg_req)
{
	ue_cfg->maxharq_tx = cfg_req->maxharq_tx;
	cvector_copy(cfg_req->carriers, ue_cfg->carriers);
	ue_cfg->phy_cfg    = cfg_req->phy_cfg;

	if (cfg_req->sp_cell_cfg) {
		make_pdsch_cfg_from_serv_cell(&cfg_req->sp_cell_cfg->sp_cell_cfg_ded, &ue_cfg->phy_cfg.pdsch);//cfg->bwps[0].pdsch;
		make_csi_cfg_from_serv_cell(&cfg_req->sp_cell_cfg->sp_cell_cfg_ded, &ue_cfg->phy_cfg.csi);
	}

	uint32_t *lcid = NULL;
	cvector_for_each_in(lcid, cfg_req->lc_ch_to_rem){
		ASSERT_IF_NOT(*lcid > 0 && "LCID=0 cannot be removed");
		ue_cfg->ue_bearers[*lcid] = {0};
	}

	sched_nr_ue_lc_ch_cfg_t *lc_ch = NULL;
	cvector_for_each_in(lc_ch, cfg_req->lc_ch_to_add){
		ASSERT_IF_NOT(lc_ch->lcid > 0 && "LCID=0 cannot be configured");
		ue_cfg->ue_bearers[lc_ch->lcid] = lc_ch->cfg;
	}

  	return OSET_OK;
}


///////////////////////////////////////////////////////////////////////////
void ue_carrier_params_init(ue_carrier_params_t *param, uint16_t rnti_, bwp_params_t *bwp_cfg_, ue_cfg_manager *uecfg_)
{
	param->rnti = rnti_;
	param->cc = bwp_cfg_->cc;
	param->cfg_ = uecfg_;
	param->bwp_cfg = bwp_cfg_;
	param->cached_dci_cfg = get_dci_cfg(&uecfg_->phy_cfg);
	memset(param->ss_id_to_cce_idx, 0, SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE*sizeof(uint32_t))

	srsran_pdcch_cfg_nr_t *pdcch  = &uecfg_->phy_cfg.pdcch;
	for (int i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; ++i) {
		if(pdcch->search_space_present[i]){
			uint32_t coreset_idx = pdcch->search_space[i].coreset_id;
			uint32_t ss_id = pdcch->search_space[i].id;
			ASSERT_IF_NOT(pdcch.coreset_present[coreset_idx],
			              "Invalid mapping search space id=%d to coreset id=%d",
			              ss_id,
			              coreset_idx);
			bwp_cce_pos_list bwp_cce_pos_lt = {0};
			//根据rnti、ss等信息计算终端candidates的first CCE位置？？？每个ue所处的cce位置不同
			get_dci_locs(pdcch->coreset[coreset_idx], pdcch->search_space[i], rnti_, bwp_cce_pos_lt);
			cvector_push_back(param->cce_positions_list, bwp_cce_pos_lt);
			param->ss_id_to_cce_idx[ss_id] = cvector_size(param->cce_positions_list) - 1;				
		}
	}
}


