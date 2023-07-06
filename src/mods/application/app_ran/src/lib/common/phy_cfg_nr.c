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
  if (srsran_harq_ack_gen_uci_cfg(&phy_cfg->harq_ack, pdsch_ack, uci_cfg) < SRSRAN_SUCCESS) {
    return false;
  }

  // Generate configuration for SR//生成SR资源配置
  // 只有处于 RRC_CONNECTED 态且保持上行同步的 UE 才会发送 SR；且 SR只能用于请求新传数据（而不是重传数据）的 UL-SCH 资源
  uint32_t sr_resource_id[SRSRAN_PUCCH_MAX_NOF_SR_RESOURCES] = {0};
  int      n = srsran_ue_ul_nr_sr_send_slot(phy_cfg->pucch.sr_resources, slot_cfg->idx, sr_resource_id);
  if (n < SRSRAN_SUCCESS) {
    ERROR("Calculating SR opportunities");
    return false;
  }

  // UE 是因为没有上行 PUSCH 资源才发送 SR 的，所以 UE 只能在 PUCCH 上发送 SR

  // 不同的UE的SR也可以在不同TTI和RB上发送，这是通过时分复用和频分复用来实现。
  // 由于 SR 资源是 UE 专用且由 eNodeB 分配的，因此 SR 资源与 UE 一一对应且 eNodeB 知道具体
  // 的对应关系。也就是说，UE 在发送 SR 信息时，并不需要指定自己的 ID（C-RNTI），eNodeB 通
  // 过 SR 资源的时频位置，就知道是哪个 UE 请求上行资源。SR 资源是通过 IE：
  // SchedulingRequestConfig 的 sr-PUCCH-ResourceIndex 字段配置的。

  // *小区内的所有 UE 通常会配置相同的 SR 周期。SR 周期的值越小，SR 上报的延时就越小，但在
  // *小区 SR 用户容量不变的情况下，每个子帧上需要预留给 SR 的 PUCCH 1 资源就越多。

  if (n > 0) {
    uci_cfg->pucch.sr_resource_id = sr_resource_id[0];//??? todo不同的ue分配的sr_resource应该不一样
    uci_cfg->o_sr                 = srsran_ra_ul_nr_nof_sr_bits((uint32_t)n);
 	// UE并不是一直有发送SR请求的需求，对于Positive SR即UE有SR请求发送，需要物理层发送SR/PUCCH，
 	// 而对于无SR发送请求的UE，在SR资源的时间点，则该SR为Negative SR
 	// sr使用pucch format0/1发送
    uci_cfg->sr_positive_present  = true;
  }

  // Generate configuration for CSI reports
  // CSI包括在PUSCH信道上发送的非周期CSI，PUCCH信道上发送的周期CSI，以及在PUCCH信道或者PUSCH信道上发送给的semi-persistent CSI
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
  // 根据harq-ack/sr/CSI复用情况，选择pucch资源
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

