/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-worker"

static harq_softbuffer_pool    g_harq_buffer_pool[SCHED_NR_MAX_CARRIERS];

harq_softbuffer_pool *harq_buffer_pool_self(uint32_t cc)
{
	return &g_harq_buffer_pool[cc];
}

void cc_worker_destoy(cc_worker *cc_w)
{
	// just idx0 for BWP-common
	bwp_manager *bwp = NULL;
	cvector_for_each_in(bwp, cc_w->bwps){
		bwp_manager_destory(bwp);
	}

	oset_hash_destroy(cc_w->slot_ues);
	oset_pool_final(&cc_w->slot_ue_pool);

	//release harqbuffer
	harq_softbuffer_pool_destory(harq_buffer_pool_self(cc_w->cfg->cc), cc_w->cfg->carrier.nof_prb, 4 * MAX_HARQ, 0);

}

void cc_worker_init(cc_worker *cc_w, cell_config_manager *params)
{
	ASSERT_IF_NOT(cvector_size(params->bwps) > SCHED_NR_MAX_BWP_PER_CELL, "cc_worker_init error: MAX_BWP_PER_CELL > 2")

	cc_w->cfg = params;
	// Pre-allocate HARQs in common pool of softbuffers
	harq_softbuffer_pool_init(&g_harq_buffer_pool[params->cc], params->carrier.nof_prb, 4 * MAX_HARQ, 0);

	cvector_reserve(cc_w->bwps[bwp_id], SCHED_NR_MAX_BWP_PER_CELL);
	// idx0 for BWP-common
	for (uint32_t bwp_id = 0; bwp_id < cvector_size(params->bwps); ++bwp_id) {
		bwp_manager bwp = {0};
		bwp_manager_init(&bwp, &params->bwps[bwp_id]);
		cvector_push_back(cc_w->bwps[bwp_id], bwp)
	}

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

dl_res_t* cc_worker_run_slot(cc_worker *cc_w, slot_point tx_sl)
{
	// Reset old sched outputs
	// 正常不会进入
	if (!slot_valid(&cc_w->last_tx_sl)) {
		cc_w->last_tx_sl = tx_sl;
	}

	while (cc_w->last_tx_sl != tx_sl) {
		cc_w->last_tx_sl++;
		//(slot_rx - 1) 将已经收到的slot上一个状态全部重置
		slot_point old_slot = cc_w->last_tx_sl - TX_ENB_DELAY - 1;
		bwp_manager *bwp = NULL;
		cvector_for_each_in(bwp, cc_w->bwps){
			//cc_w->bwps[0].grid.slots[SLOTS_IDX(old_slot)]
			bwp_slot_grid_reset(bwp->grid.slots[SLOTS_IDX(old_slot)]);//slot clear
		}
	}


	// Reserve UEs for this worker slot (select candidate UEs)
	sched_nr_ue *u = NULL, *next_u = NULL;
	oset_list_for_each_safe(&mac_manager_self()->sched.sched_ue_list, next_u, u){
		if (NULL == u->carriers[cc_w->cfg.cc]) {
			continue;
		}

		// info for a given UE on a slot to be process
		slot_ue_alloc(u, tx_sl, cc_w->cfg.cc);
	}

	// Create an BWP allocator object that will passed along to RA, SI, Data schedulers
	// todo bwp=0
	bwp_slot_allocator* bwp_alloc = bwp_slot_allocator_init(cc_w->bwps[0].grid, tx_sl);

	// Log UEs state for slot
	log_sched_slot_ues(logger, tx_sl, cfg.cc, slot_ues);

	const uint32_t ss_id    = 0;//用于接收调度SIB1的PDCCH
	slot_point     sl_pdcch = bwp_alloc.get_pdcch_tti();//tx_sl

	prb_bitmap prbs_before = bwp_alloc.res_grid()[sl_pdcch].pdschs.occupied_prbs(ss_id, srsran_dci_format_nr_1_0);//获取可用prb位
	// Allocate cell DL signalling
	sched_dl_signalling(bwp_alloc);//生成下行信号ssb

	prb_bitmap prbs_after = bwp_alloc.res_grid()[sl_pdcch].pdschs.occupied_prbs(ss_id, srsran_dci_format_nr_1_0);//获取可用prb位

	// Allocate pending SIBs//一次集中处理多个si
	cc_w->bwps[0].si.run_slot(bwp_alloc);//void si_sched::run_slot(bwp_slot_allocator& bwp_alloc)申请sib prb

	// Allocate pending RARs//一次集中处理多个rar
	cc_w->bwps[0].ra.run_slot(bwp_alloc);//void ra_sched::run_slot(bwp_slot_allocator& slot_alloc)


	//函数一次轮询处理一个ue
	// TODO: Prioritize PDCCH scheduling for DL and UL data in a Round-Robin fashion
	//以轮询方式对DL和UL数据的PDCCH调度进行优先级排序
	alloc_dl_ues(bwp_alloc);//void sched_nr_time_rr::sched_dl_users
	alloc_ul_ues(bwp_alloc);//void sched_nr_time_rr::sched_ul_users //为DCI0-X的pucch和pusch申请资源（msg3/bsr/ul_data）

	// Post-processing of scheduling decisions
	//调度决策的后处理
	postprocess_decisions(bwp_alloc);//为UCI的pusch和pucch申请资源（sr/ack/csi）

	// Log CC scheduler result
	log_sched_bwp_result(logger, bwp_alloc.get_pdcch_tti(), bwps[0].grid, slot_ues);

	// releases UE resources
	slot_ue_clear(cc_w->cfg.cc);

	return &bwp_alloc.tx_slot_grid().dl;
}

