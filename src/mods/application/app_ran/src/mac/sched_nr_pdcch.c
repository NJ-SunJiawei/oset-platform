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
					"SCHED: Failure to allocate PDCCH for %s-rnti=0x%x, SS#%lu. Cause: ",
					srsran_rnti_type_str_short(rnti_type),
					rnti,
					ss_id);
	return fmtbuf;
}

static uint32_t coreset_region_get_td_symbols(coreset_region *coreset) { return coreset->coreset_cfg->duration; }

static size_t coreset_region_nof_allocs(coreset_region *coreset) 
{
	return cvector_size(coreset->dfs_tree);
}

static uint32_t coreset_region_nof_cces(coreset_region *coreset)
{ 
	return coreset->nof_freq_res * coreset_region_get_td_symbols(coreset);
}


static void coreset_region_reset(coreset_region *coreset)
{
	cvector_clear(coreset->dfs_tree);
	cvector_clear(coreset->saved_dfs_tree);
	cvector_clear(coreset->dci_list);
}

pdcch_cce_pos_list coreset_region_get_cce_loc_table(coreset_region              *coreset, alloc_record *record)
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
	bounded_bitset current_mask = {0};//描述当前某个dci占用频域资源(存储每次申请的临时记录)
	node.dci_pos_idx = start_dci_idx;
	node.dci_pos.L   = record->aggr_idx;
	node.rnti        = record->ue != NULL ? record->ue->rnti : SRSRAN_INVALID_RNTI;
	bit_init(&current_mask, SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE * SRSRAN_CORESET_DURATION_MAX, coreset_region_nof_cces(coreset), true);
	bit_resize(&current_mask, coreset_region_nof_cces(coreset));

	// get cumulative pdcch bitmap
	if (!cvector_empty(alloc_dfs)) {
		node.total_mask = alloc_dfs[cvector_size(alloc_dfs) - 1].total_mask;//back()
	} else {
		bit_init(&node.total_mask, SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE * SRSRAN_CORESET_DURATION_MAX, coreset_region_nof_cces(coreset), true);
		bit_resize(&node.total_mask, coreset_region_nof_cces(coreset));
	}

	for (; node.dci_pos_idx < cce_locs->cce_index; ++node.dci_pos_idx) {
		node.dci_pos.ncce = cce_locs[node.dci_pos_idx];//node.dci_pos_idx对应的cce逻辑位
		//清空所有bit标志位
		bit_reset_all(&current_mask);
		//range(first cce, first cce + L)
		bit_fill(&current_mask, node.dci_pos.ncce, node.dci_pos.ncce + (1U << record->aggr_idx), true);

		bounded_bitset res = bit_and(&node.total_mask, &current_mask);
		if (bit_any(&res)) {
			// there is a PDCCH collision. Try another CCE position
			bit_final(&res);
			continue;
		}

		// Allocation successful
		node.total_mask |= current_mask;
		bit_final(&current_mask);
		cvector_push_back(alloc_dfs, node)
		record->dci->location = node.dci_pos;
		return true;
	}

	bit_final(&current_mask);
	return false;
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
		coreset->common_cce_list_active[i] = bwp_cfg_->common_cce_list_active[i];
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
		bool success = coreset_region_alloc_dfs_node(coreset, record, 0);
		if (success) {
			// DCI record allocation successful
			coreset->dci_list.push_back(record);
			return true;
		}
		if (coreset->saved_dfs_tree.empty()) {
			coreset->saved_dfs_tree = dfs_tree;
		}
	} while (get_next_dfs());

	// Revert steps to initial state, before dci record allocation was attempted
	dfs_tree.swap(saved_dfs_tree);
	return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////
void bwp_pdcch_allocator_destory(bwp_pdcch_allocator *pdcchs)
{
    //pdcch_dl_list
	pdcch_dl_t  **dl_node = NULL;
	cvector_for_each_in(dl_node, pdcchs->pdcch_dl_list){
		oset_free(*dl_node);
	}
	cvector_free(pdcchs->pdcch_dl_list);

	//pdcch_ul_list
	pdcch_ul_t  **ul_node = NULL;
	cvector_for_each_in(ul_node, pdcchs->pdcch_ul_list){
		oset_free(*ul_node);
	}
	cvector_free(pdcchs->pdcch_ul_list);

	//pddchs
	for (uint32_t cs_idx = 0; cs_idx < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++cs_idx) {
		coreset_region_destory(&pdcchs->coresets[cs_idx]);
	}
}


void bwp_pdcch_allocator_reset(bwp_pdcch_allocator *pdcchs)
{
	pdcchs->pending_dci = NULL;
    //pdcch_dl_list
	pdcch_dl_t  **dl_node = NULL;
	cvector_for_each_in(dl_node, pdcchs->pdcch_dl_list){
		oset_free(*dl_node);
	}
	cvector_free(pdcchs->pdcch_dl_list);

	//pdcch_ul_list
	pdcch_ul_t  **ul_node = NULL;
	cvector_for_each_in(ul_node, pdcchs->pdcch_ul_list){
		oset_free(*ul_node);
	}
	cvector_free(pdcchs->pdcch_ul_list);

	for (uint32_t cs_idx = 0; cs_idx < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++cs_idx) {
	  if (pdcchs->bwp_cfg->cfg.pdcch.coreset_present[cs_idx]) {
		uint32_t cs_id = pdcchs->bwp_cfg->cfg.pdcch.coreset[cs_idx].id;
		coreset_region_reset(&pdcchs->coresets[cs_id]);
	  }
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

static alloc_result bwp_pdcch_allocator_check_args_valid(bwp_pdcch_allocator *pdcchs,
											srsran_rnti_type_t		  rnti_type,
											uint16_t 				  rnti,
											uint32_t 				  ss_id,
											uint32_t 				  aggr_idx,
											srsran_dci_format_nr_t	  dci_fmt,
											const ue_carrier_params_t *user,
											bool 					  is_dl)
{
	ASSERT_IF_NOT(ss_id < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE, "Invalid SearchSpace#%d", ss_id);
	ASSERT_IF_NOT(aggr_idx < SRSRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR, "Invalid aggregation level index=%d", aggr_idx);
	int i = 0;
	bool num = false;

	// DL must be active in given slot
	if (pdcchs->bwp_cfg->slots[pdcchs->slot_idx].is_dl) {
		oset_error("%sDL is disabled for slot=%lu", log_pdcch_alloc_failure(rnti_type, ss_id, rnti), pdcchs->slot_idx);
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
		oset_error("%sChosen SearchSpace doesn't have CCE candidates for L=%lu", log_pdcch_alloc_failure(rnti_type, ss_id, rnti), aggr_idx);
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
		  oset_error("%sTrying to allocate BWP#%lu which is inactive for the UE",
								  log_pdcch_alloc_failure(rnti_type, ss_id, rnti), user->bwp_cfg->bwp_id);
		  return (alloc_result)no_rnti_opportunity;
		}
	}

	ASSERT_IF_NOT((cvector_size(pdcchs->pdcch_dl_list) + cvector_size(pdcchs->pdcch_ul_list) == bwp_pdcch_allocator_nof_allocations(pdcchs)), "Invalid PDCCH state");
	return (alloc_result)success;
}


static pdcch_dl_alloc_result bwp_pdcch_allocator_alloc_dl_pdcch_common(bwp_pdcch_allocator *pdcchs,
													srsran_rnti_type_t 		    rnti_type,
													uint16_t					rnti,
													uint32_t					ss_id,
													uint32_t					aggr_idx,
													srsran_dci_format_nr_t 	    dci_fmt,
													const ue_carrier_params_t   *user)
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
	// 申请pscch资源
	pdcch_dl_t *pdcch_dl = oset_malloc(sizeof(pdcch_dl_t));
	bool success = coreset_region_alloc_pdcch(&pdcchs->coresets[ss->coreset_id], rnti_type, true, aggr_idx, ss_id, user, &pdcch_dl->dci.ctx);

	if (!success) {
		// Remove failed PDCCH allocation
		pdcchs->pdcch_dl_list.pop_back();

		// Log PDCCH allocation failure
		srslog::log_channel& ch = user == nullptr ? logger.warning : logger.debug;
		log_pdcch_alloc_failure(ch, rnti_type, ss_id, rnti, "No available PDCCH position");

		return pdcch_dl_alloc_result_fail((alloc_result)no_cch_space);
	}

	cvector_push_back(pdcchs->pdcch_dl_list, pdcch_dl)

	// PDCCH allocation was successful
	pdcch_dl_t& pdcch = pdcch_dl_list.back();

	// Fill DCI with semi-static config
	fill_dci_from_cfg(bwp_cfg, pdcch.dci);

	// Fill DCI context information
	fill_dci_ctx_common(pdcch.dci.ctx, rnti_type, rnti, ss, dci_fmt, user);

	// register last PDCCH coreset, in case it needs to be aborted
	pending_dci = &pdcch.dci.ctx;

	return pdcch_dl_alloc_result_succ(&pdcch);
}


pdcch_dl_alloc_result bwp_pdcch_allocator_alloc_si_pdcch(bwp_pdcch_allocator *pdcchs, uint32_t ss_id, uint32_t aggr_idx)
{
	return bwp_pdcch_allocator_alloc_dl_pdcch_common(srsran_rnti_type_si, SRSRAN_SIRNTI, ss_id, aggr_idx, srsran_dci_format_nr_1_0, NULL);
}


