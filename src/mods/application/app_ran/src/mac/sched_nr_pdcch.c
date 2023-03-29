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

void bwp_pdcch_allocator_init(bwp_pdcch_allocator *pdcchs,
										bwp_params_t        *bwp_cfg_,
										uint32_t            slot_idx_,
										void                *dl_pdcchs,
										void                *ul_pdcchs)
  bwp_cfg(bwp_cfg_), pdcch_dl_list(dl_pdcchs), pdcch_ul_list(ul_pdcchs), slot_idx(slot_idx_)
{
	pdcchs->bwp_cfg = bwp_cfg_;
	pdcchs->slot_idx = slot_idx_;

  for (uint32_t cs_idx = 0; cs_idx < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++cs_idx) {
    if (bwp_cfg.cfg.pdcch.coreset_present[cs_idx]) {
      uint32_t cs_id = bwp_cfg.cfg.pdcch.coreset[cs_idx].id;
      coresets.emplace(cs_id, bwp_cfg, cs_id, slot_idx);
    }
  }
}


