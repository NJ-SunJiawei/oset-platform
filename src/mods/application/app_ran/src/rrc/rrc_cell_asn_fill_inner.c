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
#define OSET_LOG2_DOMAIN   "app-gnb-rrcInner"

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
	out->served_radio_bearer.type_        = (enum served_radio_bearer_types)srb_id;//0
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

/// Fill DRB with parameters derived from cfg
void fill_drb_inner(rrc_nr_cfg_five_qi_t *cfg_five_qi, radio_bearer_t *rb, nr_drb drb_id, struct rlc_bearer_cfg_s *out)
{
	out->lc_ch_id                         = rb->lcid;
	out->served_radio_bearer_present      = true;
	out->served_radio_bearer.c            = (uint8_t)drb_id;

	out->rlc_cfg_present = true;
	out->rlc_cfg         = cfg_five_qi->rlc_cfg;

	// MAC logical channel config
	out->mac_lc_ch_cfg_present                    = true;
	out->mac_lc_ch_cfg.ul_specific_params_present = true;
	out->mac_lc_ch_cfg.ul_specific_params.prio    = 11; // TODO
	out->mac_lc_ch_cfg.ul_specific_params.prioritised_bit_rate = (prioritised_bit_rate_opts)kbps0;
	out->mac_lc_ch_cfg.ul_specific_params.bucket_size_dur = (bucket_size_dur_opts)ms100;
	out->mac_lc_ch_cfg.ul_specific_params.lc_ch_group_present          = true;
	out->mac_lc_ch_cfg.ul_specific_params.lc_ch_group                  = 3; // TODO
	out->mac_lc_ch_cfg.ul_specific_params.sched_request_id_present     = true;
	out->mac_lc_ch_cfg.ul_specific_params.sched_request_id             = 0; // TODO
	out->mac_lc_ch_cfg.ul_specific_params.lc_ch_sr_mask                = false;
	out->mac_lc_ch_cfg.ul_specific_params.lc_ch_sr_delay_timer_applied = false;
  // TODO: add LC config to MAC
}


/// Fill csi-ResoureConfigToAddModList
void fill_csi_resource_cfg_to_add_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct csi_meas_cfg_s *csi_meas_cfg)
{
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
		csi_meas_cfg->duplex_mode = SRSRAN_DUPLEX_MODE_FDD;
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
			csi_meas_cfg->nzp_csi_rs_res_to_add_mod_list[0].res_map.density.type_          = (enum density_e_)one;//one
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
		csi_meas_cfg->csi_im_res_to_add_mod_list[0].csi_im_res_elem_pattern.type_ = (enum csi_im_res_elem_pattern_types)pattern1;//pattern1
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
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].codebook_cfg.codebook_type.type_ = (enum codebook_types)type1;//type1
		struct type1_s_ *type1 = csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].codebook_cfg.codebook_type.c.type1;
		type1->sub_type.type_ = (enum sub_types)type_i_single_panel;//type_i_single_panel
		type1->sub_type.c.type_i_single_panel.nr_of_ant_ports.type_ = (enum nr_of_ant_ports_types)two;//two
		bitstring_from_number(&type1->sub_type.c.type_i_single_panel.nr_of_ant_ports.c.two.two_tx_codebook_subset_restrict, 0b111111, 6);
		bitstring_from_number(&type1->sub_type.c.type_i_single_panel.type_i_single_panel_ri_restrict, 0x03, 8);
		type1->codebook_mode = 1;
		csi_meas_cfg->csi_report_cfg_to_add_mod_list[0].group_based_beam_report.type_ = (enum group_based_beam_report_types)disabled;//disabled
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

void fill_pdsch_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, struct pdsch_cfg_s *out)
{
  out->dmrs_dl_for_pdsch_map_type_a_present = true;
  out->dmrs_dl_for_pdsch_map_type_a.type_ = setup;
  out->dmrs_dl_for_pdsch_map_type_a.c.dmrs_add_position_present = true;
  out->dmrs_dl_for_pdsch_map_type_a.c.dmrs_add_position         = (enum dmrs_add_position_e_)pos1;

  cvector_reserve(out->tci_states_to_add_mod_list, 1);
  cvector_set_size(out->tci_states_to_add_mod_list, 1);
  out->tci_states_to_add_mod_list[0].tci_state_id = 0;
  out->tci_states_to_add_mod_list[0].qcl_type1.ref_sig.type_ = (enum qcl_ref_sig_types)ssb;//ssb
  out->tci_states_to_add_mod_list[0].qcl_type1.ref_sig.c = 0;
  out->tci_states_to_add_mod_list[0].qcl_type1.qcl_type  = (enum qcl_type_e_)type_d;

  out->res_alloc = (enum res_alloc_e_)res_alloc_type1;
  out->rbg_size  = (enum rbg_size_e_)cfg1;

  out->prb_bundling_type.type_ = (enum prb_bundling_types)static_bundling;//static_bundling
  out->prb_bundling_type.c.static.bundle_size_present = true;
  out->prb_bundling_type.c.static.bundle_size = (enum bundle_size_e_)wideband;

  // MCS Table
  // NOTE: For Table 1 or QAM64, set false and comment value
  out->mcs_table_present = false;
  out->mcs_table = (enum mcs_table_e_)qam256;

  // ZP-CSI
  cvector_reserve(out->zp_csi_rs_res_to_add_mod_list, 1);
  cvector_set_size(out->zp_csi_rs_res_to_add_mod_list, 1);
  out->zp_csi_rs_res_to_add_mod_list[0].zp_csi_rs_res_id = 0;
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.freq_domain_alloc.type_ = (enum freq_domain_alloc_e_)row4;
  bitstring_from_number(&out->zp_csi_rs_res_to_add_mod_list[0].res_map.freq_domain_alloc.c, 0b100, 12);
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.nrof_ports = (enum nrof_ports_e_)p4;

  out->zp_csi_rs_res_to_add_mod_list[0].res_map.first_ofdm_symbol_in_time_domain = 8;
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.cdm_type = (enum cdm_type_e_)fd_cdm2;
  out->zp_csi_rs_res_to_add_mod_list[0].res_map.density.type_ = (enum density_e_)one;

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
	  out->res_to_add_mod_list[i].format.type_                      = (enum pucch_format_types)format1;
      out->res_to_add_mod_list[i].format.c.f1.init_cyclic_shift     = (4 * (j % 3));
      out->res_to_add_mod_list[i].format.c.f1.nrof_symbols          = 14;
      out->res_to_add_mod_list[i].format.c.f1.start_symbol_idx      = 0;
      out->res_to_add_mod_list[i].format.c.f1.time_domain_occ       = j / 3;
      j++;
    } else if (i < 15) {
      out->res_to_add_mod_list[i].start_prb                         = 1;
      out->res_to_add_mod_list[i].second_hop_prb_present            = true;
      out->res_to_add_mod_list[i].second_hop_prb                    = cell_cfg->phy_cell.carrier.nof_prb-2;//50
      out->res_to_add_mod_list[i].format.type_                      = (enum pucch_format_types)format2;
      out->res_to_add_mod_list[i].format.c.f2.nrof_prbs             = 1;
      out->res_to_add_mod_list[i].format.c.f2.nrof_symbols          = 2;
      out->res_to_add_mod_list[i].format.c.f2.start_symbol_idx      = 2 * (j2 % 7);
      j2++;
    } else {
      out->res_to_add_mod_list[i].start_prb                         = cell_cfg->phy_cell.carrier.nof_prb -2;//50
      out->res_to_add_mod_list[i].second_hop_prb_present            = true;
      out->res_to_add_mod_list[i].second_hop_prb                    = 1;
      out->res_to_add_mod_list[i].format.type_                      = (enum pucch_format_types)format2;
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
  // //??? todo不同的ue分配的sr_resource应该不一样
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
  out->uci_on_pusch.c.beta_offsets.type_ = (enum beta_offsets_types)semi_static;//semi_static
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
	serv_cell->duplex_mode = cell_cfg->duplex_mode;
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

void free_master_cell_cfg_vector(struct cell_group_cfg_s *master_cell_group)
{
    //free rlc_bearer_to_add_mod_list
	cvector_free(master_cell_group->rlc_bearer_to_add_mod_list);
	cvector_free(master_cell_group->rlc_bearer_to_release_list);

    //free sched_request_to_add_mod_list
	cvector_free(master_cell_group->mac_cell_group_cfg.sched_request_cfg.sched_request_to_add_mod_list);
    //free tag_to_add_mod_list
	cvector_free(master_cell_group->mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list);


    //free csi-ResoureConfigToAddModList
    struct csi_res_cfg_s *csi_res_cfg = NULL;
    cvector_for_each_in(csi_res_cfg, master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.csi_meas_cfg.c.csi_res_cfg_to_add_mod_list){
		//release csi_rs_res_set_list
		if(nzp_csi_rs_ssb == csi_res_cfg->csi_rs_res_set_list.type_) cvector_free(csi_res_cfg->csi_rs_res_set_list.c.nzp_csi_rs_ssb.nzp_csi_rs_res_set_list);	
		if(csi_im_res_set_list == csi_res_cfg->csi_rs_res_set_list.type_) cvector_free(csi_res_cfg->csi_rs_res_set_list.c.csi_im_res_set_list);
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

// RRC定义了三种SRB：SRB0，SRB1，SRB2，SRB3
// (1) SRB0
// 使用CCCH逻辑信道，用于RRC连接建立/重建过程。一直存在。
// SRB0没有加密和完整性保护。
// SRB0上承载的信令有：RRCConnectionRequest、RRCConnectionReject、RRCConnectionSetup和RRCConnectionReestablishmentRequest、RRCConnectionReestablishment、RRCConnectionReestablishmentReject。
// (2)SRB1
// 使用DCCH信道，由RRC连接建立时建立。
// 当初始安全激活之后，SRB1有加密和完整性保护。
// SRB1承载所有RRC信令和部分NAS信令（SRB2未建立前）
// (3)SRB2
// 使用DCCH信道，由RRC重配时建立，初始安全激活后。
// SRB2承载NAS信令。
// (4)SRB3
// 只用于NSA组网，用在SCG上面
// 用于UE和SgNB之间传输一些特定的RRC消息，全部使用DCCH逻辑信道。
// SRB2上传送的RRC信令非常少，只有两 种：UL information transfer以及DL information transfer消息
// 在SRB2没有建立的时候，UL information transfer以及DL information transfer消息也会通过SRB1来传，
// 比如附着过程中获取终端ID的信令流程。这样做有一个缺点，安全性没有保证

// |      |    rrcsetup rq                 |pdi  1          |pdu seesion ID
// |      |    rrcsetup                    |qfi  1          |qod flow ID
// |      |                                |5qi  7/9        |fiveqi
// |rrc   |srb   0           1    2    3   |drb  1          |SDAP
// |      |--------------------------------|--------------- |
// |rlc   |     TM           AM   AM   AM  |     UM/AM      |rlc
// |      |--------------------------------|--------------- |
// |mac   |lcid  0           1    2    3   |     4          |mac

int fill_cellgroup_with_radio_bearer_cfg_inner(rrc_nr_cfg_t *              cfg,
                                         uint32_t                  rnti,
                                         enb_bearer_manager        *bearer_mapper,
                                         struct radio_bearer_cfg_s *bearers,
                                         struct cell_group_cfg_s   *out)
{
	cvector_clear(out->rlc_bearer_to_add_mod_list);
	cvector_clear(out->rlc_bearer_to_release_list);

	struct srb_to_add_mod_s *srb = NULL;
	cvector_for_each_in(srb, bearers->srb_to_add_mod_list){
		// Add SRBs
		struct rlc_bearer_cfg_s bearer_to_add_out= {0};
		// rlc config
		fill_srb_inner(cfg, srb->srb_id, &bearer_to_add_out);
		cvector_push_back(out->rlc_bearer_to_add_mod_list, bearer_to_add_out);
	}
	// Add DRBs
	struct drb_to_add_mod_s *drb = NULL;
	cvector_for_each_in(drb, bearers->drb_to_add_mod_list){
		struct rlc_bearer_cfg_s drb_to_add_out= {0};
		uint32_t lcid        = drb->drb_id + ((nr_srb)count - 1);
		radio_bearer_t *rb   = get_lcid_bearer(bearer_mapper, rnti, lcid);
		rrc_nr_cfg_five_qi_t *cfg_five_qi = oset_hash_get(cfg->five_qi_cfg, rb->five_qi, sizeof(uint32_t));
		if (is_valid(rb) && NULL != cfg_five_qi) {
			// rlc config
	  		fill_drb_inner(cfg_five_qi, rb, (nr_drb)drb->drb_id, &drb_to_add_out);
		} else {
	 		return OSET_ERROR;
		}
		cvector_push_back(out->rlc_bearer_to_add_mod_list, drb_to_add_out);
	}

	// Release DRBs
	uint8_t *drb_id = NULL;
	cvector_for_each_in(drb_id, bearers->drb_to_release_list){
		cvector_push_back(out->rlc_bearer_to_release_list, *drb_id)
	}

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

