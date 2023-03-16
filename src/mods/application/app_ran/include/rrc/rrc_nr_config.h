/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef RRC_NR_CONFIG_H_
#define RRC_NR_CONFIG_H_

#include "gnb_interface.h"
#include "lib/common/asn_interface.h"
#include "lib/rrc/rrc_message_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************rrc_cell_cfg_nr_t****************************/
// Cell/Sector configuration for NR cells
typedef struct rrc_cell_cfg_nr_s{
  int                              cell_idx;
  phy_cell_cfg_nr_t                phy_cell; // already contains all PHY-related parameters (i.e. RF port, PCI, etc.)
  uint32_t                         tac;      // Tracking area code
  uint32_t                         dl_arfcn; // DL freq already included in carrier
  uint32_t                         ul_arfcn; // UL freq also in carrier
  uint32_t                         dl_absolute_freq_point_a; // derived from DL ARFCN
  uint32_t                         ul_absolute_freq_point_a; // derived from UL ARFCN
  uint32_t                         band;
  uint32_t                         prach_root_seq_idx;
  uint32_t                         num_ra_preambles;
  uint32_t                         coreset0_idx; // Table 13-{1,...15} row index
  srsran_duplex_mode_t             duplex_mode;
  double                           ssb_freq_hz;
  uint32_t                         ssb_absolute_freq_point; // derived from DL ARFCN (SSB arfcn)
  uint32_t                         ssb_offset;
  srsran_subcarrier_spacing_t      ssb_scs;
  srsran_ssb_pattern_t             ssb_pattern;
  struct  pdcch_cfg_common_s       pdcch_cfg_common;//ASN_RRC_PDCCH_ConfigCommon_t
  struct  pdcch_cfg_s              pdcch_cfg_ded;//ASN_RRC_PDCCH_Config_t
  int8_t                           pdsch_rs_power;
}rrc_cell_cfg_nr_t;

/***************rrc_cfg_t****************************/
typedef struct rrc_cfg_s{
  uint32_t enb_id; ///< Required to pack SIB1
  // Per eNB SIBs
  srsran_mib_nr_t          mib;
  struct sib_type1_s       sib1;//ASN_RRC_SIB1_t
  enum sib_info_types_opts sibs_type;//ASN_RRC_SIB_TypeInfo_t 
  struct sib_type2_lte_s   sib2;//ASN_RRC_SIB2_t
  struct sib_type3_s       sib3;//ASN_RRC_SIB3_t
  struct mac_main_cfg_s    mac_cnfg;
  struct pusch_cfg_ded_s   pusch_cfg;
  struct ant_info_ded_s    antenna_info;
  enum p_a_opts			   pdsch_cfg;
  rrc_cfg_sr_t             sr_cfg;
  rrc_cfg_cqi_t            cqi_cfg;
  //oset_hash_t              *qci_cfg;//std::map<uint32_t, rrc_cfg_qci_t>
  srsran_cell_t            cell;
  uint32_t       num_nr_cells; /// number of configured NR cells (used to configure RF)
  uint32_t       max_mac_dl_kos;
  uint32_t       max_mac_ul_kos;
  uint32_t       rlf_release_timer_ms;
}rrc_cfg_t;

/***************rrc_nr_cfg_t****************************/
typedef oset_list2_t rrc_cell_list_nr_t;

typedef struct rrc_nr_cfg_s{
  rrc_cell_list_nr_t *cell_list; //std::vector<rrc_cell_cfg_nr_t>
  uint32_t           inactivity_timeout_ms;//100000
  uint32_t           enb_id;
  uint16_t           mcc;
  uint16_t           mnc;
  bool               is_standalone;// SA mode.
  srb_5g_cfg_t       srb1_cfg;
  srb_5g_cfg_t       srb2_cfg;
  oset_list2_t       *five_qi_cfg; //std::map<uint32_t, rrc_nr_cfg_five_qi_t>;
  uint8_t nea_preference_list[4];//CIPHERING_ALGORITHM_ID_NR_ENUM
  uint8_t nia_preference_list[4];//INTEGRITY_ALGORITHM_ID_NR_ENUM
}rrc_nr_cfg_t;

#ifdef __cplusplus
}
#endif

#endif
