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
#define SLOT_IDX(tti) (count_idx(&tti)%TTIMOD_SZ)

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
	pusch_allocator_init(&slot->puschs, bwp_cfg_, slot_idx_, slot->ul.pusch);
	slot->rar_softbuffer = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->cfg.rb_width);//bwp_cfg_->nof_prb
}

void bwp_slot_grid_destory(bwp_slot_grid *slot)
{
	//pddchs
	bwp_pdcch_allocator_destory(&slot->pdcchs);
	//pdsch
	pdsch_allocator_destory(&slot->pdschs);
	//pusch
	bwp_rb_bitmap_final(&slot->puschs.ul_prbs);

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


void bwp_slot_grid_reserve_pdsch(bwp_slot_grid *slot, prb_grant *grant)
{
	pdsch_allocator_reserve_prbs(&slot->pdschs, grant);
}

///////////////////////////////////////////////////////////////////////////

pdsch_allocator* bwp_res_grid_get_pdschs(bwp_res_grid *res, slot_point tti)
{
	pdsch_allocator *psdch_alloc = &res->slots[SLOT_IDX(tti)].pdschs;

	return psdch_alloc;
}

bwp_slot_grid* bwp_res_grid_get_slot(bwp_res_grid *res, slot_point tti)
{
	bwp_slot_grid *slot = &res->slots[SLOT_IDX(tti)];

	return slot;
}

void bwp_res_grid_destory(bwp_res_grid *res)
{
	bwp_slot_grid *slot = NULL;
	cvector_for_each_in(slot, res->slots){
		bwp_slot_grid_destory(slot);
	}

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
/////////////////////////////////////////////////////////////////////////
slot_point get_pdcch_tti(bwp_slot_allocator *bwp_alloc){ return bwp_alloc->pdcch_slot; }
slot_point get_tti_rx(bwp_slot_allocator *bwp_alloc) { return bwp_alloc->pdcch_slot - TX_ENB_DELAY; }
bwp_slot_grid *tx_slot_grid(bwp_slot_allocator *bwp_alloc) { return &bwp_alloc->bwp_grid->slots[SLOT_IDX(bwp_alloc->pdcch_slot)]; }

bwp_slot_allocator* bwp_slot_allocator_init(bwp_res_grid *bwp_grid_, slot_point pdcch_slot_)
{
	bwp_slot_allocator *bwp_alloc = oset_malloc(sizeof(bwp_slot_allocator));
	oset_assert(bwp_alloc);

	bwp_alloc->cfg = bwp_grid_->cfg;
	bwp_alloc->bwp_grid = bwp_grid_;
	bwp_alloc->pdcch_slot = pdcch_slot_;

	return bwp_alloc;
}

alloc_result bwp_slot_allocator_alloc_si(bwp_slot_allocator *bwp_alloc,
													uint32_t            aggr_idx,
													uint32_t            si_idx,
													uint32_t            si_ntx,
													prb_interval        *prbs,
													tx_harq_softbuffer  *softbuffer)
{
	static const uint32_t               ss_id   = 0;
	static const srsran_dci_format_nr_t dci_fmt = srsran_dci_format_nr_1_0;

	bwp_slot_grid *bwp_pdcch_slot = tx_slot_grid(bwp_alloc);

	// Verify there is space in PDSCH
	// 鉴定授权grap的合法性
	prb_grant grap = {0};
	alloc_result ret = pdsch_allocator_is_si_grant_valid(&bwp_pdcch_slot->pdschs, ss_id, prb_grant_interval_init(&grap, prbs));
	if (ret != (alloc_result)success) {
	  return ret;
	}

	// Allocate PDCCH
	// 申请dci，聚合2
	auto pdcch_result = bwp_pdcch_allocator_alloc_si_pdcch(&bwp_pdcch_slot->pdcchs, ss_id, aggr_idx);
	if (pdcch_result.is_error()) {
		logger.warning("SCHED: Cannot allocate SIB due to lack of PDCCH space.");
		return pdcch_result.error();
	}
	pdcch_dl_t& pdcch = *pdcch_result.value();

	// Allocate PDSCH (no need to verify again if there is space in PDSCH)
	pdsch_t& pdsch = bwp_pdcch_slot.pdschs.alloc_si_pdsch_unchecked(ss_id, prbs, pdcch.dci);

	// Generate DCI for SIB
	pdcch.dci_cfg.coreset0_bw = srsran_coreset_get_bw(&bwp_alloc->cfg.cfg.pdcch.coreset[0]);
	pdcch.dci.mcs             = 5;
	pdcch.dci.rv              = 0;
	pdcch.dci.sii             = si_idx == 0 ? 0 : 1;

	// Generate PDSCH
	srsran_slot_cfg_t slot_cfg;
	slot_cfg.idx = pdcch_slot.to_uint();
	int code     = srsran_ra_dl_dci_to_grant_nr(
	  &cfg.cell_cfg.carrier, &slot_cfg, &cfg.cfg.pdsch, &pdcch.dci, &pdsch.sch, &pdsch.sch.grant);
	if (code != SRSRAN_SUCCESS) {
		logger.warning("Error generating SIB PDSCH grant.");
		bwp_pdcch_slot.pdcchs.cancel_last_pdcch();
		bwp_pdcch_slot.dl.phy.pdsch.pop_back();
		return alloc_result::other_cause;
	}
	pdsch.sch.grant.tb[0].softbuffer.tx = softbuffer.get();

	// Store SI msg index
	bwp_pdcch_slot.dl.sib_idxs.push_back(si_idx);

	return alloc_result::success;
}

