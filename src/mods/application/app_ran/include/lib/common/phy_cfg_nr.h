/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef PHY_CFG_NR_H_
#define PHY_CFG_NR_H_

#include "oset-core.h"
#include "lib/srsran/srsran.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************
 *      PHY Config
 **************************/

//SSB configuration
typedef struct ssb_cfg_s {
  uint32_t						   periodicity_ms;
  bool                             position_in_burst[SRSRAN_SSB_NOF_CANDIDATES];
  srsran_subcarrier_spacing_t	   scs;//(default) = srsran_subcarrier_spacing_30kHz
  srsran_ssb_pattern_t			   pattern;//(default) = SRSRAN_SSB_PATTERN_A
}ssb_cfg_t;

//Obtained from configuration file or calculation
typedef struct phy_cfg_nr_s {
  srsran_duplex_config_nr_t duplex;
  srsran_sch_hl_cfg_nr_t    pdsch;
  srsran_sch_hl_cfg_nr_t    pusch;
  srsran_pucch_nr_hl_cfg_t  pucch;
  srsran_prach_cfg_t        prach;
  srsran_pdcch_cfg_nr_t     pdcch;
  srsran_harq_ack_cfg_hl_t  harq_ack;
  srsran_csi_hl_cfg_t       csi;
  srsran_carrier_nr_t       carrier;
  ssb_cfg_t                 ssb;
  uint32_t                  t_offset; ///< n-TimingAdvanceOffset //0
}phy_cfg_nr_t;

srsran_dci_cfg_nr_t get_dci_cfg(phy_cfg_nr_t *phy_cfg);
bool get_pusch_cfg(phy_cfg_nr_t *phy_cfg, 
						srsran_slot_cfg_t   *slot_cfg,
						srsran_dci_ul_nr_t  *dci,
						srsran_sch_cfg_nr_t *pusch_cfg);
bool get_pdsch_cfg(phy_cfg_nr_t *phy_cfg,
						srsran_slot_cfg_t    *slot_cfg,
						srsran_dci_dl_nr_t   *dci,
						srsran_sch_cfg_nr_t	 *pdsch_cfg);

#ifdef __cplusplus
}
#endif

#endif
