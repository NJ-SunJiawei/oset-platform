/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "srsran/common/phy_cfg_nr_default.h"

reference_cfg_t reference_cfg_init(char *args)
{
#define list_max = 10;
	char *param_list[list_max] = {NULL};
	int num = 0;
	num = oset_split(args, ',', param_list);

  for (int i = 0; i < num, i++) {
    char *param[2] = {NULL};
	int size = oset_split(param_list[i], '=', param);

    // Skip if size is invalid
    ASSERT_IF_NOT(size == 2, "Invalid reference argument '%s'", param_list[i]);

    if (param[0] == "carrier") {
      for (carrier = R_CARRIER_CUSTOM_10MHZ; carrier < R_CARRIER_COUNT; carrier = inc(carrier)) {
        if (R_CARRIER_STRING[carrier] == param.back()) {
          break;
        }
      }
      srsran_assert(carrier != R_CARRIER_COUNT, "Invalid carrier reference configuration '%s'", param.back().c_str());
    } else if (param.front() == "duplex") {
      for (duplex = R_DUPLEX_FDD; duplex < R_DUPLEX_COUNT; duplex = inc(duplex)) {
        if (R_DUPLEX_STRING[duplex] == param.back()) {
          break;
        }
      }
      srsran_assert(duplex != R_DUPLEX_COUNT, "Invalid duplex reference configuration '%s'", param.back().c_str());
    } else if (param.front() == "pdsch") {
      for (pdsch = R_PDSCH_DEFAULT; pdsch < R_PDSCH_COUNT; pdsch = inc(pdsch)) {
        if (R_PDSCH_STRING[pdsch] == param.back()) {
          break;
        }
      }
      srsran_assert(pdsch != R_PDSCH_COUNT, "Invalid PDSCH reference configuration '%s'", param.back().c_str());
    } else {
      srsran_assertion_failure("Invalid %s reference component", param.front().c_str());
    }
  }
}

void phy_cfg_nr_default_init(reference_cfg_t *reference_cfg, phy_cfg_nr_t *phy_cfg)
{
  switch (reference_cfg->carrier) {
    case R_CARRIER_CUSTOM_10MHZ:
      make_carrier_custom_10MHz(phy_cfg->carrier);
      break;
    case R_CARRIER_CUSTOM_20MHZ:
      make_carrier_custom_20MHz(phy_cfg->carrier);
      break;
    case R_CARRIER_COUNT:
      oset_error("Invalid carrier reference");
	  oset_abort();
  }

  switch (reference_cfg->duplex) {
    case R_DUPLEX_FDD:
      phy_cfg->duplex.mode = SRSRAN_DUPLEX_MODE_FDD;
      break;
    case R_DUPLEX_TDD_CUSTOM_6_4:
      make_tdd_custom_6_4(duplex);
      break;
    case R_DUPLEX_TDD_FR1_15_1:
      make_tdd_fr1_15_1(duplex);
      break;
    case R_DUPLEX_COUNT:
      oset_error("Invalid TDD reference");
	  oset_abort();
  }

  if (phy_cfg->duplex.mode == SRSRAN_DUPLEX_MODE_TDD) {
    phy_cfg->carrier.dl_center_frequency_hz = 3513.6e6;
    phy_cfg->carrier.ul_center_frequency_hz = 3513.6e6;
    phy_cfg->ssb.scs                        = srsran_subcarrier_spacing_30kHz;
  } else {
    phy_cfg->carrier.dl_center_frequency_hz = 881.5e6;
    phy_cfg->carrier.ul_center_frequency_hz = 836.6e6;
    phy_cfg->ssb.scs                        = srsran_subcarrier_spacing_15kHz;
  }
  phy_cfg->carrier.ssb_center_freq_hz = phy_cfg->carrier.dl_center_frequency_hz;
  phy_cfg->ssb.position_in_burst[0]   = true;
  phy_cfg->ssb.periodicity_ms         = 10;

  switch (reference_cfg->pdcch) {
    case R_PDCCH_CUSTOM_COMMON_SS:
      make_pdcch_custom_common_ss(phy_cfg->pdcch, phy_cfg->carrier);
      break;
  }

  switch (reference_cfg->pdsch) {
    case R_PDSCH_DEFAULT:
      make_pdsch_default(phy_cfg->pdsch);
      break;
    case R_PDSCH_TS38101_5_2_1:
      make_pdsch_2_1_1_tdd(phy_cfg->carrier, phy_cfg->pdsch);
      break;
    case R_PDSCH_COUNT:
      oset_error("Invalid PDSCH reference configuration");
	  oset_abort();
  }

  switch (reference_cfg->pusch) {
    case R_PUSCH_DEFAULT:
      make_pusch_default(phy_cfg->pusch);
      break;
  }

  switch (reference_cfg->pucch) {
    case R_PUCCH_CUSTOM_ONE:
      make_pucch_custom_one(phy_cfg->pucch);
      break;
  }

  switch (reference_cfg->harq) {
    case R_HARQ_AUTO:
      make_harq_auto(phy_cfg->harq_ack, phy_cfg->carrier, phy_cfg->duplex);
      break;
  }

  switch (reference_cfg->prach) {
    case R_PRACH_DEFAULT_LTE:
      make_prach_default_lte(phy_cfg->prach);
      break;
  }

  phy_cfg->prach.tdd_config.configured = (phy_cfg->duplex.mode == SRSRAN_DUPLEX_MODE_TDD);

  // Make default CSI report configuration always始终设置默认CSI报告配置
  phy_cfg->csi.reports[0].channel_meas_id                    = 0;
  phy_cfg->csi.reports[0].type                               = SRSRAN_CSI_REPORT_TYPE_PERIODIC;
  phy_cfg->csi.reports[0].periodic.period                    = 20;
  phy_cfg->csi.reports[0].periodic.offset                    = 9;
  phy_cfg->csi.reports[0].periodic.resource.format           = SRSRAN_PUCCH_NR_FORMAT_2;
  phy_cfg->csi.reports[0].periodic.resource.starting_prb     = 51;
  phy_cfg->csi.reports[0].periodic.resource.format           = SRSRAN_PUCCH_NR_FORMAT_2;
  phy_cfg->csi.reports[0].periodic.resource.nof_prb          = 1;
  phy_cfg->csi.reports[0].periodic.resource.nof_symbols      = 2;
  phy_cfg->csi.reports[0].periodic.resource.start_symbol_idx = 10;
  phy_cfg->csi.reports[0].quantity                           = SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI;
  phy_cfg->csi.reports[0].cqi_table                          = SRSRAN_CSI_CQI_TABLE_1;
  phy_cfg->csi.reports[0].freq_cfg                           = SRSRAN_CSI_REPORT_FREQ_WIDEBAND;
  phy_cfg->csi.csi_resources[0].type = srsran_csi_hl_resource_cfg_t::SRSRAN_CSI_HL_RESOURCE_CFG_TYPE_NZP_CSI_RS_SSB;
  phy_cfg->csi.csi_resources[0].nzp_csi_rs_ssb.nzp_csi_rs_resource_set_id_list[0]    = 0;
  phy_cfg->csi.csi_resources[0].nzp_csi_rs_ssb.nzp_csi_rs_resource_set_id_list_count = 1;
}



