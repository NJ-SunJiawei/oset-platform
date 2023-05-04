/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_harq.h"
		
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-harq"

static bool has_pending_retx(harq_proc *proc,slot_point slot_rx)
{
	bool condition1 = !empty(proc->tb);
	bool condition2 = !proc->tb[0].ack_state;
	bool condition3 = proc->slot_ack <= slot_rx;//ack窗口小于slot tx 意思是还未确认
	return condition1 && condition2 && condition3; 
}

static uint32_t nof_retx(harq_proc *proc)  { return proc->tb[0].n_rtx; }
static uint32_t max_nof_retx(harq_proc *proc)  { return proc->max_retx; }

uint32_t tbs(tb_t tb[SCHED_NR_MAX_TB]) { return tb[0].tbs; }
uint32_t ndi(tb_t tb[SCHED_NR_MAX_TB]) { return tb[0].ndi; }
uint32_t mcs(tb_t tb[SCHED_NR_MAX_TB]) { return tb[0].mcs; }

bool empty(tb_t tb[SCHED_NR_MAX_TB])
{
	for(int i = 0; i < SCHED_NR_MAX_TB; ++i){
		if(tb[i].active) return !tb[i].active;
	}
	return true;
}

bool empty(harq_proc *proc, uint32_t tb_idx)
{
	return !proc->tb[tb_idx].active;
}

slot_point	harq_slot_tx(harq_proc *harq)      { return harq->slot_tx; }
slot_point	harq_slot_ack(harq_proc *harq)      { return harq->slot_ack; }


bool harq_proc_clear_if_maxretx(harq_proc *proc, slot_point slot_rx)
{
	if (has_pending_retx(proc, slot_rx) && nof_retx(proc) + 1 > max_nof_retx(proc)) {
		proc->tb[0].active = false;
		return true;
	}
	return false;
}

int harq_proc_ack_info(harq_proc *proc, uint32_t tb_idx, bool ack)
{
	if (empty(proc, tb_idx)) {
		return OSET_ERROR;
	}
	proc->tb[tb_idx].ack_state = ack;
	if (ack) {
		proc->tb[tb_idx].active = false;
	}
	return ack ? proc->tb[tb_idx].tbs : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void harq_entity_init(harq_entity *harq_ent, uint16_t rnti_, uint32_t nprb, uint32_t nof_harq_procs)
{
	//每个ue都有一个harq实体，一个harq实体8个harq process
	harq_ent->rnti = rnti_;
	// Create HARQs
	cvector_reserve(harq_ent->dl_harqs, nof_harq_procs);
	cvector_reserve(harq_ent->ul_harqs, nof_harq_procs);

	for (uint32_t pid = 0; pid < nof_harq_procs; ++pid) {
		dl_harq_proc dl_harqs = {0};
		dl_harqs.proc.pid   = pid;
		dl_harqs.proc.prbs_ = nprb;
		cvector_push_back(harq_ent->dl_harqs, dl_harqs);

		ul_harq_proc ul_harqs = {0};
		ul_harqs.proc.pid   = pid;
		ul_harqs.proc.prbs_ = nprb;
		cvector_push_back(harq_ent->ul_harqs, ul_harqs);
	}
}

void harq_entity_destory(harq_entity       *harq_ent)
{
	// Create HARQs
	cvector_free(harq_ent->dl_harqs);
	cvector_free(harq_ent->ul_harqs);
}


void harq_entity_new_slot(harq_entity *harq_ent, slot_point slot_rx_)
{
	harq_ent->slot_rx = slot_rx_;

	dl_harq_proc *dl_h = NULL;
	cvector_for_each_in(dl_h, harq_ent->dl_harqs){
		if (harq_proc_clear_if_maxretx(&dl_h->proc ,slot_rx_)) {
			oset_info("SCHED: discarding rnti=0x%x, DL TB pid=%d. Cause: Maximum number of retx exceeded (%d)",
						harq_ent->rnti,
						dl_h->proc.pid,
						max_nof_retx(&dl_h->proc));
		}
	}

	ul_harq_proc *ul_h = NULL;
	cvector_for_each_in(ul_h, harq_ent->ul_harqs){
		if (harq_proc_clear_if_maxretx(&ul_h->proc ,slot_rx_)) {
			oset_info("SCHED: discarding rnti=0x%x, UL TB pid=%d. Cause: Maximum number of retx exceeded (%d)",
						harq_ent->rnti,
						ul_h->proc.pid,
						max_nof_retx(&ul_h->proc));
		}
  }
}

uint32_t nof_dl_harqs(harq_entity *harq_ent) { return cvector_size(harq_ent->dl_harqs); }
uint32_t nof_ul_harqs(harq_entity *harq_ent) { return cvector_size(harq_ent->ul_harqs); }

int dl_ack_info(harq_entity *harq_ent, uint32_t pid, uint32_t tb_idx, bool ack)
{ 
	return harq_proc_ack_info(&harq_ent->dl_harqs[pid], tb_idx, ack);
}

int ul_crc_info(harq_entity *harq_ent, uint32_t pid, bool ack)
{
	return harq_proc_ack_info(&harq_ent->ul_harqs[pid], 0, ack);
}

