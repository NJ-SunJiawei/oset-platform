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

bool get_uci_cfg(phy_cfg_nr_t *phy_cfg,
					srsran_slot_cfg_t *slot_cfg,
					srsran_pdsch_ack_nr_t *pdsch_ack,
					srsran_uci_cfg_nr_t   *uci_cfg)
{
  // Generate configuration for HARQ feedback//为HARQ反馈生成配置
  if (srsran_harq_ack_gen_uci_cfg(&harq_ack, pdsch_ack, uci_cfg) < SRSRAN_SUCCESS) {
    return false;
  }

  // Generate configuration for SR//生成SR资源配置
  uint32_t sr_resource_id[SRSRAN_PUCCH_MAX_NOF_SR_RESOURCES] = {0};
  int      n = srsran_ue_ul_nr_sr_send_slot(phy_cfg->pucch.sr_resources, slot_cfg->idx, sr_resource_id);
  if (n < SRSRAN_SUCCESS) {
    ERROR("Calculating SR opportunities");
    return false;
  }

  if (n > 0) {
    uci_cfg->pucch.sr_resource_id = sr_resource_id[0];
    uci_cfg->o_sr                 = srsran_ra_ul_nr_nof_sr_bits((uint32_t)n);
    uci_cfg->sr_positive_present  = true;
  }

  // Generate configuration for CSI reports //生成CSI报告的资源配置
  n = srsran_csi_reports_generate(&phy_cfg->csi, slot_cfg, uci_cfg->csi);
  if (n > SRSRAN_SUCCESS) {
    uci_cfg->nof_csi = (uint32_t)n;
  }

  return true;
}


bool get_pusch_cfg(phy_cfg_nr_t *phy_cfg, 
						srsran_slot_cfg_t   *slot_cfg,
						srsran_dci_ul_nr_t  *dci,
						srsran_sch_cfg_nr_t *pusch_cfg)
{
  return  (SRSRAN_SUCCESS == srsran_ra_ul_dci_to_grant_nr(&phy_cfg->carrier, slot_cfg, &phy_cfg->pusch, dci, pusch_cfg, &pusch_cfg->grant));
}


bool get_pdsch_cfg(phy_cfg_nr_t *phy_cfg,
						srsran_slot_cfg_t    *slot_cfg,
						srsran_dci_dl_nr_t   *dci,
						srsran_sch_cfg_nr_t	 *pdsch_cfg)
{
  return  (SRSRAN_SUCCESS == srsran_ra_dl_dci_to_grant_nr(&phy_cfg->carrier, slot_cfg, &phy_cfg->pdsch, dci, pdsch_cfg, &pdsch_cfg.grant));
}

bool get_pdsch_ack_resource(phy_cfg_nr_t *phy_cfg,
									srsran_dci_dl_nr_t   *dci_dl,
									srsran_harq_ack_resource_t *ack_resource)
{
  return (SRSRAN_SUCCESS == srsran_harq_ack_resource(&phy_cfg->harq_ack, &dci_dl, &ack_resource));
}

bool get_pusch_uci_cfg(phy_cfg_nr_t *phy_cfg,
							srsran_uci_cfg_nr_t *uci_cfg,
							srsran_sch_cfg_nr_t *pusch_cfg)
{
  // Generate configuration for PUSCH
  if (srsran_ra_ul_set_grant_uci_nr(&phy_cfg->carrier, &phy_cfg->pusch, uci_cfg, pusch_cfg) < SRSRAN_SUCCESS) {
	return false;
  }

  return true;
}

bool get_pucch_uci_cfg(phy_cfg_nr_t *phy_cfg,
							const srsran_uci_cfg_nr_t    *uci_cfg,
							srsran_pucch_nr_resource_t   *resource)
{
  // Select PUCCH resource
  if (srsran_ra_ul_nr_pucch_resource(&phy_cfg->pucch, uci_cfg, &phy_cfg->carrier.nof_prb, resource) < SRSRAN_SUCCESS) {
	ERROR("Selecting PUCCH resource");
	return false;
  }

  return true;
}

