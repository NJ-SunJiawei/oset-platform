/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_SCH_H_
#define SCHED_NR_SCH_H_

#include "mac/sched_common.h"
#include "mac/sched_nr_cfg.h"
#include "mac/ue_cfg_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  bwp_params_t  *bwp_cfg;
  uint32_t      slot_idx;
  cvector_vector_t(pdsch_t) *pdschs;//bounded_vector<pdsch_t, MAX_GRANTS>;
  bwp_rb_bitmap dl_prbs;
}pdsch_allocator;


typedef struct {
  bwp_params_t  *bwp_cfg;
  uint32_t      slot_idx;

  cvector_vector_t(pusch_t) *puschs;//bounded_vector<pusch_t, MAX_GRANTS>
  bwp_rb_bitmap ul_prbs;
}pusch_allocator;

#ifdef __cplusplus
}
#endif

#endif
