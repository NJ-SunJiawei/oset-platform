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
#include "mac/harq_softbuffer.h"

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

class harq_proc
{
  uint32_t             pid;
  uint32_t             max_retx;// = 1
  slot_point           slot_tx;
  slot_point           slot_ack;
  prb_grant            prbs_;
  tb_t                 tb[SCHED_NR_MAX_TB];
};

typedef struct {
  tx_harq_softbuffer  *softbuffer;//tx_harq_softbuffer  *tx_pool[SRSRAN_MAX_PRB_NR] alloc
  byte_buffer_t       *pdu;
}dl_harq_proc;

typedef struct {
  rx_harq_softbuffer  *softbuffer;//rx_harq_softbuffer  *rx_pool[SRSRAN_MAX_PRB_NR] alloc
}ul_harq_proc;


typedef struct{
  uint16_t           rnti;
  slot_point         slot_rx;
  A_DYN_ARRAY_OF(struct dl_harq_proc) dl_harqs;//std::vector<dl_harq_proc>
  A_DYN_ARRAY_OF(struct ul_harq_proc) ul_harqs;//std::vector<ul_harq_proc>
}harq_entity;


#ifdef __cplusplus
}
#endif

#endif
