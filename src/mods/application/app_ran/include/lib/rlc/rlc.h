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

typedef void (*bsr_callback_t)(uint32_t, uint32_t, uint32_t, uint32_t);


typedef struct {
  byte_buffer_t            *pool;
  oset_hash_t              *rlc_array; //std::map<uint16_t, std::unique_ptr<rlc_common>>
  oset_hash_t              *rlc_array_mrb;
  oset_apr_thread_rwlock_t *rwlock;
  uint32_t                 default_lcid;
  bsr_callback_t           bsr_callback;
  // Timer needed for metrics calculation
  oset_time_t              metrics_tp;
}rlc_t;

void rlc_init(rlc_t *rlc, uint32_t lcid_, bsr_callback_t bsr_callback_);

#ifdef __cplusplus
}
#endif

#endif
