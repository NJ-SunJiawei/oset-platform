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

static void bwp_slot_grid_init(bwp_slot_grid *grid, bwp_params_t *bwp_cfg_, uint32_t slot_idx_)
{
	//dl init
	cvector_reserve(grid->dl.phy.ssb, MAX_SSB);
	cvector_reserve(grid->dl.phy.pdcch_dl, MAX_GRANTS);
	cvector_reserve(grid->dl.phy.pdcch_ul, MAX_GRANTS);
	cvector_reserve(grid->dl.phy.pdsch, MAX_GRANTS);
	cvector_reserve(grid->dl.phy.nzp_csi_rs, MAX_NZP_CSI_RS);
	cvector_reserve(grid->dl.data, MAX_GRANTS);
	cvector_reserve(grid->dl.rar, MAX_GRANTS);
	cvector_reserve(grid->dl.sib_idxs, MAX_GRANTS);
	//ul init
	cvector_reserve(grid->ul.pucch, MAX_GRANTS);
	cvector_reserve(grid->ul.pusch, MAX_GRANTS);
	//ack init
	cvector_reserve(grid->pending_acks, MAX_GRANTS);

	grid->slot_idx = slot_idx_;
	grid->cfg = bwp_cfg_;
	bwp_pdcch_allocator_init(&grid->pdcchs, bwp_cfg_, slot_idx_, grid->dl.phy.pdcch_dl, grid->dl.phy.pdcch_ul);
	pdsch_allocator_init(&grid->pdschs, bwp_cfg_, slot_idx_, grid->dl.phy.pdsch);
	pusch_allocator_init(&grid->puschs, bwp_cfg_, slot_idx_, grid->dl.phy.pdsch);
	grid->rar_softbuffer = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->cfg.rb_width);//bwp_cfg_->nof_prb
}

void bwp_slot_grid_reset(bwp_slot_grid *grid)
{
	bwp_pdcch_allocator_reset(&grid->pdcchs);
	pdsch_allocator_reset(&grid->pdschs);
	pusch_allocator_reset(&grid->puschs);
	cvector_clear(grid->dl.phy.ssb);
	cvector_clear(grid->dl.phy.nzp_csi_rs);
	cvector_clear(grid->dl.data);
	cvector_clear(grid->dl.rar);
	cvector_clear(grid->dl.sib_idxs);
	cvector_clear(grid->ul.pucch);
	cvector_clear(grid->pending_acks);
}


void bwp_res_grid_init(bwp_res_grid *grid, bwp_params_t *bwp_cfg_)
{
	grid->cfg = bwp_cfg_;
	cvector_reserve(grid->slots, TTIMOD_SZ);
	//1 frame 20 slot(30khz)
	for (uint32_t sl = 0; sl < TTIMOD_SZ; ++sl){
		bwp_slot_grid_init(&grid->slots[sl], bwp_cfg_, sl % ((uint32_t)SRSRAN_NSLOTS_PER_FRAME_NR(bwp_cfg_->cell_cfg.carrier.scs)));
	}
}


