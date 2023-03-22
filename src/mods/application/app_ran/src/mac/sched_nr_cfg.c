/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_cfg.h"
#include "lib/mac/sched_nr_util.h"	
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-cfg"

//bwp_cfg from rrc configure
void bwp_params_init(bwp_params_t *cell_bwp, uint32_t bwp_id_, sched_nr_bwp_cfg_t *bwp_cfg)
{
	cell_config_manager  *cell_cfg = cell_bwp->cell_cfg;

	cell_bwp->bwp_id = bwp_id_;
	cell_bwp->cfg = bwp_cfg;
	cell_bwp->nof_prb = cell_cfg->carrier.nof_prb;
	bwp_rb_bitmap_init(&cell_bwp->cached_empty_prb_mask, bwp_cfg->rb_width, bwp_cfg->start_rb, bwp_cfg->pdsch.rbg_size_cfg_1);

	ASSERT_IF_NOT(bwp_cfg->pdcch.ra_search_space_present, "BWPs without RA search space not supported");
	const uint32_t ra_coreset_id = bwp_cfg->pdcch.ra_search_space.coreset_id;

	P     = get_P(bwp_cfg->rb_width, bwp_cfg->pdsch.rbg_size_cfg_1);
	N_rbg = get_nof_rbgs(bwp_cfg->rb_width, bwp_cfg->start_rb, bwp_cfg->pdsch.rbg_size_cfg_1);

	for (const srsran_coreset_t& cs : view_active_coresets(bwp_cfg->pdcch)) {
	coresets.emplace(cs.id);
	uint32_t rb_start                         = srsran_coreset_start_rb(&cs);
	coresets[cs.id].prb_limits                = prb_interval{rb_start, rb_start + srsran_coreset_get_bw(&cs)};
	coresets[cs.id].usable_common_ss_prb_mask = cached_empty_prb_mask;

	// TS 38.214, 5.1.2.2 - For DCI format 1_0 and common search space, lowest RB of the CORESET is the RB index = 0
	//对于DCI格式1_0和公共搜索空间，CORESET的最低RB是RB索引=0
	coresets[cs.id].usable_common_ss_prb_mask |= prb_interval(0, rb_start);
	coresets[cs.id].dci_1_0_prb_limits = prb_interval{rb_start, bwp_cfg->rb_width};

	// TS 38.214, 5.1.2.2.2 - when DCI format 1_0, common search space and CORESET#0 is configured for the cell,
	// RA type 1 allocs shall be within the CORESET#0 region
	//TS 38.214，5.1.2.2.2-当为小区配置DCI格式1_0、公共搜索空间和CORESET#0时，
	//RA 1型分配应在CORESET#0区域内
	if (bwp_cfg->pdcch.coreset_present[0]) {
	  coresets[cs.id].dci_1_0_prb_limits = coresets[cs.id].prb_limits;
	  coresets[cs.id].usable_common_ss_prb_mask |= prb_interval(coresets[cs.id].prb_limits.stop(), bwp_cfg->rb_width);
	}
	}

	// Derive params of individual slots
	uint32_t nof_slots = SRSRAN_NSLOTS_PER_FRAME_NR(bwp_cfg->numerology_idx);
	for (size_t sl = 0; sl < nof_slots; ++sl) {
		slot_cfg sl_cfg = {0};
		sl_cfg.is_dl = srsran_duplex_nr_is_dl(&cell_cfg->duplex, bwp_cfg->numerology_idx, sl);
		sl_cfg.is_ul = srsran_duplex_nr_is_ul(&cell_cfg->duplex, bwp_cfg->numerology_idx, sl);
		slots.push_back(sl_cfg);
	}

	pusch_ra_list.resize(bwp_cfg->pusch.nof_common_time_ra);
	srsran_sch_grant_nr_t grant;
	for (uint32_t m = 0; m < bwp_cfg->pusch.nof_common_time_ra; ++m) {
	int ret =
	    srsran_ra_ul_nr_time(&bwp_cfg->pusch, srsran_rnti_type_ra, srsran_search_space_type_rar, ra_coreset_id, m, &grant);
	srsran_assert(ret == SRSRAN_SUCCESS, "Failed to obtain  ");
	pusch_ra_list[m].msg3_delay = grant.k;
	ret = srsran_ra_ul_nr_time(&bwp_cfg->pusch, srsran_rnti_type_c, srsran_search_space_type_ue, ra_coreset_id, m, &grant);
	pusch_ra_list[m].K = grant.k;
	pusch_ra_list[m].S = grant.S;
	pusch_ra_list[m].L = grant.L;
	srsran_assert(ret == SRSRAN_SUCCESS, "Failed to obtain RA config");
	}
	srsran_assert(not pusch_ra_list.empty(), "Time-Domain Resource Allocation not valid");

	//计算coreset里cce结构计算

	for (uint32_t sl = 0; sl < SRSRAN_NOF_SF_X_FRAME; ++sl) {
	for (uint32_t agg_idx = 0; agg_idx < MAX_NOF_AGGR_LEVELS; ++agg_idx) {
	  rar_cce_list[sl][agg_idx].resize(SRSRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR);
	  int n = srsran_pdcch_nr_locations_coreset(&cell_cfg->bwps[0].cfg.pdcch.coreset[ra_coreset_id],
	                                            &cell_cfg->bwps[0].cfg.pdcch.ra_search_space,
	                                            0,
	                                            agg_idx,
	                                            sl,
	                                            rar_cce_list[sl][agg_idx].data());
	  srsran_assert(n >= 0, "Failed to configure RAR DCI locations");
	  rar_cce_list[sl][agg_idx].resize(n);
	}
	}

	for (uint32_t ss_id = 0; ss_id < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; ++ss_id) {
		if (not cell_cfg->bwps[0].cfg.pdcch.search_space_present[ss_id]) {
		  continue;
		}
		srsran_search_space_t *ss = cell_cfg->bwps[0].cfg.pdcch.search_space[ss_id];
		auto& coreset = cell_cfg->bwps[0].cfg.pdcch.coreset[ss->coreset_id];
		common_cce_list.emplace(ss_id);
		bwp_cce_pos_list& ss_cce_list = common_cce_list[ss_id];
		for (uint32_t sl = 0; sl < SRSRAN_NOF_SF_X_FRAME; ++sl) {
		  for (uint32_t agg_idx = 0; agg_idx < MAX_NOF_AGGR_LEVELS; ++agg_idx) {
		    ss_cce_list[sl][agg_idx].resize(SRSRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR);
		    int n = srsran_pdcch_nr_locations_coreset(
		        &coreset, &ss, SRSRAN_SIRNTI, agg_idx, sl, ss_cce_list[sl][agg_idx].data());
		    ASSERT_IF_NOT(n >= 0, "Failed to configure DCI locations of search space id=%d", ss_id);
		    ss_cce_list[sl][agg_idx].resize(n);
		  }
		}
	}
}


void cell_config_manager_init(cell_config_manager *cell_cof_manager,
										uint32_t cc_,
										sched_nr_cell_cfg_t *cell,
										sched_args_t *sched_args_)
{
	cell_cof_manager->cc = cc_;
	cell_cof_manager->default_ue_phy_cfg = get_common_ue_phy_cfg(cell);
	cell_cof_manager->sibs = &cell->sibs;

	cell_cof_manager->carrier.pci                    = cell->pci;
	cell_cof_manager->carrier.dl_center_frequency_hz = cell->dl_center_frequency_hz;
	cell_cof_manager->carrier.ul_center_frequency_hz = cell->ul_center_frequency_hz;
	cell_cof_manager->carrier.ssb_center_freq_hz     = cell->ssb_center_freq_hz;
	cell_cof_manager->carrier.nof_prb                = cell->dl_cell_nof_prb;
	cell_cof_manager->carrier.start                  = 0; // TODO: Check
	cell_cof_manager->carrier.max_mimo_layers        = cell->nof_layers;
	cell_cof_manager->carrier.offset_to_carrier      = byn_array_get_data(&cell->dl_cfg_common.freq_info_dl.scs_specific_carrier_list, 0)->offset_to_carrier;
	cell_cof_manager->carrier.scs = (srsran_subcarrier_spacing_t)cell->dl_cfg_common.init_dl_bwp.generic_params.subcarrier_spacing;

	// TDD-UL-DL-ConfigCommon
	cell_cof_manager->duplex.mode = SRSRAN_DUPLEX_MODE_FDD;//fdd模式，暂不支持tdd
	if (cell->tdd_ul_dl_cfg_common) {
		bool success = make_phy_tdd_cfg(cell->tdd_ul_dl_cfg_common, &cell_cof_manager->duplex);
		ASSERT_IF_NOT(success, "Failed to generate Cell TDD config");
	}

	// Set SSB params
	make_ssb_cfg(cell, &cell_cof_manager->ssb);

	// MIB
	make_mib_cfg(cell, &cell_cof_manager->mib);

	cell_cof_manager->bwps[4] = {0};
	// just idx0 for BWP-common
	for (uint32_t i = 0; i < 1; ++i) {
		cell_cof_manager->bwps[i].cell_cfg = cell_cof_manager;
		cell_cof_manager->bwps[i].sched_cfg = sched_args_;
		cell_cof_manager->bwps[i].cc = cell_cof_manager->cc;
		bwp_params_init(&cell_cof_manager->bwps[i], i, cell->bwps[0]);
	}
	ASSERT_IF_NOT(!cell_cof_manager->bwps.empty(), "No BWPs were configured");
}

