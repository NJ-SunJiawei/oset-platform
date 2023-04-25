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

static bool empty(tb_t tb[SCHED_NR_MAX_TB])
{
	for(int i = 0; i < SCHED_NR_MAX_TB; ++i){
		if(tb[i].active) return !tb[i].active;
	}
	return true;
}

static bool has_pending_retx(harq_proc *proc,slot_point slot_rx)
{
	bool condition1 = !empty(proc->tb);
	bool condition2 = !proc->tb[0].ack_state;
	bool condition3 = proc->slot_ack <= slot_rx;//ack窗口小于slot tx 意思是还未确认
	return condition1 && condition2 && condition3; 
}

/////////////////////////////////////////////////////////////////////////////////////
bool harq_proc_clear_if_maxretx(harq_proc *proc, slot_point slot_rx)
{
	if (has_pending_retx(slot_rx) && nof_retx() + 1 > max_nof_retx()) {
		proc->tb[0].active = false;
		return true;
	}
	return false;
}

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
						dl_h.proc.pid,
						dl_h.max_nof_retx());
		}
	}

  for (harq_proc& ul_h : ul_harqs) {
    if (ul_h.clear_if_maxretx(slot_rx)) {
      logger.info("SCHED: discarding rnti=0x%x, UL TB pid=%d. Cause: Maximum number of retx exceeded (%d)",
                  rnti,
                  ul_h.pid,
                  ul_h.max_nof_retx());
    }
  }
}

