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

static harq_softbuffer_pool    g_harq_buffer_pool[SCHED_NR_MAX_CARRIERS];

harq_softbuffer_pool *harq_buffer_pool_self(uint32_t cc)
{
	return &g_harq_buffer_pool[cc];
}
///////////////////////////////////////////////////////////////////////////////

static bool has_pending_retx(harq_proc *proc, slot_point slot_rx)
{
	bool condition1 = !empty(proc->tb);
	bool condition2 = !proc->tb[0].ack_state;
	bool condition3 = proc->slot_ack <= slot_rx;//ack窗口小于slot tx 意思是还未确认，需要重传
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

void harq_proc_reset(harq_proc *proc)
{
	proc->tb[0].ack_state = false;
	proc->tb[0].active    = false;
	proc->tb[0].n_rtx     = 0;
	proc->tb[0].mcs       = 0xffffffff;
	proc->tb[0].tbs       = 0xffffffff;
}

bool harq_proc_set_tbs(harq_proc *proc, uint32_t tbs)
{
  if (empty(proc) || nof_retx(proc) > 0) {
    return false;
  }
  proc->tb[0].tbs = tbs;
  return true;
}

bool harq_proc_set_mcs(harq_proc *proc, uint32_t mcs)
{
  if (empty(proc->tb) || nof_retx(proc) > 0) {
    return false;
  }
  proc->tb[0].mcs = mcs;
  return true;
}

bool harq_proc_new_tx(harq_proc *proc,
					   slot_point       slot_tx_,
                       slot_point       slot_ack_,
                       prb_grant        grant,
                       uint32_t         mcs,
                       uint32_t         max_retx_)
{
  if (!empty(proc->tb)) {
    return false;
  }
  harq_proc_reset(proc);
  proc->max_retx     = max_retx_;
  proc->slot_tx      = slot_tx_;
  proc->slot_ack     = slot_ack_;
  proc->prbs_        = grant;
  proc->tb[0].ndi    = !proc->tb[0].ndi;//通过NDI（翻转为初传，否则为重传）字段
  proc->tb[0].mcs    = mcs;
  proc->tb[0].tbs    = 0;
  proc->tb[0].active = true;//表示该harq是否需要重传
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void dl_harq_proc_init(dl_harq_proc *h_dl, uint32_t cc, uint32_t id, uint32_t nprb)
{
	h_dl->proc.pid  = id;
	h_dl->softbuffer = harq_softbuffer_pool_get_tx(&g_harq_buffer_pool[cc], nprb);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ul_harq_set_tbs(ul_harq_proc *h_ul, uint32_t tbs)
{
	rx_harq_softbuffer_reset(h_ul->softbuffer, tbs);
	return harq_proc_set_tbs(h_ul->proc, tbs);
}


void ul_harq_proc_fill_dci(ul_harq_proc         *h_ul, srsran_dci_ul_nr_t *dci)
{
	const static uint32_t rv_idx[4] = {0, 2, 3, 1};

	dci->pid = h_ul->proc.pid;
	dci->ndi = ndi(h_ul->proc.tb);
	dci->mcs = mcs(h_ul->proc.tb);
	dci->rv  = rv_idx[nof_retx(&h_ul->proc) % 4];
}

bool ul_harq_proc_new_tx(ul_harq_proc          *h_ul,
						slot_point          slot_tx,
						prb_grant           grant,
						uint32_t            mcs_,
						uint32_t            max_retx,
						srsran_dci_ul_nr_t  *dci)

{
  const static uint32_t rv_idx[4] = {0, 2, 3, 1};

  //对于上行，slot_tx为预调度，slot_tx时刻为收到该消息的时刻，所以slot_ack和slot_tx一致
  if (harq_proc_new_tx(&h_ul->proc, slot_tx, slot_tx, grant, mcs_, max_retx)) {
    ul_harq_proc_fill_dci(h_ul, dci);
    return true;
  }
  return false;
}

void ul_harq_proc_init(ul_harq_proc *h_ul, uint32_t cc, uint32_t id, uint32_t nprb)
{
	h_ul->proc.pid	= id;
	h_ul->softbuffer = harq_softbuffer_pool_get_rx(&g_harq_buffer_pool[cc], nprb);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void harq_entity_init(harq_entity *harq_ent, uint32_t cc, uint16_t rnti_, uint32_t nprb, uint32_t nof_harq_procs)
{
	//每个ue都有一个harq实体，一个harq实体8个harq process
	harq_ent->rnti = rnti_;
	// Create HARQs
	cvector_reserve(harq_ent->dl_harqs, nof_harq_procs);
	cvector_reserve(harq_ent->ul_harqs, nof_harq_procs);

	for (uint32_t pid = 0; pid < nof_harq_procs; ++pid) {
		dl_harq_proc dl_harqs = {0};
		dl_harq_proc_init(&dl_harqs, cc, pid, nprb);
		cvector_push_back(harq_ent->dl_harqs, dl_harqs);

		ul_harq_proc ul_harqs = {0};
		ul_harq_proc_init(&ul_harqs, cc, pid, nprb);
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
			oset_info("[%5u] SCHED: discarding rnti=0x%x, DL TB pid=%d. Cause: Maximum number of retx exceeded (%d)",
						count_idx(&slot_rx_),
						harq_ent->rnti,
						dl_h->proc.pid,
						max_nof_retx(&dl_h->proc));
		}
	}

	ul_harq_proc *ul_h = NULL;
	cvector_for_each_in(ul_h, harq_ent->ul_harqs){
		if (harq_proc_clear_if_maxretx(&ul_h->proc ,slot_rx_)) {
			oset_info("[%5u] SCHED: discarding rnti=0x%x, UL TB pid=%d. Cause: Maximum number of retx exceeded (%d)",
						count_idx(&slot_rx_),
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

dl_harq_proc* find_pending_dl_retx(harq_entity *harq_ent)
{
	dl_harq_proc *dl_harq = NULL;
	cvector_for_each_in(dl_harq, harq_ent->dl_harqs){
		if(has_pending_retx(&dl_harq->proc, harq_ent->slot_rx)) return dl_harq;
	}
	return NULL;
}

ul_harq_proc* find_pending_ul_retx(harq_entity *harq_ent)
{
	ul_harq_proc *ul_harq = NULL;
	cvector_for_each_in(ul_harq, harq_ent->ul_harqs){
		if(has_pending_retx(&ul_harq->proc, harq_ent->slot_rx)) return ul_harq;
	}
	return NULL;
}

dl_harq_proc* find_empty_dl_harq(harq_entity *harq_ent)
{
	dl_harq_proc *dl_harq = NULL;
	cvector_for_each_in(dl_harq, harq_ent->dl_harqs){
		if(empty(dl_harq->proc.tb)) return dl_harq;
	}
	return NULL;
}

ul_harq_proc* find_empty_ul_harq(harq_entity *harq_ent)
{
	ul_harq_proc *ul_harq = NULL;
	cvector_for_each_in(ul_harq, harq_ent->ul_harqs){
		if(empty(ul_harq->proc.tb)) return ul_harq;
	}
	return NULL;
}

