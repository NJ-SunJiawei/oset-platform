/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_PDCCH_H_
#define SCHED_NR_PDCCH_H_

#include "mac/sched_nr_cfg.h"
#include "mac/ue_cfg_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef cvector_vector_t(tree_node)  alloc_tree_dfs_t;

// List of PDCCH grants
typedef struct {
  uint32_t					 aggr_idx;
  uint32_t					 ss_id;
  srsran_dci_ctx_t			 dci;
  bool						 is_dl;
  ue_carrier_params_t		 *ue;
}alloc_record;

// DFS decision tree of PDCCH grants
typedef struct  {
  uint16_t				rnti;//SRSRAN_INVALID_RNTI
  uint32_t				record_idx;
  uint32_t				dci_pos_idx;//dci或者cce_idx其实值
  srsran_dci_location_t dci_pos;
  /// Accumulation of all PDCCH masks for the current solution (DFS path)
  bounded_bitset        total_mask;//记录总的coreset可用频域资源//need free
  bounded_bitset		current_mask;//描述当前某个dci占用频域资源(存储每次申请的临时记录)
}tree_node;

typedef struct {
  srsran_coreset_t        *coreset_cfg;
  uint32_t                coreset_id;
  uint32_t                slot_idx;
  uint32_t                nof_freq_res;//频域资源计数

  bwp_cce_pos_list        rar_cce_list;
  bool                    common_cce_list_active[SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE];
  bwp_cce_pos_list        common_cce_list[SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE];
  //cvector_vector_t(bwp_cce_pos_list) common_cce_list;//optional_vector<bwp_cce_pos_list>
  cvector_vector_t(alloc_record) dci_list;//bounded_vector<alloc_record, 2 * MAX_GRANTS>//已申请的dci资源合集(记录)

  cvector_vector_t(tree_node)     dfs_tree;//std::vector<tree_node>//已申请的dci资源合集(实际使用)
  cvector_vector_t(tree_node)     saved_dfs_tree;//std::vector<tree_node>//临时缓存
}coreset_region;

/**
 *handle the allocation of REs for a BWP PDCCH in a specific slot
 */
typedef struct {
  bwp_params_t    *bwp_cfg;
  uint32_t        slot_idx;
  cvector_vector_t(pdcch_dl_t *)        pdcch_dl_list;//bounded_vector<pdcch_dl_t, MAX_GRANTS>
  cvector_vector_t(pdcch_ul_t *)        pdcch_ul_list;//bounded_vector<pdcch_ul_t, MAX_GRANTS>
  coreset_region    coresets[SRSRAN_UE_DL_NR_MAX_NOF_CORESET];     //optional_array<coreset_region, SRSRAN_UE_DL_NR_MAX_NOF_CORESET>
  srsran_dci_ctx_t  *pending_dci; //Saves last PDCCH allocation, in case it needs to be aborted
}bwp_pdcch_allocator;


typedef expected(pdcch_dl_t*, alloc_result) pdcch_dl_alloc_result;
typedef expected(pdcch_ul_t*, alloc_result) pdcch_ul_alloc_result;


inline pdcch_dl_alloc_result pdcch_dl_alloc_result_succ(pdcch_dl_t* pdcch_dl)
{
	pdcch_dl_alloc_result result = {0};
	result.has_val = true;
	result.res.val = pdcch_dl;
	return result;
}

inline pdcch_dl_alloc_result pdcch_dl_alloc_result_fail(alloc_result alloc_res)
{
	pdcch_dl_alloc_result result = {0};
	result.has_val = false;
	result.res.unexpected = alloc_res;
	return result;
}

void coreset_region_reset(coreset_region *coreset);
void coreset_region_destory(coreset_region *coreset);
void coreset_region_init(coreset_region *coreset, bwp_params_t *bwp_cfg_, uint32_t coreset_id_, uint32_t slot_idx_);
bool coreset_region_alloc_pdcch(coreset_region             *coreset, 
								srsran_rnti_type_t         rnti_type,
								bool                       is_dl,
								uint32_t                   aggr_idx,
								uint32_t                   search_space_id,
								const ue_carrier_params_t  *user,
								srsran_dci_ctx_t           *dci);

//////////////////////////////////////////////////////////////////////
void bwp_pdcch_allocator_reset(bwp_pdcch_allocator *pdcchs);
void bwp_pdcch_allocator_destory(bwp_pdcch_allocator *pdcchs);
void bwp_pdcch_allocator_init(bwp_pdcch_allocator *pdcchs,
										bwp_params_t        *bwp_cfg_,
										uint32_t            slot_idx_,
										cvector_vector_t(pdcch_dl_t *) dl_pdcchs,
										cvector_vector_t(pdcch_ul_t *) ul_pdcchs);
uint32_t bwp_pdcch_allocator_nof_allocations(bwp_pdcch_allocator *pdcchs);
pdcch_dl_alloc_result bwp_pdcch_allocator_alloc_si_pdcch(bwp_pdcch_allocator *pdcchs, uint32_t ss_id, uint32_t aggr_idx);

#ifdef __cplusplus
}
#endif

#endif
