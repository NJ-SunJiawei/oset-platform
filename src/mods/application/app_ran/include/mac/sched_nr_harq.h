/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_HARQ_H_
#define SCHED_NR_HARQ_H_

#include "lib/common/slot_point.h"
#include "mac/sched_nr_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  bool	   active;//harq 重传是否激活 true激活
  bool	   ack_state;
  bool	   ndi;//通过NDI（翻转为初传，否则为重传）字段
  uint32_t n_rtx;
  uint32_t mcs;
  uint32_t tbs; //bit
}tb_t;

typedef struct {
  uint32_t             pid;
  uint32_t             max_retx;//default 4
  slot_point           slot_tx;
  slot_point           slot_ack;
  prb_grant            prbs_;
  tb_t                 tb[SCHED_NR_MAX_TB];
}harq_proc;

typedef struct {
  harq_proc           proc;
  tx_harq_softbuffer  *softbuffer;//tx_harq_softbuffer  *tx_pool[SRSRAN_MAX_PRB_NR] alloc
  byte_buffer_t       *pdu;
}dl_harq_proc;

typedef struct {
  harq_proc			proc;
  rx_harq_softbuffer  *softbuffer;//rx_harq_softbuffer  *rx_pool[SRSRAN_MAX_PRB_NR] alloc
}ul_harq_proc;


typedef struct{
  uint16_t           rnti;
  slot_point         slot_rx;
  cvector_vector_t(dl_harq_proc) dl_harqs;//std::vector<dl_harq_proc>
  cvector_vector_t(ul_harq_proc) ul_harqs;//std::vector<ul_harq_proc>
}harq_entity;

bool empty(tb_t tb[SCHED_NR_MAX_TB]);
bool empty(harq_proc *proc, uint32_t tb_idx);

uint32_t TBS(tb_t tb[SCHED_NR_MAX_TB]);
uint32_t NDI(tb_t tb[SCHED_NR_MAX_TB]);
uint32_t MCS(tb_t tb[SCHED_NR_MAX_TB]);
slot_point	harq_slot_ack(harq_proc *harq);
slot_point	harq_slot_tx(harq_proc *harq);

bool harq_proc_clear_if_maxretx(harq_proc *proc, slot_point slot_rx);
int harq_proc_ack_info(harq_proc *proc, uint32_t tb_idx, bool ack);
void harq_proc_reset(harq_proc *proc);
bool harq_proc_set_tbs(harq_proc *proc, uint32_t tbs);
bool harq_proc_set_mcs(harq_proc *proc, uint32_t mcs);
bool harq_proc_new_tx(harq_proc *proc,
					   slot_point       slot_tx_,
                       slot_point       slot_ack_,
                       prb_grant        grant,
                       uint32_t         mcs,
                       uint32_t         max_retx_);
bool harq_proc_new_retx(harq_proc *proc,
								slot_point slot_tx_,
								slot_point slot_ack_,
								prb_grant *grant);

////////////////////////////////////////////////////////////////////////////////////
void dl_harq_proc_init(dl_harq_proc *h_dl, uint32_t cc, uint32_t id, uint32_t nprb);
void dl_harq_proc_fill_dci(dl_harq_proc *h_dl, srsran_dci_dl_nr_t *dci);
bool dl_harq_proc_new_tx(dl_harq_proc *h_dl,
								slot_point          slot_tx,
								slot_point          slot_ack,
								prb_grant           *grant,
								uint32_t            mcs_,
								uint32_t            max_retx,
								srsran_dci_dl_nr_t  *dci);

////////////////////////////////////////////////////////////////////////////////////
bool ul_harq_set_tbs(ul_harq_proc *h_ul, uint32_t tbs);
void ul_harq_proc_fill_dci(ul_harq_proc          *h_ul, srsran_dci_ul_nr_t *dci);
bool ul_harq_proc_new_tx(ul_harq_proc       *h_ul,
						slot_point          slot_tx,
						prb_grant           grant,
						uint32_t            mcs_,
						uint32_t            max_retx,
						srsran_dci_ul_nr_t  *dci);
void ul_harq_proc_init(ul_harq_proc *h_ul, uint32_t cc, uint32_t id, uint32_t nprb);
////////////////////////////////////////////////////////////////////////////////////
void harq_entity_init(harq_entity *harq_ent, uint32_t cc, uint16_t rnti_, uint32_t nprb, uint32_t nof_harq_procs);
void harq_entity_destory(harq_entity *harq_ent);
void harq_entity_new_slot(harq_entity *harq_ent, slot_point slot_rx_);
uint32_t nof_dl_harqs(harq_entity *harq_ent);
uint32_t nof_ul_harqs(harq_entity *harq_ent);
int dl_ack_info(harq_entity *harq_ent, uint32_t pid, uint32_t tb_idx, bool ack);
int ul_crc_info(harq_entity *harq_ent, uint32_t pid, bool ack);
dl_harq_proc* find_pending_dl_retx(harq_entity *harq_ent);
ul_harq_proc* find_pending_ul_retx(harq_entity *harq_ent);
dl_harq_proc* find_empty_dl_harq(harq_entity *harq_ent);
ul_harq_proc* find_empty_ul_harq(harq_entity *harq_ent);
///////////////////////////////////////////////////////////////////
harq_softbuffer_pool *harq_buffer_pool_self(uint32_t cc);

#ifdef __cplusplus
}
#endif

#endif
