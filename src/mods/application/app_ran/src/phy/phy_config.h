/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef PHY_NR_CONFIG_H_
#define PHY_NR_CONFIG_H_

#include "gnb_interface.h"
#include "lib/srsran/srsran.h"
#include "lib/common/asn_interface.h"


#ifdef __cplusplus
extern "C" {
#endif

/*******************phy_cfg_t************************/
typedef struct phy_cfg_s {
  // Individual cell/sector configuration list
  cvector_vector_t(phy_cell_cfg_nr_t) phy_cell_cfg_nr; //std::vector<phy_cell_cfg_nr_t>

  // Common configuration for all cells
  //struct prach_cfg_sib_lte_s      prach_cnfg;//4G??
  //struct pdsch_cfg_common_lte_s   pdsch_cnfg;//ASN_RRC_PDSCH_ConfigCommon_t//4G??
  //struct pusch_cfg_common_lte_s   pusch_cnfg;//ASN_RRC_PUSCH_ConfigCommon_t//4G??
  //struct pucch_cfg_common_lte_s   pucch_cnfg;//ASN_RRC_PUCCH_ConfigCommon_t//4G??
  //struct srs_ul_cfg_common_lte_c  srs_ul_cnfg;//4G??
  srsran_cfr_cfg_t cfr_config;
}phy_cfg_t;

#ifdef __cplusplus
}
#endif

#endif