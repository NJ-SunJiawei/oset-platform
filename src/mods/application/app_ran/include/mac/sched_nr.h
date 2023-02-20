/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef SCHED_NR_H_
#define SCHED_NR_H_

#include "mac/sched_nr_cfg.h"
#include "mac/sched_nr_ue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // args
  sched_params_t cfg;
  // slot-specific
  slot_point     current_slot_tx;
  int            worker_count;
  A_DYN_ARRAY_OF(cc_worker)   cc_workers; //std::vector<std::unique_ptr<sched_nr_impl::cc_worker> >
  // UE Database
  OSET_POOL(ue_pool, ue);
  oset_hash_t			 *ue_db;//static_circular_map<uint16_t, std::unique_ptr<ue_nr>, SRSENB_MAX_UES>
}sched_nr;

void sched_nr_init(sched_nr *scheluder);
void sched_nr_destory(sched_nr *scheluder);

#ifdef __cplusplus
}
#endif

#endif
