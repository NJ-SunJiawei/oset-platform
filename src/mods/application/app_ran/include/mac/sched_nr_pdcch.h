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

// List of PDCCH grants
typedef struct {
  uint32_t					 aggr_idx;
  uint32_t					 ss_id;
  srsran_dci_ctx_t			 *dci;
  bool						 is_dl;
  ue_carrier_params_t		 *ue;
}alloc_record;

// DFS decision tree of PDCCH grants
typedef struct  {
  uint16_t				rnti;//SRSRAN_INVALID_RNTI
  uint32_t				record_idx;
  uint32_t				dci_pos_idx;
  srsran_dci_location_t dci_pos;
  /// Accumulation of all PDCCH masks for the current solution (DFS path)
  bounded_bitset        total_mask;//bounded_bitset<SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE * SRSRAN_CORESET_DURATION_MAX, true>;
  bounded_bitset		current_mask;
}tree_node;

typedef struct {
  srsran_coreset_t        *coreset_cfg;
  uint32_t                coreset_id;
  uint32_t                slot_idx;
  uint32_t                nof_freq_res;

  bwp_cce_pos_list        rar_cce_list;
  A_DYN_ARRAY_OF(bwp_cce_pos_list) common_cce_list;//optional_vector<bwp_cce_pos_list>
  A_DYN_ARRAY_OF(alloc_record) dci_list;//bounded_vector<alloc_record, 2 * MAX_GRANTS>

  A_DYN_ARRAY_OF(tree_node)     dfs_tree;//std::vector<tree_node>
  A_DYN_ARRAY_OF(tree_node)     saved_dfs_tree;//std::vector<tree_node>
}coreset_region;

/**
 *handle the allocation of REs for a BWP PDCCH in a specific slot
 */
typedef struct {
  bwp_params_t    *bwp_cfg;
  uint32_t        slot_idx;
  A_DYN_ARRAY_OF(pdcch_dl_t)        pdcch_dl_list;//bounded_vector<pdcch_dl_t, MAX_GRANTS>
  A_DYN_ARRAY_OF(pdcch_ul_t)        pdcch_ul_list;//bounded_vector<pdcch_ul_t, MAX_GRANTS>
  A_DYN_ARRAY_OF(coreset_region)    coresets;     //optional_array<coreset_region, SRSRAN_UE_DL_NR_MAX_NOF_CORESET>
  const srsran_dci_ctx_t  *pending_dci; //Saves last PDCCH allocation, in case it needs to be aborted
}bwp_pdcch_allocator;

#ifdef __cplusplus
}
#endif

#endif
