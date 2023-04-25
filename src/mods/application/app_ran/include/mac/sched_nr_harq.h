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
  bool	   active;
  bool	   ack_state;
  bool	   ndi;
  uint32_t n_rtx;
  uint32_t mcs;
  uint32_t tbs;
}tb_t;

typedef struct {
  uint32_t             pid;
  uint32_t             max_retx;
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

bool harq_proc_clear_if_maxretx(harq_proc *proc, slot_point slot_rx);


void harq_entity_init(harq_entity *harq_ent, uint16_t rnti_, uint32_t nprb, uint32_t nof_harq_procs);
void harq_entity_destory(harq_entity *harq_ent);
void harq_entity_new_slot(harq_entity *harq_ent, slot_point slot_rx_);

#ifdef __cplusplus
}
#endif

#endif
