/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "rrc/rrc.h"
#include "rrc/rrc_cell_asn_fill_inner.h"
			
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc-inner"

bool make_phy_tdd_cfg_inner(srsran_duplex_config_nr_t	       	*srsran_duplex_config_nr,
							        srsran_subcarrier_spacing_t	  scs,
							        struct tdd_ul_dl_cfg_common_s *tdd_ul_dl_cfg_common)
{
  if (srsran_duplex_config_nr->mode == SRSRAN_DUPLEX_MODE_FDD) {
	return true;
  }
  tdd_ul_dl_cfg_common->ref_subcarrier_spacing = (enum subcarrier_spacing_e)scs;

  switch (srsran_duplex_config_nr->tdd.pattern1.period_ms) {
	case 1:
	  tdd_ul_dl_cfg_common->pattern1.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms1;
	  break;
	case 2:
	  tdd_ul_dl_cfg_common->pattern1.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms2;
	  break;
	case 5:
	  tdd_ul_dl_cfg_common->pattern1.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms5;
	  break;
	case 10:
	  tdd_ul_dl_cfg_common->pattern1.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms10;
	  break;
	default:
	  oset_error("Invalid option for dl_ul_tx_periodicity_opts %d",srsran_duplex_config_nr->tdd.pattern1.period_ms);
	  return false;
  }
  tdd_ul_dl_cfg_common->pattern1.nrof_dl_slots	 = srsran_duplex_config_nr->tdd.pattern1.nof_dl_slots;
  tdd_ul_dl_cfg_common->pattern1.nrof_dl_symbols = srsran_duplex_config_nr->tdd.pattern1.nof_dl_symbols;
  tdd_ul_dl_cfg_common->pattern1.nrof_ul_slots	 = srsran_duplex_config_nr->tdd.pattern1.nof_ul_slots;
  tdd_ul_dl_cfg_common->pattern1.nrof_ul_symbols = srsran_duplex_config_nr->tdd.pattern1.nof_ul_symbols;

  if (srsran_duplex_config_nr->tdd.pattern2.period_ms == 0) {
	return true;
  }

  tdd_ul_dl_cfg_common->pattern2_present = true;
  switch (srsran_duplex_config_nr->tdd.pattern2.period_ms) {
	case 1:
	  tdd_ul_dl_cfg_common->pattern2.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms1;
	  break;
	case 2:
	  tdd_ul_dl_cfg_common->pattern2.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms2;
	  break;
	case 5:
	  tdd_ul_dl_cfg_common->pattern2.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms5;
	  break;
	case 10:
	  tdd_ul_dl_cfg_common->pattern2.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms10;
	  break;
	default:
	  oset_error("Invalid option for pattern2 dl_ul_tx_periodicity_opts %d",srsran_duplex_config_nr->tdd.pattern2.period_ms);
	  return false;
  }
  tdd_ul_dl_cfg_common->pattern2.nrof_dl_slots	 = srsran_duplex_config_nr->tdd.pattern2.nof_dl_slots;
  tdd_ul_dl_cfg_common->pattern2.nrof_dl_symbols = srsran_duplex_config_nr->tdd.pattern2.nof_dl_symbols;
  tdd_ul_dl_cfg_common->pattern2.nrof_ul_slots	 = srsran_duplex_config_nr->tdd.pattern2.nof_ul_slots;
  tdd_ul_dl_cfg_common->pattern2.nrof_ul_symbols = srsran_duplex_config_nr->tdd.pattern2.nof_ul_symbols;

  return true;
}


bool make_phy_rach_cfg_inner(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_prach_cfg_t *prach_cfg)
{
	srsran_duplex_mode_t duplex_mode = rrc_cell_cfg->duplex_mode;

	prach_cfg->is_nr            = true;
	prach_cfg->config_idx       = 0;//rach->rach_ConfigGeneric.prach_ConfigurationIndex
	prach_cfg->zero_corr_zone   = 0;//rach->rach_ConfigGeneric.zeroCorrelationZoneConfig
	prach_cfg->num_ra_preambles = 64;
	if (rrc_cell_cfg->num_ra_preambles != 64) {
		prach_cfg->num_ra_preambles = rrc_cell_cfg->num_ra_preambles;
	}
	prach_cfg->hs_flag    = false; // Hard-coded
	prach_cfg->tdd_config = {0};
	if (duplex_mode == SRSRAN_DUPLEX_MODE_TDD) {
		prach_cfg->tdd_config.configured = true;
	}

	// As the current PRACH is based on LTE, the freq-offset shall be subtracted 1 for aligning with NR bandwidth
	// For example. A 52 PRB cell with an freq_offset of 1 will match a LTE 50 PRB cell with freq_offset of 0
	prach_cfg->freq_offset = 1;//rach->rach_ConfigGeneric.msg1_FrequencyStart
	if (prach_cfg->freq_offset == 0) {
		oset_error("PRACH freq offset must be at least one");
		return false;
	}

	prach_cfg->root_seq_idx = (uint32_t)rrc_cell_cfg->prach_root_seq_idx;//ASN_RRC_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l839

	//todo

	return true;
};


bool make_phy_coreset_cfg(struct ctrl_res_set_s *ctrl_res_set, srsran_coreset_t *in_srsran_coreset)
{
	srsran_coreset_t srsran_coreset = {0};
	srsran_coreset.id               = ctrl_res_set->ctrl_res_set_id;
	switch (ctrl_res_set->precoder_granularity) {
	case (enum precoder_granularity_opts)same_as_reg_bundle:
	  srsran_coreset.precoder_granularity = srsran_coreset_precoder_granularity_reg_bundle;
	  break;
	case (enum precoder_granularity_opts)all_contiguous_rbs:
	  srsran_coreset.precoder_granularity = srsran_coreset_precoder_granularity_contiguous;
	default:
	  oset_error("Invalid option for precoder_granularity %d", ctrl_res_set->precoder_granularity);
	  return false;
	};

	switch (ctrl_res_set->cce_reg_map_type.types) {
	case (enum cce_reg_map_types_opts)interleaved:
	  srsran_coreset.mapping_type = srsran_coreset_mapping_type_interleaved;
	  break;
	case (enum cce_reg_map_types_opts)non_interleaved:
	  srsran_coreset.mapping_type = srsran_coreset_mapping_type_non_interleaved;
	  break;
	default:
	  oset_error("Invalid option for cce_reg_map_type: %d", ctrl_res_set->cce_reg_map_type.types);
	  return false;
	}
	srsran_coreset.duration = ctrl_res_set->dur;
	for (uint32_t i = 0; i < SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE; i++) {
		srsran_coreset.freq_resources[i] = bitstring_get(ctrl_res_set->freq_domain_res,SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE - 1 - i);
	}
	*in_srsran_coreset = srsran_coreset;
	return true;
}

bool make_phy_search_space_cfg(struct search_space_s *search_space, srsran_search_space_t *in_srsran_search_space)
{
  srsran_search_space_t srsran_search_space = {0};
  srsran_search_space.id                    = search_space->search_space_id;
  if (!search_space->ctrl_res_set_id_present) {
    oset_error("ctrl_res_set_id option not present");
    return false;
  }
  srsran_search_space.coreset_id = search_space->ctrl_res_set_id;

  srsran_search_space.duration = 1;
  if (search_space->dur_present) {
    srsran_search_space.duration = search_space->dur;
  }

  if (!search_space->nrof_candidates_present) {
    oset_error("nrof_candidates_present option not present");
    return false;
  }
  uint8_t options[] = {0, 1, 2, 3, 4, 5, 6, 8};
  srsran_search_space.nof_candidates[0] = options[search_space->nrof_candidates.aggregation_level1];
  srsran_search_space.nof_candidates[1] = options[search_space->nrof_candidates.aggregation_level2];
  srsran_search_space.nof_candidates[2] = options[search_space->nrof_candidates.aggregation_level4];
  srsran_search_space.nof_candidates[3] = options[search_space->nrof_candidates.aggregation_level8];
  srsran_search_space.nof_candidates[4] = options[search_space->nrof_candidates.aggregation_level16];

  if (!search_space->search_space_type_present) {
    oset_error("nrof_candidates option not present");
    return false;
  }
  switch (search_space->search_space_type.types) {
    case common:
      srsran_search_space.type = srsran_search_space_type_common_3;

      // dci-Format0-0-AndFormat1-0
      // If configured, the UE monitors the DCI formats 0_0 and 1_0 according to TS 38.213 [13], clause 10.1.
      if (search_space->search_space_type.c.common.dci_format0_minus0_and_format1_minus0_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_0_0;
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_1_0;
      }

      // dci-Format2-0
      // If configured, UE monitors the DCI format 2_0 according to TS 38.213 [13], clause 10.1, 11.1.1.
      if (search_space->search_space_type.c.common.dci_format2_minus0_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_2_0;
      }

      // dci-Format2-1
      // If configured, UE monitors the DCI format 2_1 according to TS 38.213 [13], clause 10.1, 11.2.
      if (search_space->search_space_type.c.common.dci_format2_minus1_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_2_1;
      }

      // dci-Format2-2
      // If configured, UE monitors the DCI format 2_2 according to TS 38.213 [13], clause 10.1, 11.3.
      if (search_space->search_space_type.c.common.dci_format2_minus2_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_2_2;
      }

      // dci-Format2-3
      // If configured, UE monitors the DCI format 2_3 according to TS 38.213 [13], clause 10.1, 11.4
      if (search_space->search_space_type.c.common.dci_format2_minus3_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_2_3;
      }

      break;
    case ue_specific:
      srsran_search_space.type = srsran_search_space_type_ue;
      switch (search_space->search_space_type.c.ue_spec.dci_formats.value) {
        case formats0_minus0_and_minus1_minus0:
          srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_0_0;
          srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_1_0;
          break;
        case formats0_minus1_and_minus1_minus1:
          srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_0_1;
          srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_1_1;
          break;
      }
      break;
    default:
      oset_error("Invalid option for search_space_type %d", search_space->search_space_type.types);
      return false;
  }
  // Copy struct and return value
  *in_srsran_search_space = srsran_search_space;
  return true;
}



bool fill_phy_pdcch_cfg_common2(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_pdcch_cfg_nr_t *pdcch)
{
	if (rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set_present) {
		//is_sa no enter
		//pdcch->coreset_present[rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set.ctrl_res_set_id] = true;
		//make_phy_coreset_cfg(rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set, &pdcch->coreset[rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set.ctrl_res_set_id]);
	}

	struct search_space_s *ss = NULL;
	cvector_for_each_in(ss, rrc_cell_cfg->pdcch_cfg_common.common_search_space_list){
		pdcch->search_space_present[ss->search_space_id] = true;
		if (! make_phy_search_space_cfg(ss, &pdcch->search_space[ss->search_space_id])) {
		  oset_error("Failed to convert SearchSpace Configuration");
		  return false;
		}
		if (rrc_cell_cfg->pdcch_cfg_common.ra_search_space_present &&\
			rrc_cell_cfg->pdcch_cfg_common.ra_search_space == ss->search_space_id) {
			pdcch->ra_search_space_present     = true;
			pdcch->ra_search_space             = pdcch->search_space[ss->search_space_id];
			pdcch->ra_search_space.type        = srsran_search_space_type_common_1;
			pdcch->ra_search_space.nof_formats = 1;
			pdcch->ra_search_space.formats[0]  = srsran_dci_format_nr_1_0;
		}
	}
	return true;
}

bool fill_ssb_pattern_scs(srsran_carrier_nr_t *carrier,
                                  srsran_ssb_pattern_t *pattern,
                                  srsran_subcarrier_spacing_t *ssb_scs)
{
	band_helper_t *band_helper = gnb_manager_self()->band_helper;
	uint16_t band = get_band_from_dl_freq_Hz_2c(band_helper, carrier->ssb_center_freq_hz);
	if (band == UINT16_MAX) {
		oset_error("Invalid band for SSB frequency %.3f MHz", carrier->ssb_center_freq_hz);
		return false;
	}

	// TODO: Generalize conversion for other SCS
	*pattern = get_ssb_pattern_2c(band_helper, band, srsran_subcarrier_spacing_15kHz);
	if (*pattern == SRSRAN_SSB_PATTERN_A) {
		*ssb_scs = carrier->scs;
	} else {
		// try to optain SSB pattern for same band with 30kHz SCS
		*pattern = get_ssb_pattern_2c(band_helper, band, srsran_subcarrier_spacing_30kHz);
		if (*pattern == SRSRAN_SSB_PATTERN_B || *pattern == SRSRAN_SSB_PATTERN_C) {
		  // SSB SCS is 30 kHz
		  *ssb_scs = srsran_subcarrier_spacing_30kHz;
		} else {
		  oset_error("Can't derive SSB pattern from band %d", band);
		  return false;
		}
	}
	return true;
}

bool fill_phy_ssb_cfg(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_ssb_cfg_t *out_ssb)
{
	*out_ssb = {0};

	srsran_carrier_nr_t  *carrier = rrc_cell_cfg->phy_cell.carrier;

	out_ssb->center_freq_hz = carrier.dl_center_frequency_hz;
	out_ssb->ssb_freq_hz    = carrier.ssb_center_freq_hz;
	if (!fill_ssb_pattern_scs(carrier, &out_ssb->pattern, &out_ssb->scs)) {
		return false;
	}

	out_ssb->duplex_mode = rrc_cell_cfg->duplex_mode;

	out_ssb->periodicity_ms = 10;//ASN_RRC_ServingCellConfigCommonSIB__ssb_PeriodicityServingCell_ms10
	return true;
}

bool fill_rach_cfg_common_default_inner(srsran_prach_cfg_t *prach_cfg, struct rach_cfg_common_s *rach_cfg_com)
{
  *rach_cfg_com = {0};
  // rach-ConfigGeneric
  rach_cfg_com->rach_cfg_generic.prach_cfg_idx             = prach_cfg->config_idx;
  rach_cfg_com->rach_cfg_generic.msg1_fdm                  = (enum msg1_fdm_e_)one;
  rach_cfg_com->rach_cfg_generic.msg1_freq_start           = prach_cfg->freq_offset;
  rach_cfg_com->rach_cfg_generic.zero_correlation_zone_cfg = prach_cfg->zero_corr_zone;
  rach_cfg_com->rach_cfg_generic.preamb_rx_target_pwr      = -110;
  rach_cfg_com->rach_cfg_generic.preamb_trans_max          = (enum preamb_trans_max_e_)n7;
  rach_cfg_com->rach_cfg_generic.pwr_ramp_step             = (enum pwr_ramp_step_e_)db4;
  rach_cfg_com->rach_cfg_generic.ra_resp_win               = (enum ra_resp_win_e_)sl10;

  // totalNumberOfRA-Preambles
  if (prach_cfg.num_ra_preambles != 64) {
    rach_cfg_com->total_nof_ra_preambs_present = true;
    rach_cfg_com->total_nof_ra_preambs         = prach_cfg->num_ra_preambles;
  }

  // ssb-perRACH-OccasionAndCB-PreamblesPerSSB
  rach_cfg_com->ssb_per_rach_occasion_and_cb_preambs_per_ssb_present = true;
  rach_cfg_com->ssb_per_rach_occasion_and_cb_preambs_per_ssb.type_ = 3;//one
  uint8_t options_one[] = {4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64};
  for (uint8_t i = 0; i < (sizeof(options_one) / sizeof(options_one[0])); ++i) {
	if(prach_cfg->num_ra_preambles == options_one[i]){
		rach_cfg_com->ssb_per_rach_occasion_and_cb_preambs_per_ssb.c = i;
		break;
	}
  }
  rach_cfg_com->ra_contention_resolution_timer = (enum ra_contention_resolution_timer_e_)sf64;
  rach_cfg_com->prach_root_seq_idx.type_ = 0;//l839
  rach_cfg_com->prach_root_seq_idx.c     = prach_cfg->root_seq_idx;
  rach_cfg_com->restricted_set_cfg       = (enum restricted_set_cfg_e_)unrestricted_set;

  return true;
}

uint8_t options_nrof_ports[] = {1, 2, 4, 8, 12, 16, 24, 32};
int8_t options_pwr_ctrl_offset_ss[] = {-3, 0, 3, 6};

static bool make_phy_nzp_csi_rs_resource(struct nzp_csi_rs_res_s *asn1_nzp_csi_rs_res,
                                             srsran_csi_rs_nzp_resource_t *out_csi_rs_nzp_resource)
{
  srsran_csi_rs_nzp_resource_t csi_rs_nzp_resource = {0};
  csi_rs_nzp_resource.id                           = asn1_nzp_csi_rs_res->nzp_csi_rs_res_id;
  switch (asn1_nzp_csi_rs_res->res_map.freq_domain_alloc.type_)) {
    case (enum freq_domain_alloc_e_)row1:
      csi_rs_nzp_resource.resource_mapping.row = srsran_csi_rs_resource_mapping_row_1;
      for (uint32_t i = 0; i < 4; i++) {
        csi_rs_nzp_resource.resource_mapping.frequency_domain_alloc[i] =
			   bitstring_get(asn1_nzp_csi_rs_res->res_map.freq_domain_alloc.c, 4 - 1 - i);
      }
      break;
    case (enum freq_domain_alloc_e_)row2:
      csi_rs_nzp_resource.resource_mapping.row = srsran_csi_rs_resource_mapping_row_2;
      for (uint32_t i = 0; i < 12; i++) {
        csi_rs_nzp_resource.resource_mapping.frequency_domain_alloc[i] =
			    bitstring_get(asn1_nzp_csi_rs_res->res_map.freq_domain_alloc.c, 12 - 1 - i);
      }
      break;
    case (enum freq_domain_alloc_e_)row4:
      csi_rs_nzp_resource.resource_mapping.row = srsran_csi_rs_resource_mapping_row_4;
      for (uint32_t i = 0; i < 3; i++) {
        csi_rs_nzp_resource.resource_mapping.frequency_domain_alloc[i] =
			    bitstring_get(asn1_nzp_csi_rs_res->res_map.freq_domain_alloc.c, 3 - 1 - i);
      }
      break;
    case (enum freq_domain_alloc_e_)other:
      csi_rs_nzp_resource.resource_mapping.row = srsran_csi_rs_resource_mapping_row_other;
      break;
    default:
      oset_error("Invalid option for freq_domain_alloc %d", asn1_nzp_csi_rs_res->res_map.freq_domain_alloc.type_);
      return false;
  }

  csi_rs_nzp_resource.resource_mapping.nof_ports        = options_nrof_ports[asn1_nzp_csi_rs_res->res_map.nrof_ports];
  csi_rs_nzp_resource.resource_mapping.first_symbol_idx = asn1_nzp_csi_rs_res->res_map.first_ofdm_symbol_in_time_domain;

  switch (asn1_nzp_csi_rs_res->res_map.cdm_type) {
    case (enum cdm_type_e_)no_cdm:
      csi_rs_nzp_resource.resource_mapping.cdm = srsran_csi_rs_cdm_nocdm;
      break;
    case (enum cdm_type_e_)fd_cdm2:
      csi_rs_nzp_resource.resource_mapping.cdm = srsran_csi_rs_cdm_fd_cdm2;
      break;
    case (enum cdm_type_e_)cdm4_fd2_td2:
      csi_rs_nzp_resource.resource_mapping.cdm = srsran_csi_rs_cdm_cdm4_fd2_td2;
      break;
    case (enum cdm_type_e_)cdm8_fd2_td4:
      csi_rs_nzp_resource.resource_mapping.cdm = srsran_csi_rs_cdm_cdm8_fd2_td4;
      break;
    default:
      oset_error("Invalid option for cdm_type %d", asn1_nzp_csi_rs_res->res_map.cdm_type);
      return false;
  }

  switch (asn1_nzp_csi_rs_res->res_map.density.type_) {
    case (enum density_e_)dot5:
      switch (asn1_nzp_csi_rs_res->res_map.density.c) {
        case (enum dot5_e_)even_prbs:
          csi_rs_nzp_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_dot5_even;
          break;
        case (enum dot5_e_)odd_prbs:
          csi_rs_nzp_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_dot5_odd;
          break;
        default:
          oset_error("Invalid option for dot5 %d", asn1_nzp_csi_rs_res->res_map.density.c);
          return false;
      }
      break;
    case (enum density_e_)one:
      csi_rs_nzp_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_one;
      break;
    case (enum density_e_)three:
      csi_rs_nzp_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_three;
      break;
    case (enum density_e_)spare:
      csi_rs_nzp_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_spare;
      break;
    default:
      oset_error("Invalid option for density %d", asn1_nzp_csi_rs_res->res_map.density.type_));
      return false;
  }
  csi_rs_nzp_resource.resource_mapping.freq_band.nof_rb   = asn1_nzp_csi_rs_res->res_map.freq_band.nrof_rbs;
  csi_rs_nzp_resource.resource_mapping.freq_band.start_rb = asn1_nzp_csi_rs_res->res_map.freq_band.start_rb;

  // Validate CSI-RS resource mapping
  if (!srsran_csi_rs_resource_mapping_is_valid(&csi_rs_nzp_resource.resource_mapping)) {
    oset_error("Resource mapping is invalid or not implemented");
    return false;
  }

  csi_rs_nzp_resource.power_control_offset = asn1_nzp_csi_rs_res->pwr_ctrl_offset;
  if (asn1_nzp_csi_rs_res->pwr_ctrl_offset_ss_present) {
    csi_rs_nzp_resource.power_control_offset_ss = options_pwr_ctrl_offset_ss[asn1_nzp_csi_rs_res->pwr_ctrl_offset_ss];
  }

  if (asn1_nzp_csi_rs_res->periodicity_and_offset_present) {
    switch (asn1_nzp_csi_rs_res->periodicity_and_offset.type_) {
      case (enum csi_res_periodicity_and_offset_type_e)slots4:
        csi_rs_nzp_resource.periodicity.period = 4;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots5:
        csi_rs_nzp_resource.periodicity.period = 5;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots8:
        csi_rs_nzp_resource.periodicity.period = 8;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots10:
        csi_rs_nzp_resource.periodicity.period = 10;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots16:
        csi_rs_nzp_resource.periodicity.period = 16;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots20:
        csi_rs_nzp_resource.periodicity.period = 20;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots32:
        csi_rs_nzp_resource.periodicity.period = 32;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots40:
        csi_rs_nzp_resource.periodicity.period = 40;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots64:
        csi_rs_nzp_resource.periodicity.period = 64;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots80:
        csi_rs_nzp_resource.periodicity.period = 80;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots160:
        csi_rs_nzp_resource.periodicity.period = 160;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c);
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots320:
        csi_rs_nzp_resource.periodicity.period = 320;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      case (enum csi_res_periodicity_and_offset_type_e)slots640:
        csi_rs_nzp_resource.periodicity.period = 640;
        csi_rs_nzp_resource.periodicity.offset = asn1_nzp_csi_rs_res->periodicity_and_offset.c;
        break;
      default:
        oset_error("Invalid option for periodicity_and_offset %s",
                          asn1_nzp_csi_rs_res->periodicity_and_offset.type_);
        return false;
    }
  } else {
    oset_error("Option periodicity_and_offset not present");
    return false;
  }

  csi_rs_nzp_resource.scrambling_id = asn1_nzp_csi_rs_res->scrambling_id;

  *out_csi_rs_nzp_resource = csi_rs_nzp_resource;
  return true;
}

static bool make_phy_zp_csi_rs_resource(struct zp_csi_rs_res_s *zp_csi_rs_res,
								  srsran_csi_rs_zp_resource_t* out_zp_csi_rs_resource)
{
   srsran_csi_rs_zp_resource_t zp_csi_rs_resource = {0};
   zp_csi_rs_resource.id						  = zp_csi_rs_res->zp_csi_rs_res_id;
   switch (zp_csi_rs_res->res_map.freq_domain_alloc.type_) {
	 case (enum freq_domain_alloc_e_)row1:
	   zp_csi_rs_resource.resource_mapping.row = srsran_csi_rs_resource_mapping_row_1;
	   for (uint32_t i = 0; i < 4; i++) {
		 zp_csi_rs_resource.resource_mapping.frequency_domain_alloc[i] =
		 	 bitstring_get(zp_csi_rs_res->res_map.freq_domain_alloc.c, 4 - 1 - i);
	   }
	   break;
	 case (enum freq_domain_alloc_e_)row2:
	   zp_csi_rs_resource.resource_mapping.row = srsran_csi_rs_resource_mapping_row_2;
	   for (uint32_t i = 0; i < 12; i++) {
		 zp_csi_rs_resource.resource_mapping.frequency_domain_alloc[i] =
			 bitstring_get(zp_csi_rs_res->res_map.freq_domain_alloc.c, 12 - 1 - i);
	   }
	   break;
	 case (enum freq_domain_alloc_e_)row4:
	   zp_csi_rs_resource.resource_mapping.row = srsran_csi_rs_resource_mapping_row_4;
	   for (uint32_t i = 0; i < 3; i++) {
		 zp_csi_rs_resource.resource_mapping.frequency_domain_alloc[i] =
			 bitstring_get(zp_csi_rs_res->res_map.freq_domain_alloc.c, 3 - 1 - i);
	   }
	   break;
	 case (enum freq_domain_alloc_e_)other:
	   zp_csi_rs_resource.resource_mapping.row = srsran_csi_rs_resource_mapping_row_other;
	   break;
	 default:
	   oset_error("Invalid option for freq_domain_alloc %s",
						 zp_csi_rs_res->res_map.freq_domain_alloc.type_);
	   return false;
   }
   zp_csi_rs_resource.resource_mapping.nof_ports		= options_nrof_ports[zp_csi_rs_res->res_map.nrof_ports];
   zp_csi_rs_resource.resource_mapping.first_symbol_idx = zp_csi_rs_res->res_map.first_ofdm_symbol_in_time_domain;
 
   switch (zp_csi_rs_res->res_map.cdm_type) {
	 case (enum cdm_type_e_)no_cdm:
	   zp_csi_rs_resource.resource_mapping.cdm = srsran_csi_rs_cdm_nocdm;
	   break;
	 case (enum cdm_type_e_)fd_cdm2:
	   zp_csi_rs_resource.resource_mapping.cdm = srsran_csi_rs_cdm_fd_cdm2;
	   break;
	 case (enum cdm_type_e_)cdm4_fd2_td2:
	   zp_csi_rs_resource.resource_mapping.cdm = srsran_csi_rs_cdm_cdm4_fd2_td2;
	   break;
	 case (enum cdm_type_e_)cdm8_fd2_td4:
	   zp_csi_rs_resource.resource_mapping.cdm = srsran_csi_rs_cdm_cdm8_fd2_td4;
	   break;
	 default:
	   oset_error("Invalid option for cdm_type %d", zp_csi_rs_res->res_map.cdm_type);
	   return false;
   }
 
   switch (zp_csi_rs_res->res_map.density.type_) {
	 case (enum density_e_)dot5:
	   switch (zp_csi_rs_res->res_map.density.c) {
		 case (enum dot5_e_)even_prbs:
		   zp_csi_rs_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_dot5_even;
		   break;
		 case (enum dot5_e_)odd_prbs:
		   zp_csi_rs_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_dot5_odd;
		   break;
		 default:
		   oset_error("Invalid option for dot5 %d", zp_csi_rs_res->res_map.density.c);
		   return false;
	   }
	   break;
	 case (enum density_e_)one:
	   zp_csi_rs_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_one;
	   break;
	 case (enum density_e_)three:
	   zp_csi_rs_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_three;
	   break;
	 case (enum density_e_)spare:
	   zp_csi_rs_resource.resource_mapping.density = srsran_csi_rs_resource_mapping_density_spare;
	   break;
	 default:
	   oset_error("Invalid option for density %s", zp_csi_rs_res->res_map.density.type_);
	   return false;
   }
   zp_csi_rs_resource.resource_mapping.freq_band.nof_rb   = zp_csi_rs_res->res_map.freq_band.nrof_rbs;
   zp_csi_rs_resource.resource_mapping.freq_band.start_rb = zp_csi_rs_res->res_map.freq_band.start_rb;
 
   // Validate CSI-RS resource mapping
   if (!srsran_csi_rs_resource_mapping_is_valid(&zp_csi_rs_resource.resource_mapping)) {
	 oset_error("Resource mapping is invalid or not implemented");
	 return false;
   }
 
   if (zp_csi_rs_res->periodicity_and_offset_present) {
	 switch (zp_csi_rs_res->periodicity_and_offset.type_) {
	   case (enum csi_res_periodicity_and_offset_type_e)slots4:
		 zp_csi_rs_resource.periodicity.period = 4;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots5:
		 zp_csi_rs_resource.periodicity.period = 5;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots8:
		 zp_csi_rs_resource.periodicity.period = 8;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots10:
		 zp_csi_rs_resource.periodicity.period = 10;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots16:
		 zp_csi_rs_resource.periodicity.period = 16;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots20:
		 zp_csi_rs_resource.periodicity.period = 20;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots32:
		 zp_csi_rs_resource.periodicity.period = 32;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots40:
		 zp_csi_rs_resource.periodicity.period = 40;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots64:
		 zp_csi_rs_resource.periodicity.period = 64;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots80:
		 zp_csi_rs_resource.periodicity.period = 80;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots160:
		 zp_csi_rs_resource.periodicity.period = 160;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots320:
		 zp_csi_rs_resource.periodicity.period = 320;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   case (enum csi_res_periodicity_and_offset_type_e)slots640:
		 zp_csi_rs_resource.periodicity.period = 640;
		 zp_csi_rs_resource.periodicity.offset = zp_csi_rs_res->periodicity_and_offset.c;
		 break;
	   default:
		 oset_error("Invalid option for periodicity_and_offset %d",
						   zp_csi_rs_res->periodicity_and_offset.type_);
		 return false;
	 }
   } else {
	 oset_error("Option periodicity_and_offset not present");
	 return false;
   }
 
   *out_zp_csi_rs_resource = zp_csi_rs_resource;
   return true;
}


bool make_pdsch_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_sch_hl_cfg_nr_t *sch_hl)
{
  if (serv_cell->csi_meas_cfg_present && serv_cell->csi_meas_cfg.type_ == (enum setup_release_e)setup) {
    struct csi_meas_cfg_s *setup = &serv_cell->csi_meas_cfg.c;

    // Configure NZP-CSI
    struct nzp_csi_rs_res_set_s *nzp_set = NULL;
    cvector_for_each_in(nzp_set, setup->nzp_csi_rs_res_set_to_add_mod_list){
      srsran_csi_rs_nzp_set_t *uecfg_set    = &sch_hl->nzp_csi_rs_sets[nzp_set->nzp_csi_res_set_id];
      uecfg_set->trs_info = nzp_set->trs_info_present;
      uecfg_set->count    = cvector_size(nzp_set->nzp_csi_rs_res);
      uint32_t count     = 0;
      for (uint8_t nzp_rs_idx = 0;  nzp_rs_idx < cvector_size(nzp_set->nzp_csi_rs_res); nzp_rs_idx++) {
        srsran_csi_rs_nzp_resource_t *res = &uecfg_set->data[count++];
        if (!make_phy_nzp_csi_rs_resource(&setup->nzp_csi_rs_res_to_add_mod_list[nzp_rs_idx], res)) {
          return false;
        }
      }
    }
  }

  if (serv_cell.init_dl_bwp.pdsch_cfg_present && serv_cell.init_dl_bwp.pdsch_cfg.type_ == (enum setup_release_e)setup) {
    struct pdsch_cfg_s *setup = serv_cell->init_dl_bwp.pdsch_cfg.c;
    if (setup->p_zp_csi_rs_res_set_present) {
      struct zp_csi_rs_res_set_s *setup_set  = setup->p_zp_csi_rs_res_set.c;
      sch_hl->p_zp_csi_rs_set.count = cvector_size(setup_set->zp_csi_rs_res_id_list);
      for (uint8_t zp_res_id = 0;  zp_res_id < cvector_size(setup_set->zp_csi_rs_res_id_list); zp_res_id++) {
        srsran_csi_rs_zp_resource_t *res  = sch_hl->p_zp_csi_rs_set.data[zp_res_id];
        if (!make_phy_zp_csi_rs_resource(&setup->zp_csi_rs_res_to_add_mod_list[zp_res_id], &res)) {
          return false;
        }
      }
    }
  }

  return true;
}


uint16_t options_csi_report_periodicity_and_offset[] = {4, 5, 8, 10, 16, 20, 40, 80, 160, 320};
float options_max_code_rate[] = {0.08, 0.15, 0.25, 0.35, 0.45, 0.6, 0.8};
static bool make_phy_csi_report(csi_report_cfg_s *csi_report_cfg,
                         		        srsran_csi_hl_report_cfg_t* in_srsran_csi_hl_report_cfg)
{
  srsran_csi_hl_report_cfg_t srsran_csi_hl_report_cfg = {0};
  switch (csi_report_cfg->report_cfg_type.type_) {
    case (enum report_cfg_type_e_)nulltype:
      srsran_csi_hl_report_cfg.type = SRSRAN_CSI_REPORT_TYPE_NONE;
      break;
    case (enum report_cfg_type_e_)periodic:
      srsran_csi_hl_report_cfg.type = SRSRAN_CSI_REPORT_TYPE_PERIODIC;
      break;
    case (enum report_cfg_type_e_)aperiodic:
      srsran_csi_hl_report_cfg.type = SRSRAN_CSI_REPORT_TYPE_APERIODIC;
      break;
    case (enum report_cfg_type_e_)semi_persistent_on_pucch:
      srsran_csi_hl_report_cfg.type = SRSRAN_CSI_REPORT_TYPE_SEMI_PERSISTENT_ON_PUCCH;
      break;
    case (enum report_cfg_type_e_)semi_persistent_on_pusch:
      srsran_csi_hl_report_cfg.type = SRSRAN_CSI_REPORT_TYPE_SEMI_PERSISTENT_ON_PUSCH;
      break;
    default:
      oset_error("Invalid option for report_cfg_type %d", csi_report_cfg->report_cfg_type.type_);
      return false;
  }

  if (srsran_csi_hl_report_cfg.type == SRSRAN_CSI_REPORT_TYPE_PERIODIC) {
    struct periodic_s_ *csi_periodic = csi_report_cfg->report_cfg_type.c.periodic;
    srsran_csi_hl_report_cfg.periodic.period = options_csi_report_periodicity_and_offset[csi_periodic->report_slot_cfg.type_];
    switch (csi_periodic->report_slot_cfg.type_) {
      case (enum csi_report_periodicity_and_offset_e_)slots4:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots5:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots8:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots10:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots16:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots20:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots40:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots80:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots160:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      case (enum csi_report_periodicity_and_offset_e_)slots320:
        srsran_csi_hl_report_cfg.periodic.offset = csi_periodic->report_slot_cfg.c;
        break;
      default:
        oset_error("Invalid option for report_slot_cfg %d", csi_periodic->report_slot_cfg.type_);
        return false;
    }
  }

  srsran_csi_hl_report_cfg.channel_meas_id = csi_report_cfg->res_for_ch_meas;

  srsran_csi_hl_report_cfg.interf_meas_present = csi_report_cfg->csi_im_res_for_interference_present;
  srsran_csi_hl_report_cfg.interf_meas_id      = csi_report_cfg->csi_im_res_for_interference;

  switch (csi_report_cfg->report_quant.type_) {
    case (enum report_quant_e_)none:
      srsran_csi_hl_report_cfg.quantity = SRSRAN_CSI_REPORT_QUANTITY_NONE;
      break;
    case (enum report_quant_e_)cri_ri_pmi_cqi:
      srsran_csi_hl_report_cfg.quantity = SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI;
      break;
    case (enum report_quant_e_)cri_ri_i1:
      srsran_csi_hl_report_cfg.quantity = SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_I1;
      break;
    case (enum report_quant_e_)cri_ri_i1_cqi:
      srsran_csi_hl_report_cfg.quantity = SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_I1_CQI;
      break;
    case (enum report_quant_e_)cri_ri_cqi:
      srsran_csi_hl_report_cfg.quantity = SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_CQI;
      break;
    case (enum report_quant_e_)cri_rsrp:
      srsran_csi_hl_report_cfg.quantity = SRSRAN_CSI_REPORT_QUANTITY_CRI_RSRP;
      break;
    case (enum report_quant_e_)ssb_idx_rsrp:
      srsran_csi_hl_report_cfg.quantity = SRSRAN_CSI_REPORT_QUANTITY_SSB_INDEX_RSRP;
      break;
    case (enum report_quant_e_)cri_ri_li_pmi_cqi:
      srsran_csi_hl_report_cfg.quantity = SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_LI_PMI_CQI;
      break;
    default:
      oset_error("Invalid option for report_quant %d", csi_report_cfg->report_quant.type_);
      return false;
  }

  if (! csi_report_cfg->report_freq_cfg_present) {
    oset_error("report_freq_cfg_present option not present");
    return false;
  }

  if (!csi_report_cfg->report_freq_cfg.cqi_format_ind_present) {
    oset_error("cqi_format_ind option not present");
    return false;
  }

  switch (csi_report_cfg->report_freq_cfg.cqi_format_ind) {
    case (enum cqi_format_ind_e_)wideband_cqi:
      srsran_csi_hl_report_cfg.freq_cfg = SRSRAN_CSI_REPORT_FREQ_WIDEBAND;
      break;
    case (enum cqi_format_ind_e_)subband_cqi:
      srsran_csi_hl_report_cfg.freq_cfg = SRSRAN_CSI_REPORT_FREQ_SUBBAND;
      break;
    default:
      oset_error("Invalid option for cqi_format_ind %s",
                        csi_report_cfg->report_freq_cfg.cqi_format_ind);
      return false;

      break;
  }

  if (! csi_report_cfg.cqi_table_present) {
    oset_error("cqi_table_present not present");
    return false;
  }

  switch (csi_report_cfg->cqi_table) {
    case (enum cqi_table_e_)table1:
      srsran_csi_hl_report_cfg.cqi_table = SRSRAN_CSI_CQI_TABLE_1;
      break;
    case (enum cqi_table_e_)table2:
      srsran_csi_hl_report_cfg.cqi_table = SRSRAN_CSI_CQI_TABLE_2;
      break;
    case (enum cqi_table_e_)table3:
      srsran_csi_hl_report_cfg.cqi_table = SRSRAN_CSI_CQI_TABLE_3;
      break;
    default:
      oset_error("Invalid option for cqi_table %d", csi_report_cfg->cqi_table);
      return false;
  }

  *in_srsran_csi_hl_report_cfg = srsran_csi_hl_report_cfg;
  return true;
}

static bool make_phy_res_config(const pucch_res_s     	   *pucch_res,
						 uint32_t					format_2_max_code_rate,
						 srsran_pucch_nr_resource_t *in_srsran_pucch_nr_resource)
{
	srsran_pucch_nr_resource_t srsran_pucch_nr_resource = {0};
	srsran_pucch_nr_resource.starting_prb 			  = pucch_res->start_prb;
	srsran_pucch_nr_resource.intra_slot_hopping		  = pucch_res->intra_slot_freq_hop_present;
	if (pucch_res->second_hop_prb_present) {
		srsran_pucch_nr_resource.second_hop_prb = pucch_res->second_hop_prb;
	}
	switch (pucch_res->format.type_) {
	case (enum pucch_format_types)format0:
	  srsran_pucch_nr_resource.format = SRSRAN_PUCCH_NR_FORMAT_0;
	  break;
	case (enum pucch_format_types)format1:
	  srsran_pucch_nr_resource.format				= SRSRAN_PUCCH_NR_FORMAT_1;
	  srsran_pucch_nr_resource.initial_cyclic_shift = pucch_res->format.c.f1.init_cyclic_shift;
	  srsran_pucch_nr_resource.nof_symbols			= pucch_res->format.c.f1.nrof_symbols;
	  srsran_pucch_nr_resource.start_symbol_idx 	= pucch_res->format.c.f1.start_symbol_idx;
	  srsran_pucch_nr_resource.time_domain_occ		= pucch_res->format.c.f1.time_domain_occ;
	  break;
	case (enum pucch_format_types)format2:
	  srsran_pucch_nr_resource.format			= SRSRAN_PUCCH_NR_FORMAT_2;
	  srsran_pucch_nr_resource.nof_symbols		= pucch_res->format.c.f2.nrof_symbols;
	  srsran_pucch_nr_resource.start_symbol_idx = pucch_res->format.c.f2.start_symbol_idx;
	  srsran_pucch_nr_resource.nof_prb			= pucch_res->format.c.f2.nrof_prbs;
	  break;
	case (enum pucch_format_types)format3:
	  srsran_pucch_nr_resource.format = SRSRAN_PUCCH_NR_FORMAT_3;
	  oset_error("SRSRAN_PUCCH_NR_FORMAT_3 conversion not supported");
	  return false;
	case (enum pucch_format_types)format4:
	  srsran_pucch_nr_resource.format = SRSRAN_PUCCH_NR_FORMAT_4;
	  oset_error("SRSRAN_PUCCH_NR_FORMAT_4 conversion not supported");
	  return false;
	default:
	  srsran_pucch_nr_resource.format = SRSRAN_PUCCH_NR_FORMAT_ERROR;
	  return false;
	}
	srsran_pucch_nr_resource.max_code_rate = format_2_max_code_rate;
	*in_srsran_pucch_nr_resource			 = srsran_pucch_nr_resource;
	return true;
}


bool make_csi_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_csi_hl_cfg_t* csi_hl)
{
	if (serv_cell->csi_meas_cfg_present && serv_cell->csi_meas_cfg.type_ == (enum setup_release_e)setup) {
		struct csi_meas_cfg_s *setup = &serv_cell->csi_meas_cfg.c;

		// Configure CSI-Report
		for (uint32_t i = 0; i < cvector_size(setup->csi_report_cfg_to_add_mod_list); ++i) {
			 struct csi_report_cfg_s *csi_rep = setup->csi_report_cfg_to_add_mod_list[i];
			if (! make_phy_csi_report(csi_rep, &csi_hl->reports[i])) {
				return false;
			}
			if (csi_rep->report_cfg_type.type_ == (enum report_cfg_type_e_)periodic) {
				struct pucch_cfg_s *pucch_setup = serv_cell.ul_cfg.init_ul_bwp.pucch_cfg.c;
				srsran_pucch_nr_resource_t *resource    = &csi_hl->reports[i].periodic.resource;
				uint32_t    pucch_resource_id           = csi_rep->report_cfg_type.c.periodic.pucch_csi_res_list[0].pucch_res;
				struct pucch_res_s *asn1_resource               = pucch_setup->res_to_add_mod_list[pucch_resource_id];
				uint32_t    format2_rate                = 0;
				if (pucch_setup->format2_present &&
					pucch_setup->format2.type_ == (enum setup_release_e)setup &&
					pucch_setup->format2.c.max_code_rate_present) {
					format2_rate = options_max_code_rate[pucch_setup->format2.c.max_code_rate];
				}
				if (! make_phy_res_config(asn1_resource, format2_rate, resource)) {
					return false;
				}
			}
		}
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

int fill_rach_cfg_common_inner(rrc_cell_cfg_nr_t *cell_cfg, struct rach_cfg_common_s *rach)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	// rach-ConfigGeneric
	rach->rach_cfg_generic.prach_cfg_idx = 0;
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_TDD) {
		// Note: Give more time margin to fit RAR
		rach->rach_cfg_generic.prach_cfg_idx = 8;
	}
	rach->rach_cfg_generic.msg1_fdm            		 = (enum msg1_fdm_e_)one;
	rach->rach_cfg_generic.msg1_freq_start           = 1; // zero not supported with current PRACH implementation
	rach->rach_cfg_generic.zero_correlation_zone_cfg = 0;
	rach->rach_cfg_generic.preamb_rx_target_pwr      = -110;
	rach->rach_cfg_generic.preamb_trans_max          = (enum preamb_trans_max_e_)n7;
	rach->rach_cfg_generic.pwr_ramp_step             = (enum pwr_ramp_step_e_)db4;
	rach->rach_cfg_generic.ra_resp_win               = (enum ra_resp_win_e_)sl10;

	// totalNumberOfRA-Preambles
	if (cell_cfg->num_ra_preambles != 64) {
		rach->total_nof_ra_preambs_present = true;
		rach->total_nof_ra_preambs         = cell_cfg->num_ra_preambles;
	}

	// ssb-perRACH-OccasionAndCB-PreamblesPerSSB
	rach->ssb_per_rach_occasion_and_cb_preambs_per_ssb_present = true;
	rach->ssb_per_rach_occasion_and_cb_preambs_per_ssb.type_ = 3;//one
	uint8_t options_one[] = {4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64};
	for (uint8_t i = 0; i < (sizeof(options_one) / sizeof(options_one[0])); ++i) {
	  if(cell_cfg->num_ra_preambles == options_one[i]){
		  rach->ssb_per_rach_occasion_and_cb_preambs_per_ssb.c = i;
		  break;
	  }
	}

	rach->ra_contention_resolution_timer = (enum ra_contention_resolution_timer_e_)sf64;
	rach->prach_root_seq_idx.type_ = 0;//l839
	rach->prach_root_seq_idx.c	   = cell_cfg->prach_root_seq_idx;
	rach->restricted_set_cfg       = (enum restricted_set_cfg_e_)unrestricted_set;

	return OSET_OK;
}

void fill_ul_cfg_common_sib_inner(rrc_cell_cfg_nr_t *cell_cfg, struct ul_cfg_common_sib_s *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;
	band_helper_t *band_helper = gnb_manager_self()->band_helper;

	cvector_reserve(out->freq_info_ul.freq_band_list, 1);
	cvector_set_size(out->freq_info_ul.freq_band_list, 1);
	out->freq_info_ul.freq_band_list[0].freq_band_ind_nr_present = true;
	out->freq_info_ul.freq_band_list[0].freq_band_ind_nr         = cell_cfg->band;

	out->freq_info_ul.absolute_freq_point_a_present = true;
	out->freq_info_ul.absolute_freq_point_a =  get_abs_freq_point_a_arfcn_2c(band_helper, cell_cfg->phy_cell.carrier.nof_prb, cell_cfg->ul_arfcn);

	cvector_reserve(out->freq_info_ul.scs_specific_carrier_list, 1);
	cvector_set_size(out->freq_info_ul.scs_specific_carrier_list, 1);
	out->freq_info_ul.scs_specific_carrier_list[0].offset_to_carrier = cell_cfg->phy_cell.carrier.offset_to_carrier;
	out->freq_info_ul.scs_specific_carrier_list[0].subcarrier_spacing = cell_cfg->phy_cell.carrier.scs;
	out->freq_info_ul.scs_specific_carrier_list[0].carrier_bw = cell_cfg->phy_cell.carrier.nof_prb;

	out->freq_info_ul.p_max_present = true;
	out->freq_info_ul.p_max         = 10;

	out->init_ul_bwp.generic_params.location_and_bw = 14025;
	out->init_ul_bwp.generic_params.subcarrier_spacing = cell_cfg->phy_cell.carrier.scs;

	out->init_ul_bwp.rach_cfg_common_present = true;
	out->init_ul_bwp.rach_cfg_common.type_ = setup;
	fill_rach_cfg_common_inner(cell_cfg, &out->init_ul_bwp.rach_cfg_common.c);

	out.init_ul_bwp.pusch_cfg_common_present = true;
	out->init_ul_bwp.pusch_cfg_common.type_  = setup;
	struct pusch_cfg_common_s *pusch         = &out->init_ul_bwp.pusch_cfg_common.c;

	cvector_reserve(pusch->pusch_time_domain_alloc_list, 1);
	cvector_set_size(pusch->pusch_time_domain_alloc_list, 1);
	pusch->pusch_time_domain_alloc_list[0].k2_present           = true
	pusch->pusch_time_domain_alloc_list[0].k2                   = 4;
	pusch->pusch_time_domain_alloc_list[0].map_type       	  = (enum map_type_e_)type_a;
	pusch->pusch_time_domain_alloc_list[0].start_symbol_and_len = 27;

	pusch->p0_nominal_with_grant_present              = true;
	pusch->p0_nominal_with_grant                      = -76;

	out->init_ul_bwp.pucch_cfg_common_present = true;
	out->init_ul_bwp.pucch_cfg_common.type_   = setup;
	struct pucch_cfg_common_s *pucch          = &out->init_ul_bwp.pucch_cfg_common.c;
	pucch->pucch_res_common_present           = true;
	pucch->pucch_res_common                   = 11;
	pucch->pucch_group_hop                    = (enum pucch_group_hop_e_)neither;
	pucch->p0_nominal_present                 = true;
	pucch->p0_nominal                         = -90;

	out->time_align_timer_common = (enum time_align_timer_e)infinity;
}


// Called for SA and NSA
void fill_pdcch_cfg_common_inner(rrc_cell_cfg_nr_t *cell_cfg, struct pdcch_cfg_common_s *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	out->ctrl_res_set_zero_present = false;
	out->search_space_zero_present = false;

	out->common_ctrl_res_set_present = cell_cfg->pdcch_cfg_common.common_ctrl_res_set_present;
	out->common_ctrl_res_set         = cell_cfg->pdcch_cfg_common.common_ctrl_res_set;
	out->common_search_space_list    = cell_cfg->pdcch_cfg_common.common_search_space_list;

	out->search_space_sib1_present           = true;
	out->search_space_sib1                   = 0;
	out->search_space_other_sys_info_present = true;
	out->search_space_other_sys_info         = 0; //1
	out->paging_search_space_present         = true;
	out->paging_search_space                 = 1;
	out->ra_search_space_present             = true;
	out->ra_search_space                     = 1;
}

void fill_pdsch_cfg_common_inner(rrc_cell_cfg_nr_t *cell_cfg, struct pdsch_cfg_common_s *out)
{
	cvector_reserve(out->pdsch_time_domain_alloc_list, 1);
	cvector_set_size(out->pdsch_time_domain_alloc_list, 1);
	out->pdsch_time_domain_alloc_list[0].map_type = (enum map_type_e_)type_a;
	out->pdsch_time_domain_alloc_list[0].start_symbol_and_len = 40;
}


// Called for SA
void fill_init_dl_bwp_inner(rrc_cell_cfg_nr_t *cell_cfg, struct bwp_dl_common_s *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	out->generic_params.location_and_bw    = 14025;
	out->generic_params.subcarrier_spacing = cell_cfg->phy_cell.carrier.scs;

	out->pdcch_cfg_common_present = true;
	out->pdcch_cfg_common.type_ = setup;
	fill_pdcch_cfg_common_inner(cell_cfg, &out->pdcch_cfg_common.c);

	out->pdsch_cfg_common_present = true;
	out->pdsch_cfg_common.type_ = setup;
	fill_pdsch_cfg_common_inner(cell_cfg, &out->pdsch_cfg_common.c);
}


void fill_dl_cfg_common_sib_inner(rrc_cell_cfg_nr_t *cell_cfg, struct dl_cfg_common_sib_s *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;
	band_helper_t *band_helper = gnb_manager_self()->band_helper;

	uint32_t scs_hz = SRSRAN_SUBC_SPACING_NR(cell_cfg->phy_cell.carrier.scs);
	uint32_t prb_bw = scs_hz * SRSRAN_NRE;

	cvector_reserve(out->freq_info_dl.freq_band_list, 1);
	cvector_set_size(out->freq_info_dl.freq_band_list, 1);
	out->freq_info_dl.freq_band_list[0].freq_band_ind_nr_present = true;
	out->freq_info_dl.freq_band_list[0].freq_band_ind_nr = cell_cfg->band;

	double	 ssb_freq_start 	 = cell_cfg->ssb_freq_hz - SRSRAN_SSB_BW_SUBC * scs_hz / 2;
	double	 offset_point_a_hz   = ssb_freq_start - nr_arfcn_to_freq_2c(band_helper, cell_cfg->dl_absolute_freq_point_a);
	uint32_t offset_point_a_prbs = offset_point_a_hz / prb_bw;
	out->freq_info_dl.offset_to_point_a = offset_point_a_prbs;

	cvector_reserve(out->freq_info_dl.scs_specific_carrier_list, 1);
	cvector_set_size(out->freq_info_dl.scs_specific_carrier_list, 1);
	out->freq_info_dl.scs_specific_carrier_list[0].offset_to_carrier = cell_cfg->phy_cell.carrier.offset_to_carrier;
	out->freq_info_dl.scs_specific_carrier_list[0].subcarrier_spacing = cell_cfg->phy_cell.carrier.scs;
	out->freq_info_dl.scs_specific_carrier_list[0].carrier_bw = cell_cfg->phy_cell.carrier.nof_prb;

	fill_init_dl_bwp_inner(cell_cfg, out->init_dl_bwp);
	// disable InitialBWP-Only fields
	out->init_dl_bwp.pdcch_cfg_common.c.ctrl_res_set_zero_present = false;
	out->init_dl_bwp.pdcch_cfg_common.c.search_space_zero_present = false;

	out->bcch_cfg.mod_period_coeff = (enum mod_period_coeff_opts)n4;

	out->pcch_cfg.default_paging_cycle = (enum  paging_cycle_e)rf128;
	out->pcch_cfg.nand_paging_frame_offset.type_ = 0;//one_t
	out->pcch_cfg.ns = (enum ns_e_)one;
}

void fill_tdd_ul_dl_config_common_inner(rrc_cell_cfg_nr_t *cell_cfg, struct tdd_ul_dl_cfg_common_s *tdd)
{
  ASSERT_IF_NOT(cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_TDD, "This function should only be called for TDD configs");
  // TDD UL-DL config
  tdd->ref_subcarrier_spacing  = (enum subcarrier_spacing_e)cell_cfg->phy_cell.carrier.scs;
  tdd->pattern1.dl_ul_tx_periodicity = (enum dl_ul_tx_periodicity_e_)ms10;
  tdd->pattern1.nrof_dl_slots        = 6;
  tdd->pattern1.nrof_dl_symbols      = 0;
  tdd->pattern1.nrof_ul_slots        = 4;
  tdd->pattern1.nrof_ul_symbols      = 0;
}

int fill_serv_cell_cfg_common_sib_inner(rrc_cell_cfg_nr_t *cell_cfg, struct serving_cell_cfg_common_sib_s *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	fill_dl_cfg_common_sib_inner(cell_cfg, &out->dl_cfg_common);

	out->ul_cfg_common_present = true;
	fill_ul_cfg_common_sib_inner(cell_cfg, &out->ul_cfg_common);
	
	bitstring_from_number(&out->ssb_positions_in_burst.in_one_group, 0x80, 8);

	out->ssb_periodicity_serving_cell = (enum ssb_periodicity_serving_cell_sib_e_)ms10;

	// The time advance offset is not supported by the current PHY
	out->n_timing_advance_offset_present = true;
	out->n_timing_advance_offset         = (enum n_timing_advance_offset_e_)n0;

	// TDD UL-DL config
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_TDD) {
		out->tdd_ul_dl_cfg_common_present = true;
		fill_tdd_ul_dl_config_common_inner(cell_cfg, out->tdd_ul_dl_cfg_common);
	}

	out->ss_pbch_block_pwr = cell_cfg->pdsch_rs_power;

  return OSET_OK;
}

/// Fill SRB with parameters derived from cfg
void fill_srb_inner(rrc_nr_cfg_t *cfg, nr_srb srb_id, struct rlc_bearer_cfg_s *out)
{
	ASSERT_IF_NOT(srb_id > srb0 && srb_id < count, "Invalid srb_id argument");

	out->lc_ch_id                         = srb_to_lcid(srb_id);
	out->served_radio_bearer_present      = true;
	out->served_radio_bearer.type_        = 0;//srb_id
	out->served_radio_bearer.c            = (uint8_t)srb_id;

	if (srb_id == srb1) {
		if (cfg->srb1_cfg.present) {
		  out->rlc_cfg_present = true;
		  out->rlc_cfg         = cfg->srb1_cfg.rlc_cfg;
		} else {
		  out->rlc_cfg_present = false;
		}
	} else if (srb_id == srb2) {
		if (cfg->srb2_cfg.present) {
		  out->rlc_cfg_present = true;
		  out->rlc_cfg         = cfg->srb2_cfg.rlc_cfg;
		} else {
		  out->rlc_cfg_present = false;
		}
	} else {
		out->rlc_cfg_present           = true;
		out->rlc_cfg.types = (enum rlc_types_opts)am;
		struct ul_am_rlc_s *ul_am     = out->rlc_cfg.c.am.ul_am_rlc;
		ul_am->sn_field_len_present   = true;
		ul_am->sn_field_len      = (enum sn_field_len_am_opts)size12;
		ul_am->t_poll_retx       = (enum t_poll_retx_opts)ms45;
		ul_am->poll_pdu          = (enum poll_pdu_opts)infinity;
		ul_am->poll_byte         = (enum poll_byte_opts)infinity;
		ul_am->max_retx_thres    = (enum max_retx_thres_opts)t8;

		struct dl_am_rlc_s *dl_am     = out->rlc_cfg.c.am.dl_am_rlc;
		dl_am->sn_field_len_present   = true;
		dl_am->sn_field_len      = (enum sn_field_len_am_opts)size12;
		dl_am->t_reassembly      = (enum t_reassembly_opts)ms35;
		dl_am->t_status_prohibit = (enum t_status_prohibit_opts)ms0;
	}

	// mac-LogicalChannelConfig -- Cond LCH-Setup
	out->mac_lc_ch_cfg_present                    = true;
	out->mac_lc_ch_cfg.ul_specific_params_present = true;
	out->mac_lc_ch_cfg.ul_specific_params.prio    = (srb_id == srb1) ? 1 : 3;
	out->mac_lc_ch_cfg.ul_specific_params.prioritised_bit_rate = ( enum prioritised_bit_rate_e_)infinity;
	out->mac_lc_ch_cfg.ul_specific_params.bucket_size_dur = (enum bucket_size_dur_e_)ms5;
	out->mac_lc_ch_cfg.ul_specific_params.lc_ch_group_present          = true;
	out->mac_lc_ch_cfg.ul_specific_params.lc_ch_group                  = 0;
	out->mac_lc_ch_cfg.ul_specific_params.sched_request_id_present     = true;
	out->mac_lc_ch_cfg.ul_specific_params.sched_request_id             = 0;
	out->mac_lc_ch_cfg.ul_specific_params.lc_ch_sr_mask                = false;
	out->mac_lc_ch_cfg.ul_specific_params.lc_ch_sr_delay_timer_applied = false;
}

/// Fill csi-ResoureConfigToAddModList
void fill_csi_resource_cfg_to_add_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct csi_meas_cfg_s *csi_meas_cfg)
{
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
		cvector_reserve(csi_meas_cfg->csi_res_cfg_to_add_mod_list, 3);
		cvector_set_size(csi_meas_cfg->csi_res_cfg_to_add_mod_list, 3);

		csi_meas_cfg->csi_res_cfg_to_add_mod_list[0].csi_res_cfg_id = 0;
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[0].csi_rs_res_set_list.type_ = 0;//nzp_csi_rs_ssb
		cvector_push_back(csi_meas_cfg->csi_res_cfg_to_add_mod_list[0].csi_rs_res_set_list.c.nzp_csi_rs_ssb.nzp_csi_rs_res_set_list, 0);
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[0].bwp_id   = 0;
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[0].res_type = (enum res_type_e_)periodic;


		csi_meas_cfg->csi_res_cfg_to_add_mod_list[1].csi_res_cfg_id = 1;
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[1].csi_rs_res_set_list.type_ = 1;//csi_im_res_set_list
		cvector_push_back(csi_meas_cfg->csi_res_cfg_to_add_mod_list[1].csi_rs_res_set_list.c.csi_im_res_set_list, 0);
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[1].bwp_id   = 0;
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[1].res_type = (enum res_type_e_)periodic;


		csi_meas_cfg->csi_res_cfg_to_add_mod_list[2].csi_res_cfg_id = 2;
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[2].csi_rs_res_set_list.type_ = 0;//nzp_csi_rs_ssb
		cvector_push_back(csi_meas_cfg->csi_res_cfg_to_add_mod_list[2].csi_rs_res_set_list.c.nzp_csi_rs_ssb.nzp_csi_rs_res_set_list, 1);
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[2].bwp_id   = 0;
		csi_meas_cfg->csi_res_cfg_to_add_mod_list[2].res_type = (enum res_type_e_)periodic;
	}
}

/// Fill lists of NZP-CSI-RS-Resource and NZP-CSI-RS-ResourceSet with gNB config
void fill_nzp_csi_rs_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct csi_meas_cfg_s *csi_meas_cfg)
{
	ASSERT_IF_NOT(cfg->is_standalone, "Not support NSA now!")

	if (cfg->is_standalone) {
		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			cvector_reserve(csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list, 5);
			cvector_set_size(csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list, 5);

			// item 0
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].nzp_csi_rs_res_id = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.freq_domain_alloc.type_ = (enum freq_domain_alloc_e_)row2;//row2
			bitstring_from_number(&csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.freq_domain_alloc.c, 0x800, 12);
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.nrof_ports                 = (enum nrof_ports_e_)p1;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.first_ofdm_symbol_in_time_domain = 4;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.cdm_type                   = (enum cdm_type_e_)no_cdm;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.density.type_          = 1;//one
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.freq_band.start_rb     = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.freq_band.nrof_rbs     = cell_cfg->phy_cell.carrier.nof_prb;//52
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].pwr_ctrl_offset                = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].pwr_ctrl_offset_ss_present     = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].pwr_ctrl_offset_ss             = (enum pwr_ctrl_offset_ss_e_)db0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].scrambling_id                  = cell_cfg->phy_cell.cell_id;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset.type_ = (enum csi_res_periodicity_and_offset_type_e)slots80;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset.c     = 1;
			// optional
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].qcl_info_periodic_csi_rs_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].qcl_info_periodic_csi_rs         = 0;

			// item 1
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].nzp_csi_rs_res_id = 1;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].res_map.freq_domain_alloc.type_ = (enum freq_domain_alloc_e_)row1;//row1
			bitstring_from_number(&csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].res_map.freq_domain_alloc.c, 0x1, 4);
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].res_map.nrof_ports                 = (enum nrof_ports_e_)p1;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].res_map.first_ofdm_symbol_in_time_domain = 4;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].res_map.cdm_type                   = (enum cdm_type_e_)no_cdm;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].res_map.density.type_          = 2;//three
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].res_map.freq_band.start_rb     = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].res_map.freq_band.nrof_rbs     = cell_cfg->phy_cell.carrier.nof_prb;//52
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].pwr_ctrl_offset                = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].pwr_ctrl_offset_ss_present     = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].pwr_ctrl_offset_ss             = (enum pwr_ctrl_offset_ss_e_)db0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].scrambling_id                  = cell_cfg->phy_cell.cell_id;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].periodicity_and_offset_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].periodicity_and_offset.type_ = (enum csi_res_periodicity_and_offset_type_e)slots40;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].periodicity_and_offset.c     = 11;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].qcl_info_periodic_csi_rs_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[1].qcl_info_periodic_csi_rs         = 0;

			// item 2
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].nzp_csi_rs_res_id = 2;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].res_map.freq_domain_alloc.type_ = (enum freq_domain_alloc_e_)row1;//row1
			bitstring_from_number(&csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].res_map.freq_domain_alloc.c, 0x1, 4);
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].res_map.nrof_ports                 = (enum nrof_ports_e_)p1;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].res_map.first_ofdm_symbol_in_time_domain = 8;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].res_map.cdm_type                   = (enum cdm_type_e_)no_cdm;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].res_map.density.type_          = 2;//three
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].res_map.freq_band.start_rb     = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].res_map.freq_band.nrof_rbs     = cell_cfg->phy_cell.carrier.nof_prb;//52
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].pwr_ctrl_offset                = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].pwr_ctrl_offset_ss_present     = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].pwr_ctrl_offset_ss             = (enum pwr_ctrl_offset_ss_e_)db0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].scrambling_id                  = cell_cfg->phy_cell.cell_id;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].periodicity_and_offset_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].periodicity_and_offset.type_ = (enum csi_res_periodicity_and_offset_type_e)slots40;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].periodicity_and_offset.c     = 11;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].qcl_info_periodic_csi_rs_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[2].qcl_info_periodic_csi_rs         = 0;

			// item 3
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].nzp_csi_rs_res_id = 3;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].res_map.freq_domain_alloc.type_ = (enum freq_domain_alloc_e_)row1;//row1
			bitstring_from_number(&csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].res_map.freq_domain_alloc.c, 0x1, 4);
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].res_map.nrof_ports                 = (enum nrof_ports_e_)p1;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].res_map.first_ofdm_symbol_in_time_domain = 4;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].res_map.cdm_type                   = (enum cdm_type_e_)no_cdm;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].res_map.density.type_          = 2;//three
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].res_map.freq_band.start_rb     = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].res_map.freq_band.nrof_rbs     = cell_cfg->phy_cell.carrier.nof_prb;//52
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].pwr_ctrl_offset                = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].pwr_ctrl_offset_ss_present     = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].pwr_ctrl_offset_ss             = (enum pwr_ctrl_offset_ss_e_)db0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].scrambling_id                  = cell_cfg->phy_cell.cell_id;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].periodicity_and_offset_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].periodicity_and_offset.type_ = (enum csi_res_periodicity_and_offset_type_e)slots40;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].periodicity_and_offset.c     = 12;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].qcl_info_periodic_csi_rs_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[3].qcl_info_periodic_csi_rs         = 0;

			// item 4
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].nzp_csi_rs_res_id = 4;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].res_map.freq_domain_alloc.type_ = (enum freq_domain_alloc_e_)row1;//row1
			bitstring_from_number(&csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].res_map.freq_domain_alloc.c, 0x1, 4);
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].res_map.nrof_ports                 = (enum nrof_ports_e_)p1;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].res_map.first_ofdm_symbol_in_time_domain = 8;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].res_map.cdm_type                   = (enum cdm_type_e_)no_cdm;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].res_map.density.type_          = 2;//three
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].res_map.freq_band.start_rb     = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].res_map.freq_band.nrof_rbs     = cell_cfg->phy_cell.carrier.nof_prb;//52
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].pwr_ctrl_offset                = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].pwr_ctrl_offset_ss_present     = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].pwr_ctrl_offset_ss             = (enum pwr_ctrl_offset_ss_e_)db0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].scrambling_id                  = cell_cfg->phy_cell.cell_id;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].periodicity_and_offset_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].periodicity_and_offset.type_ = (enum csi_res_periodicity_and_offset_type_e)slots40;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].periodicity_and_offset.c     = 12;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].qcl_info_periodic_csi_rs_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[4].qcl_info_periodic_csi_rs         = 0;
		} else {
			cvector_reserve(csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list, 5);
			cvector_set_size(csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list, 5);

			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].nzp_csi_rs_res_id = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.freq_domain_alloc.type_ = (enum freq_domain_alloc_e_)row2;//row2
			bitstring_from_number(&csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.freq_domain_alloc.c, 0b100000000000, 12);
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.nrof_ports                 = (enum nrof_ports_e_)p1;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.first_ofdm_symbol_in_time_domain = 4;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.cdm_type                   = (enum cdm_type_e_)no_cdm;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.density.type_          = 1;//one
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.freq_band.start_rb     = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.freq_band.nrof_rbs     = cell_cfg->phy_cell.carrier.nof_prb;//52
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].pwr_ctrl_offset                = 0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].pwr_ctrl_offset_ss_present     = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].pwr_ctrl_offset_ss             = (enum pwr_ctrl_offset_ss_e_)db0;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].scrambling_id                  = cell_cfg->phy_cell.cell_id;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset.type_ = (enum csi_res_periodicity_and_offset_type_e)slots80;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset.c     = 0;
			// optional
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].qcl_info_periodic_csi_rs_present = true;
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].qcl_info_periodic_csi_rs         = 0;
		}

		// Fill NZP-CSI Resource Sets
		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			cvector_reserve(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list, 2);
			cvector_set_size(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list, 2);

			// item 0
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[0].nzp_csi_res_set_id = 0;
			cvector_reserve(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[0].nzp_csi_rs_res, 1);
			cvector_set_size(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[0].nzp_csi_rs_res, 1);
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[0].nzp_csi_rs_res[0] = 0

			// item 1
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[1].nzp_csi_res_set_id = 1;
			cvector_reserve(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[1].nzp_csi_rs_res, 4);
			cvector_set_size(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[1].nzp_csi_rs_res, 4);
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[1].nzp_csi_rs_res[0] = 1;
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[1].nzp_csi_rs_res[1] = 2;
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[1].nzp_csi_rs_res[2] = 3;
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[1].nzp_csi_rs_res[3] = 4;
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[1].trs_info_present  = true;
			  //    // Skip TRS info
	} else {
			cvector_reserve(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list, 1);
			cvector_set_size(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list, 1)
			// item 0
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[0].nzp_csi_res_set_id = 0;
			cvector_reserve(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[0].nzp_csi_rs_res, 1);
			cvector_set_size(csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[0].nzp_csi_rs_res, 1);
			csi_meas_cfg->nzp_csi_rs_res_set_to_add_mod_list[0].nzp_csi_rs_res[0] = 0;
			// Skip TRS info
		}
	}
}

void fill_csi_im_resource_cfg_to_add_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct csi_meas_cfg_s *csi_meas_cfg)
{
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
		// csi-IM-ResourceToAddModList
		cvector_reserve(csi_meas_cfg->csi_im_res_to_add_mod_list, 1);
		cvector_set_size(csi_meas_cfg->csi_im_res_to_add_mod_list, 1);

		csi_meas_cfg->csi_im_res_to_add_mod_list[0].csi_im_res_id                   = 0;
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].csi_im_res_elem_pattern_present = true;
		// csi-im-resource pattern1
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].csi_im_res_elem_pattern.type_ = 1;//pattern1
		struct pattern1_s_ *csi_res_pattern1 = &csi_meas_cfg->csi_im_res_to_add_mod_list[0].csi_im_res_elem_pattern.c.pattern1;
		csi_res_pattern1->subcarrier_location_p1 = (enum subcarrier_location_p1_e_)s8;
		csi_res_pattern1->symbol_location_p1 = 8;
		// csi-im-resource freqBand
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].freq_band_present  = true;
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].freq_band.start_rb = 0;
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].freq_band.nrof_rbs = cell_cfg->phy_cell.carrier.nof_prb;//52
		// csi-im-resource periodicity_and_offset
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].periodicity_and_offset_present = true;
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].periodicity_and_offset.type_ = (csi_res_periodicity_and_offset_type_e)slots80;
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].periodicity_and_offset.c = 1;

		// csi-IM-ResourceSetToAddModList
		cvector_reserve(csi_meas_cfg->csi_im_res_set_to_add_mod_list, 1);
		cvector_set_size(csi_meas_cfg->csi_im_res_set_to_add_mod_list, 1);
		csi_meas_cfg->csi_im_res_set_to_add_mod_list[0].csi_im_res_set_id = 0;
		cvector_push_back(csi_meas_cfg->csi_im_res_set_to_add_mod_list[0].csi_im_res, 0);
	}
}

/// Fill list of CSI-ReportConfig with gNB config
int fill_csi_report_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, csi_meas_cfg_s& csi_meas_cfg)
{
	ASSERT_IF_NOT(cfg->is_standalone, "Not support NSA now!")

	if (cfg->is_standalone) {
		cvector_reserve(csi_meas_cfg->csi_report_cfg_to_add_mod_list, 1);
		cvector_set_size(csi_meas_cfg->csi_report_cfg_to_add_mod_list, 1);

		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_id                       = 0;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].res_for_ch_meas                     = 0;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].csi_im_res_for_interference_present = true;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].csi_im_res_for_interference         = 1;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.type_ = (enum report_cfg_type_e_)periodic;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.report_slot_cfg.type_ = (enum csi_report_periodicity_and_offset_e_)slots80;

		cvector_reserve(csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.pucch_csi_res_list, 1);
		cvector_set_size(csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.pucch_csi_res_list, 1);

		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.pucch_csi_res_list[0].ul_bw_part_id = 0;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.pucch_csi_res_list[0].pucch_res = 17; // was 17 in orig PCAP, but code for NSA it was set to 1
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_quant.type_ = (enum report_quant_e_)cri_ri_pmi_cqi;
		// Report freq config (optional)
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_freq_cfg_present                = true;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_freq_cfg.cqi_format_ind_present = true;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_freq_cfg.cqi_format_ind = (enum cqi_format_ind_e_)wideband_cqi;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_freq_cfg.pmi_format_ind_present = true;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_freq_cfg.pmi_format_ind = (enum pmi_format_ind_e_)wideband_pmi;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].time_restrict_for_ch_meass = (enum time_restrict_for_ch_meass_e_)not_cfgured;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].time_restrict_for_interference_meass = (enum time_restrict_for_interference_meass_e_)not_cfgured;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].codebook_cfg_present = true;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].codebook_cfg.codebook_type.type_ = 0;//type1

		struct type1_s_ *type1 = csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].codebook_cfg.codebook_type.c.type1;
		type1->sub_type.type_ = 0;//type_i_single_panel
		type1->sub_type.c.type_i_single_panel.nr_of_ant_ports.type_ = 0;//two
		bitstring_from_number(&type1->sub_type.c.type_i_single_panel.nr_of_ant_ports.c.two.two_tx_codebook_subset_restrict, 0b111111, 6);
		bitstring_from_number(&type1->sub_type.c.type_i_single_panel.type_i_single_panel_ri_restrict, 0x03, 8);
		type1->codebook_mode = 1;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].group_based_beam_report.type_ = 1;//disabled
		// Skip CQI table (optional)
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].cqi_table_present = true;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].cqi_table         = (enum cqi_table_e_)table1;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].subband_size      = (enum subband_size_e_)value1;

		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.type_ = (enum report_cfg_type_e_)periodic;
			csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.report_slot_cfg.type_ = (enum csi_report_periodicity_and_offset_e_)slots80;
			csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.report_slot_cfg.c = 1;
		} else {
			csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.type_ = (enum report_cfg_type_e_)periodic;
			csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.report_slot_cfg.type_ = (enum csi_report_periodicity_and_offset_e_)slots80;
			csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].report_cfg_type.c.periodic.report_slot_cfg.c = 7;
		}
	}

  return OSET_OK;
}


/// Fill CSI-MeasConfig with gNB config
int fill_csi_meas_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct csi_meas_cfg_s *csi_meas_cfg)
{
  // Fill CSI resource config
  fill_csi_resource_cfg_to_add_inner(cfg, cell_cfg, csi_meas_cfg);

  // Fill NZP-CSI Resources
  fill_nzp_csi_rs_from_enb_cfg_inner(cfg, cell_cfg, csi_meas_cfg);

  if (cfg->is_standalone) {
    // CSI IM config
    fill_csi_im_resource_cfg_to_add_inner(cfg, cell_cfg, csi_meas_cfg);

    // CSI report config
    fill_csi_report_from_enb_cfg_inner(cfg, cell_cfg, csi_meas_cfg);
  }

  return OSET_OK;
}

void fill_pdsch_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, pdsch_cfg_s& out)
{
  out->dmrs_dl_for_pdsch_map_type_a_present = true;
  out->dmrs_dl_for_pdsch_map_type_a.type_ = setup;
  out->dmrs_dl_for_pdsch_map_type_a.c.dmrs_add_position_present = true;
  out->dmrs_dl_for_pdsch_map_type_a.c.dmrs_add_position         = (enum dmrs_add_position_e_)pos1;

  cvector_reserve(out->tci_states_to_add_mod_list, 1);
  cvector_set_size(out->tci_states_to_add_mod_list, 1);
  out->tci_states_to_add_mod_list[0].tci_state_id = 0;
  out->tci_states_to_add_mod_list[0].qcl_type1.ref_sig.type_ = 1;//ssb
  out->tci_states_to_add_mod_list[0].qcl_type1.ref_sig.c = 0;
  out->tci_states_to_add_mod_list[0].qcl_type1.qcl_type  = (enum qcl_type_e_)type_d;

  out->res_alloc = (enum res_alloc_e_)res_alloc_type1;
  out->rbg_size  = (enum rbg_size_e_)cfg1;
  out->prb_bundling_type.type_ = 0;//static_bundling
  out->prb_bundling_type.c.static.bundle_size_present = true;
  out->prb_bundling_type.c.static.bundle_size = (enum bundle_size_e_)wideband;

  // MCS Table
  // NOTE: For Table 1 or QAM64, set false and comment value
  // out->mcs_table_present = true;
  // out->mcs_table = qam256;

  // ZP-CSI
  cvector_reserve(out->zp_csi_rs_res_to_add_mod_list, 1);
  cvector_set_size(out->zp_csi_rs_res_to_add_mod_list, 1);
  out->zp_csi_rs_res_to_add_mod_list[0].zp_csi_rs_res_id = 0;
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.freq_domain_alloc.type_ = 2;//row4
  bitstring_from_number(&out->zp_csi_rs_res_to_add_mod_list[0].res_map.freq_domain_alloc.c, 0b100, 12);
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.nrof_ports = (enum nrof_ports_e_)p4;

  out->zp_csi_rs_res_to_add_mod_list[0].res_map.first_ofdm_symbol_in_time_domain = 8;
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.cdm_type = (enum cdm_type_e_)fd_cdm2;
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.density.type_ = 1;//one

  out->zp_csi_rs_res_to_add_mod_list[0].res_map.freq_band.start_rb     = 0;
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.freq_band.nrof_rbs     = cell_cfg->phy_cell.carrier.nof_prb;
  out->zp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset_present = true;
  out->zp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset.type_ = (enum csi_res_periodicity_and_offset_type_e)slots80;
  out->zp_csi_rs_res_to_add_mod_list[0].periodicity_and_offset.c = 1;

  out->p_zp_csi_rs_res_set_present = false; // TEMP
  //out->p_zp_csi_rs_res_set.type_ = (enum setup_release_e)setup;
  //out->p_zp_csi_rs_res_set.c.zp_csi_rs_res_set_id = 0;
  //out->p_zp_csi_rs_res_set.c.zp_csi_rs_res_id_list.resize(1);
  //out->p_zp_csi_rs_res_set.c.zp_csi_rs_res_id_list[0] = 0;

}


/// Fill InitDlBwp with gNB config
int fill_init_dl_bwp_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct bwp_dl_ded_s *init_dl_bwp)
{
  init_dl_bwp->pdcch_cfg_present     = true;
  init_dl_bwp->pdcch_cfg.type_       = setup;
  init_dl_bwp->pdcch_cfg.c = cell_cfg->pdcch_cfg_ded;

  init_dl_bwp->pdsch_cfg_present = true;
  init_dl_bwp->pdsch_cfg.type_       = setup;
  fill_pdsch_cfg_from_enb_cfg_inner(cfg, cell_cfg, &init_dl_bwp->pdsch_cfg.c);

  // TODO: ADD missing fields
  return OSET_OK;
}

void fill_pucch_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct pucch_cfg_s *out)
{
  // Make 2 PUCCH resource sets
  cvector_reserve(out->res_set_to_add_mod_list, 2);
  cvector_set_size(out->res_set_to_add_mod_list, 2);

  // Make PUCCH resource set for 1-2 bit
  for (uint32_t set_id = 0; set_id < cvector_size(out->res_set_to_add_mod_list); ++set_id) {
  	struct pucch_res_set_s *res_set   = &out->res_set_to_add_mod_list[set_id];
	res_set->pucch_res_set_id = set_id;
	cvector_reserve(res_set->res_list, 8);
	cvector_set_size(res_set->res_list, 8);
    for (uint32_t i = 0; i < cvector_size(res_set->res_list); ++i) {
		if (cfg->is_standalone) {
			res_set->res_list[i] = i + set_id * 8;
		} else {
			res_set->res_list[i] = set_id;
		}
    }
  }

  // Make 3 possible resources
  cvector_reserve(out->res_to_add_mod_list, 18);
  cvector_set_size(out->res_to_add_mod_list, 18);
  uint32_t j = 0, j2 = 0;
  for (uint32_t i = 0; i < cvector_size(out->res_to_add_mod_list); ++i) {
    out->res_to_add_mod_list[i].pucch_res_id                = i;
    out->res_to_add_mod_list[i].intra_slot_freq_hop_present = false;
    if (i < 8 || i == 16) {
      out->res_to_add_mod_list[i].start_prb                         = cell_cfg->phy_cell.carrier.nof_prb - 1;//51
      out->res_to_add_mod_list[i].second_hop_prb_present            = true;
      out->res_to_add_mod_list[i].second_hop_prb                    = 0;
	  out->res_to_add_mod_list[i].format.type_                      = 1;//format1
      out->res_to_add_mod_list[i].format.c.f1.init_cyclic_shift     = (4 * (j % 3));
      out->res_to_add_mod_list[i].format.c.f1.nrof_symbols          = 14;
      out->res_to_add_mod_list[i].format.c.f1.start_symbol_idx      = 0;
      out->res_to_add_mod_list[i].format.c.f1.time_domain_occ       = j / 3;
      j++;
    } else if (i < 15) {
      out->res_to_add_mod_list[i].start_prb                         = 1;
      out->res_to_add_mod_list[i].second_hop_prb_present            = true;
      out->res_to_add_mod_list[i].second_hop_prb                    = cell_cfg->phy_cell.carrier.nof_prb-2;//50
      out->res_to_add_mod_list[i].format.type_                      = 2;//format2
      out->res_to_add_mod_list[i].format.c.f2.nrof_prbs             = 1;
      out->res_to_add_mod_list[i].format.c.f2.nrof_symbols          = 2;
      out->res_to_add_mod_list[i].format.c.f2.start_symbol_idx      = 2 * (j2 % 7);
      j2++;
    } else {
      out->res_to_add_mod_list[i].start_prb                         = cell_cfg->phy_cell.carrier.nof_prb -2;//50
      out->res_to_add_mod_list[i].second_hop_prb_present            = true;
      out->res_to_add_mod_list[i].second_hop_prb                    = 1;
      out->res_to_add_mod_list[i].format.type_                      = 2;//format2
      out->res_to_add_mod_list[i].format.c.f2.nrof_prbs             = 1;
      out->res_to_add_mod_list[i].format.c.f2.nrof_symbols          = 2;
      out->res_to_add_mod_list[i].format.c.f2.start_symbol_idx      = 2 * (j2 % 7);
      j2++;
    }
  }

  out->format1_present = true;
  out->format1.type_ = setup;

  out->format2_present = true;
  out->format2.type_ = setup;
  out->format2.c.max_code_rate_present = true;
  out->format2.c.max_code_rate         = (enum pucch_max_code_rate_e)zero_dot25;
  // NOTE: IMPORTANT!! The gNB expects the CSI to be reported along with HARQ-ACK
  // If simul_harq_ack_csi_present = false, PUCCH might not be decoded properly when CSI is reported
  out->format2.c.simul_harq_ack_csi_present = true;

  // SR resources
  cvector_reserve(out->sched_request_res_to_add_mod_list, 1);
  cvector_set_size(out->sched_request_res_to_add_mod_list, 1);
  out->sched_request_res_to_add_mod_list[0].sched_request_res_id              = 1;
  out->sched_request_res_to_add_mod_list[0].sched_request_id                  = 0;
  out->sched_request_res_to_add_mod_list[0].periodicity_and_offset_present    = true;
  out->sched_request_res_to_add_mod_list[0].periodicity_and_offset.type_      = (enum periodicity_and_offset_e_)sl40;
  out->sched_request_res_to_add_mod_list[0].periodicity_and_offset.c          = 8;
  out->sched_request_res_to_add_mod_list[0].res_present                       = true;
  out->sched_request_res_to_add_mod_list[0].res                               = 2;

  // DL data
  if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
	cvector_reserve(out->dl_data_to_ul_ack, 1);
	cvector_set_size(out->dl_data_to_ul_ack, 1);
	out->dl_data_to_ul_ack[0] = 4;
  } else {
	cvector_reserve(out->dl_data_to_ul_ack, 6);
	cvector_set_size(out->dl_data_to_ul_ack, 6);
	out->dl_data_to_ul_ack[0] = 6;
	out->dl_data_to_ul_ack[1] = 5;
	out->dl_data_to_ul_ack[2] = 4;
	out->dl_data_to_ul_ack[3] = 4;
	out->dl_data_to_ul_ack[4] = 4;
	out->dl_data_to_ul_ack[5] = 4;
  }
}

void fill_pusch_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct pusch_cfg_s *out)
{
  out->dmrs_ul_for_pusch_map_type_a_present = true;
  out->dmrs_ul_for_pusch_map_type_a.type_ = setup;
  out->dmrs_ul_for_pusch_map_type_a.c.dmrs_add_position_present = true;
  out->dmrs_ul_for_pusch_map_type_a.c.dmrs_add_position         = (enum dmrs_add_position_e_)pos1;
  // PUSH power control skipped
  out->res_alloc = (enum res_alloc_e_)res_alloc_type1;

  // UCI
  out->uci_on_pusch_present = true;
  out->uci_on_pusch.type_   = setup;
  out->uci_on_pusch.c.beta_offsets_present = true;
  out->uci_on_pusch.c.beta_offsets.set_semi_static();
  out->uci_on_pusch.c.beta_offsets.type_ = 1;//semi_static
  struct beta_offsets_s *beta_offset_semi_static   = &out->uci_on_pusch.c.beta_offsets.c.semi;
  beta_offset_semi_static->beta_offset_ack_idx1_present       = true;
  beta_offset_semi_static->beta_offset_ack_idx1               = 9;
  beta_offset_semi_static->beta_offset_ack_idx2_present       = true;
  beta_offset_semi_static->beta_offset_ack_idx2               = 9;
  beta_offset_semi_static->beta_offset_ack_idx3_present       = true;
  beta_offset_semi_static->beta_offset_ack_idx3               = 9;
  beta_offset_semi_static->beta_offset_csi_part1_idx1_present = true;
  beta_offset_semi_static->beta_offset_csi_part1_idx1         = 6;
  beta_offset_semi_static->beta_offset_csi_part1_idx2_present = true;
  beta_offset_semi_static->beta_offset_csi_part1_idx2         = 6;
  beta_offset_semi_static->beta_offset_csi_part2_idx1_present = true;
  beta_offset_semi_static->beta_offset_csi_part2_idx1         = 6;
  beta_offset_semi_static->beta_offset_csi_part2_idx2_present = true;
  beta_offset_semi_static->beta_offset_csi_part2_idx2         = 6;

  out->uci_on_pusch.c.scaling = (enum scaling_e_)f1;
}

void fill_init_ul_bwp_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct bwp_ul_ded_s *out)
{
  if (cfg->is_standalone) {
    out->pucch_cfg_present = true;
	out->pucch_cfg.type_ = setup;
    fill_pucch_cfg_from_enb_cfg_inner(cfg, cell_cfg, &out->pucch_cfg.c);

    out->pusch_cfg_present = true;
	out->pusch_cfg.type_ = setup;
    fill_pusch_cfg_from_enb_cfg_inner(cfg, cell_cfg, &out->pusch_cfg.c);
  }
}

/// Fill InitUlBwp with gNB config
void fill_ul_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct ul_cfg_s *out)
{
  out->init_ul_bwp_present = true;
  fill_init_ul_bwp_from_enb_cfg_inner(cfg, cell_cfg, &out->init_ul_bwp);
}

/// Fill ServingCellConfig with gNB config
int fill_serv_cell_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, uint32_t cc, struct serving_cell_cfg_s *serv_cell)
{
	rrc_cell_cfg_nr_t *cell_cfg = &cfg->cell_list[cc];

	serv_cell->csi_meas_cfg_present = true;
	serv_cell->csi_meas_cfg.type_ = setup;
	HANDLE_ERROR(fill_csi_meas_from_enb_cfg_inner(cfg, cell_cfg, &serv_cell->csi_meas_cfg.c));

	serv_cell->init_dl_bwp_present = true;
	fill_init_dl_bwp_from_enb_cfg_inner(cfg, cell_cfg, &serv_cell->init_dl_bwp);

	serv_cell->first_active_dl_bwp_id_present = true;
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
		serv_cell->first_active_dl_bwp_id = 0;
	} else {
		serv_cell->first_active_dl_bwp_id = 1;
	}

	serv_cell->ul_cfg_present = true;
	fill_ul_cfg_from_enb_cfg_inner(cfg, cell_cfg, &serv_cell->ul_cfg);

  // TODO: remaining fields

  return OSET_OK;
}


/// Fill spCellConfig with gNB config
int fill_sp_cell_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, uint32_t cc, struct sp_cell_cfg_s *sp_cell)
{
  if (!cfg->is_standalone) {
    //sp_cell.recfg_with_sync_present = true;
	//HANDLE_ERROR(fill_recfg_with_sync_from_enb_cfg_inner(cfg, cc, sp_cell.recfg_with_sync));
  }

  sp_cell->sp_cell_cfg_ded_present = true;
  HANDLE_ERROR(fill_serv_cell_from_enb_cfg_inner(cfg, cc, sp_cell->sp_cell_cfg_ded));

  return OSET_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void free_master_cell_cfg_dyn_array(struct cell_group_cfg_s *master_cell_group)
{
    //free rlc_bearer_to_add_mod_list
	cvector_free(master_cell_group->rlc_bearer_to_add_mod_list);

    //free sched_request_to_add_mod_list
	cvector_free(master_cell_group->mac_cell_group_cfg.sched_request_cfg.sched_request_to_add_mod_list);
    //free tag_to_add_mod_list
	cvector_free(master_cell_group->mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list);


    //free csi-ResoureConfigToAddModList
    struct csi_res_cfg_s *csi_res_cfg = NULL;
    cvector_for_each_in(csi_res_cfg, master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.csi_res_cfg_to_add_mod_list){
		//release csi_rs_res_set_list
		if(0 == csi_res_cfg->csi_rs_res_set_list.type_) cvector_free(csi_res_cfg->csi_rs_res_set_list.c.nzp_csi_rs_ssb.nzp_csi_rs_res_set_list);	
		if(1 == csi_res_cfg->csi_rs_res_set_list.type_) cvector_free(csi_res_cfg->csi_rs_res_set_list.c.csi_im_res_set_list);
	}
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.csi_res_cfg_to_add_mod_list);
	//free NZP-CSI-RS-Resource
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.nzp_csi_rs_res_to_add_mod_list);

	//free NZP-CSI Resource Sets
	struct nzp_csi_rs_res_set_s *nzp_csi_res_set = NULL;
	cvector_for_each_in(nzp_csi_res_set, master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.nzp_csi_rs_res_set_to_add_mod_list){
		cvector_free(nzp_csi_res_set->nzp_csi_rs_res);
	}
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.nzp_csi_rs_res_set_to_add_mod_list);
	//free csi-IM-ResourceToAddModList
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.csi_im_res_to_add_mod_list);

	//free csi-IM-ResourceSetToAddModList
	struct csi_im_res_set_s *csi_im_res_set = NULL;
	cvector_for_each_in(csi_im_res_set, master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.csi_im_res_set_to_add_mod_list){
		cvector_free(csi_im_res_set->csi_im_res);
	}
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.csi_im_res_set_to_add_mod_list);

	//free CSI-ReportConfig
	struct csi_report_cfg_s *csi_report = NULL;
	cvector_for_each_in(csi_report, master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.csi_report_cfg_to_add_mod_list){
		cvector_free(csi_report->report_cfg_type.c.periodic.pucch_csi_res_list);
	}
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.csi_report_cfg_to_add_mod_list);

	//free init_dl_bwp pdcch_cfg  //???cell_cfg->pdcch_cfg_ded
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.init_dl_bwp.pdcch_cfg.c.ctrl_res_set_to_add_mod_list);
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.init_dl_bwp.pdcch_cfg.c.search_spaces_to_add_mod_list);

	//free init_dl_bwp pdsch_cfg
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.init_dl_bwp.pdsch_cfg.c.tci_states_to_add_mod_list);
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.init_dl_bwp.pdsch_cfg.c.zp_csi_rs_res_to_add_mod_list);

	//free init_dl_bwp pucch_cfg
	struct pucch_res_set_s *pucch_res_set = NULL;
	cvector_for_each_in(pucch_res_set, master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg.init_ul_bwp.pucch_cfg.c.res_set_to_add_mod_list){
		cvector_free(pucch_res_set->res_list);
	}
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg.init_ul_bwp.pucch_cfg.c.res_set_to_add_mod_list);
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg.init_ul_bwp.pucch_cfg.c.res_to_add_mod_list);
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg.init_ul_bwp.pucch_cfg.c.sched_request_res_to_add_mod_list);
	cvector_free(master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg.init_ul_bwp.pucch_cfg.c.dl_data_to_ul_ack);

}

/// Fill MasterCellConfig with gNB config
int fill_master_cell_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, uint32_t cc, struct cell_group_cfg_s *out)
{
	out->cell_group_id = 0;

	cvector_reserve(out->rlc_bearer_to_add_mod_list, 1);
	cvector_set_size(out->rlc_bearer_to_add_mod_list, 1);
	fill_srb_inner(cfg, srb1, &out->rlc_bearer_to_add_mod_list[0]);

	// mac-CellGroupConfig -- Need M
	out->mac_cell_group_cfg_present                   = true;
	out->mac_cell_group_cfg.sched_request_cfg_present = true;
	cvector_reserve(out->mac_cell_group_cfg.sched_request_cfg.sched_request_to_add_mod_list, 1);
	cvector_set_size(out->mac_cell_group_cfg.sched_request_cfg.sched_request_to_add_mod_list, 1);
	out->mac_cell_group_cfg.sched_request_cfg.sched_request_to_add_mod_list[0].sched_request_id = 0;
	out->mac_cell_group_cfg.sched_request_cfg.sched_request_to_add_mod_list[0].sr_trans_max = (enum sr_trans_max_e_)n64;

	out->mac_cell_group_cfg.bsr_cfg_present                  = true;
	out->mac_cell_group_cfg.bsr_cfg.periodic_bsr_timer       = (enum periodic_bsr_timer_e_)sf20;
	out->mac_cell_group_cfg.bsr_cfg.retx_bsr_timer           = (enum retx_bsr_timer_e_)sf320;

	out->mac_cell_group_cfg.tag_cfg_present                  = true;
	cvector_reserve(out->mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list, 1);
	cvector_set_size(out->mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list, 1);
	out->mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list[0].tag_id           = 0;
	out->mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list[0].time_align_timer = (enum time_align_timer_e)infinity;

	out->mac_cell_group_cfg.phr_cfg_present = true;
	out->mac_cell_group_cfg.phr_cfg.type_ = setup;
	struct phr_cfg_s *phr                 = &out->mac_cell_group_cfg.phr_cfg.c;
	phr->phr_periodic_timer               = (enum  phr_periodic_timer_e_)sf500;
	phr->phr_prohibit_timer               = (enum  phr_prohibit_timer_e_)sf200;
	phr->phr_tx_pwr_factor_change         = (enum  phr_tx_pwr_factor_change_e_)db3;
	phr->multiple_phr                     = false;
	phr->dummy                            = false;
	phr->phr_type2_other_cell             = false;
	phr->phr_mode_other_cg                = (enum  phr_mode_other_cg_e_)real;
	out->mac_cell_group_cfg.skip_ul_tx_dynamic = false;
	out->mac_cell_group_cfg.phr_cfg_present    = false; // Note: not supported

	// physicalCellGroupConfig -- Need M
	out->phys_cell_group_cfg_present          = true;
	out->phys_cell_group_cfg.p_nr_fr1_present = true;
	out->phys_cell_group_cfg.p_nr_fr1         = 10;
	out->phys_cell_group_cfg.pdsch_harq_ack_codebook = (enum pdsch_harq_ack_codebook_e_)dynamic_value;

	// spCellConfig -- Need M
	out.sp_cell_cfg_present = true;
	fill_sp_cell_cfg_from_enb_cfg_inner(cfg, cc, &out->sp_cell_cfg);

  return OSET_OK;
}

void free_sib1_dyn_arrary(struct sib1_s *sib1)
{
	//release plmn_id_list
	struct plmn_id_info_s *plmn_id_info = NULL;
	cvector_for_each_in(plmn_id_info, sib1->cell_access_related_info.plmn_id_list){
		//release plmn_id_list
		cvector_free(plmn_id_info->plmn_id_list);	
	}
	cvector_free(sib1->cell_access_related_info.plmn_id_list);
	
	//release freq_info_dl.freq_band_list
	cvector_free(sib1->serving_cell_cfg_common.dl_cfg_common.freq_info_dl.freq_band_list);
	//release freq_info_dl.scs_specific_carrier_list
	cvector_free(sib1->serving_cell_cfg_common.dl_cfg_common.freq_info_dl.scs_specific_carrier_list);
	//release init_dl_bwp.common_search_space_list
	cvector_free(sib1->serving_cell_cfg_common.dl_cfg_common.init_dl_bwp.pdcch_cfg_common.c.common_search_space_list);
	//release pdsch_cfg_common.pdsch_time_domain_alloc_list
	cvector_free(sib1->serving_cell_cfg_common.dl_cfg_common.init_dl_bwp.pdsch_cfg_common.c.pdsch_time_domain_alloc_list);
	

	
	//release freq_info_ul.freq_band_list
	cvector_free(sib1->serving_cell_cfg_common.ul_cfg_common.freq_info_ul.freq_band_list);
	//release freq_info_dl.scs_specific_carrier_list
	cvector_free(sib1->serving_cell_cfg_common.ul_cfg_common.freq_info_ul.scs_specific_carrier_list);
	//release pusch_cfg_common.pusch_time_domain_alloc_list
	cvector_free(sib1->serving_cell_cfg_common.ul_cfg_common.init_ul_bwp.pusch_cfg_common.c.pusch_time_domain_alloc_list);
	
	//release sched_info_list
	struct sched_info_s *sched_info = NULL;
	cvector_for_each_in(sched_info, sib1->si_sched_info.sched_info_list){
		//release sib_map_info
		cvector_free(sched_info->sib_map_info);
	}		
	cvector_free(sib1->si_sched_info.sched_info_list);

}

int fill_sib1_from_enb_cfg_inner(rrc_cell_cfg_nr_t *cell_cfg, struct sib1_s *sib1)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	sib1->cell_sel_info_present            = true;
	sib1->cell_sel_info.q_rx_lev_min       = -70;
	sib1->cell_sel_info.q_qual_min_present = true;
	sib1->cell_sel_info.q_qual_min         = -20;

	cvector_reserve(sib1->cell_access_related_info.plmn_id_list, 1);
	cvector_set_size(sib1->cell_access_related_info.plmn_id_list, 1);
	cvector_reserve(sib1->cell_access_related_info.plmn_id_list[0].plmn_id_list, 1);
	cvector_set_size(sib1->cell_access_related_info.plmn_id_list[0].plmn_id_list, 1);
	sib1->cell_access_related_info.plmn_id_list[0].plmn_id_list[0].mcc_present = true;
	mcc_to_bytes(cfg->mcc ,sib1->cell_access_related_info.plmn_id_list[0].plmn_id_list[0].mcc);
	mnc_to_bytes(cfg->mnc,\
		         sib1->cell_access_related_info.plmn_id_list[0].plmn_id_list[0].mnc,\
		         &sib1->cell_access_related_info.plmn_id_list[0].plmn_id_list[0].nof_mnc_digits);

	sib1->cell_access_related_info.plmn_id_list[0].tac_present = true
	bitstring_from_number(sib1->cell_access_related_info.plmn_id_list[0].tac, cell_cfg->tac, 24);
	bitstring_from_number(sib1->cell_access_related_info.plmn_id_list[0].cell_id, (cfg->enb_id << 8U) + cell_cfg->phy_cell.cell_id, 36);
	sib1->cell_access_related_info.plmn_id_list[0].cell_reserved_for_oper = (enum cell_reserved_for_oper_e_)not_reserved;

	sib1->conn_est_fail_ctrl_present             = true;
	sib1->conn_est_fail_ctrl.conn_est_fail_count = (enum conn_est_fail_count_e_)n1;
	sib1->conn_est_fail_ctrl.conn_est_fail_offset_validity = (enum conn_est_fail_offset_validity_e_)s30;
	sib1->conn_est_fail_ctrl.conn_est_fail_offset_present = true;
	sib1->conn_est_fail_ctrl.conn_est_fail_offset         = 1;

	sib1->si_sched_info_present                                  = true;
	sib1->si_sched_info.si_request_cfg.rach_occasions_si_present = true;
	sib1->si_sched_info.si_request_cfg.rach_occasions_si.rach_cfg_si.ra_resp_win = (enum ra_resp_win_e_)sl8;
	sib1->si_sched_info.si_win_len = (enum si_win_len_e_)s160;

	cvector_reserve(sib1->si_sched_info.sched_info_list, 1);
	cvector_set_size(sib1->si_sched_info.sched_info_list, 1);
	sib1->si_sched_info.sched_info_list[0].si_broadcast_status = (enum si_broadcast_status_e_)broadcasting;
	sib1->si_sched_info.sched_info_list[0].si_periodicity = (enum si_periodicity_e_)rf16;
	cvector_reserve(sib1->si_sched_info.sched_info_list[0].sib_map_info, 1);
	cvector_set_size(sib1->si_sched_info.sched_info_list[0].sib_map_info, 1);
	// scheduling of SI messages
	sib1->si_sched_info.sched_info_list[0].sib_map_info[0].type              = (enum type_e_)sib_type2;
	sib1->si_sched_info.sched_info_list[0].sib_map_info[0].value_tag_present = true;
	sib1->si_sched_info.sched_info_list[0].sib_map_info[0].value_tag         = 0;

	sib1->serving_cell_cfg_common_present = true;
	HANDLE_ERROR(fill_serv_cell_cfg_common_sib_inner(cfg, &sib1->serving_cell_cfg_common));

	sib1->ue_timers_and_consts_present    = true;
	sib1->ue_timers_and_consts.t300 = (enum t300_e_)ms1000;
	sib1->ue_timers_and_consts.t301 = (enum t301_e_)ms1000;
	sib1->ue_timers_and_consts.t310 = (enum t310_e_)ms1000;
	sib1->ue_timers_and_consts.n310 = (enum n310_e_)n1;
	sib1->ue_timers_and_consts.t311 = (enum t311_e_)ms30000;
	sib1->ue_timers_and_consts.n311 = (enum n311_e_)n1;
	sib1->ue_timers_and_consts.t319 = (enum t319_e_)ms1000;

  return OSET_OK;
}

int fill_mib_from_enb_cfg_inner(rrc_cell_cfg_nr_t *cell_cfg, struct mib_s *mib)
{
	bitstring_from_number(&mib->sys_frame_num, 0, 6);
	switch (cell_cfg->phy_cell.carrier.scs) {
	case srsran_subcarrier_spacing_15kHz:
	case srsran_subcarrier_spacing_60kHz:
	  mib->sub_carrier_spacing_common = (enum sub_carrier_spacing_common_e_)scs15or60;
	  break;
	case srsran_subcarrier_spacing_30kHz:
	case srsran_subcarrier_spacing_120kHz:
	  mib->sub_carrier_spacing_common = (enum sub_carrier_spacing_common_e_)scs30or120;
	  break;
	default:
	  oset_error("Invalid carrier SCS=%d Hz", SRSRAN_SUBC_SPACING_NR(cell_cfg->phy_cell.carrier.scs));
	  oset_abort();
	}
	mib->ssb_subcarrier_offset            = cell_cfg->ssb_offset;
	mib->dmrs_type_a_position             = (enum dmrs_type_a_position_e_)pos2;
	mib->pdcch_cfg_sib1.search_space_zero = 0;
	mib->pdcch_cfg_sib1.ctrl_res_set_zero = cell_cfg->coreset0_idx;
	mib->cell_barred                      = (enum cell_barred_e_)not_barred;
	mib->intra_freq_resel                 = (enum intra_freq_resel_e_)allowed;
	bitstring_from_number(&mib->spare, 0, 1);
	return OSET_OK;
}

