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
  cvector_vector_t(pdsch_t) pdschs;//bounded_vector<pdsch_t, MAX_GRANTS>;
  bwp_rb_bitmap dl_prbs;
}pdsch_allocator;


typedef struct {
  bwp_params_t  *bwp_cfg;
  uint32_t      slot_idx;

  cvector_vector_t(pusch_t) puschs;//bounded_vector<pusch_t, MAX_GRANTS>
  bwp_rb_bitmap ul_prbs;
}pusch_allocator;

prb_bitmap pdsch_allocator_occupied_prbs(pdsch_allocator *pdsch_alloc, uint32_t ss_id, srsran_dci_format_nr_t dci_fmt);
void pdsch_allocator_reset(pdsch_allocator *pdsch);
void pdsch_allocator_init(pdsch_allocator *pdsch, bwp_params_t *cfg_, uint32_t slot_index, cvector_vector_t(pdsch_t) pdsch_lst);
///////////////////////////////////////////////////////////////////////////
void pusch_allocator_reset(pusch_allocator *pusch);
void pusch_allocator_init(pusch_allocator *pusch, bwp_params_t *cfg_, uint32_t slot_index,  cvector_vector_t(pusch_t) pusch_lst);

#ifdef __cplusplus
}
#endif

#endif
