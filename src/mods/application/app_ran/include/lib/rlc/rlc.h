/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef LIB_RLC_H_
#define LIB_RLC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/rlc/rlc_common.h"
#include "lib/rlc/rlc_interface_types.h"
#include "lib/rlc/rlc_metrics.h"

typedef struct {
  byte_buffer_t            *pool;
  rlc_map_t                rlc_array;
  rlc_map_t                rlc_array_mrb;
  oset_apr_thread_rwlock_t *rwlock;
  uint32_t                 default_lcid;
  bsr_callback_t           bsr_callback;
  // Timer needed for metrics calculation
  time_point               metrics_tp;
}rlc_t;


#ifdef __cplusplus
}
#endif

#endif
