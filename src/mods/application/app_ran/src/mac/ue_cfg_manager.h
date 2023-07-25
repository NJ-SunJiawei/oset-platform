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
  cvector_vector_t(sched_nr_ue_cc_cfg_t)    carriers;//bounded_vector<sched_nr_ue_cc_cfg_t, SCHED_NR_MAX_CARRIERS>
  mac_lc_ch_cfg_t                           ue_bearers[SCHED_NR_MAX_LCID];
  phy_cfg_nr_t                              phy_cfg;//rrc会更新
}ue_cfg_manager;


/// Semi-static configuration of a UE for a given CC.
/// 给定CC的UE的半静态配置。
typedef struct ue_carrier_params_s{

  uint16_t rnti;// = SRSRAN_INVALID_RNTI
  uint32_t cc;// = SRSRAN_MAX_CARRIERS

  ue_cfg_manager   *cfg_;//会从rcc信令更新
  bwp_params_t     *bwp_cfg;//首次rrc获取并转存的配置

  // derived
  cvector_vector_t(bwp_cce_pos_list)  cce_positions_list;//pdcch candidates list
  uint32_t                            ss_id_to_cce_idx[SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE];//对应ss_id的cce_positions_list下标
  srsran_dci_cfg_nr_t                 cached_dci_cfg;
}ue_carrier_params_t;


void ue_cfg_manager_init(ue_cfg_manager *ue_cfg, uint32_t enb_cc_idx);
int ue_cfg_manager_apply_config_request(ue_cfg_manager *ue_cfg, sched_nr_ue_cfg_t *cfg_req);
///////////////////////////////////////////////////////////////////////////////
void ue_carrier_params_init(ue_carrier_params_t *param, uint16_t rnti_, bwp_params_t *bwp_cfg_, ue_cfg_manager *uecfg_);
uint32_t ue_carrier_params_get_k1(ue_carrier_params_t *param, slot_point pdsch_slot);
srsran_search_space_t* ue_carrier_params_get_ss(ue_carrier_params_t *param, uint32_t ss_id);
pdcch_cce_pos_list ue_carrier_params_cce_pos_list(ue_carrier_params_t *param, uint32_t search_id, uint32_t slot_idx, uint32_t aggr_idx);
srsran_dci_cfg_nr_t ue_carrier_params_get_dci_cfg(ue_carrier_params_t *param);
int ue_carrier_params_find_ss_id(ue_carrier_params_t *param, srsran_dci_format_nr_t dci_fmt);
int ue_carrier_params_fixed_pdsch_mcs(ue_carrier_params_t *param);
int ue_carrier_params_fixed_pusch_mcs(ue_carrier_params_t *param);
phy_cfg_nr_t *ue_carrier_params_phy(ue_carrier_params_t *param);

//////////////////////////////////////////////////////////////////////////////
bool is_rnti_type_valid_in_search_space(srsran_rnti_type_t rnti_type, srsran_search_space_type_t ss_type);

#ifdef __cplusplus
}
#endif

#endif
