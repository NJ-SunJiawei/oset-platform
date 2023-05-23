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

/// See TS 38.321, 5.1.3 - RAP transmission
int ra_sched_dl_rach_info(ra_sched *ra, rar_info_t *rar_info)
{
	// RA-RNTI = 1 + s_id + 14 × t_id + 14 × 80 × f_id + 14 × 80 × 8 × ul_carrier_id
	// s_id = index of the first OFDM symbol (0 <= s_id < 14)
	// t_id = index of first slot of the PRACH (0 <= t_id < 80)
	// f_id = index of the PRACH in the freq domain (0 <= f_id < 8) (for FDD, f_id=0)
	// ul_carrier_id = 0 for NUL and 1 for SUL carrier
	//根据时频信息gnb可以推算出ue的ra_rnti，用于msg2消息的加扰
	uint16_t ra_rnti = 1 + rar_info->ofdm_symbol_idx + 14 * slot_idx(rar_info->prach_slot) + 14 * 80 * rar_info->freq_idx;

	oset_info("SCHED: New PRACH slot=%d, preamble=%d, ra-rnti=0x%x, temp_crnti=0x%x, ta_cmd=%d, msg3_size=%d",
	          count_idx(&rar_info->prach_slot),
	          rar_info->preamble_idx,
	          ra_rnti,
	          rar_info->temp_crnti,
	          rar_info->ta_cmd,
	          rar_info->msg3_size);

	// find pending rar with same RA-RNTI
	// 时频相同，RA-RNTI相同的ra
	pending_rar_t *r = NULL;
	cvector_for_each_in(r, ra->pending_rars){
		if (slot_point_equal(&r->prach_slot, &rar_info->prach_slot) && r->ra_rnti == ra_rnti) {
			if (MAX_GRANTS == cvector_size(r->msg3_grant)) {
				//PRACH被忽略，因为已达到每个tti的最大RAR授权数
				oset_error("PRACH ignored, as the the maximum number of RAR grants per tti has been reached");
				return OSET_ERROR;
			}
			cvector_push_back(r->msg3_grant, *rar_info);
			return OSET_OK;
		}
	}

	// create new RAR
	pending_rar_t p = {0};
	p.ra_rnti                            = ra_rnti;
	p.prach_slot                         = rar_info->prach_slot;
	const static uint32_t prach_duration = 1;
	slot_point t_max = slot_point_add_jump(rar_info->prach_slot, cvector_size(ra->bwp_cfg->slots));
	slot_point t_init = slot_point_add_jump(rar_info->prach_slot, prach_duration);
	for (slot_point t = t_init; slot_point_less(&t, &t_max); slot_point_plus_plus(&t)) {
		if (ra->bwp_cfg->slots[slot_idx(t)].is_dl) {
			slot_interval_init(p.rar_win, t, slot_point_add_jump(t, ra->bwp_cfg->cfg.rar_window_size));//rar windows
			break;
		}
	}
	cvector_push_back(p.msg3_grant, *rar_info);
	cvector_push_back(ra->pending_rars, p);
	return OSET_OK;
}


///////////////////////////////ra_sched///////////////////////////////////////////
static void ra_sched_destory(ra_sched *ra)
{
	pending_rar_t *elem = NULL;

	cvector_for_each_in(elem, ra->pending_rars){
		cvector_free(elem->msg3_grant);
	}
	cvector_free(ra->pending_rars);
}

static void ra_sched_init(ra_sched *ra, bwp_params_t *bwp_cfg_)
{
	  ra->bwp_cfg = bwp_cfg_;
}

static alloc_result ra_sched_allocate_pending_rar(ra_sched *ra,
													bwp_slot_allocator *slot_grid,
													pending_rar_t      *rar,
													uint32_t           *nof_grants_allocs)
{
	const uint32_t rar_aggr_level = 2;
	slot_point     sl_pdcch = get_pdcch_tti(slot_grid);
	//bwp_cfg->cfg.pdcch.ra_search_space.id
	prb_bitmap	 *prbs = pdsch_allocator_occupied_prbs(bwp_res_grid_get_pdschs(slot_grid->bwp_grid, sl_pdcch),
														ra->bwp_cfg->cfg.pdcch.ra_search_space.id,
														srsran_dci_format_nr_1_0);

	alloc_result ret = (alloc_result)other_cause;
	span_t(dl_sched_rar_info_t) msg3_grants = {0};
	span(msg3_grants, rar->msg3_grant, cvector_size(rar->msg3_grant));

	//msg3从最大size开始申请资源，无法满足则减小zize
	for (uint32_t nof_grants_alloc = cvector_size(rar->msg3_grant); nof_grants_alloc > 0; nof_grants_alloc--) {
		ret = (alloc_result)invalid_coderate;
		uint32_t start_prb_idx = 0;

		//msg2 prb >= 4
		for (uint32_t nprb = 4; nprb < ra->bwp_cfg->cfg.rb_width && ret == (alloc_result)invalid_coderate; ++nprb) {
			prb_interval interv = find_empty_interval_of_length(prbs, nprb, start_prb_idx);
			start_prb_idx 	  = interv.start_;
			if (prb_interval_length(&interv) == nprb) {
				span_t(dl_sched_rar_info_t) msg3_grant_tmp = {0};
				subspan(msg3_grant_tmp, msg3_grants, 0, nof_grants_alloc);
				ret = bwp_slot_allocator_alloc_rar_and_msg3(slot_grid, rar->ra_rnti, rar_aggr_level, &interv, &msg3_grant_tmp);
			} else {
				ret = (alloc_result)no_sch_space;
			}
		}

		// If allocation was not successful because there were not enough RBGs, try allocating fewer Msg3 grants
		if (ret != (alloc_result)invalid_coderate && ret != (alloc_result)no_sch_space) {
			break;
		}
	}

	if (ret != (alloc_result)success) {
		oset_info("SCHED: RAR allocation for L=%d was postponed. Cause=%s", rar_aggr_level, to_string(ret));
	}

	*nof_grants_allocs = nof_grants_alloc;
	return ret;
}

void ra_sched_run_slot(bwp_slot_allocator *slot_alloc, ra_sched *ra)
{
	slot_point pdcch_slot = get_pdcch_tti(slot_alloc);
	slot_point msg3_slot  = pdcch_slot + ra->bwp_cfg->pusch_ra_list[0].msg3_delay;
	if (!ra->bwp_cfg->slots[slot_idx(&pdcch_slot)].is_dl || !ra->bwp_cfg->slots[slot_idx(&msg3_slot)].is_ul) {
		// RAR only allowed if PDCCH is available and respective Msg3 slot is available for UL
		return;
	}

	for (int i = 0; i < cvector_size(ra->pending_rars); ) { 
		pending_rar_t *rar = ra->pending_rars[i];

		// In case of RAR outside RAR window:
		// - if window has passed, discard RAR
		// - if window hasn't started, stop loop, as RARs are ordered by TTI
		if (!slot_interval_contains(&rar->rar_win, pdcch_slot)) {
			if (slot_point_greater_equal(&pdcch_slot, &rar->rar_win.stop_)) {
				oset_warn("SCHED: Could not transmit RAR within the window=[%u,%u], PRACH=%u, RAR=%u",
							count_idx(&rar->rar_win.start_),
							count_idx(&rar->rar_win.stop_),
							count_idx(&rar->prach_slot),
							count_idx(&pdcch_slot));
				cvector_erase(ra->pending_rars, i);
				continue;
			}
			return;
		}

		// Try to schedule DCIs + RBGs for RAR Grants
		// 既要分配msg2(rar)消息的下行资源，也要分配msg3的上行资源
		// 时频相同，RA-RNTI相同的ra, msg2消息共用，msg3消息申请cvector_size(rar->msg3_grant)
		uint32_t     nof_rar_allocs = 0;
		alloc_result ret            = ra_sched_allocate_pending_rar(slot_alloc, rar, &nof_rar_allocs);

		if (ret == (alloc_result)success) {
				// If RAR allocation was successful:
				// - in case all Msg3 grants were allocated, remove pending RAR, and continue with following RAR
				// - otherwise, erase only Msg3 grants that were allocated, and stop iteration
				if (nof_rar_allocs == cvector_size(rar->msg3_grant)) {
				cvector_erase(ra->pending_rars, i); //已分配prb，可以删除
			} else {
				std::copy(rar->msg3_grant.begin() + nof_rar_allocs, rar->msg3_grant.end(), rar->msg3_grant.begin());
				rar->msg3_grant.resize(cvector_size(rar->msg3_grant) - nof_rar_allocs);
				break;
			}
		} else {
			// If RAR allocation was not successful:
			// - in case of unavailable PDCCH space, try next pending RAR allocation
			// - otherwise, stop iteration
			if (ret != (alloc_result)no_cch_space) {
				break;
			}
			++i;
		}
	}
}


///////////////////////////////bwp_manager///////////////////////////////////////
void bwp_manager_destory(bwp_manager *bwp)
{
	//ra
	ra_sched_destory(&bwp->ra);
	//si
	si_sched_destory(&bwp->si);
	//bwp_res_grid
	bwp_res_grid_destory(bwp->grid);
}

void bwp_manager_init(bwp_manager *bwp, bwp_params_t *bwp_cfg)
{
	bwp->cfg = bwp_cfg;
	ra_sched_init(&bwp->ra, bwp_cfg);
	si_sched_init(&bwp->si, bwp_cfg);
	bwp_res_grid_init(&bwp->grid, bwp_cfg);
}


