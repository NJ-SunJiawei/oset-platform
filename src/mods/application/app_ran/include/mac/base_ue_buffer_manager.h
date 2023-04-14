/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef BASE_UE_BUFFER_MANAGER_H_
#define BASE_UE_BUFFER_MANAGER_H_

#include "mac/sched_nr_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  MAX_LC_ID 	     (MAX_NR_NOF_BEARERS - 1)
#define  MAX_LCG_ID	     7 // Should import from sched_interface and sched_nr_interface
#define  MAX_SRB_LC_ID   MAX_NR_SRB_ID
#define  MAX_NOF_LCIDS   MAX_LC_ID + 1
#define  MAX_NOF_LCGS	 MAX_LCG_ID + 1
#define  pbr_infinity	 -1

typedef struct {
  mac_lc_ch_cfg_t cfg;
  int			  buf_tx;
  int			  buf_prio_tx;
  int			  Bj;
  int			  bucket_size;
}logical_channel;

typedef struct {
  uint16_t         rnti;
  logical_channel  channels[MAX_NOF_LCIDS];
  int              lcg_bsr[MAX_NOF_LCGS];
}base_ue_buffer_manager;

void base_ue_buffer_manager_init(base_ue_buffer_manager *base_ue, uint16_t rnti_);

#ifdef __cplusplus
}
#endif

#endif
