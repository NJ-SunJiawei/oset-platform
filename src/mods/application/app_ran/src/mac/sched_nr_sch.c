/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_sch.h"
			
			
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-sch"

alloc_result pdsch_allocator_is_grant_valid_common(pdsch_allocator *pdsch_alloc, 
																	srsran_search_space_type_t ss_type,
																	srsran_dci_format_nr_t     dci_fmt,
																	uint32_t                   coreset_id,
																	prb_grant                  *grant)
{
	// DL must be active in given slot
	if (!pdsch_alloc->bwp_cfg.slots[pdsch_alloc->slot_idx].is_dl) {
		oset_error("DL is disabled for slot=%lu", pdsch_alloc->slot_idx);
		return (alloc_result)no_sch_space;
	}

	// No space in Scheduler PDSCH output list
	if (MAX_GRANTS == cvector_size(pdsch_alloc->pdschs)) {
		oset_error("Maximum number of PDSCHs={} reached", cvector_size(pdsch_alloc->pdschs));
		return (alloc_result)no_sch_space;
	}

	// TS 38.214, 5.1.2.2 - "The UE shall assume that when the scheduling grant is received with DCI format 1_0, then
	//                       downlink resource allocation type 1 is used."
	if (dci_fmt == srsran_dci_format_nr_1_0 && !is_alloc_type1(grant)) {
		oset_error("DL Resource Allocation type 1 must be used in case of DCI format 1_0");
		return (alloc_result)invalid_grant_params;
	}

	// TS 38.214 - 5.1.2.2 - For DCI format 1_0 and Common Search Space, the list of available PRBs is limited by the
	//                       rb_start and bandwidth of the coreset
	if (dci_fmt == srsran_dci_format_nr_1_0 && SRSRAN_SEARCH_SPACE_IS_COMMON(ss_type)) {
		// Grant PRBs do not collide with CORESET PRB limits (in case of common SearchSpace)
		// 是否越界
		if (prb_grant_collides(dci_fmt_1_0_excluded_prbs(pdsch_alloc->bwp_cfg, coreset_id), grant)) {
			oset_error("Provided PRB grant={%lu, %lu} falls outside common CORESET PRB boundaries", grant->alloc.interv.start_, grant->alloc.interv.stop_);
			return (alloc_result)sch_collision;
		}
	}

	// Grant PRBs do not collide with previous PDSCH allocations
	// 检查授权的区间未被使用
	if (prb_grant_collides(pdsch_alloc->dl_prbs, grant)) {
		oset_error("Provided PRB grant={%lu, %lu} collides with allocations previously made", grant->alloc.interv.start_, grant->alloc.interv.stop_);
		return (alloc_result)sch_collision;
	}

	return (alloc_result)success;
}


alloc_result pdsch_allocator_is_si_grant_valid(pdsch_allocator *pdsch_alloc, uint32_t ss_id, prb_grant *grant)
{
	// Verify SearchSpace validity
	const srsran_search_space_t* ss = get_ss(pdsch_alloc->bwp_cfg, ss_id);
	if (ss == NULL) {
		// Couldn't find SearchSpace
		oset_error("SearchSpace has not been configured");
		return (alloc_result)invalid_grant_params;
	}
	return pdsch_allocator_is_grant_valid_common(pdsch_alloc, ss->type, srsran_dci_format_nr_1_0, ss->coreset_id, grant);
}


/// Marks a range of PRBS as occupied, preventing further allocations
void pdsch_allocator_reserve_prbs(pdsch_allocator *pdsch_alloc, prb_grant *grant)
{
	bwp_rb_bitmap_add_by_prb_grant(&pdsch_alloc->dl_prbs, grant);
}


/// Get available PRBs for allocation
prb_bitmap pdsch_allocator_occupied_prbs(pdsch_allocator *pdsch_alloc, uint32_t ss_id, srsran_dci_format_nr_t dci_fmt)
{
  if (dci_fmt == srsran_dci_format_nr_1_0) {
	const srsran_search_space_t *ss = get_ss(pdsch_alloc->bwp_cfg, ss_id);
	if (ss != NULL && SRSRAN_SEARCH_SPACE_IS_COMMON(ss->type)) {
		//剔除coreset dci 不可用区域
		bwp_rb_bitmap_add_by_prb_bitmap(&pdsch_alloc->dl_prbs, get_prbs(dci_fmt_1_0_excluded_prbs(pdsch_alloc->bwp_cfg, ss->coreset_id)));
	}
  }
  return get_prbs(&pdsch_alloc->dl_prbs);
}

void pdsch_allocator_reset(pdsch_allocator *pdsch)
{
	cvector_clear(pdsch->pdschs);
	bit_reset_all(&pdsch->dl_prbs.prbs_);
	bit_reset_all(&pdsch->dl_prbs.rbgs_);
}


void pdsch_allocator_init(pdsch_allocator *pdsch, bwp_params_t *cfg_, uint32_t slot_index, cvector_vector_t(pdsch_t) pdsch_lst)
{
	pdsch->bwp_cfg = cfg_;
	pdsch->slot_idx = slot_index;
	pdsch->pdschs = pdsch_lst;
	//dl_prbs
	bwp_rb_bitmap_init(&pdsch->dl_prbs, cfg_->cfg.rb_width, cfg_->cfg.start_rb, cfg_->cfg.pdsch.rbg_size_cfg_1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void pusch_allocator_reset(pusch_allocator *pusch)
{
	cvector_clear(pusch->puschs);
	bit_reset_all(&pusch->ul_prbs.prbs_);
	bit_reset_all(&pusch->ul_prbs.rbgs_);
}

void pusch_allocator_init(pusch_allocator *pusch, bwp_params_t *cfg_, uint32_t slot_index,  cvector_vector_t(pusch_t) pusch_lst)
{
	pusch->bwp_cfg = cfg_;
	pusch->slot_idx = slot_idx;
	pusch->puschs = pusch_lst;
	//ul_prbs
	bwp_rb_bitmap_init(&pusch->ul_prbs, cfg_->cfg.rb_width, cfg_->cfg.start_rb, cfg_->cfg.pdsch.rbg_size_cfg_1);
}

