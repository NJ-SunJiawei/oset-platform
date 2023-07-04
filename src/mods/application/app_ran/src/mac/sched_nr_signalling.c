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

void si_sched_init(si_sched *si, bwp_params_t *bwp_cfg_)
{
	si->bwp_cfg = bwp_cfg_;
	cvector_reserve(si->pending_sis, 10);

	ASSERT_IF_NOT(cvector_size(bwp_cfg_->cell_cfg.sibs) <= 10, "si_sched_init error");

	for (uint32_t i = 0; i < cvector_size(bwp_cfg_->cell_cfg.sibs); ++i) {
		si_msg_ctxt_t si_ct = {0};
		si_ct.n              = i;
		si_ct.len_bytes      = bwp_cfg_->cell_cfg.sibs[i].len;
		si_ct.period_frames  = bwp_cfg_->cell_cfg.sibs[i].period_rf;
		si_ct.win_len_slots  = bwp_cfg_->cell_cfg.sibs[i].si_window_slots;
		slot_point_init(&si_ct.win_start);
		si_ct.si_softbuffer  = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->nof_prb);
		cvector_push_back(si->pending_sis, si_ct);
	}
}

void si_sched_run_slot(bwp_slot_allocator *bwp_alloc, si_sched *si_s)
{
	if (!bwp_alloc->cfg->cfg.pdcch.coreset_present[0]) {
		// CORESET#0 must be present, otherwise SIs are not allocated
		// TODO: provide proper config
		return;
	}
	const uint32_t si_aggr_level = 2;
	const uint32_t ss_id         = 0;
	slot_point     sl_pdcch      = get_pdcch_tti(bwp_alloc);
	prb_bitmap     *prbs         = pdsch_allocator_occupied_prbs(bwp_res_grid_get_pdschs(bwp_alloc->bwp_grid, sl_pdcch), ss_id, srsran_dci_format_nr_1_0);

	// Update SI windows
	uint32_t N = cvector_size(si_s->bwp_cfg->slots);//一帧slot数量
	si_msg_ctxt_t *si = NULL;
	cvector_for_each_in(si, si_s->pending_sis){
		uint32_t x = (si->n - 1) * si->win_len_slots; //si->n窗口数量, x单位ms

		if (!slot_valid(&si->win_start)) {
		  bool start_window = false;
		  if (si->n == 0) {
		    // SIB1 (slot index zero of even frames)
		    // SIB1在偶数帧的0号slot上发送  
		    start_window = (slot_idx(&sl_pdcch) == 0 && slot_sfn(sl_pdcch) % 2 == 0);
		  } else {
		    // 5.2.2.3.2 - Acquisition of SI message
		    // SI不包含sib1
		    // SI起始帧       sfn%t=floor(x/N)
		    // SI起始slot    slot=x%N
		    start_window = (slot_sfn(sl_pdcch) % si->period_frames == x / N) && (slot_idx(&sl_pdcch) == x % N);
		  }
		  if (start_window) {
		    // If start of SI message window
		    si->win_start = sl_pdcch;
		    si->n_tx      = 0;
		  }
		} else if (si->win_start + si->win_len_slots >= sl_pdcch) {
		  // If end of SI message window
		  ASSERT_IF_NOT(si->n == 0, "[%5u] SCHED: Could not allocate SIB1, len=%d. Cause: %s", GET_RSLOT_ID(sl_pdcch), si->len_bytes, to_string(si->result));
		  oset_warn("[%5u] SCHED: Could not allocate SI message idx=%d, len=%d. Cause: %s", GET_RSLOT_ID(sl_pdcch), si->n, si->len_bytes, to_string(si->result));
		  slot_clear(&si->win_start);
		}
	}

	// Schedule pending SIBs
	if (!si_s->bwp_cfg->slots[slot_idx(&sl_pdcch)].is_dl) {
		return;
	}

	si_msg_ctxt_t *si = NULL;
	cvector_for_each_in(si, si_s->pending_sis){
		if (!slot_valid(&si->win_start) && si->n_tx >= MAX_SIB_TX) {
			continue;
		}

		// Attempt grants with increasing number of PRBs (if the number of PRBs is too low, the coderate is invalid)
		si->result         = (alloc_result)invalid_coderate;
		// 当调度PDSCH的DCI由SI-RNTI加扰，即PDSCH传输SIB1信息时，TBSize的大小不能超过2976
		uint32_t     nprbs = 8;//应该根据mcs/tb size计算prb数量???
		prb_interval grant = find_empty_interval_of_length(prbs, nprbs, 0);//仅是查找可能位置，并不在bitmap上占用
		if (prb_interval_length(&grant) >= nprbs) {
		  si->result = bwp_slot_allocator_alloc_si(bwp_alloc, si_aggr_level, si->n, si->n_tx, &grant, &si->si_softbuffer->buffer);//申请prb资源
		  if (si->result == (alloc_result)success) {
		    // SIB scheduled successfully
		 	slot_clear(&si->win_start);
		    si->n_tx++;
		    if (si->n == 0) {
		      oset_debug("[%5u] SCHED: Allocated SIB1, len=%d.", GET_RSLOT_ID(sl_pdcch), si.len_bytes);
		    } else {
		      oset_debug("[%5u] SCHED: Allocated SI message idx=%d, len=%d.", GET_RSLOT_ID(sl_pdcch), si.n, si.len_bytes);
		    }
		  }
		}
		if (si->result != (alloc_result)success) {
		  oset_warn("[%5u] SCHED: Failed to allocate SI%s%d ntx=%d", GET_RSLOT_ID(sl_pdcch), si.n == 0 ? "B" : " message idx=", si.n + 1, si.n_tx);
		}
	}
}


static void sched_ssb_basic(slot_point sl_point,
			                     uint32_t    ssb_periodicity,
			                     srsran_mib_nr_t *mib,
			                     cvector_vector_t(ssb_t) ssb_list)
{
	if (MAX_SSB == cvector_size(ssb_list)) {
		oset_error("[%5u] SCHED: Failed to allocate SSB", GET_RSLOT_ID(sl_point));
		return;
	}
	// If the periodicity is 0, it means that the parameter was not passed by the upper layers.
	// In that case, we use default value of 5ms (see Clause 4.1, TS 38.213)
	// 如果周期为0，则表示上层未传递参数。
	// 在这种情况下，我们使用默认值5ms（见TS 38.213第4.1条）
	if (ssb_periodicity == 0) {
		ssb_periodicity = DEFAULT_SSB_PERIODICITY;
	}

	uint32_t sl_cnt =  count_idx(&sl_point);
	// Perform mod operation of slot index by ssb_periodicity;
	// "ssb_periodicity * nof_slots_per_subframe" gives the number of slots in 1 ssb_periodicity time interval
	uint32_t sl_point_mod = sl_cnt % (ssb_periodicity * (uint32_t)nof_slots_per_subframe(&sl_point));

	// code below is simplified, it assumes 15kHz subcarrier spacing and sub 3GHz carrier caseA sub6以内位于子帧0和1的2、8 OFDM符号。
	// PSS在SS／PBCH块的第l个OFDM符号上, OFDM同步
	// SSS在SS／PBCH块的第3个OFDM符号上
	// 在每个周期内，多个SSB块被限制在某一个5ms的半帧内
	// PBCH的信道编码采用Polar码,调制采用QPSK调制
	// PBCH上承载着MIB消息，物理信道PBCH上的内容包括23bit MIB+8 bit additional PBCH payload,加上CRC一共56bit
	// SFN低4位在additional PBCH payload,    SFN的高6位在MIB中（所以MIB数据80ms改变一次）
	if (sl_point_mod == 0) {
		ssb_t           ssb_msg = {0};
		srsran_mib_nr_t mib_msg = *mib;
		//计算帧号
		mib_msg.sfn             = slot_sfn(&sl_point);
		//计算是否在前半帧
		mib_msg.hrf             = (slot_idx(&sl_point) % SRSRAN_NSLOTS_PER_FRAME_NR(srsran_subcarrier_spacing_15kHz) >=
		           						SRSRAN_NSLOTS_PER_FRAME_NR(srsran_subcarrier_spacing_15kHz) / 2);
		// This corresponds to "Position in Burst" = 1000
		mib_msg.ssb_idx = 0;
		// Remaining MIB parameters remain constant

		// Pack mib message to be sent to PHY(asn1c encode mib)
		int packing_ret_code = srsran_pbch_msg_nr_mib_pack(&mib_msg, &ssb_msg.pbch_msg);
		oset_assert(packing_ret_code == SRSRAN_SUCCESS, "[%5u] SSB packing returned en error", GET_RSLOT_ID(sl_point));
		cvector_push_back(ssb_list, ssb_msg);
	}
}

static void sched_nzp_csi_rs(srsran_csi_rs_nzp_set_t nzp_csi_rs_sets_cfg[SRSRAN_PHCH_CFG_MAX_NOF_CSI_RS_SETS],
								   srsran_slot_cfg_t *slot_cfg,
								   cvector_vector_t(srsran_csi_rs_nzp_resource_t) csi_rs_list)
{
	for(int nzp_idx = 0; nzp_idx < SRSRAN_PHCH_CFG_MAX_NOF_CSI_RS_SETS; nzp_idx++){
		srsran_csi_rs_nzp_set_t *set = &nzp_csi_rs_sets_cfg[nzp_idx];
		// For each NZP-CSI-RS resource available in the set
		for (uint32_t i = 0; i < set->count; ++i) {
			// Select resource
			const srsran_csi_rs_nzp_resource_t *nzp_csi_resource = &set->data[i];

			// Check if the resource is scheduled for this slot
			if (srsran_csi_rs_send(&nzp_csi_resource->periodicity, slot_cfg)) {
				if (MAX_NZP_CSI_RS == cvector_size(csi_rs_list)) {
					oset_error("SCHED: Failed to allocate NZP-CSI RS");
					return;
				}
				cvector_push_back(csi_rs_list, *nzp_csi_resource);
			}
		}
	}
}


//PBCH ssb
void sched_dl_signalling(bwp_slot_allocator *bwp_alloc)
{
	bwp_params_t      *bwp_params = bwp_alloc->cfg;
	slot_point        sl_pdcch    = get_pdcch_tti(bwp_alloc);
	bwp_slot_grid     *sl_grid    = get_tx_slot_grid(bwp_alloc);

	srsran_slot_cfg_t cfg = {0};
	cfg.idx = count_idx(&sl_pdcch);

	// Schedule SSB 一个SSB由PSS、SSS和PBCH三部分组成
	sched_ssb_basic(sl_pdcch, bwp_params->cell_cfg->ssb.periodicity_ms, &bwp_params->cell_cfg->mib, sl_grid->dl.phy.ssb);

	// Mark SSB region as occupied
	if (!cvector_empty(sl_grid->dl.phy.ssb)) {
		float ssb_offset_hz = bwp_params->cell_cfg->carrier.ssb_center_freq_hz - bwp_params->cell_cfg->carrier.dl_center_frequency_hz;
		int      ssb_offset_rb = ceil(ssb_offset_hz / (15000.0f * 12));
		int      ssb_start_rb  = bwp_params->cell_cfg->carrier.nof_prb / 2 + ssb_offset_rb - 10;//ssb相对pointA的rb偏移量
		uint32_t ssb_len_rb    = 20;  //(240scs)20RB
		oset_assert(ssb_start_rb >= 0 && ssb_start_rb + ssb_len_rb < bwp_params->cell_cfg->carrier.nof_prb);

		prb_interval interval = {(uint32_t)ssb_start_rb, ssb_start_rb + ssb_len_rb};
		prb_grant grap = prb_grant_interval_init(&interval);
		bwp_slot_grid_reserve_pdsch(sl_grid, &grap);
	}

	// Schedule NZP-CSI-RS 
	// nzp非零RE需要填充数据，先记录下来。zp零功率不填充数据，无需记录。
	sched_nzp_csi_rs(bwp_params->cfg.pdsch.nzp_csi_rs_sets, cfg, sl_grid->dl.phy.nzp_csi_rs);
}



