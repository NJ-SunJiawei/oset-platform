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
			ASSERT_IF_NOT(pdcch->coreset_present[coreset_idx],
			              "Invalid mapping search space id=%d to coreset id=%d",
			              ss_id,
			              coreset_idx);
			bwp_cce_pos_list bwp_cce_pos_lt = {0};
			//根据rnti、ss等信息计算终端candidates的first CCE位置，每个ue所处的cce位置不同
			get_dci_locs(&pdcch->coreset[coreset_idx], &pdcch->search_space[i], rnti_, bwp_cce_pos_lt);
			//记录ss_id下每个slot的cce合集
			cvector_push_back(param->cce_positions_list, bwp_cce_pos_lt);
			//对应ss_id的cce_positions_list下标映射
			//ss_id = 0   param->cce_positions_list[0]  param->ss_id_to_cce_idx[0] = 0
			//ss_id = 1   param->cce_positions_list[1]  param->ss_id_to_cce_idx[1] = 1
			param->ss_id_to_cce_idx[ss_id] = cvector_size(param->cce_positions_list) - 1;				
		}
	}
}

static bwp_cce_pos_list cce_pos_list(ue_carrier_params_t *param, uint32_t search_id)
{
  return param->cce_positions_list[param->ss_id_to_cce_idx[search_id]];
}

pdcch_cce_pos_list ue_carrier_params_cce_pos_list(ue_carrier_params_t *param, uint32_t search_id, uint32_t slot_idx, uint32_t aggr_idx)
{
  if (cvector_size(param->cce_positions_list) > param->ss_id_to_cce_idx[search_id]) {
	bwp_cce_pos_list lst = cce_pos_list(param, search_id);
	return lst[slot_idx][aggr_idx];
  }
  return {0};
}


uint32_t ue_carrier_params_get_k1(ue_carrier_params_t *param, slot_point pdsch_slot)
{
  if (param->cfg_.phy_cfg.duplex.mode == SRSRAN_DUPLEX_MODE_TDD) {
	return param->cfg_.phy_cfg.harq_ack.dl_data_to_ul_ack[count_idx(&pdsch_slot) % param->cfg_.phy_cfg.duplex.tdd.pattern1.period_ms];
  }
  return param->cfg_.phy_cfg.harq_ack.dl_data_to_ul_ack[count_idx(&pdsch_slot) % param->cfg_.phy_cfg.harq_ack.nof_dl_data_to_ul_ack];
}


/// Get SearchSpace based on SearchSpaceId
srsran_search_space_t* ue_carrier_params_get_ss(ue_carrier_params_t *param, uint32_t ss_id)
{
	if (param->cfg_->phy_cfg.pdcch.search_space_present[ss_id]) {
		// UE-dedicated SearchSpace
		return &param->bwp_cfg->cfg.pdcch.search_space[ss_id];
	}
	return NULL;
}

static bool contains_dci_format(srsran_search_space_t *ss, srsran_dci_format_nr_t dci_fmt)
{
	for(int i = 0; i < ss->nof_formats; ++i){
		if(dci_fmt == ss->formats[i]){
			return true;
		}
	}
	return false;
}

int ue_carrier_params_find_ss_id(ue_carrier_params_t *param, srsran_dci_format_nr_t dci_fmt)
{
	static const uint32_t           aggr_idx  = 2;                  // TODO: Make it dynamic
	static const srsran_rnti_type_t rnti_type = srsran_rnti_type_c; // TODO: Use TC-RNTI for Msg4
	int i = 0;

	srsran_pdcch_cfg_nr_t *pdcch  = &param->cfg_->phy_cfg.pdcch;

	for (i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; ++i) {
		if(pdcch->search_space_present[i]){
			// Prioritize UE-dedicated SearchSpaces确定UE专用搜索空间的优先级
			srsran_search_space_t *ss = pdcch->search_space[i];
			if (ss->type == srsran_search_space_type_ue &&\
				ss->nof_candidates[aggr_idx] > 0 &&\
				contains_dci_format(ss, dci_fmt) &&\
				is_rnti_type_valid_in_search_space(rnti_type, ss->type)) {
				return ss->id;
			}
		}
	}

	// Search Common SearchSpaces
	for (i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; ++i) {
		if(pdcch->search_space_present[i]){
			srsran_search_space_t *ss = pdcch->search_space[i];
			if (SRSRAN_SEARCH_SPACE_IS_COMMON(ss.type) &&\
				ss->nof_candidates[aggr_idx] > 0 &&\
				contains_dci_format(ss, dci_fmt) &&\
				is_rnti_type_valid_in_search_space(rnti_type, ss->type)) {
				return ss->id;
			}
		}
	}

	return -1;
}


srsran_dci_cfg_nr_t ue_carrier_params_get_dci_cfg(ue_carrier_params_t *param)
{ 
	return param->cached_dci_cfg;
}

int ue_carrier_params_fixed_pdsch_mcs(ue_carrier_params_t *param)
{ 
	return param->bwp_cfg->sched_cfg.fixed_dl_mcs;
}

int ue_carrier_params_fixed_pusch_mcs(ue_carrier_params_t *param)
{
	return param->bwp_cfg->sched_cfg.fixed_ul_mcs;
}

phy_cfg_nr_t *ue_carrier_params_phy(ue_carrier_params_t *param)
{
	return &param->cfg_->phy_cfg;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/// Helper function to verify if RNTI type can be placed in specified search space
/// Based on 38.213, Section 10.1
bool is_rnti_type_valid_in_search_space(srsran_rnti_type_t rnti_type, srsran_search_space_type_t ss_type)
{
  switch (ss_type) {
    case srsran_search_space_type_common_0:  // fall-through
    case srsran_search_space_type_common_0A: // Other SIBs
      return rnti_type == srsran_rnti_type_si;
    case srsran_search_space_type_common_1:
      return rnti_type == srsran_rnti_type_ra || rnti_type == srsran_rnti_type_tc || rnti_type == srsran_rnti_type_c;
    case srsran_search_space_type_common_2:
      return rnti_type == srsran_rnti_type_p;
    case srsran_search_space_type_common_3:
      return rnti_type == srsran_rnti_type_c; // TODO: Fix
    case srsran_search_space_type_ue:
      return rnti_type == srsran_rnti_type_c || rnti_type == srsran_rnti_type_cs || rnti_type == srsran_rnti_type_sp_csi;
    default:
      break;
  }
  return false;
}

