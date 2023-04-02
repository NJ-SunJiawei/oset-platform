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

static void coreset_region_reset(coreset_region *coreset){
	cvector_clear(coreset->dfs_tree);
	cvector_clear(coreset->saved_dfs_tree);
	cvector_clear(coreset->dci_list);
}

static void coreset_region_init(coreset_region *coreset, bwp_params_t *bwp_cfg_, uint32_t coreset_id_, uint32_t slot_idx_)
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

void bwp_pdcch_allocator_reset(bwp_pdcch_allocator *pdcchs)
{
	pdcchs->pending_dci = NULL;
	cvector_clear(pdcchs->pdcch_dl_list);
	cvector_clear(pdcchs->pdcch_ul_list);
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
										cvector_vector_t(pdcch_dl_t) dl_pdcchs,
										cvector_vector_t(pdcch_ul_t) ul_pdcchs)
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


