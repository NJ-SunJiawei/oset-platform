/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_PDCCH_H_
#define SCHED_NR_PDCCH_H_

#include "mac/sched_nr_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *handle the allocation of REs for a BWP PDCCH in a specific slot
 */
struct {
  bwp_params_t    *bwp_cfg;
  uint32_t        slot_idx;

  pdcch_dl_list_t&        pdcch_dl_list;
  pdcch_ul_list_t&        pdcch_ul_list;
  slot_coreset_list       coresets;
  const srsran_dci_ctx_t* pending_dci = nullptr; /// Saves last PDCCH allocation, in case it needs to be aborted
}bwp_pdcch_allocator;

#ifdef __cplusplus
}
#endif

#endif
