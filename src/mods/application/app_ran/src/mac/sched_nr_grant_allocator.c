/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_grant_allocator.h"
		
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-grant"

static void bwp_slot_grid_init(bwp_slot_grid *slot, bwp_params_t *bwp_cfg_, uint32_t slot_idx_)
{
	//dl init
	cvector_reserve(slot->dl.phy.ssb, MAX_SSB);
	cvector_reserve(slot->dl.phy.pdcch_dl, MAX_GRANTS);
	cvector_reserve(slot->dl.phy.pdcch_ul, MAX_GRANTS);
	cvector_reserve(slot->dl.phy.pdsch, MAX_GRANTS);
	cvector_reserve(slot->dl.phy.nzp_csi_rs, MAX_NZP_CSI_RS);
	cvector_reserve(slot->dl.data, MAX_GRANTS);
	cvector_reserve(slot->dl.rar, MAX_GRANTS);
	cvector_reserve(slot->dl.sib_idxs, MAX_GRANTS);
	//ul init
	cvector_reserve(slot->ul.pucch, MAX_GRANTS);
	cvector_reserve(slot->ul.pusch, MAX_GRANTS);
	//ack init
	cvector_reserve(slot->pending_acks, MAX_GRANTS);

	slot->slot_idx = slot_idx_;
	slot->cfg = bwp_cfg_;
	bwp_pdcch_allocator_init(&slot->pdcchs, bwp_cfg_, slot_idx_, slot->dl.phy.pdcch_dl, slot->dl.phy.pdcch_ul);
	pdsch_allocator_init(&slot->pdschs, bwp_cfg_, slot_idx_, slot->dl.phy.pdsch);
	pusch_allocator_init(&slot->puschs, bwp_cfg_, slot_idx_, slot->dl.phy.pdsch);
	slot->rar_softbuffer = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->cfg.rb_width);//bwp_cfg_->nof_prb
}

void bwp_slot_grid_destory(bwp_slot_grid *slot)
{
	//dl
	cvector_free(slot->dl.phy.ssb);
	cvector_free(slot->dl.phy.pdcch_dl);
	cvector_free(slot->dl.phy.pdcch_ul);
	cvector_free(slot->dl.phy.pdsch);
	cvector_free(slot->dl.phy.nzp_csi_rs);
	cvector_free(slot->dl.data);
	cvector_free(slot->dl.rar);
	cvector_free(slot->dl.sib_idxs);
	//ul
	cvector_free(slot->ul.pucch);
	cvector_free(slot->ul.pusch);
	//ack
	cvector_free(slot->pending_acks);
	//pddchs
	for (uint32_t cs_idx = 0; cs_idx < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++cs_idx) {
		cvector_free(slot->pdcchs.coresets[cs_idx].dci_list);
		cvector_free(slot->pdcchs.coresets[cs_idx].dfs_tree);
		cvector_free(slot->pdcchs.coresets[cs_idx].saved_dfs_tree);
	}
}

void bwp_slot_grid_reset(bwp_slot_grid *slot)
{
	bwp_pdcch_allocator_reset(&slot->pdcchs);
	pdsch_allocator_reset(&slot->pdschs);
	pusch_allocator_reset(&slot->puschs);
	cvector_clear(slot->dl.phy.ssb);
	cvector_clear(slot->dl.phy.nzp_csi_rs);
	cvector_clear(slot->dl.data);
	cvector_clear(slot->dl.rar);
	cvector_clear(slot->dl.sib_idxs);
	cvector_clear(slot->ul.pucch);
	cvector_clear(slot->pending_acks);
}

void bwp_res_grid_destory(bwp_res_grid *res)
{
	cvector_free(res->slots);
}


void bwp_res_grid_init(bwp_res_grid *res, bwp_params_t *bwp_cfg_)
{
	res->cfg = bwp_cfg_;
	//TTIMOD_SZ todo???
	cvector_reserve(res->slots, TTIMOD_SZ);
	//1 frame 20 slot(30khz)
	for (uint32_t sl = 0; sl < TTIMOD_SZ; ++sl){
		bwp_slot_grid  slot_grid = {0};
		bwp_slot_grid_init(&slot_grid, bwp_cfg_, sl % ((uint32_t)SRSRAN_NSLOTS_PER_FRAME_NR(bwp_cfg_->cell_cfg.carrier.scs)));
		cvector_push_back(res->slots, slot_grid);
	}
}


