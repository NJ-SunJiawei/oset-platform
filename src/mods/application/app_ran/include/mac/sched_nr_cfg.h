/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef SCHED_NR_CFG_H_
#define SCHED_NR_CFG_H_

#include "oset-core.h"
#include "lib/srsran/srsran.h"
#include "lib/common/phy_cfg_nr.h"
#include "mac/sched_nr_rb.h"

#define MAX_NOF_AGGR_LEVELS  5

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sched_args_s sched_args_t;

typedef struct sched_nr_bwp_cfg_s {
  uint32_t                 start_rb;//0
  uint32_t                 rb_width;//100
  srsran_pdcch_cfg_nr_t    pdcch;
  srsran_sch_hl_cfg_nr_t   pdsch;
  srsran_sch_hl_cfg_nr_t   pusch;
  srsran_pucch_nr_hl_cfg_t pucch;
  srsran_harq_ack_cfg_hl_t harq_ack;
  uint32_t                 rar_window_size;//10 // See TS 38.331, ra-ResponseWindow: {1, 2, 4, 8, 10, 20, 40, 80}
  uint32_t                 numerology_idx;//0
}sched_nr_bwp_cfg_t;



/************************************************************************/
typedef oset_list2_t cce_pos_list[MAX_NOF_AGGR_LEVELS];//pdcch_cce_pos_list == <uint32_t, SRSRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR>
typedef cce_pos_list bwp_cce_pos_list[SRSRAN_NOF_SF_X_FRAME];


/// Table specifying if a slot has DL or UL enabled
typedef struct {
  bool is_dl;
  bool is_ul;
}slot_cfg;

typedef struct {
  uint32_t msg3_delay; ///< Includes K2 and delta. See TS 36.214 6.1.2.1.1-2/4/5
  uint32_t K;
  uint32_t S;
  uint32_t L;
}pusch_ra_time_cfg;


typedef struct {
  prb_interval	prb_limits;
  prb_interval	dci_1_0_prb_limits; /// See TS 38.214, section 5.1.2.2
  bwp_rb_bitmap usable_common_ss_prb_mask;
}coreset_cached_params;


/// Structure that extends the sched_nr_interface::bwp_cfg_t passed by upper layers with other
/// derived BWP-specific params
typedef struct bwp_params_s {
  uint32_t             bwp_id;
  uint32_t             cc;
  sched_nr_bwp_cfg_t   cfg;
  cell_config_manager cell_cfg;
  sched_args_t        sched_cfg;
  sched_nr_bwp_cfg_t  bwp_cfg;
  // derived params
  uint32_t              P;
  uint32_t              N_rbg;
  uint32_t              nof_prb;
  oset_list2_t          *slots;//<slot_cfg, SRSRAN_NOF_SF_X_FRAME>
  oset_list2_t          *pusch_ra_list;//pusch_ra_time_cfg

  bwp_cce_pos_list      *rar_cce_list;//<uint32_t, SRSRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR>
  oset_list2_t          *common_cce_list; //bwp_cce_pos_list
  bwp_rb_bitmap         cached_empty_prb_mask;
  oset_list2_t          *coresets;//coreset_cached_params
}bwp_params_t;


/// Structure packing a single cell config params, and sched args
typedef struct cell_config_manager {
  uint32_t                        cc;
  srsran_carrier_nr_t             carrier;
  srsran_mib_nr_t                 mib;
  ssb_cfg_t                       ssb;
  oset_list2_t                    *bwps;  //bwp_params_t// idx0 for BWP-common
  oset_list2_t                    *sibs; //sched_nr_cell_cfg_sib_t
  struct dl_cfg_common_sib_s      dl_cfg_common;
  struct ul_cfg_common_sib_s      ul_cfg_common;
  srsran_duplex_config_nr_t       duplex;
  sched_args_t                    sched_args;
  phy_cfg_nr_t                    default_ue_phy_cfg;
};


/// Structure packing both the sched args and all gNB NR cell configurations
typedef struct sched_params_s {
  sched_args_t                     sched_cfg;
  oset_list2_t                     *cells; //cell_config_manager
}sched_params_t;


#ifdef __cplusplus
}
#endif

#endif
