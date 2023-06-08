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

	// 在接入阶段且调度PDSCH的DCI通过P-RNTI、RA-RNTI和SI-RNTI加扰时，PDSCH只能使用QPSK调制，即Qm=2
	// 这样设计的原因是小区在发送寻呼、msg2和sib1信息时需要覆盖整个小区
	// 则需要通过限制调制阶数的方式降低码率，提高PDSCH的解调性能，保证覆盖

	// Allocate pending SIBs
	si_sched_run_slot(bwp_alloc, &cc_w->bwps[0].si);

	// Allocate pending RARs
	// 一次集中处理多个rar
	ra_sched_run_slot(bwp_alloc, &cc_w->bwps[0].ra);

	// TODO: Prioritize PDCCH scheduling for DL and UL data in a Round-Robin fashion
	cc_worker_alloc_dl_ues(cc_w, bwp_alloc);
	// 为DCI0-X的pucch和pusch申请资源(msg3/bsr/ul_data)
	alloc_ul_ues(bwp_alloc);

	// Post-processing of scheduling decisions
	//调度决策的后处理
	postprocess_decisions(bwp_alloc);//为UCI的pusch和pucch申请资源（sr/ack/csi）

	// Log CC scheduler result
	log_sched_bwp_result(logger, bwp_alloc.get_pdcch_tti(), bwps[0].grid, slot_ues);

	// releases UE resources
	slot_ue_clear(cc_w->cfg->cc);

	return &bwp_alloc.tx_slot_grid().dl;
}

