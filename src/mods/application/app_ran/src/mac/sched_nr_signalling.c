/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_signalling.h"
#include "mac/sched_nr_worker.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-si"

void si_sched_destory(si_sched *si)
{
	cvector_free(si->pending_sis);
}

void si_sched_init(si_sched *si,bwp_params_t *bwp_cfg_)
{
	si->bwp_cfg = bwp_cfg_;
	cvector_reserve(si->pending_sis, 10);

	ASSERT_IF_NOT(cvector_size(bwp_cfg_->cell_cfg.sibs) <= 10, "si_sched_init error")

	for (uint32_t i = 0; i < cvector_size(bwp_cfg_->cell_cfg.sibs); ++i) {
		si_msg_ctxt_t si_ct = {0};
		si_ct.n              = i;
		si_ct.len_bytes      = bwp_cfg_->cell_cfg.sibs[i].len;
		si_ct.period_frames  = bwp_cfg_->cell_cfg.sibs[i].period_rf;
		si_ct.win_len_slots  = bwp_cfg_->cell_cfg.sibs[i].si_window_slots;
		si_ct.si_softbuffer  = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->nof_prb);
		cvector_push_back(si->pending_sis, si_ct);
	}
}

//PBCH ssb
void sched_dl_signalling(bwp_slot_allocator *bwp_alloc)
{
  bwp_params_t      *bwp_params = bwp_alloc->cfg;
  slot_point        sl_pdcch    = get_pdcch_tti(bwp_alloc);
  bwp_slot_grid     *sl_grid    = tx_slot_grid(bwp_alloc);

  srsran_slot_cfg_t cfg = {0};
  cfg.idx = count_idx(&sl_pdcch);

  // Schedule SSB
  sched_ssb_basic(sl_pdcch, bwp_params.cell_cfg.ssb.periodicity_ms, bwp_params.cell_cfg.mib, sl_grid.dl.phy.ssb);

  // Mark SSB region as occupied将SSB区域标记为已占用
  if (!sl_grid.dl.phy.ssb.empty()) {
    float ssb_offset_hz =
        bwp_params.cell_cfg.carrier.ssb_center_freq_hz - bwp_params.cell_cfg.carrier.dl_center_frequency_hz;
    int      ssb_offset_rb = ceil(ssb_offset_hz / (15000.0f * 12));
    int      ssb_start_rb  = bwp_params.cell_cfg.carrier.nof_prb / 2 + ssb_offset_rb - 10;
    uint32_t ssb_len_rb    = 20;  //(240scs)20RB
    oset_assert(ssb_start_rb >= 0 && ssb_start_rb + ssb_len_rb < bwp_params.cell_cfg.carrier.nof_prb);
    sl_grid.reserve_pdsch(prb_grant({(uint32_t)ssb_start_rb, ssb_start_rb + ssb_len_rb}));//void add(const prb_interval& prbs)
  }

  // Schedule NZP-CSI-RS
  sched_nzp_csi_rs(bwp_params.cfg.pdsch.nzp_csi_rs_sets, cfg, sl_grid.dl.phy.nzp_csi_rs);
}



