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
	cvector_reserve(ue_cfg->carriers, 1);//SCHED_NR_MAX_CARRIERS
	ue_cfg->maxharq_tx = 4;
	ue_cfg->carriers[enb_cc_idx].active = true;
	ue_cfg->carriers[enb_cc_idx].cc     = enb_cc_idx;
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




	
	std::fill(ss_id_to_cce_idx.begin(), ss_id_to_cce_idx.end(), SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE);
	const auto& pdcch        = phy().pdcch;
	auto        ss_view      = srsran::make_optional_span(pdcch.search_space, pdcch.search_space_present);
	auto        coreset_view = srsran::make_optional_span(pdcch.coreset, pdcch.coreset_present);
	for (const auto& ss : ss_view) {
	srsran_assert(coreset_view.contains(ss.coreset_id),
	              "Invalid mapping search space id=%d to coreset id=%d",
	              ss.id,
	              ss.coreset_id);
	cce_positions_list.emplace_back();
	get_dci_locs(coreset_view[ss.coreset_id], ss, rnti, cce_positions_list.back());
	ss_id_to_cce_idx[ss.id] = cce_positions_list.size() - 1;
	}
}


