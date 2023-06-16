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

srsran_search_space_t* get_ss(bwp_params_t *param, uint32_t ss_id)
{
  return param->cfg.pdcch.search_space_present[ss_id] ? &param->cfg.pdcch.search_space[ss_id] : NULL;
}


void get_dci_locs(srsran_coreset_t      *coreset,
                      srsran_search_space_t *search_space,
                      uint16_t             rnti,
                      pdcch_cce_pos_list  (*cce_locs)[MAX_NOF_AGGR_LEVELS])
{
	for (uint32_t sl = 0; sl < SRSRAN_NOF_SF_X_FRAME; ++sl) {
		for (uint32_t agg_idx = 0; agg_idx < MAX_NOF_AGGR_LEVELS; ++agg_idx) {
		  uint32_t n = srsran_pdcch_nr_locations_coreset(coreset, search_space, rnti, agg_idx, sl, cce_locs[sl][agg_idx].cce_addr);
		  cce_locs[sl][agg_idx].cce_index = n;
		}
	}
}

prb_interval* coreset_prb_range(bwp_params_t *param, uint32_t cs_id)
{
	return &param->coresets[cs_id].prb_limits;
}

bwp_rb_bitmap* dci_fmt_1_0_excluded_prbs(bwp_params_t *param, uint32_t cs_id)
{
	return &param->coresets[cs_id].usable_common_ss_prb_mask;
}

static void bwp_params_destory(bwp_params_t *cell_bwp)
{
	cvector_free(cell_bwp->slots);
	cvector_free(cell_bwp->pusch_ra_list);
}

//bwp_cfg from rrc configure
static void bwp_params_init(bwp_params_t *cell_bwp, uint32_t bwp_id_, sched_nr_bwp_cfg_t *bwp_cfg)
{
	cell_config_manager  *cell_cfg = cell_bwp->cell_cfg;

	cell_bwp->bwp_id = bwp_id_;
	cell_bwp->cfg = *bwp_cfg;
	//cell_bwp->bwp_cfg = *bwp_cfg;//???
	cell_bwp->nof_prb = cell_cfg->carrier.nof_prb;
	bwp_rb_bitmap_init(&cell_bwp->cached_empty_prb_mask, bwp_cfg->rb_width, bwp_cfg->start_rb, bwp_cfg->pdsch.rbg_size_cfg_1);

	ASSERT_IF_NOT(bwp_cfg->pdcch.ra_search_space_present, "BWPs without RA search space not supported");
	const uint32_t ra_coreset_id = bwp_cfg->pdcch.ra_search_space.coreset_id; //coreset_id 0

	cell_bwp->P     = get_P(bwp_cfg->rb_width, bwp_cfg->pdsch.rbg_size_cfg_1);
	cell_bwp->N_rbg = get_nof_rbgs(bwp_cfg->rb_width, bwp_cfg->start_rb, bwp_cfg->pdsch.rbg_size_cfg_1);

	for(int i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++i){
		if(!bwp_cfg->pdcch.coreset_present[i]){
			/// Get a range of active coresets in a PDCCH configuration
			continue;
		}
		srsran_coreset_t *cs = bwp_cfg->pdcch.coreset[i];

		// [cs_id = 0, f[0] = 1, offset = 1] RB start = 1, RB end = 49, rb_width = 52
		// [cs_id = 1, f[0] = 1, offset = 0] RB start = 0, RB end = 48, rb_width = 52
		// [cs_id = 2, f[0] = 1, offset = 0] RB start = 0, RB end = 48, rb_width = 52

		uint32_t rb_start = srsran_coreset_start_rb(cs);
		cell_bwp->coresets[cs->id].active  = true;
		prb_interval_init(&cell_bwp->coresets[cs->id].prb_limits, rb_start, rb_start + srsran_coreset_get_bw(cs));
		cell_bwp->coresets[cs->id].usable_common_ss_prb_mask = cell_bwp->cached_empty_prb_mask;

		// TS 38.214, 5.1.2.2 - For DCI format 1_0 and common search space, lowest RB of the CORESET is the RB index = 0
		// cached_empty_prb_mask     [0~~~~~~~~~e]
		// usable_common_ss_prb_mask [0~s        ]
		// dci_1_0_prb_limits        [  s~~~~~~~e]
		// prb_limits                [  s~~~~s   ]
		prb_interval interval = {0};
		prb_interval_init(&interval, 0, rb_start);
		bwp_rb_bitmap_add_by_prb_interval(cell_bwp->coresets[cs->id].usable_common_ss_prb_mask, &interval);
		prb_interval_init(&cell_bwp->coresets[cs->id].dci_1_0_prb_limits, rb_start, bwp_cfg->rb_width);//bwp_cfg->rb_width从0开始涵盖整个bwp区间

		// TS 38.214, 5.1.2.2.2 - when DCI format 1_0, common search space and CORESET#0 is configured for the cell,
		// RA type 1 allocs shall be within the CORESET#0 region
		// cached_empty_prb_mask     [0~~~~~~~~~e]
		// usable_common_ss_prb_mask [0~s     s~e]
		// dci_1_0_prb_limits        [  s~~~~~s  ]
		// prb_limits                [  s~~~~~s  ]
		if (bwp_cfg->pdcch.coreset_present[0]) {
		  cell_bwp->coresets[cs->id].dci_1_0_prb_limits =  cell_bwp->coresets[cs->id].prb_limits;
		  prb_interval interval = {0};
		  prb_interval_init(&interval, cell_bwp->coresets[cs.id].prb_limits.stop_, bwp_cfg->rb_width);
		  bwp_rb_bitmap_add_by_prb_interval(cell_bwp->coresets[cs->id].usable_common_ss_prb_mask, &interval);////???todo
		}
	}

	// Derive params of individual slots
	uint32_t nof_slots = SRSRAN_NSLOTS_PER_FRAME_NR(bwp_cfg->numerology_idx);
	for (size_t sl = 0; sl < nof_slots; ++sl) {
		slot_cfg sl_cfg = {0};
		sl_cfg.is_dl = srsran_duplex_nr_is_dl(&cell_cfg->duplex, bwp_cfg->numerology_idx, sl);
		sl_cfg.is_ul = srsran_duplex_nr_is_ul(&cell_cfg->duplex, bwp_cfg->numerology_idx, sl);
		cvector_push_back(cell_bwp->slots, sl_cfg);
	}

	cvector_reserve(cell_bwp->pusch_ra_list, bwp_cfg->pusch.nof_common_time_ra);
	srsran_sch_grant_nr_t grant = {0};
	for (uint32_t m = 0; m < bwp_cfg->pusch.nof_common_time_ra; ++m) {
		pusch_ra_time_cfg pusch_ra_time = {0};
		int ret = srsran_ra_ul_nr_time(&bwp_cfg->pusch, srsran_rnti_type_ra, srsran_search_space_type_rar, ra_coreset_id, m, &grant);
		ASSERT_IF_NOT(ret == SRSRAN_SUCCESS, "Failed to obtain");
		pusch_ra_time.msg3_delay = grant.k;//get form pusch-configCommon
		ret = srsran_ra_ul_nr_time(&bwp_cfg->pusch, srsran_rnti_type_c, srsran_search_space_type_ue, ra_coreset_id, m, &grant);
		pusch_ra_time.K = grant.k;
		pusch_ra_time.S = grant.S;
		pusch_ra_time.L = grant.L;
		ASSERT_IF_NOT(ret == SRSRAN_SUCCESS, "Failed to obtain RA config");
		cvector_push_back(cell_bwp->pusch_ra_list, pusch_ra_time)
	}
	ASSERT_IF_NOT(cvector_size(cell_bwp->pusch_ra_list) > 0, "Time-Domain Resource Allocation not valid");

	//计算coreset里cce结构计算
	for (uint32_t sl = 0; sl < SRSRAN_NOF_SF_X_FRAME; ++sl) {
		for (uint32_t agg_idx = 0; agg_idx < MAX_NOF_AGGR_LEVELS; ++agg_idx) {
		  // 计算在slot_idx下，聚合等级为agg_idx时，dci可能的起始位置
		  int n = srsran_pdcch_nr_locations_coreset(&cell_bwp->cfg.pdcch.coreset[ra_coreset_id],
		                                            &cell_bwp->cfg.pdcch.ra_search_space,
		                                            0,
		                                            agg_idx,
		                                            sl,
		                                            cell_bwp->rar_cce_list[sl][agg_idx].cce_addr);
		  ASSERT_IF_NOT(n >= 0, "Failed to configure RAR DCI locations");
		  cell_bwp->rar_cce_list[sl][agg_idx].cce_index = n;
		}
	}

	for (uint32_t ss_id = 0; ss_id < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; ++ss_id) {
		if (!cell_bwp->cfg.pdcch.search_space_present[ss_id]) {
		  continue;
		}
		srsran_search_space_t *ss = cell_bwp->cfg.pdcch.search_space[ss_id];
		bwp_cce_pos_list ss_cce_list = {0};
		for (uint32_t sl = 0; sl < SRSRAN_NOF_SF_X_FRAME; ++sl) {
		  for (uint32_t agg_idx = 0; agg_idx < MAX_NOF_AGGR_LEVELS; ++agg_idx) {
		    int n = srsran_pdcch_nr_locations_coreset(&cell_bwp->cfg.pdcch.coreset[ss->coreset_id], 
													&ss,
													SRSRAN_SIRNTI,
													agg_idx,
													sl,
													ss_cce_list[sl][agg_idx].cce_addr);
		    ASSERT_IF_NOT(n >= 0, "Failed to configure DCI locations of search space id=%d", ss_id);
		    ss_cce_list[sl][agg_idx].cce_index = n;
		  }
		}
		cell_bwp->common_cce_list[ss_id] = ss_cce_list;
	}
}

void cell_config_manager_destory(cell_config_manager *cell_cof_manager)
{	
	cvector_free(cell_cof_manager->sibs);
	// just idx0 for BWP-common
	bwp_params_t *bwp = NULL;
	cvector_for_each_in(bwp, cell_cof_manager->bwps){
		bwp_params_destory(bwp);
	}
	cvector_free(cell_cof_manager->bwps);
}

void cell_config_manager_init(cell_config_manager *cell_cof_manager,
										uint32_t cc_,
										sched_nr_cell_cfg_t *cell,
										sched_args_t *sched_args_)
{
	cell_cof_manager->cc = cc_;
	cell_cof_manager->sched_args = sched_args_;
	cell_cof_manager->default_ue_phy_cfg = get_common_ue_phy_cfg(cell);
	cell_cof_manager->sibs = cell->sibs;

	cell_cof_manager->carrier.pci                    = cell->pci;
	cell_cof_manager->carrier.dl_center_frequency_hz = cell->dl_center_frequency_hz;
	cell_cof_manager->carrier.ul_center_frequency_hz = cell->ul_center_frequency_hz;
	cell_cof_manager->carrier.ssb_center_freq_hz     = cell->ssb_center_freq_hz;
	cell_cof_manager->carrier.nof_prb                = cell->dl_cell_nof_prb;
	cell_cof_manager->carrier.start                  = 0; // TODO: Check
	cell_cof_manager->carrier.max_mimo_layers        = cell->nof_layers;
	cell_cof_manager->carrier.offset_to_carrier      = cell->dl_cfg_common.freq_info_dl.scs_specific_carrier_list[0].offset_to_carrier;
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

	cvector_reserve(cell_cof_manager->bwps, cvector_size(cell->bwps));
	// just idx0 for BWP-common
	for (uint32_t i = 0; i < cvector_size(cell->bwps); ++i) {
		bwp_params_t bwp_params = {0};
		bwp_params.cell_cfg = cell_cof_manager;
		bwp_params.sched_cfg = sched_args_;
		bwp_params.cc = cell_cof_manager->cc;
		bwp_params_init(&bwp_params, i, &cell->bwps[i]);
		cvector_push_back(cell_cof_manager->bwps, bwp_params)
	}
	ASSERT_IF_NOT(!cvector_empty(cell_cof_manager->bwps), "No BWPs were configured");

}

