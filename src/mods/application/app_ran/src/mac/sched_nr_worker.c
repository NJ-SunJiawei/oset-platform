/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_worker.h"
#include "mac/sched_nr_bwp.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-worker"


static void log_sched_slot_ues(slot_point pdcch_slot, cc_worker *cc_w)
{
	char dumpstr[1024] = {0};
	char *p = NULL, *last = NULL;
	char *use_comma = "";
	oset_hash_index_t *hi = NULL;

	if (0 == oset_hash_count(cc_w->slot_ues)) {
		return;
	}

	last = dumpstr + 1024;
	p = dumpstr;
	
  	p = oset_slprintf(p, last, "[%5u] SCHED: UE candidates, pdcch_tti=%u, cc=%u: [", GET_RSLOT_ID(pdcch_slot), pdcch_slot, cc);


	for (hi = oset_hash_first(cc_w->slot_ues); hi; hi = oset_hash_next(hi)) {
		uint16_t rnti = *(uint16_t *)oset_hash_this_key(hi);
		slot_ue  *ue = oset_hash_this_val(hi);

		p = oset_slprintf(p, last, "%s{rnti=0x%x", use_comma, rnti);
		if (ue->dl_active) {
			p = oset_slprintf(p, last, ", dl_bs=%u", ue->dl_bytes);
		}
		if (ue->ul_active) {
			p = oset_slprintf(p, last, ", ul_bs=%u", ue->ul_bytes);
		}
		p = oset_slprintf(p, last, "}");
		use_comma = ", ";
	}
	oset_log_print(OSET_LOG2_DEBUG, "%s]", dumpstr);
}

static void cc_worker_alloc_dl_ues(cc_worker *cc_w, bwp_slot_allocator *bwp_alloc)
{
	if (!cc_w->cfg->sched_args->pdsch_enabled) {
		return;
	}
	sched_nr_time_rr_sched_dl_users(bwp_alloc, cc_w->slot_ue_list);
}

static void cc_worker_alloc_ul_ues(cc_worker *cc_w, bwp_slot_allocator *bwp_alloc)
{
	if (! cc_w->cfg.sched_args.pusch_enabled) {
		return;
	}
	sched_nr_time_rr_sched_ul_users(bwp_alloc, cc_w->slot_ue_list);
}

void cc_worker_destoy(cc_worker *cc_w)
{
	// just idx0 for BWP-common
	bwp_manager *bwp = NULL;
	cvector_for_each_in(bwp, cc_w->bwps){
		bwp_manager_destory(bwp);
	}

	slot_ue_destory(cc_w->cfg->cc);
	//release harqbuffer
	harq_softbuffer_pool_destory(harq_buffer_pool_self(cc_w->cfg->cc), cc_w->cfg->carrier.nof_prb, 2 * SRSENB_MAX_UES * SCHED_NR_MAX_HARQ);

}

void cc_worker_init(cc_worker *cc_w, cell_config_manager *params)
{
	ASSERT_IF_NOT(cvector_size(params->bwps) > SCHED_NR_MAX_BWP_PER_CELL, "cc_worker_init error: MAX_BWP_PER_CELL > 2")

	cc_w->cfg = params;
	// Pre-allocate HARQs in common pool of softbuffers
	harq_softbuffer_pool_init(harq_buffer_pool_self(params->cc), params->carrier.nof_prb, 2 * SRSENB_MAX_UES * SCHED_NR_MAX_HARQ);

	cvector_reserve(cc_w->bwps[bwp_id], SCHED_NR_MAX_BWP_PER_CELL);
	// idx0 for BWP-common
	for (uint32_t bwp_id = 0; bwp_id < cvector_size(params->bwps); ++bwp_id) {
		bwp_manager bwp = {0};
		bwp_manager_init(&bwp, &params->bwps[bwp_id]);
		cvector_push_back(cc_w->bwps[bwp_id], bwp)
	}
	
	slot_point_init(&cc_w->last_tx_sl);
	oset_pool_init(&cc_w->slot_ue_pool, SRSENB_MAX_UES);
	cc_w->slot_ues = oset_hash_make();	
}

void cc_worker_dl_rach_info(cc_worker *cc_w, rar_info_t *rar_info)
{
	// idx0 for BWP-common
	bwp_manager *bwp = NULL;
	cvector_for_each_in(bwp, cc_w->bwps){
		ra_sched_dl_rach_info(&bwp->ra, rar_info);
	}
}

// 超前预处理上行uci/pusch配置(时频位置等)，用于上行解码
// 对pusch uci进行映射并计算新的tbs
// ??? pucch和pdsch不会重叠，pucch位置由rrc配置位于资源两端边缘，pdsch位于中间，ue放置时候会注意
static void cc_worker_postprocess_decisions(cc_worker *cc_w, bwp_slot_allocator *bwp_alloc)
{
	srsran_slot_cfg_t slot_cfg = {0};

	slot_point sl_point = get_pdcch_tti(bwp_alloc);
	bwp_slot_grid *bwp_slot = get_slot_grid(bwp_alloc, sl_point);
	slot_cfg.idx = count_idx(&sl_point);

	// 当前发送为tx调度时刻,上行方向承载uci/pusch在work-dl预处理，记录tx时刻配置(uci tx-k1时刻预授权 pusch tx-k2时刻预授权)
	// 当前接收为rx调度时刻, rx+delay=tx,当上行到达tx时刻，work-ul通过预存信息开始正式处理。

	slot_ue **ue_pair = NULL;
	cvector_for_each_in(ue_pair, cc_w->slot_ue_list){
		slot_ue *ue = *ue_pair;
		// Group pending HARQ ACKs
		// 单个ue待处理ack资源合集
		srsran_pdsch_ack_nr_t ack = {0};

		harq_ack_t *h_ack = NULL;
		cvector_for_each_in(h_ack, bwp_slot->pending_acks){
			if (h_ack->res.rnti == ue->ue->rnti) {
				ack.nof_cc = 1;  //cell
				srsran_harq_ack_m_t ack_m = {0};
				ack_m.resource            = h_ack->res;
				ack_m.present             = true;
				srsran_harq_ack_insert_m(&ack, &ack_m);
			}
		}

		// 如果UE还没有配置dedicated PUCCH resource时,初始接入对应的PUCCH 资源只能是PUCCH format 0/1,
		// PUCCH-ConfigCommon 用于配置小区级PUCCH参数（common），用于初始接入

		// 获取uci ack\sr\csi相关时频资源配置
		srsran_uci_cfg_nr_t uci_cfg = {0};
		if (!get_uci_cfg(ue_carrier_params_phy(&ue->ue->bwp_cfg), &slot_cfg, &ack, &uci_cfg)) {
			oset_error("Error getting UCI configuration");
			continue;
		}

		if (uci_cfg.ack.count == 0 && uci_cfg.nof_csi == 0 && uci_cfg.o_sr == 0) {
			continue;
		}

		// Rel-l5不支持同一个用户在同一个slot上的PUCCH和PUSCH并发
		// 当UE在上报UCI的时隙上同时有PUCCH和PUSCH时,UE使用PUSCH上报UCI信息.
		
		// 由于DCI_ul的分配,每个UE的PUSCH信道所占用的RB是不重叠的,各个UE之间的UCI信息并不会互相干扰,
		// 因此每个UE的CQI/PMI/RI都可以和ACK/NACK一起,在同一个上行slot中发送给eNB
		bool has_pusch = false;
		pusch_t *pusch = NULL;
		// bwp_slot->ul.pusch ~~ bwp_slot->puschs.puschs
		cvector_for_each_in(pusch, bwp_slot->ul.pusch){
		  if (pusch->sch.grant.rnti == ue->ue->rnti) {
		    // Put UCI configuration in PUSCH config
		    has_pusch = true;

		    // If has PUSCH, no SR shall be received
		    // 因为已经存在PUSCH资源，不需要再通过SR发送新的资源请求，所以PUSCH携带的UCI信息里并不包括SR信息
		    uci_cfg.o_sr = 0;

			// UCI只能在非DMRS符号上发送，在发送UCI的符号上，映射方式取决于用于发送UCI的RE可用总数和UCI需要的RE总数。
            // 如果剩余需要映射的UCI的RE总数大于可用RE总数的一般，那么是改符号上是连续映射的，否则均匀映射在改符号上以达到分集增益
		    if (!get_pusch_uci_cfg(ue_carrier_params_phy(&ue->ue->bwp_cfg),  &uci_cfg, &pusch->sch)) {
		      oset_error("Error setting UCI configuration in PUSCH");
		      continue;
		    }
		    break;
		  }
		}

		// PUCCH的资源是非常有限的，整个带宽一般只有几个RB分配给PUCCH使用，比如20MHz带宽有100个RB，供PUCCH使用的RB可能不到10个
		// 必须让多个UE的UCI信息共用到相同的RE资源格中,同一个RE中复用的UE个数越多，eNB能够正确解码出各个UE的UCI的概率就越低

		// SR                   HARQ-ACK             描述
		// +                    PUCCH format 0       通过Mcs参数来区分 SR和HARQ
		// -                    PUCCH format 0       通过Mcs参数来区分 SR和HARQ
		// +/- PUCCH format 0   PUCCH format 1       Drop SR
		// + PUCCH format 1     PUCCH format 1       使用SR resource发送HARQ-ACK
		// - PUCCH format 1     PUCCH format 1       使用HAQK-ACK resource发送HARQ-ACK
		// +/-                  PUCCH format 2/3/4   使用HAQK-ACK resource,追加ceil(log2(K+1))比特,K指的是与HARQ-ACK PUCCH resource有时域重叠的SR个数
		if (!has_pusch) {
		  // If any UCI information is triggered, schedule PUCCH
		  //如果触发任何UCI信息，则安排PUCCH
		  if (MAX_GRANTS = cvector_size(bwp_slot->ul.pucch)) {
		    oset_warn("SCHED: Cannot fit pending UCI into PUCCH");
		    continue;
		  }

		  pucch_t pucch = {0};
		  uci_cfg.pucch.rnti = ue->ue->rnti;
		  pucch_candidate_t candidates = {0};
		  candidates.uci_cfg = uci_cfg;
		  if (!get_pucch_uci_cfg(ue_carrier_params_phy(&ue->ue->bwp_cfg), &uci_cfg,  &candidates.resource)) {
		    oset_error("Error getting UCI CFG");
		    continue;
		  }
		  cvector_push_back(pucch.candidates, candidates);

		  // If this slot has a SR opportunity and the selected PUCCH format is 1, consider negative SR.
		  // SR PUCCH format 1 + HARQ-ACK PUCCH format 1, 在positive SR下，使用SR的PUCCH资源发送HARQ-ACK
		  if (uci_cfg.o_sr > 0 && uci_cfg.ack.count > 0 &&
		      candidates.resource.format == SRSRAN_PUCCH_NR_FORMAT_1) {
		    // Set SR negative
		    if (uci_cfg.o_sr > 0) {
		      uci_cfg.sr_positive_present = false;
		    }

		    // Append new resource
		    // 追加 在Format1下SR negative/harq-ack复用场景下的解析。使用harq-ack的PUCCH资源发送HARQ-ACK
			pucch_candidate_t candidates_add = {0};
			candidates_add.uci_cfg = uci_cfg;
		    if (!get_pucch_uci_cfg(ue_carrier_params_phy(&ue->ue->bwp_cfg), &uci_cfg,  &candidates_add.resource)) {
		      oset_error("Error getting UCI CFG");
		      continue;
		    }
			cvector_push_back(pucch.candidates, candidates_add);
		  }

		  cvector_push_back(bwp_slot->ul.pucch, pucch);
		}
	}
}


dl_res_t* cc_worker_run_slot(cc_worker *cc_w, slot_point tx_sl, oset_list_t *ue_db)
{
	// Reset old sched outputs
	if (!slot_valid(&cc_w->last_tx_sl)) {
		cc_w->last_tx_sl = tx_sl;
	}

	while (slot_point_no_equal(&cc_w->last_tx_sl, &tx_sl)) {
		slot_point_plus_plus(&cc_w->last_tx_sl);
		//(slot_rx - 1) 将已经收到的slot前一个状态全部重置
		slot_point old_slot = slot_point_sub_jump(cc_w->last_tx_sl, TX_ENB_DELAY + 1);
		bwp_manager *bwp = NULL;
		cvector_for_each_in(bwp, cc_w->bwps){
			//cc_w->bwps[0].grid.slots[SLOTS_IDX(old_slot)]
			//slot resource reset
			bwp_slot_grid_reset(bwp_res_grid_get_slot(&bwp->grid, old_slot)]);
		}
	}


	// Reserve UEs for this worker slot (select candidate UEs)
	sched_nr_ue *u = NULL, *next_u = NULL;
	oset_list_for_each_safe(ue_db, next_u, u){
		if (NULL == u->carriers[cc_w->cfg->cc]) {
			continue;
		}

		// info for a given UE on a slot to be process
		slot_ue_alloc(u, tx_sl, cc_w->cfg->cc);
	}

	// Create an BWP allocator object that will passed along to RA, SI, Data schedulers
	// todo bwp=0
	bwp_slot_allocator *bwp_alloc = bwp_slot_allocator_init(cc_w->bwps[0].grid, tx_sl);

	// Log UEs state for slot
	log_sched_slot_ues(tx_sl, cc_w);

	const uint32_t ss_id    = 0;//si info searchspace_id
	slot_point     sl_pdcch = get_pdcch_tti(bwp_alloc);

	// 计算可用prb位
	prb_bitmap *prbs_before = pdsch_allocator_occupied_prbs(bwp_res_grid_get_pdschs(bwp_alloc->bwp_grid, sl_pdcch), ss_id, srsran_dci_format_nr_1_0);

	// Allocate cell DL signalling(PBCH)
	sched_dl_signalling(bwp_alloc);

	prb_bitmap *prbs_after = pdsch_allocator_occupied_prbs(bwp_res_grid_get_pdschs(bwp_alloc->bwp_grid, sl_pdcch), ss_id, srsran_dci_format_nr_1_0);

	// todo aggr_level = 2

	// 在接入阶段且调度PDSCH的DCI通过P-RNTI、RA-RNTI和SI-RNTI加扰时，PDSCH只能使用QPSK调制，即Qm=2
	// 这样设计的原因是小区在发送寻呼、msg2和sib1信息时需要覆盖整个小区
	// 则需要通过限制调制阶数的方式降低码率，提高PDSCH的解调性能，保证覆盖

	// Allocate pending SIBs
	si_sched_run_slot(bwp_alloc, &cc_w->bwps[0].si);

	// Allocate pending RARs
	// 一次集中处理多个rar
	ra_sched_run_slot(bwp_alloc, &cc_w->bwps[0].ra);

	// TODO: Prioritize PDCCH scheduling for DL and UL data in a Round-Robin fashion
	// todo 现阶段一个tti调度一个ue
	cc_worker_alloc_dl_ues(cc_w, bwp_alloc);
	// 为DCI0-X的pucch和pusch申请资源(msg3/bsr/ul_data)
	cc_worker_alloc_ul_ues(cc_w, bwp_alloc);

	// Post-processing of scheduling decisions
	//调度决策的后处理
	//为UCI复用pusch申请的资源
	//为UCI预处理PUCCH配置，用于上行信道解析
	cc_worker_postprocess_decisions(cc_w, bwp_alloc);

	// Log CC scheduler result
	log_sched_bwp_result(get_pdcch_tti(bwp_alloc), cc_w->bwps[0].grid, cc_w->slot_ue_list);

	// releases UE resources
	slot_ue_clear(cc_w->cfg->cc);

	return &(get_tx_slot_grid(bwp_alloc)->dl);
}

