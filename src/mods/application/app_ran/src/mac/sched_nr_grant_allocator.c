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
	BOUNDED_ARRAY_SET(&grid->dl.phy.ssb, MAX_SSB);
	BOUNDED_ARRAY_SET(&grid->dl.phy.pdcch_dl, MAX_GRANTS);
	BOUNDED_ARRAY_SET(&grid->dl.phy.pdcch_ul, MAX_GRANTS);
	BOUNDED_ARRAY_SET(&grid->dl.phy.pdsch, MAX_GRANTS);
	BOUNDED_ARRAY_SET(&grid->dl.phy.nzp_csi_rs, MAX_NZP_CSI_RS);
	BOUNDED_ARRAY_SET(&grid->dl.data, MAX_GRANTS);
	BOUNDED_ARRAY_SET(&grid->dl.rar, MAX_GRANTS);
	BOUNDED_ARRAY_SET(&grid->dl.sib_idxs, MAX_GRANTS);
	//ul init
	BOUNDED_ARRAY_SET(&grid->ul.pucch, MAX_GRANTS);
	BOUNDED_ARRAY_SET(&grid->ul.pusch, MAX_GRANTS);
	//ack init
	BOUNDED_ARRAY_SET(&grid->pending_acks, MAX_GRANTS);

	grid->slot_idx = slot_idx_;
	grid->cfg = bwp_cfg_;
	bwp_pdcch_allocator_init(grid->pdcchs, slot_idx_, &grid->dl.phy.pdcch_dl, &grid->dl.phy.pdcch_ul);

	grid->rar_softbuffer = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->cfg.rb_width);//bwp_cfg_->nof_prb

}


void bwp_res_grid_init(bwp_res_grid *grid, bwp_params_t *bwp_cfg_)
{
	grid->cfg = bwp_cfg_;
	//1 frame 20 slot(30khz)
	for (uint32_t sl = 0; sl < TTIMOD_SZ; ++sl){
		bwp_slot_grid_init(&grid->slots[sl], bwp_cfg_, sl % ((uint32_t)SRSRAN_NSLOTS_PER_FRAME_NR(bwp_cfg_->cell_cfg.carrier.scs)));
	}
}


