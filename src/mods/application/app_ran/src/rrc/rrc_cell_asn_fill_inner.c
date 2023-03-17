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

bool make_phy_tdd_cfg(srsran_duplex_config_nr_t	       	*srsran_duplex_config_nr,
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


bool make_phy_rach_cfg(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_prach_cfg_t *prach_cfg)
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
	case same_as_reg_bundle:
	  srsran_coreset.precoder_granularity = srsran_coreset_precoder_granularity_reg_bundle;
	  break;
	case all_contiguous_rbs:
	  srsran_coreset.precoder_granularity = srsran_coreset_precoder_granularity_contiguous;
	default:
	  oset_error("Invalid option for precoder_granularity %d", ctrl_res_set->precoder_granularity);
	  return false;
	};

	switch (ctrl_res_set->cce_reg_map_type.types) {
	case interleaved:
	  srsran_coreset.mapping_type = srsran_coreset_mapping_type_interleaved;
	  break;
	case non_interleaved:
	  srsran_coreset.mapping_type = srsran_coreset_mapping_type_non_interleaved;
	  break;
	default:
	  oset_error("Invalid option for cce_reg_map_type: %d", ctrl_res_set->cce_reg_map_type.types);
	  return false;
	}
	srsran_coreset.duration = ctrl_res_set->dur;
	for (uint32_t i = 0; i < SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE; i++) {
	srsran_coreset.freq_resources[i] = bit_get(ctrl_res_set->freq_domain_res,SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE - 1 - i);
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
		pdcch->coreset_present[rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set.ctrl_res_set_id] = true;
		make_phy_coreset_cfg(rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set, &pdcch->coreset[rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set.ctrl_res_set_id]);
	}

	for (int i = 0; i < byn_array_get_count(&rrc_cell_cfg->pdcch_cfg_common.common_search_space_list); i++) {
		struct search_space_s *ss = byn_array_get_data(&rrc_cell_cfg->pdcch_cfg_common.common_search_space_list, i);

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
	rach->restricted_set_cfg                = (enum restricted_set_cfg_e_)unrestricted_set;

	return SRSRAN_SUCCESS;
}

void fill_ul_cfg_common_sib_inner(rrc_cell_cfg_nr_t *cell_cfg, struct ul_cfg_common_sib_s *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;
	band_helper_t *band_helper = gnb_manager_self()->band_helper;

	byn_array_set_bounded(&out->freq_info_ul.freq_band_list, 1);
	struct nr_multi_band_info_s *nr_multi_band_info = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct nr_multi_band_info_s));
	byn_array_add(&out->freq_info_ul.freq_band_list, nr_multi_band_info);
	nr_multi_band_info->freq_band_ind_nr_present = true;
	nr_multi_band_info->freq_band_ind_nr         = cell_cfg->band;

	out->freq_info_ul.absolute_freq_point_a_present = true;
	out->freq_info_ul.absolute_freq_point_a =  get_abs_freq_point_a_arfcn_2c(band_helper, cell_cfg->phy_cell.carrier.nof_prb, cell_cfg->ul_arfcn);

	byn_array_set_bounded(&out->freq_info_ul.scs_specific_carrier_list, 1);
	struct scs_specific_carrier_s *scs_specific_carrier = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct scs_specific_carrier_s));
	byn_array_add(&out->freq_info_ul.scs_specific_carrier_list, scs_specific_carrier);
	scs_specific_carrier->offset_to_carrier = cell_cfg->phy_cell.carrier.offset_to_carrier;
	scs_specific_carrier->subcarrier_spacing = cell_cfg->phy_cell.carrier.scs;
	scs_specific_carrier->carrier_bw = cell_cfg->phy_cell.carrier.nof_prb;

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

	byn_array_set_bounded(&pusch->pusch_time_domain_alloc_list, 1);
	struct pusch_time_domain_res_alloc_s *pusch_time_domain_res_alloc = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct pusch_time_domain_res_alloc_s));
	byn_array_add(&pusch->pusch_time_domain_alloc_list, pusch_time_domain_res_alloc);
	pusch_time_domain_res_alloc->k2_present           = true
	pusch_time_domain_res_alloc->k2                   = 4;
	pusch_time_domain_res_alloc->map_type       	  = (enum map_type_e_)type_a;
	pusch_time_domain_res_alloc->start_symbol_and_len = 27;

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
	out->search_space_other_sys_info         = 1;
	out->paging_search_space_present         = true;
	out->paging_search_space                 = 1;
	out->ra_search_space_present             = true;
	out->ra_search_space                     = 1;
}

void fill_pdsch_cfg_common_inner(rrc_cell_cfg_nr_t *cell_cfg, struct pdsch_cfg_common_s *out)
{
	byn_array_set_bounded(&out->pdsch_time_domain_alloc_list, 1);
	struct pdsch_time_domain_res_alloc_s *pdsch_time_domain_res_alloc = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct pdsch_time_domain_res_alloc_s));
	byn_array_add(&out->pdsch_time_domain_alloc_list, pdsch_time_domain_res_alloc);
	pdsch_time_domain_res_alloc->map_type = (enum map_type_e_)type_a;
	pdsch_time_domain_res_alloc->start_symbol_and_len = 40;
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

	byn_array_set_bounded(&out->freq_info_dl.freq_band_list, 1);
	struct nr_multi_band_info_s *nr_multi_band_info = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct nr_multi_band_info_s));
	byn_array_add(&out->freq_info_dl.freq_band_list, nr_multi_band_info);
	nr_multi_band_info->freq_band_ind_nr_present = true;
	nr_multi_band_info->freq_band_ind_nr = cell_cfg->band;

	double	 ssb_freq_start 	 = cell_cfg->ssb_freq_hz - SRSRAN_SSB_BW_SUBC * scs_hz / 2;
	double	 offset_point_a_hz   = ssb_freq_start - nr_arfcn_to_freq_2c(band_helper, cell_cfg->dl_absolute_freq_point_a);
	uint32_t offset_point_a_prbs = offset_point_a_hz / prb_bw;
	out->freq_info_dl.offset_to_point_a = offset_point_a_prbs;

	byn_array_set_bounded(&out->freq_info_dl.scs_specific_carrier_list, 1);
	struct scs_specific_carrier_s *scs_specific_carrier = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct scs_specific_carrier_s));
	byn_array_add(&out->freq_info_dl.scs_specific_carrier_list, scs_specific_carrier);
	scs_specific_carrier->offset_to_carrier = cell_cfg->phy_cell.carrier.offset_to_carrier;
	scs_specific_carrier->subcarrier_spacing = cell_cfg->phy_cell.carrier.scs;
	scs_specific_carrier->carrier_bw = cell_cfg->phy_cell.carrier.nof_prb;

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

	out->ssb_periodicity_serving_cell = (enum ssb_periodicity_serving_cell_e_sib)ms10;

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
		byn_array_set_bounded(&csi_meas_cfg->csi_res_cfg_to_add_mod_list, 3);

		struct csi_res_cfg_s *csi_res_cfg0 = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct csi_res_cfg_s));
		byn_array_add(&csi_meas_cfg->csi_res_cfg_to_add_mod_list, csi_res_cfg0);
		csi_res_cfg0->csi_res_cfg_id = 0;
		csi_res_cfg0->csi_rs_res_set_list.type_ = 0;//nzp_csi_rs_ssb
		struct uint8_t *nzp = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct uint8_t));
		byn_array_add(&csi_res_cfg0->csi_rs_res_set_list.c.nzp_csi_rs_ssb.nzp_csi_rs_res_set_list, nzp);
		*nzp = 0;
		csi_res_cfg0->bwp_id   = 0;
		csi_res_cfg0->res_type = (enum res_type_e_)periodic;

		struct csi_res_cfg_s *csi_res_cfg1 = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct csi_res_cfg_s));
		byn_array_add(&csi_meas_cfg->csi_res_cfg_to_add_mod_list, csi_res_cfg1);
		csi_res_cfg1->csi_res_cfg_id = 1;
		csi_res_cfg1->csi_rs_res_set_list.type_ = 1;//csi_im_res_set_list
		struct uint8_t *im_res = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct uint8_t));
		byn_array_add(&csi_res_cfg0->csi_rs_res_set_list.c.csi_im_res_set_list, im_res);
		*im_res = 0;
		csi_res_cfg1->bwp_id   = 0;
		csi_res_cfg1->res_type = (enum res_type_e_)periodic;

		struct csi_res_cfg_s *csi_res_cfg2 = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct csi_res_cfg_s));
		byn_array_add(&csi_meas_cfg->csi_res_cfg_to_add_mod_list, csi_res_cfg2);
		csi_res_cfg2->csi_res_cfg_id = 2;
		csi_res_cfg2->csi_rs_res_set_list.type_ = 0;//nzp_csi_rs_ssb
		struct uint8_t *nzp2 = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct uint8_t));
		byn_array_add(&csi_res_cfg0->csi_rs_res_set_list.c.nzp_csi_rs_ssb.nzp_csi_rs_res_set_list, nzp2);
		*nzp2 = 1;
		csi_res_cfg2->bwp_id   = 0;
		csi_res_cfg2->res_type = (enum res_type_e_)periodic;
	}
}

/// Fill lists of NZP-CSI-RS-Resource and NZP-CSI-RS-ResourceSet with gNB config
void fill_nzp_csi_rs_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct csi_meas_cfg_s *csi_meas_cfg)
{
	ASSERT_IF_NOT(cfg->is_standalone, "Not support NSA now!")

	if (cfg->is_standalone) {
		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			byn_array_set_bounded(&csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list, 5);

			struct nzp_csi_rs_res_s *nzp_csi_res0 = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct nzp_csi_rs_res_s));
			byn_array_add(&csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list, nzp_csi_res0);
			// item 0
			nzp_csi_res0->res_map.freq_domain_alloc.type_ = 1;//row2
			bitstring_from_number(&nzp_csi_res0->res_map.freq_domain_alloc.c, 0x800, 12);
			nzp_csi_res0->res_map.nrof_ports                 = asn1::rrc_nr::csi_rs_res_map_s::nrof_ports_opts::p1;
			nzp_csi_res0->res_map.first_ofdm_symbol_in_time_domain = 4;
			nzp_csi_res0->res_map.cdm_type.value                   = asn1::rrc_nr::csi_rs_res_map_s::cdm_type_opts::no_cdm;
			nzp_csi_res0->res_map.density.set_one();
			nzp_csi_res0->res_map.freq_band.start_rb     = 0;
			nzp_csi_res0->res_map.freq_band.nrof_rbs     = 52;
			nzp_csi_res0->pwr_ctrl_offset                = 0;
			nzp_csi_res0->pwr_ctrl_offset_ss_present     = true;
			nzp_csi_res0->pwr_ctrl_offset_ss.value       = asn1::rrc_nr::nzp_csi_rs_res_s::pwr_ctrl_offset_ss_opts::db0;
			nzp_csi_res0->scrambling_id                  = cfg.cell_list[0].phy_cell.cell_id;
			nzp_csi_res0->periodicity_and_offset_present = true;
			nzp_csi_res0->periodicity_and_offset.set_slots80();
			nzp_csi_res0->periodicity_and_offset.slots80() = 1;
			// optional
			nzp_csi_res[0].qcl_info_periodic_csi_rs_present = true;
			nzp_csi_res[0].qcl_info_periodic_csi_rs         = 0;
			// item 1
			nzp_csi_res[1]                   = nzp_csi_res[0];
			nzp_csi_res[1].nzp_csi_rs_res_id = 1;
			nzp_csi_res[1].res_map.freq_domain_alloc.set_row1();
			nzp_csi_res[1].res_map.freq_domain_alloc.row1().from_number(0x1);
			nzp_csi_res[1].res_map.nrof_ports.value = asn1::rrc_nr::csi_rs_res_map_s::nrof_ports_opts::p1;
			nzp_csi_res[1].res_map.cdm_type.value   = asn1::rrc_nr::csi_rs_res_map_s::cdm_type_opts::no_cdm;
			nzp_csi_res[1].res_map.density.set_three();
			nzp_csi_res[1].periodicity_and_offset.set_slots40();
			nzp_csi_res[1].periodicity_and_offset.slots40() = 11;
			// item 2
			nzp_csi_res[2]                                          = nzp_csi_res[1];
			nzp_csi_res[2].nzp_csi_rs_res_id                        = 2;
			nzp_csi_res[2].res_map.first_ofdm_symbol_in_time_domain = 8;
			// item 3
			nzp_csi_res[3]                                          = nzp_csi_res[1];
			nzp_csi_res[3].nzp_csi_rs_res_id                        = 3;
			nzp_csi_res[3].res_map.first_ofdm_symbol_in_time_domain = 4;
			nzp_csi_res[3].periodicity_and_offset.set_slots40();
			nzp_csi_res[3].periodicity_and_offset.slots40() = 12;
			// item 4
			nzp_csi_res[4]                                          = nzp_csi_res[1];
			nzp_csi_res[4].nzp_csi_rs_res_id                        = 4;
			nzp_csi_res[4].res_map.first_ofdm_symbol_in_time_domain = 8;
			nzp_csi_res[4].periodicity_and_offset.set_slots40();
			nzp_csi_res[4].periodicity_and_offset.slots40() = 12;
		} else {
			csi_meas_cfg.nzp_csi_rs_res_to_add_mod_list.resize(1);
			auto& nzp_csi_res                = csi_meas_cfg.nzp_csi_rs_res_to_add_mod_list;
			nzp_csi_res[0].nzp_csi_rs_res_id = 0;
			nzp_csi_res[0].res_map.freq_domain_alloc.set_row2();
			nzp_csi_res[0].res_map.freq_domain_alloc.row2().from_number(0b100000000000);
			nzp_csi_res[0].res_map.nrof_ports.value                 = asn1::rrc_nr::csi_rs_res_map_s::nrof_ports_opts::p1;
			nzp_csi_res[0].res_map.first_ofdm_symbol_in_time_domain = 4;
			nzp_csi_res[0].res_map.cdm_type.value                   = asn1::rrc_nr::csi_rs_res_map_s::cdm_type_opts::no_cdm;
			nzp_csi_res[0].res_map.density.set_one();
			nzp_csi_res[0].res_map.freq_band.start_rb = 0;
			nzp_csi_res[0].res_map.freq_band.nrof_rbs = 52;
			nzp_csi_res[0].pwr_ctrl_offset            = 0;
			// Skip pwr_ctrl_offset_ss_present
			nzp_csi_res[0].periodicity_and_offset_present       = true;
			nzp_csi_res[0].periodicity_and_offset.set_slots80() = 0;
			// optional
			nzp_csi_res[0].qcl_info_periodic_csi_rs_present = true;
			nzp_csi_res[0].qcl_info_periodic_csi_rs         = 0;
		}

		// Fill NZP-CSI Resource Sets
		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			csi_meas_cfg.nzp_csi_rs_res_set_to_add_mod_list.resize(2);
			auto& nzp_csi_res_set = csi_meas_cfg.nzp_csi_rs_res_set_to_add_mod_list;
			// item 0
			nzp_csi_res_set[0].nzp_csi_res_set_id = 0;
			nzp_csi_res_set[0].nzp_csi_rs_res.resize(1);
			nzp_csi_res_set[0].nzp_csi_rs_res[0] = 0;
			// item 1
			nzp_csi_res_set[1].nzp_csi_res_set_id = 1;
			nzp_csi_res_set[1].nzp_csi_rs_res.resize(4);
			nzp_csi_res_set[1].nzp_csi_rs_res[0] = 1;
			nzp_csi_res_set[1].nzp_csi_rs_res[1] = 2;
			nzp_csi_res_set[1].nzp_csi_rs_res[2] = 3;
			nzp_csi_res_set[1].nzp_csi_rs_res[3] = 4;
			nzp_csi_res_set[1].trs_info_present  = true;
			  //    // Skip TRS info
	} else {
			csi_meas_cfg.nzp_csi_rs_res_set_to_add_mod_list.resize(1);
			auto& nzp_csi_res_set                 = csi_meas_cfg.nzp_csi_rs_res_set_to_add_mod_list;
			nzp_csi_res_set[0].nzp_csi_res_set_id = 0;
			nzp_csi_res_set[0].nzp_csi_rs_res.resize(1);
			nzp_csi_res_set[0].nzp_csi_rs_res[0] = 0;
			// Skip TRS info
		}
	}
}

/// Fill CSI-MeasConfig with gNB config
int fill_csi_meas_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct csi_meas_cfg_s *csi_meas_cfg)
{
  //  // Fill CSI Report
  //  if (fill_csi_report_from_enb_cfg(cfg, csi_meas_cfg) != SRSRAN_SUCCESS) {
  //    get_logger(cfg).error("Failed to configure eNB CSI Report");
  //    return OSET_ERROR;
  //  }

  // Fill CSI resource config
  fill_csi_resource_cfg_to_add_inner(cfg, cell_cfg, csi_meas_cfg);

  // Fill NZP-CSI Resources
  fill_nzp_csi_rs_from_enb_cfg_inner(cfg, cell_cfg, csi_meas_cfg);

  if (cfg->is_standalone) {
    // CSI IM config
    fill_csi_im_resource_cfg_to_add(cfg, cell_cfg, csi_meas_cfg);

    // CSI report config
    fill_csi_report_from_enb_cfg(cfg, cell_cfg, csi_meas_cfg);
  }

  return OSET_OK;
}


/// Fill ServingCellConfig with gNB config
int fill_serv_cell_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, uint32_t cc, struct serving_cell_cfg_s *serv_cell)
{
	rrc_cell_cfg_nr_t *cell_cfg = oset_list2_find(cfg->cell_list, cc)->data;

	serv_cell->csi_meas_cfg_present = true;
	serv_cell->csi_meas_cfg.type_ = setup;
	HANDLE_ERROR(fill_csi_meas_from_enb_cfg_inner(cfg, cell_cfg, &serv_cell->csi_meas_cfg.c));

	serv_cell->init_dl_bwp_present = true;
	fill_init_dl_bwp_from_enb_cfg(cfg, cell_cfg, &serv_cell->init_dl_bwp);

	serv_cell->first_active_dl_bwp_id_present = true;
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
		serv_cell->first_active_dl_bwp_id = 0;
	} else {
		serv_cell->first_active_dl_bwp_id = 1;
	}

	serv_cell->ul_cfg_present = true;
	fill_ul_cfg_from_enb_cfg(cfg, cell_cfg, serv_cell->ul_cfg);

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


/// Fill MasterCellConfig with gNB config
int fill_master_cell_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, uint32_t cc, struct cell_group_cfg_s *out)
{
	out->cell_group_id = 0;

	byn_array_set_bounded(&out->rlc_bearer_to_add_mod_list, 1);
	struct rlc_bearer_cfg_s *rlc_bearer = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct rlc_bearer_cfg_s));
	byn_array_add(&out->rlc_bearer_to_add_mod_list, rlc_bearer);
	fill_srb_inner(cfg, srb1, rlc_bearer);

	// mac-CellGroupConfig -- Need M
	out->mac_cell_group_cfg_present                   = true;
	out->mac_cell_group_cfg.sched_request_cfg_present = true;
	byn_array_set_bounded(&out->mac_cell_group_cfg.sched_request_cfg.sched_request_to_add_mod_list, 1);
	struct sched_request_to_add_mod_s *sr = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct sched_request_to_add_mod_s));
	byn_array_add(&out->mac_cell_group_cfg.sched_request_cfg.sched_request_to_add_mod_list, sr);
	sr->sched_request_id = 0;
	sr->sr_trans_max = (enum sr_trans_max_e_)n64;

	out->mac_cell_group_cfg.bsr_cfg_present                  = true;
	out->mac_cell_group_cfg.bsr_cfg.periodic_bsr_timer       = (enum periodic_bsr_timer_e_)sf20;
	out->mac_cell_group_cfg.bsr_cfg.retx_bsr_timer           = (enum retx_bsr_timer_e_)sf320;

	out->mac_cell_group_cfg.tag_cfg_present                  = true;
	byn_array_set_bounded(&out->mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list, 1);
	struct tag_s *tag = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct tag_s));
	byn_array_add(&out->mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list, tag);
	tag->tag_id           = 0;
	tag->time_align_timer = (enum time_align_timer_e)infinity;

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

int fill_sib1_from_enb_cfg_inner(rrc_cell_cfg_nr_t *cell_cfg, struct sib1_s *sib1)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	sib1->cell_sel_info_present            = true;
	sib1->cell_sel_info.q_rx_lev_min       = -70;
	sib1->cell_sel_info.q_qual_min_present = true;
	sib1->cell_sel_info.q_qual_min         = -20;

	byn_array_set_bounded(&sib1->cell_access_related_info.plmn_id_list, 1);
	struct plmn_id_info_s *plmn_id_info = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct plmn_id_info_s));
	byn_array_add(&sib1->cell_access_related_info.plmn_id_list, plmn_id_info);

	byn_array_set_bounded(&plmn_id_info->plmn_id_list, 1);
	struct plmn_id_s *plmn_id = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct plmn_id_s));
	byn_array_add(&plmn_id_info->plmn_id_list, plmn_id);
	plmn_id->mcc_present = true;
	mcc_to_bytes(cfg->mcc ,plmn_id->mcc);
	mnc_to_bytes(cfg->mnc ,plmn_id->mnc, &plmn_id->nof_mnc_digits);

	plmn_id_info->tac_present = true
	bitstring_from_number(plmn_id_info->tac, cell_cfg->tac, 24);
	bitstring_from_number(plmn_id_info->cell_id, (cfg->enb_id << 8U) + cell_cfg->phy_cell.cell_id, 36);
	plmn_id_info->cell_reserved_for_oper = (enum cell_reserved_for_oper_e_)not_reserved;

	sib1->conn_est_fail_ctrl_present             = true;
	sib1->conn_est_fail_ctrl.conn_est_fail_count = (enum conn_est_fail_count_e_)n1;
	sib1->conn_est_fail_ctrl.conn_est_fail_offset_validity = (enum conn_est_fail_offset_validity_e_)s30;
	sib1->conn_est_fail_ctrl.conn_est_fail_offset_present = true;
	sib1->conn_est_fail_ctrl.conn_est_fail_offset         = 1;

	sib1->si_sched_info_present                                  = true;
	sib1->si_sched_info.si_request_cfg.rach_occasions_si_present = true;
	sib1->si_sched_info.si_request_cfg.rach_occasions_si.rach_cfg_si.ra_resp_win = (enum ra_resp_win_e_)sl8;
	sib1->si_sched_info.si_win_len = (enum si_win_len_e_)s20;

	byn_array_set_bounded(&sib1->si_sched_info.sched_info_list, 1);
	struct sched_info_s *sched_info = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct sched_info_s));
	byn_array_add(&sib1->si_sched_info.sched_info_list, sched_info);
	sched_info->si_broadcast_status = (enum si_broadcast_status_e_)broadcasting;
	sched_info->si_periodicity = (enum si_periodicity_e_)rf16;

	byn_array_set_bounded(&sched_info->sib_map_info, 1);
	struct sib_type_info_s *sib_type = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(struct sib_type_info_s));
	byn_array_add(&sched_info->sib_map_info, sib_type);
	// scheduling of SI messages
	sib_type->type              = (enum type_e_)sib_type2;
	sib_type->value_tag_present = true;
	sib_type->value_tag         = 0;

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

