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

#include "mac/sched_cfg.h"

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
  int              lcg_bsr[MAX_NOF_LCGS];//上行ue请求bsr缓存
}base_ue_buffer_manager;

void base_ue_buffer_manager_init(base_ue_buffer_manager *base_ue, uint16_t rnti_);
void base_ue_buffer_manager_config_lcids(base_ue_buffer_manager *base_ue, mac_lc_ch_cfg_t bearer_cfg_list[SCHED_NR_MAX_LCID]);
int base_ue_buffer_manager_get_dl_tx_total(base_ue_buffer_manager *base_ue);
int base_ue_buffer_manager_get_bsr(base_ue_buffer_manager *base_ue);
int base_ue_buffer_manager_ul_bsr(base_ue_buffer_manager *base_ue, uint32_t lcg_id, uint32_t val);
int base_ue_buffer_manager_dl_buffer_state(base_ue_buffer_manager *base_ue, uint8_t lcid, uint32_t tx_queue, uint32_t prio_tx_queue);

#ifdef __cplusplus
}
#endif

#endif
