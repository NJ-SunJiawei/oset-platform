/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "lib/common/phy_cfg_nr.h"

srsran_dci_cfg_nr_t get_dci_cfg(phy_cfg_nr_t *phy_cfg)
{
  srsran_dci_cfg_nr_t dci_cfg = {0};

  // Assume BWP bandwidth equals full channel bandwidth
  dci_cfg.coreset0_bw       = phy_cfg->pdcch.coreset_present[0] ? srsran_coreset_get_bw(&phy_cfg->pdcch.coreset[0]) : 0;
  dci_cfg.bwp_dl_initial_bw = phy_cfg->carrier.nof_prb;
  dci_cfg.bwp_dl_active_bw  = phy_cfg->carrier.nof_prb;
  dci_cfg.bwp_ul_initial_bw = phy_cfg->carrier.nof_prb;
  dci_cfg.bwp_ul_active_bw  = phy_cfg->carrier.nof_prb;

  // Iterate over all SS to select monitoring options
  for (uint32_t i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; i++) {
    // Skip not configured SS
    if (! phy_cfg->pdcch.search_space_present[i]) {
      continue;
    }

    // Iterate all configured formats
    for (uint32_t j = 0; j < phy_cfg->pdcch.search_space[i].nof_formats; j++) {
      if ((phy_cfg->pdcch.search_space[i].type == srsran_search_space_type_common_3 ||
           phy_cfg->pdcch.search_space[i].type == srsran_search_space_type_common_1) &&
          phy_cfg->pdcch.search_space[i].formats[j] == srsran_dci_format_nr_0_0) {
        dci_cfg.monitor_common_0_0 = true;
      } else if (phy_cfg->pdcch.search_space[i].type == srsran_search_space_type_ue &&
                 phy_cfg->pdcch.search_space[i].formats[j] == srsran_dci_format_nr_0_0) {
        dci_cfg.monitor_0_0_and_1_0 = true;
      } else if (phy_cfg->pdcch.search_space[i].type == srsran_search_space_type_ue &&
                 phy_cfg->pdcch.search_space[i].formats[j] == srsran_dci_format_nr_0_1) {
        dci_cfg.monitor_0_1_and_1_1 = true;
      }
    }
  }

  // Set PUSCH parameters
  dci_cfg.enable_sul     = false;
  dci_cfg.enable_hopping = false;

  // Set Format 0_1 and 1_1 parameters
  dci_cfg.carrier_indicator_size = 0;
  dci_cfg.harq_ack_codebok       = phy_cfg->harq_ack.harq_ack_codebook;
  dci_cfg.nof_rb_groups          = 0;

  // Format 0_1 specific configuration (for PUSCH only)
  dci_cfg.nof_ul_bwp      = 0;
  dci_cfg.nof_ul_time_res = (phy_cfg->pusch.nof_dedicated_time_ra > 0)
                                ? phy_cfg->pusch.nof_dedicated_time_ra
                                : (phy_cfg->pusch.nof_common_time_ra > 0) ? phy_cfg->pusch.nof_common_time_ra : SRSRAN_MAX_NOF_TIME_RA;
  dci_cfg.nof_srs                        = 1;
  dci_cfg.nof_ul_layers                  = 1;
  dci_cfg.pusch_nof_cbg                  = 0;
  dci_cfg.report_trigger_size            = 0;
  dci_cfg.enable_transform_precoding     = false;
  dci_cfg.dynamic_dual_harq_ack_codebook = false;
  dci_cfg.pusch_tx_config_non_codebook   = false;
  dci_cfg.pusch_ptrs                     = false;
  dci_cfg.pusch_dynamic_betas            = false;
  dci_cfg.pusch_alloc_type               = phy_cfg->pusch.alloc;
  dci_cfg.pusch_dmrs_type                = phy_cfg->pusch.dmrs_type;
  dci_cfg.pusch_dmrs_max_len             = phy_cfg->pusch.dmrs_max_length;

  // Format 1_1 specific configuration (for PDSCH only)
  dci_cfg.nof_dl_bwp      = 0;
  dci_cfg.nof_dl_time_res = (phy_cfg->pdsch.nof_dedicated_time_ra > 0)
                                ? phy_cfg->pdsch.nof_dedicated_time_ra
                                : (phy_cfg->pdsch.nof_common_time_ra > 0) ? phy_cfg->pdsch.nof_common_time_ra : SRSRAN_MAX_NOF_TIME_RA;
  dci_cfg.nof_aperiodic_zp       = 0;
  dci_cfg.pdsch_nof_cbg          = 0;
  dci_cfg.nof_dl_to_ul_ack       = phy_cfg->harq_ack.nof_dl_data_to_ul_ack;
  dci_cfg.pdsch_inter_prb_to_prb = false;
  dci_cfg.pdsch_rm_pattern1      = false;
  dci_cfg.pdsch_rm_pattern2      = false;
  dci_cfg.pdsch_2cw              = false;
  dci_cfg.multiple_scell         = false;
  dci_cfg.pdsch_tci              = false;
  dci_cfg.pdsch_cbg_flush        = false;
  dci_cfg.pdsch_dynamic_bundling = false;
  dci_cfg.pdsch_alloc_type       = phy_cfg->pdsch.alloc;
  dci_cfg.pdsch_dmrs_type        = phy_cfg->pdsch.dmrs_type;
  dci_cfg.pdsch_dmrs_max_len     = phy_cfg->pdsch.dmrs_max_length;

  return dci_cfg;
}


bool get_pusch_cfg(phy_cfg_nr_t *phy_cfg, 
						srsran_slot_cfg_t   *slot_cfg,
						srsran_dci_ul_nr_t  *dci,
						srsran_sch_cfg_nr_t *pusch_cfg)
{
  return srsran_ra_ul_dci_to_grant_nr(&phy_cfg->carrier, slot_cfg, &phy_cfg->pusch, dci, pusch_cfg, &pusch_cfg->grant) ==
         SRSRAN_SUCCESS;
}

