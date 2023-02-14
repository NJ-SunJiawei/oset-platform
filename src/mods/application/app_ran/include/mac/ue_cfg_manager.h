/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef UE_CFG_MANAGER_H_
#define UE_CFG_MANAGER_H_

#include "mac/sched_nr_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t                                  maxharq_tx;//4
  A_DYN_ARRAY_OF(sched_nr_ue_cc_cfg_t)      carriers;//bounded_vector<sched_nr_ue_cc_cfg_t, SCHED_NR_MAX_CARRIERS>
  mac_lc_ch_cfg_t                           ue_bearers[SCHED_NR_MAX_LCID];
  phy_cfg_nr_t                              phy_cfg;
}ue_cfg_manager;


/// Semi-static configuration of a UE for a given CC.
/// 给定CC的UE的半静态配置。
typedef struct ue_carrier_params_s{

  uint16_t rnti;// = SRSRAN_INVALID_RNTI
  uint32_t cc;// = SRSRAN_MAX_CARRIERS

  ue_cfg_manager   *cfg_;
  bwp_params_t     *bwp_cfg;

  // derived
  A_DYN_ARRAY_OF(bwp_cce_pos_list)    cce_positions_list;//std::vector<bwp_cce_pos_list>
  uint32_t                            ss_id_to_cce_idx[SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE];
  srsran_dci_cfg_nr_t                 cached_dci_cfg;
}ue_carrier_params_t;


#ifdef __cplusplus
}
#endif

#endif
