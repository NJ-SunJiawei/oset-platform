/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_GRANT_ALLOCATOR_H
#define SCHED_NR_GRANT_ALLOCATOR_H

#include "mac/sched_nr_pdcch.h"
#include "mac/sched_nr_sch.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct harq_ack {
  phy_cfg_nr_t                *phy_cfg;
  srsran_harq_ack_resource_t  res;
}harq_ack_t;
/// save data for scheduler to keep track of previous allocations
/// This only contains information about a given slot

typedef struct {
  uint32_t            slot_idx;
  bwp_params_t        *cfg;

  dl_res_t                    dl;
  ul_sched_t                  ul;
  cvector_vector_t(harq_ack_t)  pending_acks;//bounded_vector<harq_ack_t, MAX_GRANTS>;
  bwp_pdcch_allocator pdcchs; /// slot PDCCH resource allocator
  pdsch_allocator     pdschs; /// slot PDSCH resource allocator
  pusch_allocator     puschs; /// slot PUSCH resource allocator

  tx_harq_softbuffer  *rar_softbuffer;
}bwp_slot_grid;


typedef struct {
  bwp_params_t  *cfg;
  // TTIMOD_SZ is the longest allocation in the future
  cvector_vector_t(bwp_slot_grid) slots;//bounded_vector<bwp_slot_grid, TTIMOD_SZ>;
}bwp_res_grid;

void bwp_slot_grid_destory(bwp_slot_grid *slot);
void bwp_slot_grid_reset(bwp_slot_grid *slot);

void bwp_res_grid_destory(bwp_res_grid *res);
void bwp_res_grid_init(bwp_res_grid *res, bwp_params_t *bwp_cfg_);

#ifdef __cplusplus
}
#endif

#endif
