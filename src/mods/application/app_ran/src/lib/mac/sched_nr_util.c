/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "lib/mac/sched_nr_util.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-util"

bool make_phy_tdd_cfg(tdd_ul_dl_cfg_common_s *tdd_ul_dl_cfg_common,
                      		srsran_duplex_config_nr_t *in_srsran_duplex_config_nr)
{
  srsran_duplex_config_nr_t srsran_duplex_config_nr = {0};
  srsran_duplex_config_nr.mode                      = SRSRAN_DUPLEX_MODE_TDD;

  switch (tdd_ul_dl_cfg_common->pattern1.dl_ul_tx_periodicity) {
    case (enum dl_ul_tx_periodicity_e_)ms1:
      srsran_duplex_config_nr.tdd.pattern1.period_ms = 1;
      break;
    case (enum dl_ul_tx_periodicity_e_)ms2:
      srsran_duplex_config_nr.tdd.pattern1.period_ms = 2;
      break;
    case (enum dl_ul_tx_periodicity_e_)ms5:
      srsran_duplex_config_nr.tdd.pattern1.period_ms = 5;
      break;
    case (enum dl_ul_tx_periodicity_e_)ms10:
      srsran_duplex_config_nr.tdd.pattern1.period_ms = 10;
      break;

    case (enum dl_ul_tx_periodicity_e_)ms1p25:
    case (enum dl_ul_tx_periodicity_e_)ms0p5:
    case (enum dl_ul_tx_periodicity_e_)ms0p625:
    case (enum dl_ul_tx_periodicity_e_)ms2p5:
    default:
      oset_error("Invalid option for dl_ul_tx_periodicity_opts %d",
                        tdd_ul_dl_cfg_common->pattern1.dl_ul_tx_periodicity);
      return false;
  }
  srsran_duplex_config_nr.tdd.pattern1.nof_dl_slots   = tdd_ul_dl_cfg_common->pattern1.nrof_dl_slots;
  srsran_duplex_config_nr.tdd.pattern1.nof_dl_symbols = tdd_ul_dl_cfg_common->pattern1.nrof_dl_symbols;
  srsran_duplex_config_nr.tdd.pattern1.nof_ul_slots   = tdd_ul_dl_cfg_common->pattern1.nrof_ul_slots;
  srsran_duplex_config_nr.tdd.pattern1.nof_ul_symbols = tdd_ul_dl_cfg_common->pattern1.nrof_ul_symbols;

  if (tdd_ul_dl_cfg_common->pattern2_present) {
    switch (tdd_ul_dl_cfg_common->pattern2.dl_ul_tx_periodicity) {
      case (enum dl_ul_tx_periodicity_e_)ms1:
        srsran_duplex_config_nr.tdd.pattern2.period_ms = 1;
        break;
      case (enum dl_ul_tx_periodicity_e_)ms2:
        srsran_duplex_config_nr.tdd.pattern2.period_ms = 2;
        break;
      case (enum dl_ul_tx_periodicity_e_)ms5:
        srsran_duplex_config_nr.tdd.pattern2.period_ms = 5;
        break;
      case (enum dl_ul_tx_periodicity_e_)ms10:
        srsran_duplex_config_nr.tdd.pattern2.period_ms = 10;
        break;

      case (enum dl_ul_tx_periodicity_e_)ms1p25:
      case (enum dl_ul_tx_periodicity_e_)ms0p5:
      case (enum dl_ul_tx_periodicity_e_)ms0p625:
      case (enum dl_ul_tx_periodicity_e_)ms2p5:
      default:
        oset_error("Invalid option for pattern2 dl_ul_tx_periodicity_opts %d",
                          tdd_ul_dl_cfg_common->pattern2.dl_ul_tx_periodicity);
        return false;
    }

    srsran_duplex_config_nr.tdd.pattern2.nof_dl_slots   = tdd_ul_dl_cfg_common->pattern2.nrof_dl_slots;
    srsran_duplex_config_nr.tdd.pattern2.nof_dl_symbols = tdd_ul_dl_cfg_common->pattern2.nrof_dl_symbols;
    srsran_duplex_config_nr.tdd.pattern2.nof_ul_slots   = tdd_ul_dl_cfg_common->pattern2.nrof_ul_slots;
    srsran_duplex_config_nr.tdd.pattern2.nof_ul_symbols = tdd_ul_dl_cfg_common->pattern2.nrof_ul_symbols;
  }

  // Copy and return struct
  *in_srsran_duplex_config_nr = srsran_duplex_config_nr;

  return true;
}

bool make_phy_rach_cfg(rach_cfg_common_s *asn1_type,
					          srsran_duplex_mode_t duplex_mode,
					           srsran_prach_cfg_t *prach_cfg)
{
	prach_cfg->is_nr			  = true;
	prach_cfg->config_idx 	  = asn1_type->rach_cfg_generic.prach_cfg_idx;
	prach_cfg->zero_corr_zone   = (uint32_t)asn1_type->rach_cfg_generic.zero_correlation_zone_cfg;
	prach_cfg->num_ra_preambles = 64;
	if (asn1_type->total_nof_ra_preambs_present) {
		prach_cfg->num_ra_preambles = asn1_type->total_nof_ra_preambs;
	}
	prach_cfg->hs_flag	= false; // Hard-coded
	prach_cfg->tdd_config = {0};
	if (duplex_mode == SRSRAN_DUPLEX_MODE_TDD) {
		prach_cfg->tdd_config.configured = true;
	}

	// As the current PRACH is based on LTE, the freq-offset shall be subtracted 1 for aligning with NR bandwidth
	// For example. A 52 PRB cell with an freq_offset of 1 will match a LTE 50 PRB cell with freq_offset of 0
	prach_cfg->freq_offset = (uint32_t)asn1_type->rach_cfg_generic.msg1_freq_start;
	if (prach_cfg->freq_offset == 0) {
		oset_error("PRACH freq offset must be at least one");
		return false;
	}

	switch (asn1_type->prach_root_seq_idx.type_) {
		case l839:
		  prach_cfg->root_seq_idx = (uint32_t)asn1_type->prach_root_seq_idx.c;
		  break;
		case l139:
		  prach_cfg->root_seq_idx = (uint32_t)asn1_type->prach_root_seq_idx.c;
		default:
		  oset_error("Not-implemented option for prach_root_seq_idx type %d",
						  asn1_type->prach_root_seq_idx.type_);
		  return false;
  }

  return true;
};

uint32_t coreset_nof_cces(srsran_coreset_t *coreset)
{
  bool* res_active   = coreset->freq_resources[0];
  uint32_t nof_freq_res = std::count(res_active, res_active + SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE, true);
  return nof_freq_res * coreset.duration;
}

void make_mib_cfg(sched_nr_cell_cfg_t *cfg, srsran_mib_nr_t *mib)
{
  *mib            = {0};
  mib->scs_common = (srsran_subcarrier_spacing_t)cfg->dl_cfg_common.init_dl_bwp.generic_params.subcarrier_spacing;
  mib->ssb_offset = cfg->ssb_offset;
  mib->dmrs_typeA_pos         = (srsran_dmrs_sch_typeA_pos_t)cfg->dmrs_type_a_position;
  mib->coreset0_idx           = cfg->pdcch_cfg_sib1.ctrl_res_set_zero;
  mib->ss0_idx                = cfg->pdcch_cfg_sib1.search_space_zero;
  mib->cell_barred            = false;
  mib->intra_freq_reselection = true;
}

void make_ssb_cfg(sched_nr_cell_cfg_t *cfg, ssb_cfg_t* ssb)
{
	band_helper_t *band_helper = gnb_manager_self()->band_helper;

	ssb->periodicity_ms    = cfg->ssb_periodicity_ms;
	ssb->position_in_burst = {0};
	uint32_t N             = 8;
	for (uint32_t i = 0; i < N; ++i) {
		ssb->position_in_burst[i] = bit_get(&cfg->ssb_positions_in_burst.in_one_group,i)
	}
	if (cfg->ssb_positions_in_burst.group_presence_present) {
		for (uint32_t i = 1; i < 8; ++i) {
		  if (bit_get(&cfg->ssb_positions_in_burst.group_presence,i)) {
		    	//std::copy(
		    	//    ssb->position_in_burst.begin(), ssb->position_in_burst.begin() + N, ssb->position_in_burst.begin() + i * N);
				memcpy(ssb->position_in_burst + i * N, ssb->position_in_burst, 8);
		  }
		}
	}
	ssb->scs     = (srsran_subcarrier_spacing_t)cfg->ssb_scs;
	ssb->pattern = SRSRAN_SSB_PATTERN_A;
	if (byn_array_get_count(&cfg->dl_cfg_common->freq_info_dl.freq_band_list) > 0){
		struct nr_multi_band_info_s *multi_band_info = byn_array_get_data(&cfg->dl_cfg_common->freq_info_dl.freq_band_list, 0);
		if(multi_band_info->freq_band_ind_nr_present) {
			uint32_t band = multi_band_info->freq_band_ind_nr;
			ssb->pattern  = get_ssb_pattern_2c(band_helper, band, ssb->scs);
		}
	}
}

phy_cfg_nr_t get_common_ue_phy_cfg(sched_nr_cell_cfg_t *cfg)
{
	phy_cfg_nr_t ue_phy_cfg = {0};

	// TDD UL-DL config
	ue_phy_cfg.duplex.mode = SRSRAN_DUPLEX_MODE_FDD;
	if (cfg->tdd_ul_dl_cfg_common) {
		bool success = make_phy_tdd_cfg(cfg->tdd_ul_dl_cfg_common, &ue_phy_cfg.duplex);
		ASSERT_IF_NOT(success, "Failed to convert Cell TDDConfig to UEPHYConfig");
	}

	ue_phy_cfg.pdcch = cfg->bwps[0].pdcch;
	ue_phy_cfg.pdsch = cfg->bwps[0].pdsch;
	ue_phy_cfg.pusch = cfg->bwps[0].pusch;
	ue_phy_cfg.pucch = cfg->bwps[0].pucch;
	make_phy_rach_cfg(cfg->ul_cfg_common.init_ul_bwp.rach_cfg_common.c,
	                        (cfg->tdd_ul_dl_cfg_common != NULL) ? SRSRAN_DUPLEX_MODE_TDD : SRSRAN_DUPLEX_MODE_FDD,
	                        &ue_phy_cfg.prach);
	ue_phy_cfg.harq_ack                       = cfg->bwps[0].harq_ack;
	ue_phy_cfg.csi                            = {0}; // disable CSI until RA is complete
	ue_phy_cfg.carrier.pci                    = cfg->pci;
	ue_phy_cfg.carrier.dl_center_frequency_hz = cfg->dl_center_frequency_hz;
	ue_phy_cfg.carrier.ul_center_frequency_hz = cfg->ul_center_frequency_hz;
	ue_phy_cfg.carrier.ssb_center_freq_hz     = cfg->ssb_center_freq_hz;
	ue_phy_cfg.carrier.offset_to_carrier = cfg->dl_cfg_common.freq_info_dl.scs_specific_carrier_list[0].offset_to_carrier;
	ue_phy_cfg.carrier.scs =
	  (srsran_subcarrier_spacing_t)cfg->dl_cfg_common.init_dl_bwp.generic_params.subcarrier_spacing;
	ue_phy_cfg.carrier.nof_prb         = cfg->dl_cell_nof_prb;
	ue_phy_cfg.carrier.max_mimo_layers = cfg->nof_layers;
	make_ssb_cfg(cfg, &ue_phy_cfg.ssb);

	// remove UE-specific SearchSpaces (they will be added later via RRC)
	for (uint32_t i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; ++i) {
		if (ue_phy_cfg.pdcch.search_space_present[i] && ue_phy_cfg.pdcch.search_space[i].type == srsran_search_space_type_ue) {
			ue_phy_cfg.pdcch.search_space_present[i] = false;
			ue_phy_cfg.pdcch.search_space[i]         = {0};
		}
	}

  return ue_phy_cfg;
}

