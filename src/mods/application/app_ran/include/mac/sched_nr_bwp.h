/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_BWP_H_
#define SCHED_NR_BWP_H_

#include "mac/sched_nr_grant_allocator.h"
#include "mac/sched_nr_signalling.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint16_t	     ra_rnti;
  slot_point	 prach_slot;
  slot_interval  rar_win;
  cvector_vector_t(rar_info_t)  msg3_grant;//bounded_vector<dl_sched_rar_info_t, MAX_GRANTS>
}pending_rar_t;

/// RAR/Msg3 scheduler
typedef struct {
  pending_rar_t  ra_sched;
  bwp_params_t   *bwp_cfg;
  oset_queue_t   *pending_rars;//pending_rar_t
}ra_sched;

typedef struct {
  bwp_params_t                   *cfg;
  // channel-specific schedulers
  si_sched                       si;
  ra_sched                       ra;
  void                           *data_sched;//sched_nr_base
  // Stores pending allocations and PRB bitmaps
  bwp_res_grid grid;
}bwp_manager;

void bwp_manager_init(bwp_manager *bwp_m, bwp_params_t *bwp_cfg);

#ifdef __cplusplus
}
#endif

#endif
