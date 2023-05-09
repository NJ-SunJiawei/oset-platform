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

#define POS_IN_BURST_FIRST_BIT_IDX 0
#define POS_IN_BURST_SECOND_BIT_IDX 1
#define POS_IN_BURST_THIRD_BIT_IDX 2
#define POS_IN_BURST_FOURTH_BIT_IDX 3

#define DEFAULT_SSB_PERIODICITY 5
#define MAX_SIB_TX 8

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

static void sched_ssb_basic(slot_point sl_point,
			                     uint32_t    ssb_periodicity,
			                     srsran_mib_nr_t *mib,
			                     cvector_vector_t(ssb_t) ssb_list)
{
	if (MAX_SSB == cvector_size(ssb_list)) {
		oset_error("[%5lu] SCHED: Failed to allocate SSB", get_rx_slot_idx(sl_point));
		return;
	}
	// If the periodicity is 0, it means that the parameter was not passed by the upper layers.
	// In that case, we use default value of 5ms (see Clause 4.1, TS 38.213)
	//如果周期为0，则表示上层未传递参数。
	//在这种情况下，我们使用默认值5ms（见TS 38.213第4.1条）
	if (ssb_periodicity == 0) {
		ssb_periodicity = DEFAULT_SSB_PERIODICITY;
	}

	uint32_t sl_cnt =  count_idx(&sl_point);
	// Perform mod operation of slot index by ssb_periodicity;
	// "ssb_periodicity * nof_slots_per_subframe" gives the number of slots in 1 ssb_periodicity time interval
	uint32_t sl_point_mod = sl_cnt % (ssb_periodicity * (uint32_t)nof_slots_per_subframe(&sl_point));

	// code below is simplified, it assumes 15kHz subcarrier spacing and sub 3GHz carrier
	if (sl_point_mod == 0) {
		ssb_t           ssb_msg = {0};
		srsran_mib_nr_t mib_msg = *mib;
		mib_msg.sfn             = slot_sfn(&sl_point);//计算帧号
		mib_msg.hrf             = (slot_idx(&sl_point) % SRSRAN_NSLOTS_PER_FRAME_NR(srsran_subcarrier_spacing_15kHz) >=
		           SRSRAN_NSLOTS_PER_FRAME_NR(srsran_subcarrier_spacing_15kHz) / 2);//计算是否后半帧
		// This corresponds to "Position in Burst" = 1000
		mib_msg.ssb_idx = 0;
		// Remaining MIB parameters remain constant

		// Pack mib message to be sent to PHY(asn1c encode mib)
		int packing_ret_code = srsran_pbch_msg_nr_mib_pack(&mib_msg, &ssb_msg.pbch_msg);
		oset_assert(packing_ret_code == SRSRAN_SUCCESS, "[%5lu] SSB packing returned en error");
		cvector_push_back(ssb_list, ssb_msg)
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
  sched_ssb_basic(sl_pdcch, bwp_params->cell_cfg.ssb.periodicity_ms, &bwp_params->cell_cfg.mib, sl_grid->dl.phy.ssb);

  // Mark SSB region as occupied
  if (!cvector_empty(sl_grid->dl.phy.ssb)) {
    float ssb_offset_hz =
        bwp_params->cell_cfg.carrier.ssb_center_freq_hz - bwp_params->cell_cfg.carrier.dl_center_frequency_hz;
    int      ssb_offset_rb = ceil(ssb_offset_hz / (15000.0f * 12));
    int      ssb_start_rb  = bwp_params->cell_cfg.carrier.nof_prb / 2 + ssb_offset_rb - 10;
    uint32_t ssb_len_rb    = 20;  //(240scs)20RB
    oset_assert(ssb_start_rb >= 0 && ssb_start_rb + ssb_len_rb < bwp_params->cell_cfg.carrier.nof_prb);
    sl_grid.reserve_pdsch(prb_grant({(uint32_t)ssb_start_rb, ssb_start_rb + ssb_len_rb}));//void add(const prb_interval& prbs)
  }

  // Schedule NZP-CSI-RS
  sched_nzp_csi_rs(bwp_params->cfg.pdsch.nzp_csi_rs_sets, cfg, sl_grid->dl.phy.nzp_csi_rs);
}



