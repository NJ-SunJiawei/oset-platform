/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef SCHED_CFG_H_
#define SCHED_CFG_H_

#include "gnb_config_parser.h"
#include "lib/mac/sched_interface.h"
#include "lib/mac/mac_metrics.h"
#include "lib/common/common.h"
#include "mac/sched_rb.h"
#include "mac/harq_softbuffer.h"

#define MAX_NOF_AGGR_LEVELS  5

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	int      cce_index;//cce候选块的数量
	uint32_t cce_addr[SRSRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR];//first cce id集合
}pdcch_cce_pos_list;//bounded_vector<uint32_t, SRSRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR>

//typedef cvector_vector_t(uint32_t) pdcch_cce_pos_list;
typedef pdcch_cce_pos_list bwp_cce_pos_list[SRSRAN_NOF_SF_X_FRAME][MAX_NOF_AGGR_LEVELS];
//using bwp_cce_pos_list   = std::array<std::array<pdcch_cce_pos_list, MAX_NOF_AGGR_LEVELS>, SRSRAN_NOF_SF_X_FRAME>;


/// Table specifying if a slot has DL or UL enabled
typedef struct slot_cfg_s{
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
  bool          active;
  prb_interval	prb_limits;//coreset频谱配置宽度
  prb_interval	dci_1_0_prb_limits; /// See TS 38.214, section 5.1.2.2
  bwp_rb_bitmap usable_common_ss_prb_mask;
}coreset_cached_params;


/// Structure that extends the bwp_cfg_t passed by upper layers with other
/// derived BWP-specific params
/// rrc层获取并处理后的bwp参数
typedef struct bwp_params_s {
  uint32_t             bwp_id;
  uint32_t             cc;
  sched_nr_bwp_cfg_t   cfg;//初始化时rrc获取并转存的sched配置
  cell_config_manager  *cell_cfg;

  sched_args_t         *sched_cfg;
  //sched_nr_bwp_cfg_t   bwp_cfg;//???todo 可能不需要
  // derived params
  uint32_t              P;
  uint32_t              N_rbg;
  uint32_t              nof_prb;
  cvector_vector_t(slot_cfg)          slots;//<slot_cfg, SRSRAN_NOF_SF_X_FRAME>
  cvector_vector_t(pusch_ra_time_cfg) pusch_ra_list;//std::vector<pusch_ra_time_cfg>

  bwp_cce_pos_list                  rar_cce_list;//cvector_vector_type(uint32_t) rar_cce_list[SRSRAN_NOF_SF_X_FRAME][MAX_NOF_AGGR_LEVELS]
  bwp_cce_pos_list                  common_cce_list[SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE]; //optional_vector<bwp_cce_pos_list>
  bwp_rb_bitmap                     cached_empty_prb_mask;
  coreset_cached_params             coresets[SRSRAN_UE_DL_NR_MAX_NOF_CORESET];//optional_vector<coreset_cached_params>
}bwp_params_t;


/// Structure packing a single cell config params, and sched args
typedef struct {
  uint32_t                        cc;
  srsran_carrier_nr_t             carrier;
  srsran_mib_nr_t                 mib;
  ssb_cfg_t                       ssb;
  cvector_vector_t(bwp_params_t)  bwps; //std::vector<bwp_params_t>// idx0 for BWP-common
  cvector_vector_t(sched_nr_cell_cfg_sib_t)  sibs; //std::vector<sched_nr_cell_cfg_sib_t>
  struct dl_cfg_common_sib_s      dl_cfg_common;
  struct ul_cfg_common_sib_s      ul_cfg_common;
  srsran_duplex_config_nr_t       duplex;
  sched_args_t                    *sched_args;
  phy_cfg_nr_t                    default_ue_phy_cfg;
}cell_config_manager;


/// Structure packing both the sched args and all gNB NR cell configurations
typedef struct sched_params_s {
  sched_args_t                           *sched_cfg;
  cvector_vector_t(cell_config_manager)  cells; //std::vector<cell_config_manager> 
}sched_params_t;

srsran_search_space_t* get_ss(bwp_params_t *param, uint32_t ss_id);

void get_dci_locs(srsran_coreset_t      *coreset,
                      srsran_search_space_t *search_space,
                      uint16_t             rnti,
                      pdcch_cce_pos_list  (*cce_locs)[MAX_NOF_AGGR_LEVELS]);

bwp_rb_bitmap* dci_fmt_1_0_excluded_prbs(bwp_params_t *param, uint32_t cs_id);
void cell_config_manager_init(cell_config_manager *cell_cof_manager,
										uint32_t cc_,
										sched_nr_cell_cfg_t *cell,
										sched_args_t *sched_args_);

#ifdef __cplusplus
}
#endif

#endif
