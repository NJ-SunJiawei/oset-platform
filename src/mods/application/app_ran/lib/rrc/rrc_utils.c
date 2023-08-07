/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "gnb_common.h"
#include "lib/rrc/rrc_utils.h"
				
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librrcUtils"

uint8_t options_nrof_ports[] = {1, 2, 4, 8, 12, 16, 24, 32};
int8_t options_pwr_ctrl_offset_ss[] = {-3, 0, 3, 6};
uint16_t options_csi_report_periodicity_and_offset[] = {4, 5, 8, 10, 16, 20, 40, 80, 160, 320};
float options_max_code_rate[] = {0.08, 0.15, 0.25, 0.35, 0.45, 0.6, 0.8};


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

bool make_phy_nzp_csi_rs_resource(struct nzp_csi_rs_res_s *asn1_nzp_csi_rs_res,
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

bool make_phy_zp_csi_rs_resource(struct zp_csi_rs_res_s *zp_csi_rs_res,
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

bool make_phy_csi_report(csi_report_cfg_s *csi_report_cfg,
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
	srsran_csi_hl_report_cfg.interf_meas_id 	 = csi_report_cfg->csi_im_res_for_interference;

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

bool make_phy_res_config(const pucch_res_s 		 *pucch_res,
					   uint32_t 				  format_2_max_code_rate,
					   srsran_pucch_nr_resource_t *in_srsran_pucch_nr_resource)
{
  srsran_pucch_nr_resource_t srsran_pucch_nr_resource = {0};
  srsran_pucch_nr_resource.starting_prb 			= pucch_res->start_prb;
  srsran_pucch_nr_resource.intra_slot_hopping		= pucch_res->intra_slot_freq_hop_present;
  if (pucch_res->second_hop_prb_present) {
	  srsran_pucch_nr_resource.second_hop_prb = pucch_res->second_hop_prb;
  }
  switch (pucch_res->format.type_) {
  case (enum pucch_format_types)format0:
	srsran_pucch_nr_resource.format = SRSRAN_PUCCH_NR_FORMAT_0;
	break;
  case (enum pucch_format_types)format1:
	srsran_pucch_nr_resource.format 			  = SRSRAN_PUCCH_NR_FORMAT_1;
	srsran_pucch_nr_resource.initial_cyclic_shift = pucch_res->format.c.f1.init_cyclic_shift;
	srsran_pucch_nr_resource.nof_symbols		  = pucch_res->format.c.f1.nrof_symbols;
	srsran_pucch_nr_resource.start_symbol_idx	  = pucch_res->format.c.f1.start_symbol_idx;
	srsran_pucch_nr_resource.time_domain_occ	  = pucch_res->format.c.f1.time_domain_occ;
	break;
  case (enum pucch_format_types)format2:
	srsran_pucch_nr_resource.format 		  = SRSRAN_PUCCH_NR_FORMAT_2;
	srsran_pucch_nr_resource.nof_symbols	  = pucch_res->format.c.f2.nrof_symbols;
	srsran_pucch_nr_resource.start_symbol_idx = pucch_res->format.c.f2.start_symbol_idx;
	srsran_pucch_nr_resource.nof_prb		  = pucch_res->format.c.f2.nrof_prbs;
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
  *in_srsran_pucch_nr_resource			   = srsran_pucch_nr_resource;
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
		 uecfg_set->count	 = cvector_size(nzp_set->nzp_csi_rs_res);
		 uint32_t count 	= 0;
		 for (uint8_t nzp_rs_idx = 0;  nzp_rs_idx < cvector_size(nzp_set->nzp_csi_rs_res); nzp_rs_idx++) {
		   srsran_csi_rs_nzp_resource_t *res = &uecfg_set->data[count++];
		   if (!make_phy_nzp_csi_rs_resource(&setup->nzp_csi_rs_res_to_add_mod_list[nzp_rs_idx], res)) {
			 return false;
		   }
		 }
	   }
	 }

	 if (serv_cell->init_dl_bwp.pdsch_cfg_present && serv_cell->init_dl_bwp.pdsch_cfg.type_ == (enum setup_release_e)setup) {
	   struct pdsch_cfg_s *setup = serv_cell->init_dl_bwp.pdsch_cfg.c;
	   if (setup->p_zp_csi_rs_res_set_present) {
		 struct zp_csi_rs_res_set_s *setup_set	= setup->p_zp_csi_rs_res_set.c;
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
			   uint32_t    pucch_resource_id		   = csi_rep->report_cfg_type.c.periodic.pucch_csi_res_list[0].pucch_res;
			   struct pucch_res_s *asn1_resource			   = pucch_setup->res_to_add_mod_list[pucch_resource_id];
			   uint32_t    format2_rate 			   = 0;
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

int make_rlc_config_t(struct rlc_cfg_c *asn1_type, uint8_t bearer_id, rlc_config_t* cfg_out)
{
  rlc_config_t rlc_cfg = {0};
  rlc_cfg.rat          = (srsran_rat_t)nr;
  switch (asn1_type->types) {
    case (enum rlc_types_opts)am:
      rlc_cfg = default_rlc_am_nr_config(12);
      if (asn1_type->c.am.dl_am_rlc.sn_field_len_present && asn1_type->c.am.ul_am_rlc.sn_field_len_present &&
          asn1_type->c.am.dl_am_rlc.sn_field_len != asn1_type->c.am.ul_am_rlc.sn_field_len) {
        oset_warn("NR RLC sequence number length is not the same in uplink and downlink");
        return OSET_ERROR;
      }
      rlc_cfg.rlc_mode = (rlc_mode_t)am;
      switch (asn1_type->c.am.dl_am_rlc.sn_field_len) {
        case (enum sn_field_len_am_opts)size12:
          rlc_cfg.am_nr.tx_sn_field_length = (rlc_am_nr_sn_size_t)size12bits;
          rlc_cfg.am_nr.rx_sn_field_length = (rlc_am_nr_sn_size_t)size12bits;
          break;
        case (enum sn_field_len_am_opts)size18:
          rlc_cfg.am_nr.tx_sn_field_length = (rlc_am_nr_sn_size_t)size18bits;
          rlc_cfg.am_nr.rx_sn_field_length = (rlc_am_nr_sn_size_t)size18bits;
          break;
        default:
          break;
      }
      rlc_cfg.am_nr.t_poll_retx       = asn1_type->c.am.ul_am_rlc.t_poll_retx.to_number();
      rlc_cfg.am_nr.poll_pdu          = asn1_type->c.am.ul_am_rlc.poll_pdu.to_number();
      rlc_cfg.am_nr.poll_byte         = asn1_type->c.am.ul_am_rlc.poll_byte.to_number();
      rlc_cfg.am_nr.max_retx_thresh   = asn1_type->c.am.ul_am_rlc.max_retx_thres.to_number();
      rlc_cfg.am_nr.t_reassembly      = asn1_type->c.am.dl_am_rlc.t_reassembly.to_number();
      rlc_cfg.am_nr.t_status_prohibit = asn1_type->c.am.dl_am_rlc.t_status_prohibit.to_number();
      break;
    case (enum rlc_types_opts)um_bi_dir:
      rlc_cfg                       = default_rlc_um_nr_config();
      rlc_cfg.rlc_mode              = (rlc_mode_t)um;
      rlc_cfg.um_nr.t_reassembly_ms = asn1_type->c.um_bi_dir.dl_um_rlc.t_reassembly.to_number();
      rlc_cfg.um_nr.bearer_id       = bearer_id;
      if (asn1_type->c.um_bi_dir.dl_um_rlc.sn_field_len_present &&
          asn1_type->c.um_bi_dir.ul_um_rlc.sn_field_len_present &&
          asn1_type->c.um_bi_dir.dl_um_rlc.sn_field_len != asn1_type->c.um_bi_dir.ul_um_rlc.sn_field_len) {
        oset_warn("NR RLC sequence number length is not the same in uplink and downlink");
        return OSET_ERROR;
      }

      switch (asn1_type->c.um_bi_dir.dl_um_rlc.sn_field_len) {
        case (enum sn_field_len_um_opts)size6:
          rlc_cfg.um_nr.sn_field_length = (rlc_um_nr_sn_size_t)size6bits;
          break;
        case (enum sn_field_len_um_opts)size12:
          rlc_cfg.um_nr.sn_field_length = (rlc_um_nr_sn_size_t)size12bits;
          break;
        default:
          break;
      }
      rlc_cfg.um_nr.t_reassembly_ms = asn1_type->c.um_bi_dir.dl_um_rlc.t_reassembly.to_number();
      break;
    case (enum rlc_types_opts)um_uni_dir_dl:
      oset_warn("NR RLC type um_uni_dir_dl is not supported");
      return OSET_ERROR;
    case (enum rlc_types_opts)um_uni_dir_ul:
      oset_warn("NR RLC type um_uni_dir_ul is not supported");
      return OSET_ERROR;
    default:
      break;
  }

  *cfg_out = rlc_cfg;
  return OSET_OK;
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

/////////////////////////////////////////////////////////////////////////////////////
bool API_rrc_mac_make_pdsch_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_sch_hl_cfg_nr_t *sch_hl)
{
	make_pdsch_cfg_from_serv_cell(serv_cell, sch_hl);
}

bool API_rrc_mac_make_csi_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_csi_hl_cfg_t* csi_hl)
{
	make_csi_cfg_from_serv_cell(serv_cell, csi_hl);
}

