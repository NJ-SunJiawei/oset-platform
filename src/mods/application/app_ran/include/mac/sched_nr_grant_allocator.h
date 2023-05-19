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
  uint32_t            slot_idx;//转换后的slotid
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Class responsible for jointly filling the DL/UL sched result fields and allocate RB/PDCCH resources in the RB grid
 * to avoid potential RB/PDCCH collisions
 */
typedef struct{
	bwp_params_t  *cfg;
	bwp_res_grid  *bwp_grid;
	slot_point    pdcch_slot;
}bwp_slot_allocator;

void bwp_slot_grid_destory(bwp_slot_grid *slot);
void bwp_slot_grid_reset(bwp_slot_grid *slot);
void bwp_slot_grid_reserve_pdsch(bwp_slot_grid *slot, prb_grant *grant);

pdsch_allocator* bwp_res_grid_get_pdschs(bwp_res_grid *res, slot_point tti);
bwp_slot_grid* bwp_res_grid_get_slot(bwp_res_grid *res, slot_point tti);
void bwp_res_grid_destory(bwp_res_grid *res);
void bwp_res_grid_init(bwp_res_grid *res, bwp_params_t *bwp_cfg_);

slot_point get_pdcch_tti(bwp_slot_allocator *bwp_alloc);
slot_point get_rx_tti(bwp_slot_allocator *bwp_alloc);
bwp_slot_grid *get_tx_slot_grid(bwp_slot_allocator *bwp_alloc);
bwp_slot_grid *get_slot_grid(bwp_slot_allocator *bwp_alloc, slot_point slot);
bwp_slot_allocator* bwp_slot_allocator_init(bwp_res_grid *bwp_grid_, slot_point pdcch_slot_);
alloc_result bwp_slot_allocator_alloc_si(bwp_slot_allocator *bwp_alloc,
													uint32_t            aggr_idx,
													uint32_t            si_idx,
													uint32_t            si_ntx,
													prb_interval        *prbs,
													tx_harq_softbuffer  *softbuffer);

alloc_result bwp_slot_allocator_alloc_rar_and_msg3(bwp_slot_allocator *slot_grid,
													uint16_t	   ra_rnti,
													uint32_t	   aggr_idx,
													prb_interval   *interv,
													span_t(dl_sched_rar_info_t) *pending_rachs);


#ifdef __cplusplus
}
#endif

#endif
