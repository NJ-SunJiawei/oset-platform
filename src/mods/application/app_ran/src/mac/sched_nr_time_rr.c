/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_time_rr.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-time-rr"

typedef bool *sched_time_rr_callback(bwp_slot_allocator *, slot_ue *);

static bool round_robin_apply(bwp_slot_allocator *bwp_alloc, cvector_vector_t(slot_ue *) slot_ue_list, uint32_t rr_count, sched_time_rr_callback func)
{
	uint32_t size = cvector_size(slot_ue_list);
	uint32_t it   = 0;

	if (0 == size) {
		return false;
	}

	it += (rr_count % size);

	for (uint32_t count = 0; count < size; ++count, ++it) {
		if (it == size) {
		  // wrap-around
		  it = 0;
		}

		if (func(bwp_alloc, slot_ue_list[it])) {
			return true;//一次只处理一个ue
		}
	}
	return false；
}


static bool sched_dl_retxs(bwp_slot_allocator *bwp_alloc, slot_ue *ue)
{	
	if (ue->h_dl != NULL && has_pending_retx(&ue->h_dl.proc, get_rx_tti(bwp_alloc))) {
		int ss_id = ue_carrier_params_find_ss_id(&ue->ue->bwp_cfg, srsran_dci_format_nr_1_0);
		if (ss_id < 0) {
		  return false;
		}
		//ue->h_dl->proc.prbs_重传bitmap
		alloc_result res = bwp_slot_allocator_alloc_pdsch(bwp_alloc, ue, ss_id, &ue->h_dl->proc.prbs_);
		if (res == (alloc_result)success) {
			return true;
		}
	}
	return false;
}

static bool sched_dl_newtxs(bwp_slot_allocator *bwp_alloc, slot_ue *ue)
{
	if (ue->dl_bytes > 0 && ue->h_dl != NULL && empty(ue->h_dl.proc.tb)) {
		int ss_id = ue_carrier_params_find_ss_id(&ue->ue->bwp_cfg, srsran_dci_format_nr_1_0);
		if (ss_id < 0) {
			return false;
		}
		// todo ？？？ 应该根据当前ue->dl_bytes数据计算RB
		// Find the largest set of available RBGs possible
		prb_grant	 prbs = find_optimal_dl_grant(bwp_alloc, ue, ss_id);
		alloc_result res  = bwp_slot_allocator_alloc_pdsch(bwp_alloc, ue, ss_id, &prbs);
		if (res == (alloc_result)success) {
			return true;
		}
	}
	return false;
}

static bool sched_ul_retxs(bwp_slot_allocator *bwp_alloc, slot_ue *ue)
{	
	if (ue->h_ul != NULL && has_pending_retx(&ue->h_ul.proc, get_rx_tti(bwp_alloc))) {
		//ue->h_dl->proc.prbs_重传bitmap
		alloc_result res = bwp_slot_allocator_alloc_pusch(bwp_alloc, ue, &ue->h_ul->proc.prbs_);
		if (res == (alloc_result)success) {
			return true;
		}
	}
	return false;
}

static bool sched_ul_newtxs(bwp_slot_allocator *bwp_alloc, slot_ue *ue)
{
	if (ue->ul_bytes > 0 && ue->h_ul != NULL && empty(ue->h_ul.proc.tb)) {
		// todo 应该根据当前ue->ul_bytes数据计算RB
		prb_interval interval = {0, bwp_alloc->cfg->cfg.rb_width};
		prb_grant prbs = prb_grant_interval_init(&interval);
		alloc_result res  = bwp_slot_allocator_alloc_pusch(bwp_alloc, ue, &prbs);
		if (res == (alloc_result)success) {
			return true;
		}
	}
	return false;
}


void sched_nr_time_rr_sched_dl_users(bwp_slot_allocator *bwp_alloc, cvector_vector_t(slot_ue *) slot_ue_list)
{
	slot_point sl_pdcch = get_pdcch_tti(bwp_alloc);

	// Start with retxs
	if(round_robin_apply(bwp_alloc, slot_ue_list, count_idx(&sl_pdcch), sched_dl_retxs)){
		return;
	}
	// Move on to new txs
	round_robin_apply(bwp_alloc, slot_ue_list, count_idx(&sl_pdcch), sched_dl_newtxs);
}

void sched_nr_time_rr_sched_ul_users(bwp_slot_allocator *bwp_alloc, cvector_vector_t(slot_ue *) slot_ue_list)
{
	slot_point sl_pdcch = get_pdcch_tti(bwp_alloc);

	// Start with retxs
	if(round_robin_apply(bwp_alloc, slot_ue_list, count_idx(&sl_pdcch), sched_ul_retxs)){
		return;
	}
	// Move on to new txs
	round_robin_apply(bwp_alloc, slot_ue_list, count_idx(&sl_pdcch), sched_ul_newtxs);
}

