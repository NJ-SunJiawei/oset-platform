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

static const  char *log_pdsch = "SCHED: Failure to allocate PDSCH. Cause: ";

pdsch_t* pdsch_allocator_alloc_pdsch_unchecked(pdsch_allocator *pdsch_alloc,
														uint32_t                   coreset_id,
														srsran_search_space_type_t ss_type,
														srsran_dci_format_nr_t     dci_fmt,
														prb_grant                  *grant,
														srsran_dci_dl_nr_t         *out_dci)
{
  // Create new PDSCH entry in output PDSCH list
  pdsch_t *pdsch = oset_malloc(sizeof(pdsch_t));
  oset_assert(pdsch);
  cvector_push_back(pdsch_alloc->pdschs, pdsch);

  // Register allocated PRBs in accumulated bitmap
  pdsch_allocator_reserve_prbs(pdsch_alloc, grant);

  // Fill DCI with PDSCH freq/time allocation information
  // 时域资源分配
  out_dci->time_domain_assigment = 0;//pdsch-TimeDomainAllocationList  typeA k0= 0 s=2 len=12
  // 频域资源
  if (is_alloc_type0(grant)) {
	out_dci->freq_domain_assigment = bit_to_uint64(&grant->alloc.rbgs);
  } else {
    uint32_t rb_start = grant->alloc.interv.start_, nof_prb = pdsch_alloc->bwp_cfg->nof_prb;
	// coreset0所属，dci和pdsch范围限制在coreset0频域范围内
    if (SRSRAN_SEARCH_SPACE_IS_COMMON(ss_type)) {
      prb_interval *lims = coreset_prb_range(pdsch_alloc->bwp_cfg, coreset_id);
      if (dci_fmt == srsran_dci_format_nr_1_0) {
        ASSERT_IF_NOT(rb_start >= lims->start_, "Invalid PRB grant");
        rb_start -= lims->start_;//coreset0->offset 去除rb_start偏移量
      }
	  // TS 38.214, 5.1.2.2.2 - when DCI format 1_0, common search space and CORESET#0 is configured for the cell,
	  // RA type 1 allocs shall be within the CORESET#0 region
      if (coreset_id == 0) {
		nof_prb = prb_interval_length(lims);
      }
    }
    ASSERT_IF_NOT(rb_start + prb_interval_length(&grant->alloc.interv) <= nof_prb, "Invalid PRB grant");
    out_dci->freq_domain_assigment = srsran_ra_nr_type1_riv(nof_prb, rb_start, prb_interval_length(&grant->alloc.interv));
  }

  return pdsch;
}

//not use
pdsch_alloc_result pdsch_allocator_alloc_si_pdsch(pdsch_allocator *pdsch_alloc, uint32_t ss_id, prb_grant *grant, srsran_dci_dl_nr_t *dci)
{
  alloc_result code = pdsch_allocator_is_si_grant_valid(ss_id, grant);
  if (code != (alloc_result)success) {
	return pdsch_alloc_result_fail(code);
  }
  return pdsch_alloc_result_succ(pdsch_allocator_alloc_si_pdsch_unchecked(pdsch_alloc, ss_id, grant, dci));
}


pdsch_t* pdsch_allocator_alloc_si_pdsch_unchecked(pdsch_allocator *pdsch_alloc, uint32_t ss_id, prb_grant *grant, srsran_dci_dl_nr_t *dci)
{
  // Verify SearchSpace validity
  const srsran_search_space_t* ss = get_ss(pdsch_alloc->bwp_cfg, ss_id);
  ASSERT_IF_NOT(ss != NULL, "%sSearchSpace has not been configured", log_pdsch);
  return pdsch_allocator_alloc_pdsch_unchecked(pdsch_alloc, ss->coreset_id, ss->type, srsran_dci_format_nr_1_0, grant, dci);
}


alloc_result pdsch_allocator_is_grant_valid_common(pdsch_allocator *pdsch_alloc, 
																	srsran_search_space_type_t ss_type,
																	srsran_dci_format_nr_t     dci_fmt,
																	uint32_t                   coreset_id,
																	prb_grant                  *grant)
{
	// DL must be active in given slot
	if (!pdsch_alloc->bwp_cfg.slots[pdsch_alloc->slot_idx].is_dl) {
		oset_error("%sDL is disabled for slot=%lu", log_pdsch, pdsch_alloc->slot_idx);
		return (alloc_result)no_sch_space;
	}

	// No space in Scheduler PDSCH output list
	if (MAX_GRANTS == cvector_size(pdsch_alloc->pdschs)) {
		oset_error("%sMaximum number of PDSCHs={} reached", log_pdsch, cvector_size(pdsch_alloc->pdschs));
		return (alloc_result)no_sch_space;
	}

	// TS 38.214, 5.1.2.2 - "The UE shall assume that when the scheduling grant is received with DCI format 1_0, then
	//                       downlink resource allocation type 1 is used."
	if (dci_fmt == srsran_dci_format_nr_1_0 && !is_alloc_type1(grant)) {
		oset_error("%sDL Resource Allocation type 1 must be used in case of DCI format 1_0", log_pdsch);
		return (alloc_result)invalid_grant_params;
	}

	// TS 38.214 - 5.1.2.2 - For DCI format 1_0 and Common Search Space, the list of available PRBs is limited by the
	//                       rb_start and bandwidth of the coreset
	if (dci_fmt == srsran_dci_format_nr_1_0 && SRSRAN_SEARCH_SPACE_IS_COMMON(ss_type)) {
		// Grant PRBs do not collide with CORESET PRB limits (in case of common SearchSpace)
		// 是否越界
		if (prb_grant_collides(dci_fmt_1_0_excluded_prbs(pdsch_alloc->bwp_cfg, coreset_id), grant)) {
			oset_error("%sProvided PRB grant={%lu, %lu} falls outside common CORESET PRB boundaries", log_pdsch, grant->alloc.interv.start_, grant->alloc.interv.stop_);
			return (alloc_result)sch_collision;
		}
	}

	// Grant PRBs do not collide with previous PDSCH allocations
	// 检查授权的区间未被使用
	if (prb_grant_collides(pdsch_alloc->dl_prbs, grant)) {
		oset_error("%sProvided PRB grant={%lu, %lu} collides with allocations previously made", log_pdsch, grant->alloc.interv.start_, grant->alloc.interv.stop_);
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
		oset_error("%sSearchSpace has not been configured", log_pdsch);
		return (alloc_result)invalid_grant_params;
	}
	return pdsch_allocator_is_grant_valid_common(pdsch_alloc, ss->type, srsran_dci_format_nr_1_0, ss->coreset_id, grant);
}

alloc_result pdsch_allocator_is_rar_grant_valid(pdsch_allocator *pdsch_alloc, prb_grant *grant)
{
  ASSERT_IF_NOT(pdsch_alloc->bwp_cfg.cfg.pdcch.ra_search_space_present, "Attempting RAR allocation in BWP with no raSearchSpace");
  return pdsch_allocator_is_grant_valid_common(pdsch_alloc,
												pdsch_alloc->bwp_cfg.cfg.pdcch.ra_search_space.type,
												srsran_dci_format_nr_1_0,
												pdsch_alloc->bwp_cfg.cfg.pdcch.ra_search_space.coreset_id,
												grant);
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
	pdsch_t  **pdsch_node = NULL;
	cvector_for_each_in(pdsch_node, pdsch->pdschs){
		oset_free(*pdsch_node);
	}
	cvector_clear(pdsch->pdschs);

	bwp_rb_bitmap_reset(&pdsch->dl_prbs);
}

void pdsch_allocator_destory(pdsch_allocator *pdsch)
{
	pdsch_t  **pdsch_node = NULL;
	cvector_for_each_in(pdsch_node, pdsch->pdschs){
		oset_free(*pdsch_node);
	}
	cvector_free(pdsch->pdschs);
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
alloc_result pusch_allocator_has_grant_space(pusch_allocator *pusch, uint32_t nof_grants, bool verbose)
{
  // UL must be active in given slot
  if (!pusch->bwp_cfg.slots[pusch->slot_idx].is_ul) {
    if (verbose) {
      log_pusch_alloc_failure(bwp_cfg.logger.error, "UL is disabled for slot=%lu", pusch->slot_idx);
    }
    return alloc_result::no_sch_space;
  }

  // No space in Scheduler PDSCH output list
  if (puschs.size() + nof_grants > puschs.capacity()) {
    if (verbose) {
      log_pusch_alloc_failure(bwp_cfg.logger.warning, "Maximum number of PUSCHs={} reached.", puschs.capacity());
    }
    return alloc_result::no_sch_space;
  }

  return alloc_result::success;
}


void pusch_allocator_reset(pusch_allocator *pusch)
{
	pusch_t  **pusch_node = NULL;
	cvector_for_each_in(pusch_node, pusch->puschs){
		oset_free(*pusch_node);
	}
	cvector_clear(pusch->puschs);

	bwp_rb_bitmap_reset(&pusch->ul_prbs);
}

void pusch_allocator_destory(pusch_allocator *pusch)
{
	pusch_t  **pusch_node = NULL;
	cvector_for_each_in(pusch_node, pusch->puschs){
		oset_free(*pusch_node);
	}
	cvector_free(pusch->puschs);
}
void pusch_allocator_init(pusch_allocator *pusch, bwp_params_t *cfg_, uint32_t slot_index,  cvector_vector_t(pusch_t) pusch_lst)
{
	pusch->bwp_cfg = cfg_;
	pusch->slot_idx = slot_idx;
	pusch->puschs = pusch_lst;
	//ul_prbs
	bwp_rb_bitmap_init(&pusch->ul_prbs, cfg_->cfg.rb_width, cfg_->cfg.start_rb, cfg_->cfg.pdsch.rbg_size_cfg_1);
}

