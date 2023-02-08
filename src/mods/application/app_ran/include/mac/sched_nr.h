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
#include "lib/srsran/config.h"
#include "lib/common/slot_point.h"

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

}sched_nr;


#ifdef __cplusplus
}
#endif

#endif
