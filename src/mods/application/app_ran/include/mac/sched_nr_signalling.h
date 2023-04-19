/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_INTERFACE_TYPES_H_
#define RRC_INTERFACE_TYPES_H_

#include "mac/sched_common.h"
#include "mac/sched_nr_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // args
  uint32_t n; /// 0 for SIB1, n/index in schedulingInfoList in si-SchedulingInfo in SIB1
  uint32_t len_bytes; /// length in bytes of SIB1 / SI message
  uint32_t win_len_slots; /// window length in slots
  uint32_t period_frames; /// periodicity of SIB1/SI window in frames

  // state
  uint32_t		n_tx; /// nof transmissions of the same SIB1 / SI message
  alloc_result	result; /// last attempt to schedule SI // = (alloc_result)invalid_coderate
  slot_point	win_start; /// start of SI window, invalid if outside
  tx_harq_softbuffer *si_softbuffer;//srsran_softbuffer_tx_t
}si_msg_ctxt_t;

/// scheduler for SIBs
typedef struct {
  bwp_params_t                  *bwp_cfg;
  cvector_vector_t(si_msg_ctxt_t)  pending_sis; /// configured SIB1 and SI messages //bounded_vector<si_msg_ctxt_t, 10>
}si_sched;

void si_sched_destory(si_sched *si);
void si_sched_init(si_sched *c,bwp_params_t *bwp_cfg_);
#ifdef __cplusplus
}
#endif

#endif
