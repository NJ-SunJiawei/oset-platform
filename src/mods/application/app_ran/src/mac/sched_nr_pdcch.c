/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_pdcch.h"
			
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-pdcch"

//////////////////////////////////////////////////////////////////////////////
static char* log_pdcch_alloc_failure(srsran_rnti_type_t           rnti_type,
		                             uint32_t             ss_id,
		                             uint16_t             rnti)
{
	// Log PDCCH allocation failure
	char fmtbuf[128] = {0};
	oset_snprintf(fmtbuf,
					sizeof(fmtbuf),
					"SCHED: Failure to allocate PDCCH for %s-rnti=0x%x, SS#%u. Cause: ",
					srsran_rnti_type_str_short(rnti_type),
					rnti,
					ss_id);
	return fmtbuf;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void fill_dci_dl_from_cfg(bwp_params_t *bwp_cfg, srsran_dci_dl_nr_t *dci)
{
	dci->bwp_id	   = bwp_cfg->bwp_id;
	dci->cc_id	   = bwp_cfg->cc;
	dci->tpc	   = 1; //用于功率控制
	dci->coreset0_bw = bwp_cfg.cfg.pdcch.coreset_present[0] ? prb_interval_length(coreset_prb_range(bwp_cfg, 0)) : 0;
}

void fill_dci_ul_from_cfg(bwp_params_t *bwp_cfg, srsran_dci_ul_nr_t *dci)
{
	dci->bwp_id = bwp_cfg->bwp_id;
	dci->cc_id  = bwp_cfg->cc;
	dci->tpc	  = 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t coreset_region_get_td_symbols(coreset_region *coreset) { return coreset->coreset_cfg->duration; }

static size_t coreset_region_nof_allocs(coreset_region *coreset) 
{
	return cvector_size(coreset->dfs_tree);
}

static uint32_t coreset_region_nof_cces(coreset_region *coreset)
{ 
	return coreset->nof_freq_res * coreset_region_get_td_symbols(coreset);
}

static pdcch_cce_pos_list coreset_region_get_cce_loc_table(coreset_region              *coreset, alloc_record *record)
{
  switch (record->dci->rnti_type) {
    case srsran_rnti_type_ra:
      return coreset->rar_cce_list[coreset->slot_idx][record->aggr_idx];
    case srsran_rnti_type_si:
      return coreset->common_cce_list[record->ss_id][coreset->slot_idx][record->aggr_idx];
    case srsran_rnti_type_c:
    case srsran_rnti_type_tc:
    case srsran_rnti_type_mcs_c:
    case srsran_rnti_type_sp_csi:
      return ue_carrier_params_cce_pos_list(record->ue, record->ss_id, coreset->slot_idx, record->aggr_idx);
    default:
      oset_error("Invalid RNTI type=%s", srsran_rnti_type_str(record->dci->rnti_type));
      break;
  }
  return {0};
}


static bool coreset_region_alloc_dfs_node(coreset_region             *coreset, alloc_record *record, uint32_t start_dci_idx)
{
	alloc_tree_dfs_t alloc_dfs = coreset->dfs_tree;
	// Get DCI Location Table
	// 根据聚合等级/ss_id/slot_idx获取当前的dci候选合集
	pdcch_cce_pos_list cce_locs = coreset_region_get_cce_loc_table(coreset, record);
	if (start_dci_idx >= cce_locs->cce_index) {
		return false;
	}

	tree_node node = {0};
	node.dci_pos_idx = start_dci_idx;
	node.dci_pos.L   = record->aggr_idx;
	node.rnti        = record->ue != NULL ? record->ue->rnti : SRSRAN_INVALID_RNTI;
	bit_init(&node.current_mask, SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE * SRSRAN_CORESET_DURATION_MAX, coreset_region_nof_cces(coreset), true);
	bit_resize(&node.current_mask, coreset_region_nof_cces(coreset));

	// get cumulative pdcch bitmap
	if (!cvector_empty(alloc_dfs)) {
		node.total_mask = alloc_dfs[cvector_size(alloc_dfs) - 1].total_mask;//back()
	} else {
		bit_init(&node.total_mask, SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE * SRSRAN_CORESET_DURATION_MAX, coreset_region_nof_cces(coreset), true);
		bit_resize(&node.total_mask, coreset_region_nof_cces(coreset));
	}

	for (; node.dci_pos_idx < cce_locs->cce_index; ++node.dci_pos_idx) {
		node.dci_pos.ncce = cce_locs[node.dci_pos_idx];//node.dci_pos_idx对应的first cce id
		//清空所有bit标志位
		bit_reset_all(&node.current_mask);
		//range(first cce, first cce + L)
		bit_fill(&node.current_mask, node.dci_pos.ncce, node.dci_pos.ncce + (1U << record->aggr_idx), true);

		bounded_bitset res = bit_and(&node.total_mask, &node.current_mask);
		if (bit_any(&res)) {
			// there is a PDCCH collision. Try another CCE position
			continue;
		}

		// Allocation successful
		bit_or_eq(&node.total_mask, &node.current_mask);
		cvector_push_back(alloc_dfs, node)
		record->dci->location = node.dci_pos;
		return true;
	}
	return false;
}

void coreset_region_reset(coreset_region *coreset)
{
	cvector_clear(coreset->dfs_tree);
	cvector_clear(coreset->saved_dfs_tree);
	cvector_clear(coreset->dci_list);
}

void coreset_region_destory(coreset_region *coreset)
{
	cvector_free(coreset->dfs_tree);
	cvector_free(coreset->saved_dfs_tree);
	cvector_free(coreset->dci_list);
}

void coreset_region_init(coreset_region *coreset, bwp_params_t *bwp_cfg_, uint32_t coreset_id_, uint32_t slot_idx_)
{
	int i = 0;

	cvector_reserve(coreset->dci_list, 2 * MAX_GRANTS);
	
	coreset->coreset_cfg = &bwp_cfg_->cfg.pdcch.coreset[coreset_id_];
	coreset->coreset_id = coreset_id_;
	coreset->slot_idx = slot_idx_;
	coreset->rar_cce_list = bwp_cfg_->rar_cce_list;
	for(i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE, ++i){
		coreset->common_cce_list[i] = bwp_cfg_->common_cce_list[i];
	}


	for (uint32_t i = 0; i < SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE; ++i) {
	  if (bwp_cfg_->cfg.pdcch.coreset[coreset_id_].freq_resources[i]) {
		  coreset->nof_freq_res++;
	  }
	}

	ASSERT_IF_NOT(coreset->coreset_cfg->duration <= SRSRAN_CORESET_DURATION_MAX,
	            "Possible number of time-domain OFDM symbols in CORESET must be within {1,2,3}");
	ASSERT_IF_NOT((coreset->nof_freq_res * 6) <= bwp_cfg_->cell_cfg->carrier.nof_prb,
	            "The number of frequency resources=%d of CORESET#%d exceeds BWP bandwidth=%d",
	            coreset->nof_freq_res,
	            coreset->coreset_id,
	            bwp_cfg_->cell_cfg->carrier.nof_prb);
}

bool coreset_region_get_next_dfs(coreset_region            *coreset)
{
	//从最高位尝试，first cce往高位重选
	do {
		if (cvector_empty(coreset->dfs_tree)) {
			// If we reach root, the allocation failed
			return false;
		}
		// Attempt to re-add last tree node, but with a higher node child index
		uint32_t start_child_idx = coreset->dfs_tree[cvector_size(coreset->dfs_tree) - 1].dci_pos_idx + 1;
		cvector_pop_back(coreset->dfs_tree);
		while (cvector_size(coreset->dfs_tree) < cvector_size(coreset->dci_list) &&\
				coreset_region_alloc_dfs_node(coreset, coreset->dci_list[cvector_size(coreset->dfs_tree)], start_child_idx)) {
			start_child_idx = 0;
		}
	} while (cvector_size(coreset->dfs_tree) < cvector_size(coreset->dci_list));

	// Finished computation of next DFS node
	return true;
}

bool coreset_region_alloc_pdcch(coreset_region             *coreset, 
								srsran_rnti_type_t         rnti_type,
								bool                       is_dl,
								uint32_t                   aggr_idx,
								uint32_t                   search_space_id,
								const ue_carrier_params_t  *user,
								srsran_dci_ctx_t           *dci)
{
	cvector_clear(coreset->saved_dfs_tree);

	alloc_record  record = {0};
	record.dci            = dci;
	record.ue             = user;
	record.aggr_idx       = aggr_idx;
	record.ss_id          = search_space_id;
	record.is_dl          = is_dl;
	record.dci->rnti_type = rnti_type;

	// Try to allocate grant. If it fails, attempt the same grant, but using a different permutation of past grant DCI
	// positions
	do {
		bool success = coreset_region_alloc_dfs_node(coreset, &record, 0);
		if (success) {
			// DCI record allocation successful
			// 记录成功的dci record参数
			cvector_push_back(coreset->dci_list, record)
			return true;
		}
		//临时缓存之前已成功的dfs记录
		if (cvector_empty(coreset->saved_dfs_tree)) {
			cvector_copy(coreset->dfs_tree, coreset->saved_dfs_tree)
		}
	} while (coreset_region_get_next_dfs(coreset));

	// Revert steps to initial state, before dci record allocation was attempted
	cvector_copy(coreset->saved_dfs_tree, coreset->dfs_tree);
	return false;
}

void coreset_region_rem_last_pdcch(coreset_region *coreset)
{
	oset_assert(!cvector_empty(coreset->dci_list), "called when no PDCCH have yet been allocated");

	// Remove DCI record
	cvector_pop_back(coreset->dfs_tree);
	cvector_pop_back(coreset->dci_list);
}

///////////////////////////////////////////////////////////////////////////////////////////////
void bwp_pdcch_allocator_cancel_last_pdcch(bwp_pdcch_allocator *pdcchs)
{
	oset_assert(pdcchs->pending_dci != NULL, "Trying to abort PDCCH allocation that does not exist");
	uint32_t cs_id = pdcchs->pending_dci->coreset_id;

	if (!cvector_empty(pdcchs->pdcch_dl_list) && (&pdcchs->pdcch_dl_list[cvector_size(pdcchs->pdcch_dl_list) - 1].dci.ctx == pdcchs->pending_dci)) {
		pdcch_dl_t* last_pdcch_dl = pdcchs->pdcch_dl_list[cvector_size(pdcchs->pdcch_dl_list) - 1];
		cvector_pop_back(pdcchs->pdcch_dl_list);
		oset_free(last_pdcch_dl);
	} else if (!cvector_empty(pdcchs->pdcch_ul_list) && (&pdcchs->pdcch_ul_list.[cvector_size(pdcchs->pdcch_ul_list) - 1].dci.ctx == pdcchs->pending_dci)) {
		pdcch_ul_t* last_pdcch_ul = pdcchs->pdcch_ul_list[cvector_size(pdcchs->pdcch_ul_list) - 1];
		cvector_pop_back(pdcchs->pdcch_ul_list);
		oset_free(last_pdcch_ul);
	} else {
		oset_error("Invalid DCI context provided to be removed");
		return;
	}

	coreset_region_rem_last_pdcch(&pdcchs->coresets[cs_id]);
	pdcchs->pending_dci = NULL;
}

void bwp_pdcch_allocator_destory(bwp_pdcch_allocator *pdcchs)
{
    //pdcch_dl_list
	cvector_free_each_and_free(pdcchs->pdcch_dl_list, oset_free);

	//pdcch_ul_list
	cvector_free_each_and_free(pdcchs->pdcch_ul_list, oset_free);

	//pddchs
	for (uint32_t cs_idx = 0; cs_idx < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++cs_idx) {
		coreset_region_destory(&pdcchs->coresets[cs_idx]);
	}
}


void bwp_pdcch_allocator_reset(bwp_pdcch_allocator *pdcchs)
{
	pdcchs->pending_dci = NULL;
    //pdcch_dl_list
	cvector_free_each_and_free(pdcchs->pdcch_dl_list, oset_free);
	//pdcch_ul_list
	cvector_free_each_and_free(pdcchs->pdcch_ul_list, oset_free);

	for (uint32_t cs_idx = 0; cs_idx < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++cs_idx) {
		coreset_region_reset(&pdcchs->coresets[cs_idx]);
	}
}

void bwp_pdcch_allocator_init(bwp_pdcch_allocator *pdcchs,
										bwp_params_t        *bwp_cfg_,
										uint32_t            slot_idx_,
										cvector_vector_t(pdcch_dl_t *) dl_pdcchs,
										cvector_vector_t(pdcch_ul_t *) ul_pdcchs)
{
	pdcchs->bwp_cfg = bwp_cfg_;
	pdcchs->slot_idx = slot_idx_;
	pdcchs->pdcch_dl_list = dl_pdcchs;
	pdcchs->pdcch_ul_list = ul_pdcchs;

  for (uint32_t cs_idx = 0; cs_idx < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++cs_idx) {
    if (bwp_cfg_->cfg.pdcch.coreset_present[cs_idx]) {
      uint32_t cs_id = bwp_cfg_->cfg.pdcch.coreset[cs_idx].id;
	  coreset_region_init(&pdcchs->coresets[cs_id], bwp_cfg_, cs_id, slot_idx);
    }
  }
}

uint32_t bwp_pdcch_allocator_nof_allocations(bwp_pdcch_allocator *pdcchs)
{
	uint32_t count = 0;
	for(int i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; i++){
		coreset_region *coreset = &pdcchs->coresets[i];
		count += coreset_region_nof_allocs(coreset);
	}
	return count;
}

static void bwp_pdcch_allocator_fill_dci_ctx_common(bwp_pdcch_allocator *pdcchs,
												srsran_dci_ctx_t             *dci,
												srsran_rnti_type_t           rnti_type,
												uint16_t                     rnti,
												srsran_search_space_t        *ss,
												srsran_dci_format_nr_t       dci_fmt,
												ue_carrier_params_t          *ue)
{
  // Note: Location is filled by coreset_region class.
  dci->ss_type    = ss->type;
  dci->coreset_id = ss->coreset_id;
  srsran_coreset_t* coreset =
      ue == NULL ? &pdcchs->bwp_cfg->cfg.pdcch.coreset[ss->coreset_id] : &ue->cfg_->phy_cfg.pdcch.coreset[ss.coreset_id];
  dci->coreset_start_rb = srsran_coreset_start_rb(coreset);
  dci->rnti_type        = rnti_type;
  dci->rnti             = rnti;
  dci->format           = dci_fmt;
}


static alloc_result bwp_pdcch_allocator_check_args_valid(bwp_pdcch_allocator *pdcchs,
											srsran_rnti_type_t		  rnti_type,
											uint16_t 				  rnti,
											uint32_t 				  ss_id,
											uint32_t 				  aggr_idx,
											srsran_dci_format_nr_t	  dci_fmt,
											ue_carrier_params_t       *user,
											bool 					  is_dl)
{
	ASSERT_IF_NOT(ss_id < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE, "Invalid SearchSpace#%d", ss_id);
	ASSERT_IF_NOT(aggr_idx < SRSRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR, "Invalid aggregation level index=%d", aggr_idx);
	int i = 0;
	bool num = false;

	// DL must be active in given slot
	if (pdcchs->bwp_cfg->slots[pdcchs->slot_idx].is_dl) {
		oset_error("%sDL is disabled for slot=%u", log_pdcch_alloc_failure(rnti_type, ss_id, rnti), pdcchs->slot_idx);
		return (alloc_result)no_cch_space;
	}

	// Verify SearchSpace validity
	const srsran_search_space_t* ss =
	  (user == NULL)
		  ? (rnti_type == srsran_rnti_type_ra ? &pdcchs->bwp_cfg->cfg.pdcch.ra_search_space : get_ss(pdcchs->bwp_cfg, ss_id))
		  : ue_carrier_params_get_ss(user ,ss_id);
	if (ss == NULL) {
		// Couldn't find SearchSpace
		oset_error("%sSearchSpace has not been configured", log_pdcch_alloc_failure(rnti_type, ss_id, rnti));
		return (alloc_result)invalid_grant_params;
	}

	if (ss->nof_candidates[aggr_idx] == 0) {
		// No valid DCI position candidates given aggregation level
		oset_error("%sChosen SearchSpace doesn't have CCE candidates for L=%u", log_pdcch_alloc_failure(rnti_type, ss_id, rnti), aggr_idx);
		return (alloc_result)invalid_grant_params;
	}

	if (!is_rnti_type_valid_in_search_space(rnti_type, ss->type)) {
		// RNTI type doesnt match SearchSpace type
		oset_error("%sChosen SearchSpace type \"%s\" does not match rnti_type.",
								log_pdcch_alloc_failure(rnti_type, ss_id, rnti), srsran_ss_type_str(ss->type));
		return (alloc_result)invalid_grant_params;
	}

	for(i = 0; i < ss->nof_formats; i++){
		if(dci_fmt == ss->formats[i]) num = true;
	}
	if(!num) {
		oset_error("%sChosen SearchSpace does not support chosen dci format=%s",
								log_pdcch_alloc_failure(rnti_type, ss_id, rnti), srsran_dci_format_nr_string(dci_fmt));
		return (alloc_result)invalid_grant_params;
	}

	if (is_dl) {
		if (MAX_GRANTS == cvector_size(pdcchs->pdcch_dl_list)) {
		  oset_error("%sMaximum number of allocations=%lz reached", log_pdcch_alloc_failure(rnti_type, ss_id, rnti), cvector_size(pdcchs->pdcch_dl_list));
		  return (alloc_result)no_cch_space;
		}
	} else if (MAX_GRANTS == cvector_size(pdcchs->pdcch_ul_list)) {
		oset_error("%sMaximum number of UL allocations=%lz reached", log_pdcch_alloc_failure(rnti_type, ss_id, rnti), cvector_size(pdcchs->pdcch_ul_list));
		return (alloc_result)no_cch_space;
	}

	if (user != NULL) {
		if (user->bwp_cfg->bwp_id != pdcchs->bwp_cfg->bwp_id) {
		  oset_error("%sTrying to allocate BWP#%u which is inactive for the UE",
								  log_pdcch_alloc_failure(rnti_type, ss_id, rnti), user->bwp_cfg->bwp_id);
		  return (alloc_result)no_rnti_opportunity;
		}
	}

	ASSERT_IF_NOT((cvector_size(pdcchs->pdcch_dl_list) + cvector_size(pdcchs->pdcch_ul_list) == bwp_pdcch_allocator_nof_allocations(pdcchs)), "Invalid PDCCH state");
	return (alloc_result)success;
}

pdcch_ul_alloc_result bwp_pdcch_allocator_alloc_ul_pdcch(bwp_pdcch_allocator *pdcchs,
																	uint32_t			  ss_id,
																	uint32_t			  aggr_idx,
																	ue_carrier_params_t   *user)

{
	static const srsran_dci_format_nr_t dci_fmt = srsran_dci_format_nr_0_0; // TODO: make it configurable
	alloc_result r = bwp_pdcch_allocator_check_args_valid(srsran_rnti_type_c, user->rnti, ss_id, aggr_idx, dci_fmt, user, false);
	if (r != (alloc_result)success) {
		return pdcch_ul_alloc_result_fail(r);
	}

	const srsran_search_space_t *ss = ue_carrier_params_get_ss(user, ss_id);

	// Add new UL PDCCH to sched result
	pdcch_ul_t *pdcch_ul = oset_malloc(sizeof(pdcch_ul_t));
	oset_assert(pdcch_ul);

	bool success = coreset_region_alloc_pdcch(&pdcchs->coresets[ss->coreset_id], srsran_rnti_type_c, false, aggr_idx, ss_id, user, &pdcch_ul->dci.ctx);
	if (!success) {
		// Remove failed PDCCH allocation
		oset_free(pdcch_ul);

		// Log PDCCH allocation failure
		oset_error("%sNo available PDCCH position", log_pdcch_alloc_failure(srsran_rnti_type_c, ss_id, user->rnti));

		return pdcch_ul_alloc_result_fail((alloc_result)no_cch_space);
	}

	// PDCCH allocation was successful
	cvector_push_back(pdcchs->pdcch_ul_list, pdcch_ul);

	// Fill DCI with semi-static config
	fill_dci_ul_from_cfg(pdcchs->bwp_cfg, pdcch_ul->dci);

	// Fill DCI context information
	bwp_pdcch_allocator_fill_dci_ctx_common(pdcchs, &pdcch_ul->dci.ctx, srsran_rnti_type_c, user->rnti, ss, dci_fmt, user);

	// register last PDCCH coreset, in case it needs to be aborted
	pdcchs->pending_dci = &pdcch_ul->dci.ctx;

	return pdcch_dl_alloc_result_succ(pdcch_ul);
}


static pdcch_dl_alloc_result bwp_pdcch_allocator_alloc_dl_pdcch_common(bwp_pdcch_allocator *pdcchs,
													srsran_rnti_type_t 		    rnti_type,
													uint16_t					rnti,
													uint32_t					ss_id,
													uint32_t					aggr_idx,
													srsran_dci_format_nr_t 	    dci_fmt,
												    ue_carrier_params_t         *user)
{
	//校验参数合法性
	alloc_result r = bwp_pdcch_allocator_check_args_valid(pdcchs, rnti_type, rnti, ss_id, aggr_idx, dci_fmt, user, true);
	if (r != (alloc_result)success) {
		return pdcch_dl_alloc_result_fail(r);
	}
	const srsran_search_space_t *ss =
	  (user == NULL)
		  ? (rnti_type == srsran_rnti_type_ra ? &pdcchs->bwp_cfg.cfg.pdcch.ra_search_space : get_ss(pdcchs->bwp_cfg, ss_id))
		  : ue_carrier_params_get_ss(user, ss_id);

	// Add new DL PDCCH to sched result
	// 某个PDCCH信道的所有候选位置的CCE位置，与该PDCCH信道的聚合等级、搜索空间类型、子帧号、可用CCE总个数、RNTI等参数有关
	// 在同个子帧时刻，一旦某个CCE已经被分配，则不能再分配给其他用户
	// 对于SIB、寻呼、RAR、TPC、集群组呼这些共享信道对应的PDCCH，需要在公共空间进行CCE的调度，其它的则在UE专用的搜索空间中调度
	pdcch_dl_t *pdcch_dl = oset_malloc(sizeof(pdcch_dl_t));
	oset_assert(pdcch_dl);

	bool success = coreset_region_alloc_pdcch(&pdcchs->coresets[ss->coreset_id], rnti_type, true, aggr_idx, ss_id, user, &pdcch_dl->dci.ctx);
	if (!success) {
		// Remove failed PDCCH allocation
		oset_free(pdcch_dl);

		// Log PDCCH allocation failure
		oset_error("%sNo available PDCCH position", log_pdcch_alloc_failure(rnti_type, ss_id, rnti));
		return pdcch_dl_alloc_result_fail((alloc_result)no_cch_space);
	}

	// PDCCH allocation was successful
	cvector_push_back(pdcchs->pdcch_dl_list, pdcch_dl);

	// Fill DCI with semi-static config
	fill_dci_dl_from_cfg(pdcchs->bwp_cfg, &pdcch_dl->dci);

	// Fill DCI context information
	bwp_pdcch_allocator_fill_dci_ctx_common(pdcchs, &pdcch_dl->dci.ctx, rnti_type, rnti, ss, dci_fmt, user);

	// register last PDCCH coreset, in case it needs to be aborted
	pdcchs->pending_dci = &pdcch_dl->dci.ctx;

	return pdcch_dl_alloc_result_succ(pdcch_dl);
}


pdcch_dl_alloc_result bwp_pdcch_allocator_alloc_si_pdcch(bwp_pdcch_allocator *pdcchs, uint32_t ss_id, uint32_t aggr_idx)
{
	return bwp_pdcch_allocator_alloc_dl_pdcch_common(srsran_rnti_type_si, SRSRAN_SIRNTI, ss_id, aggr_idx, srsran_dci_format_nr_1_0, NULL);
}

pdcch_dl_alloc_result bwp_pdcch_allocator_alloc_rar_pdcch(bwp_pdcch_allocator *pdcchs, uint16_t ra_rnti, uint32_t aggr_idx)
{
	ASSERT_IF_NOT(pdcchs->bwp_cfg->cfg.pdcch.ra_search_space_present, "Allocating RAR PDCCH in BWP without RA SearchSpace");
	return bwp_pdcch_allocator_alloc_dl_pdcch_common(srsran_rnti_type_ra, ra_rnti, pdcchs->bwp_cfg->cfg.pdcch.ra_search_space.id, aggr_idx, srsran_dci_format_nr_1_0, NULL);
}

pdcch_dl_alloc_result bwp_pdcch_allocator_alloc_dl_pdcch(bwp_pdcch_allocator *pdcchs,
																	srsran_rnti_type_t         rnti_type,
																	uint32_t                   ss_id,
																	uint32_t                   aggr_idx,
																	ue_carrier_params_t        *user)
{
	static const srsran_dci_format_nr_t dci_fmt = srsran_dci_format_nr_1_0; // TODO: make it configurable
	ASSERT_IF_NOT(rnti_type == srsran_rnti_type_c || rnti_type == srsran_rnti_type_tc,
	            "Invalid RNTI type=%s for UE-specific PDCCH",
	            srsran_rnti_type_str_short(rnti_type));
	return bwp_pdcch_allocator_alloc_dl_pdcch_common(pdcchs, rnti_type, user->rnti, ss_id, aggr_idx, dci_fmt, user);
}

