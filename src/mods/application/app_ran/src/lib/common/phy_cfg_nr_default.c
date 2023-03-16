/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "lib/common/phy_cfg_nr_default.h"

static void make_carrier_custom_10MHz(srsran_carrier_nr_t *carrier)
{
  carrier->nof_prb                = 52;
  carrier->max_mimo_layers        = 1;
  carrier->pci                    = 500;
  carrier->dl_center_frequency_hz = 2.6e9;
  carrier->ssb_center_freq_hz     = carrier->dl_center_frequency_hz;
  carrier->offset_to_carrier      = 0;
  carrier->scs                    = srsran_subcarrier_spacing_15kHz;
}

static void make_carrier_custom_20MHz(srsran_carrier_nr_t *carrier)
{
  carrier->nof_prb                = 106;
  carrier->max_mimo_layers        = 1;
  carrier->pci                    = 500;
  carrier->dl_center_frequency_hz = 2.6e9;
  carrier->ssb_center_freq_hz     = carrier->dl_center_frequency_hz;
  carrier->offset_to_carrier      = 0;
  carrier->scs                    = srsran_subcarrier_spacing_15kHz;
}

static void make_tdd_custom_6_4(srsran_duplex_config_nr_t *conf)
{
  // Set the duplex mode to TDD
  conf->mode = SRSRAN_DUPLEX_MODE_TDD;

  // Select TDD config
  srsran_tdd_config_nr_t *tdd = &conf->tdd;

  // Initialise pattern
  *tdd = {0};

  // Enable pattern 1
  tdd->pattern1.period_ms      = 10;
  tdd->pattern1.nof_dl_slots   = 6;
  tdd->pattern1.nof_dl_symbols = 0;
  tdd->pattern1.nof_ul_slots   = 4;
  tdd->pattern1.nof_ul_symbols = 0;

  // Disable pattern 2
  tdd->pattern2.period_ms = 0;
}

static void make_tdd_fr1_15_1(srsran_duplex_config_nr_t *conf)
{ // Set the duplex mode to TDD
  conf->mode = SRSRAN_DUPLEX_MODE_TDD;

  // Select TDD config
  srsran_tdd_config_nr_t *tdd = &conf->tdd;

  // Initialise pattern
  *tdd = {0};

  // Enable pattern 1
  tdd->pattern1.period_ms      = 5;
  tdd->pattern1.nof_dl_slots   = 3;
  tdd->pattern1.nof_dl_symbols = 10;
  tdd->pattern1.nof_ul_slots   = 1;
  tdd->pattern1.nof_ul_symbols = 2;

  // Disable pattern 2
  tdd->pattern2.period_ms = 0;
}

static void make_pdcch_custom_common_ss(srsran_pdcch_cfg_nr_t *pdcch,  srsran_carrier_nr_t *carrier)
{
  // Configure CORESET ID 1
  pdcch->coreset_present[1]              = true;
  pdcch->coreset[1].id                   = 1;
  pdcch->coreset[1].duration             = 1;
  pdcch->coreset[1].mapping_type         = srsran_coreset_mapping_type_non_interleaved;
  pdcch->coreset[1].precoder_granularity = srsran_coreset_precoder_granularity_reg_bundle;

  // Generate frequency resources for the full BW
  for (uint32_t i = 0; i < SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE; i++) {
    pdcch->coreset[1].freq_resources[i] = i < SRSRAN_FLOOR(carrier->nof_prb, 6);
  }

  // Configure Search Space 1 as common
  pdcch->search_space_present[1]     = true;
  pdcch->search_space[1].id          = 1;
  pdcch->search_space[1].coreset_id  = 1;
  pdcch->search_space[1].duration    = 1;
  pdcch->search_space[1].formats[0]  = srsran_dci_format_nr_0_0; // DCI format for PUSCH
  pdcch->search_space[1].formats[1]  = srsran_dci_format_nr_1_0; // DCI format for PDSCH
  pdcch->search_space[1].nof_formats = 2;
  pdcch->search_space[1].type        = srsran_search_space_type_common_3;

  // Generate 1 candidate for each aggregation level if possible
  for (uint32_t L = 0; L < SRSRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR; L++) {
    pdcch->search_space[1].nof_candidates[L] =
        SRSRAN_MIN(2, srsran_pdcch_nr_max_candidates_coreset(&pdcch->coreset[1], L));
  }

  pdcch->ra_search_space_present = true;
  pdcch->ra_search_space         = pdcch->search_space[1];
  pdcch->ra_search_space.type    = srsran_search_space_type_common_1;
}

static void make_pdsch_default(srsran_sch_hl_cfg_nr_t *pdsch)
{
  // Select PDSCH time resource allocation
  pdsch->common_time_ra[0].k            = 0;
  pdsch->common_time_ra[0].mapping_type = srsran_sch_mapping_type_A;
  pdsch->common_time_ra[0].sliv = srsran_ra_type2_to_riv(SRSRAN_NSYMB_PER_SLOT_NR - 1, 1, SRSRAN_NSYMB_PER_SLOT_NR);
  pdsch->nof_common_time_ra     = 1;

  // Set contiguous PRBs as default
  pdsch->alloc = srsran_resource_alloc_type1;

  // Setup PDSCH DMRS type A position
  pdsch->typeA_pos = srsran_dmrs_sch_typeA_pos_2;
}

void make_nzp_csi_rs_ts38101_table_5_2_1(const srsran_carrier_nr_t *carrier, srsran_csi_rs_nzp_set_t *trs)
{
  // Set defaults
  *trs = {0};

  trs->trs_info = true;
  trs->count    = 4;

  srsran_csi_rs_nzp_resource_t *res1 = &trs->data[0];
  srsran_csi_rs_nzp_resource_t *res2 = &trs->data[1];
  srsran_csi_rs_nzp_resource_t *res3 = &trs->data[2];
  srsran_csi_rs_nzp_resource_t *res4 = &trs->data[3];

  res1->resource_mapping.frequency_domain_alloc[0] = true;
  res2->resource_mapping.frequency_domain_alloc[0] = true;
  res3->resource_mapping.frequency_domain_alloc[0] = true;
  res4->resource_mapping.frequency_domain_alloc[0] = true;

  res1->resource_mapping.first_symbol_idx = 6;
  res2->resource_mapping.first_symbol_idx = 10;
  res3->resource_mapping.first_symbol_idx = 6;
  res4->resource_mapping.first_symbol_idx = 10;

  res1->resource_mapping.nof_ports = 1;
  res2->resource_mapping.nof_ports = 1;
  res3->resource_mapping.nof_ports = 1;
  res4->resource_mapping.nof_ports = 1;

  res1->resource_mapping.cdm = srsran_csi_rs_cdm_nocdm;
  res2->resource_mapping.cdm = srsran_csi_rs_cdm_nocdm;
  res3->resource_mapping.cdm = srsran_csi_rs_cdm_nocdm;
  res4->resource_mapping.cdm = srsran_csi_rs_cdm_nocdm;

  res1->resource_mapping.density = srsran_csi_rs_resource_mapping_density_three;
  res2->resource_mapping.density = srsran_csi_rs_resource_mapping_density_three;
  res3->resource_mapping.density = srsran_csi_rs_resource_mapping_density_three;
  res4->resource_mapping.density = srsran_csi_rs_resource_mapping_density_three;

  if (carrier.scs == srsran_subcarrier_spacing_15kHz) {
    res1->periodicity.period = 20;
    res2->periodicity.period = 20;
    res3->periodicity.period = 20;
    res4->periodicity.period = 20;

    res1->periodicity.offset = 10;
    res2->periodicity.offset = 10;
    res3->periodicity.offset = 11;
    res4->periodicity.offset = 11;
  } else if (carrier.scs == srsran_subcarrier_spacing_30kHz) {
    res1->periodicity.period = 40;
    res2->periodicity.period = 40;
    res3->periodicity.period = 40;
    res4->periodicity.period = 40;

    res1->periodicity.offset = 20;
    res2->periodicity.offset = 20;
    res3->periodicity.offset = 21;
    res4->periodicity.offset = 21;
  } else {
    oset_error("Invalid subcarrier spacing %d kHz", 15U << (uint32_t)carrier->scs);
	oset_abort();
  }

  res1->resource_mapping.freq_band = {0, carrier->nof_prb};
  res2->resource_mapping.freq_band = {0, carrier->nof_prb};
  res3->resource_mapping.freq_band = {0, carrier->nof_prb};
  res4->resource_mapping.freq_band = {0, carrier->nof_prb};
}


static make_pdsch_2_1_1_tdd(srsran_carrier_nr_t *carrier, srsran_sch_hl_cfg_nr_t *pdsch)
{
  // Select PDSCH time resource allocation
  pdsch->common_time_ra[0].mapping_type = srsran_sch_mapping_type_A;
  pdsch->common_time_ra[0].k            = 0;
  pdsch->common_time_ra[0].sliv = srsran_ra_type2_to_riv(SRSRAN_NSYMB_PER_SLOT_NR - 2, 2, SRSRAN_NSYMB_PER_SLOT_NR);
  pdsch->nof_common_time_ra     = 1;

  // Setup PDSCH DMRS
  pdsch->typeA_pos                 = srsran_dmrs_sch_typeA_pos_2;
  pdsch->dmrs_typeA.present        = true;
  pdsch->dmrs_typeA.additional_pos = srsran_dmrs_sch_add_pos_2;

  // Make default CSI-RS for tracking from TS38101 Table 5.2.1
  make_nzp_csi_rs_ts38101_table_5_2_1(carrier, pdsch->nzp_csi_rs_sets[0]);
}

static void make_pdsch_2_1_1_tdd(srsran_carrier_nr_t *carrier, srsran_sch_hl_cfg_nr_t *pdsch)
{
  // Select PDSCH time resource allocation
  pdsch.common_time_ra[0].mapping_type = srsran_sch_mapping_type_A;
  pdsch.common_time_ra[0].k            = 0;
  pdsch.common_time_ra[0].sliv = srsran_ra_type2_to_riv(SRSRAN_NSYMB_PER_SLOT_NR - 2, 2, SRSRAN_NSYMB_PER_SLOT_NR);
  pdsch.nof_common_time_ra     = 1;

  // Setup PDSCH DMRS
  pdsch.typeA_pos                 = srsran_dmrs_sch_typeA_pos_2;
  pdsch.dmrs_typeA.present        = true;
  pdsch.dmrs_typeA.additional_pos = srsran_dmrs_sch_add_pos_2;

  // Make default CSI-RS for tracking from TS38101 Table 5.2.1
  make_nzp_csi_rs_ts38101_table_5_2_1(carrier, pdsch.nzp_csi_rs_sets[0]);
}

static void make_pusch_default(srsran_sch_hl_cfg_nr_t *pusch)
{
  // Select PUSCH time resource allocation
  pusch->common_time_ra[0].k            = 4;
  pusch->common_time_ra[0].mapping_type = srsran_sch_mapping_type_A;
  pusch->common_time_ra[0].sliv         = srsran_ra_type2_to_riv(SRSRAN_NSYMB_PER_SLOT_NR, 0, SRSRAN_NSYMB_PER_SLOT_NR);
  pusch->nof_common_time_ra             = 1;

  // Setup PUSCH DMRS type A position
  pusch->typeA_pos = srsran_dmrs_sch_typeA_pos_2;

  pusch->scaling                  = 1.0f;
  pusch->beta_offsets.ack_index1  = 9;
  pusch->beta_offsets.ack_index2  = 9;
  pusch->beta_offsets.ack_index3  = 9;
  pusch->beta_offsets.csi1_index1 = 6;
  pusch->beta_offsets.csi1_index2 = 6;
  pusch->beta_offsets.csi2_index1 = 6;
  pusch->beta_offsets.csi2_index2 = 6;
}

static void make_pucch_custom_one(srsran_pucch_nr_hl_cfg_t *pucch)
{
  // PUCCH Resource for format 1
  srsran_pucch_nr_resource_t resource_small = {0};
  resource_small.starting_prb               = 0;
  resource_small.format                     = SRSRAN_PUCCH_NR_FORMAT_1;
  resource_small.initial_cyclic_shift       = 0;
  resource_small.nof_symbols                = 14;
  resource_small.start_symbol_idx           = 0;
  resource_small.time_domain_occ            = 0;

  // PUCCH Resource for format 2
  srsran_pucch_nr_resource_t resource_big = {0};
  resource_big.starting_prb               = 51;
  resource_big.format                     = SRSRAN_PUCCH_NR_FORMAT_2;
  resource_big.nof_prb                    = 1;
  resource_big.nof_symbols                = 2;
  resource_big.start_symbol_idx           = 12;

  // Resource for SR
  srsran_pucch_nr_resource_t resource_sr = {0};
  resource_sr.starting_prb               = 51;
  resource_sr.format                     = SRSRAN_PUCCH_NR_FORMAT_1;
  resource_sr.initial_cyclic_shift       = 0;
  resource_sr.nof_symbols                = 14;
  resource_sr.start_symbol_idx           = 0;
  resource_sr.time_domain_occ            = 0;

  pucch->enabled = true;

  // Set format 1 for 1-2 bits
  pucch->sets[0].resources[0]  = resource_small;
  pucch->sets[0].resources[1]  = resource_small;
  pucch->sets[0].resources[2]  = resource_small;
  pucch->sets[0].resources[3]  = resource_small;
  pucch->sets[0].resources[4]  = resource_small;
  pucch->sets[0].resources[5]  = resource_small;
  pucch->sets[0].resources[6]  = resource_small;
  pucch->sets[0].resources[7]  = resource_small;
  pucch->sets[0].nof_resources = 8;

  // Set format 2 for more bits
  pucch->sets[1].resources[0]  = resource_big;
  pucch->sets[1].resources[1]  = resource_big;
  pucch->sets[1].resources[2]  = resource_big;
  pucch->sets[1].resources[3]  = resource_big;
  pucch->sets[1].resources[4]  = resource_big;
  pucch->sets[1].resources[5]  = resource_big;
  pucch->sets[1].resources[6]  = resource_big;
  pucch->sets[1].resources[7]  = resource_big;
  pucch->sets[1].nof_resources = 8;

  // Configure scheduling request
  pucch->sr_resources[1].configured = true;
  pucch->sr_resources[1].sr_id      = 0;
  pucch->sr_resources[1].period     = 40;
  pucch->sr_resources[1].offset     = 8;
  pucch->sr_resources[1].resource   = resource_sr;
}

static void make_harq_auto(srsran_harq_ack_cfg_hl_t *      harq,
                                          srsran_carrier_nr_t *carrier,
                                          srsran_duplex_config_nr_t *duplex_cfg)
{
	if (duplex_cfg->mode == SRSRAN_DUPLEX_MODE_TDD) {
		srsran_tdd_config_nr_t *tdd_cfg = &duplex_cfg->tdd;

		// Generate as many entries as DL slots
		harq->nof_dl_data_to_ul_ack = SRSRAN_MIN(tdd_cfg->pattern1.nof_dl_slots, SRSRAN_MAX_NOF_DL_DATA_TO_UL);
		if (tdd_cfg.pattern1.nof_dl_symbols > 0) {
		  harq->nof_dl_data_to_ul_ack++;
		}

		// Set PDSCH to ACK timing delay to 4 or more
		for (uint32_t n = 0; n < harq->nof_dl_data_to_ul_ack; n++) {
			// Set the first slots into the first UL slot
			if (harq->nof_dl_data_to_ul_ack >= 4 && n < (harq->nof_dl_data_to_ul_ack - 4)) {
				harq->dl_data_to_ul_ack[n] = harq->nof_dl_data_to_ul_ack - n;
				continue;
			}

			// After that try if n+4 is UL slot
			if (srsran_duplex_nr_is_ul(duplex_cfg, carrier->scs, n + 4)) {
				harq->dl_data_to_ul_ack[n] = 4;
				continue;
			}

			// Otherwise set delay to the first UL slot of the next TDD period
			harq->dl_data_to_ul_ack[n] = (tdd_cfg->pattern1.period_ms + tdd_cfg->pattern1.nof_dl_slots) - n;
		}
	} else {
		harq->dl_data_to_ul_ack[0]  = 4;
		harq->nof_dl_data_to_ul_ack = 1;
	}

	// Zero the rest
	for (uint32_t i = harq->nof_dl_data_to_ul_ack; i < SRSRAN_MAX_NOF_DL_DATA_TO_UL; i++) {
		harq->dl_data_to_ul_ack[i] = 0;
	}

	// Select dynamic HARQ-ACK codebook
	harq->harq_ack_codebook = srsran_pdsch_harq_ack_codebook_dynamic;
}

static void make_prach_default_lte(srsran_prach_cfg_t *prach)
{
	prach->is_nr 		   = true;
	prach->config_idx	   = 8;
	prach->root_seq_idx	   = 0;
	prach->zero_corr_zone   = 0;
	prach->freq_offset	   = 4;
	prach->num_ra_preambles = 64;
	prach->hs_flag		   = false;
}

void phy_cfg_nr_default_init(reference_cfg_t *reference_cfg, phy_cfg_nr_t *phy_cfg)
{
  switch (reference_cfg->carrier) {
    case R_CARRIER_CUSTOM_10MHZ:
      make_carrier_custom_10MHz(&phy_cfg->carrier);
      break;
    case R_CARRIER_CUSTOM_20MHZ:
      make_carrier_custom_20MHz(&phy_cfg->carrier);
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
      make_tdd_custom_6_4(&phy_cfg->duplex);
      break;
    case R_DUPLEX_TDD_FR1_15_1:
      make_tdd_fr1_15_1(&phy_cfg->duplex);
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
      make_pdcch_custom_common_ss(&phy_cfg->pdcch, &phy_cfg->carrier);
      break;
  }

  switch (reference_cfg->pdsch) {
    case R_PDSCH_DEFAULT:
      make_pdsch_default(&phy_cfg->pdsch);
      break;
    case R_PDSCH_TS38101_5_2_1:
      make_pdsch_2_1_1_tdd(&phy_cfg->carrier, &phy_cfg->pdsch);
      break;
    case R_PDSCH_COUNT:
      oset_error("Invalid PDSCH reference configuration");
	  oset_abort();
  }

  switch (reference_cfg->pusch) {
    case R_PUSCH_DEFAULT:
      make_pusch_default(&phy_cfg->pusch);
      break;
  }

  switch (reference_cfg->pucch) {
    case R_PUCCH_CUSTOM_ONE:
      make_pucch_custom_one(&phy_cfg->pucch);
      break;
  }

  switch (reference_cfg->harq) {
    case R_HARQ_AUTO:
      make_harq_auto(&phy_cfg->harq_ack, &phy_cfg->carrier, &phy_cfg->duplex);
      break;
  }

  switch (reference_cfg->prach) {
    case R_PRACH_DEFAULT_LTE:
      make_prach_default_lte(&phy_cfg->prach);
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

