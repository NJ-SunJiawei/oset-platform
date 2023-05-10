/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#include "gnb_common.h"
#include "gnb_config_parser.h"
#include "rrc/rrc_nr_config.h"
#include "phy/phy_nr_config.h"

void gnb_arg_default(all_args_t      *args)
{
    //enb.conf
	args->enb.enb_id = 0x0; //gnb_manager_self()->config.nr_stack.ngap.gnb_id=0x0;
	args->enb.n_prb = 50;//set 4g  5g=52
	args->enb.nof_ports = 1;
	args->enb.transmission_mode = 1;
	args->enb.p_a = 0;//Power allocation rho_a (-6, -4.77, -3, -1.77, 0, 1, 2, 3)
	args->enb_files.sib_config = "sib.conf";
	args->enb_files.rr_config = "rr.conf";
	args->enb_files.rb_config = "rb.conf";
	args->enb.dl_earfcn = 0;
	args->rf.srate_hz = 0;
	args->rf.rx_gain = 50;
	args->rf.tx_gain = 80;
	args->rf.tx_gain_ch[0] = -1;
	args->rf.tx_gain_ch[1] = -1;
	args->rf.tx_gain_ch[2] = -1;
	args->rf.tx_gain_ch[3] = -1;
	args->rf.tx_gain_ch[4] = -1;
	args->rf.dl_freq = -1;
	args->rf.ul_freq = -1;
	args->rf.device_name = "zmq";
	args->rf.device_args = "fail_on_disconnect=true,tx_port=tcp://*:2000,rx_port=tcp://localhost:2001,id=enb,base_srate=11.52e6";
	args->rf.time_adv_nsamples = "auto";
    /* Downlink Channel emulator section */
	args->phy.dl_channel_args.enable = false;
	args->phy.dl_channel_args.awgn_enable = false;
	args->phy.dl_channel_args.awgn_signal_power_dBfs = 0;
	args->phy.dl_channel_args.awgn_snr_dB = 30;
	args->phy.dl_channel_args.fading_enable = false;
	args->phy.dl_channel_args.fading_model = "none"
	args->phy.dl_channel_args.delay_enable = false;
	args->phy.dl_channel_args.delay_min_us = 10;
	args->phy.dl_channel_args.delay_max_us = 100;
	args->phy.dl_channel_args.delay_period_s = 3600;
	args->phy.dl_channel_args.delay_init_time_s = 0;
	args->phy.dl_channel_args.rlf_enable = false;
	args->phy.dl_channel_args.rlf_t_on_ms = 10000;
	args->phy.dl_channel_args.rlf_t_off_ms = 2000;
	args->phy.dl_channel_args.hst_enable = false;
	args->phy.dl_channel_args.hst_period_s = 7.2;
	args->phy.dl_channel_args.hst_fd_hz = 750;
	args->phy.dl_channel_args.hst_init_time_s =0;
	args->phy.ul_channel_args.enable = false;
	args->phy.ul_channel_args.awgn_enable = false;
	args->phy.ul_channel_args.awgn_signal_power_dBfs = 30;
	args->phy.ul_channel_args.awgn_snr_dB = 30;
	args->phy.ul_channel_args.fading_enable = false;
	args->phy.ul_channel_args.fading_model = "none";
	args->phy.ul_channel_args.delay_enable = false;
	args->phy.ul_channel_args.delay_period_s = 3600;
	args->phy.ul_channel_args.delay_init_time_s = 0;
	args->phy.ul_channel_args.delay_max_us = 100;
	args->phy.ul_channel_args.delay_min_us = 10;
	args->phy.ul_channel_args.rlf_enable = false;
	args->phy.ul_channel_args.rlf_t_on_ms = 10000;
	args->phy.ul_channel_args.rlf_t_off_ms = 2000;
	args->phy.ul_channel_args.hst_enable = false;
	args->phy.ul_channel_args.hst_period_s = 7.2;
	args->phy.ul_channel_args.hst_fd_hz = 750;
	args->phy.ul_channel_args.hst_init_time_s = 0;
    /* CFR section */
	args->phy.cfr_args.enable = false;
	args->phy.cfr_args.enable = "manual";//SRSRAN_CFR_THR_MANUAL
	args->phy.cfr_args.manual_thres = 0.5;
	args->phy.cfr_args.strength = 1;
	args->phy.cfr_args.auto_target_papr = 8;
	args->phy.cfr_args.ema_alpha = 0.0143;//1.0f / (float)SRSRAN_CP_NORM_NSYMB;

	/* Expert section */
	args->general.metrics_period_secs = 1.0;
	args->general.metrics_csv_enable = false;
	args->general.metrics_csv_filename = "/tmp/enb_metrics.csv";
	//args->phy.pusch_max_its = 8;
	args->phy.pusch_8bit_decoder = false;
	args->phy.pusch_meas_evm = false;
	args->phy.tx_amplitude = 1;
	args->phy.nof_phy_threads = 3;//1
	args->phy.nof_prach_threads = 1;
	args->phy.max_prach_offset_us = 30;
	args->phy.equalizer_mode = "mmse";
	args->phy.estimator_fil_w = 0.1;
	args->general.report_json_enable = false;
	args->general.report_json_filename = "/tmp/enb_report.json";
	args->general.report_json_asn1_oct = false;
	//args->general.alarms_log_enable = false;
	//args->general.alarms_filename = "/tmp/enb_alarms.log";
	args->general.tracing_enable = false;
	args->general.tracing_filename = "/tmp/enb_tracing.log";
	args->general.tracing_buffcapacity = 1000000;
	args->general.rrc_inactivity_timer = 30000;
	//args->general.print_buffer_state = false;
	args->general.eea_pref_list = "EEA0, EEA2, EEA1";
	args->general.eia_pref_list = "EIA2, EIA1, EIA0";
	args->general.max_mac_dl_kos = 100;
	args->general.max_mac_ul_kos = 100;
	args->general.rlf_release_timer_ms = 4000;
	args->phy.extended_cp = false;
	args->phy.rx_gain_offset = 62;

	/* stack section */
	args->nr_stack.mac_nr.sched_cfg.fixed_dl_mcs = 28;
	args->nr_stack.mac_nr.sched_cfg.fixed_ul_mcs = 28;
	args->nr_stack.mac_nr.sched_cfg.pdsch_enabled = true;
	args->nr_stack.mac_nr.sched_cfg.pusch_enabled = true;
	args->nr_stack.mac_nr.sched_cfg.auto_refill_buffer = false;
	args->nr_stack.mac_nr.pcap.enable = false; 
	args->nr_stack.mac_nr.pcap.filename = "/tmp/enb_mac_nr.pcap";
	args->nr_stack.ngap_pcap.enable = false;
	args->nr_stack.ngap_pcap.filename = "/tmp/enb_ngap.pcap";
    args->nr_stack.ngap.gnb_id = args->enb.enb_id;
	args->nr_stack.ngap.gnb_name = "app-gnb";
	args->nr_stack.ngap.cell_id = 0x0;
    args->nr_stack.ngap.tac= 0x0;
    string_to_mcc("001", &args->nr_stack.ngap.mcc);
    string_to_mnc("01", &args->nr_stack.ngap.mnc);
    args->nr_stack.ngap.amf_addr = "127.0.0.2";
    args->nr_stack.ngap.ngc_bind_addr = "127.0.1.1";
    args->nr_stack.ngap.gtp_bind_addr = "127.0.1.1";

	srsran_use_standard_symbol_size(false);
}

void gnb_arg_second(all_args_t      *args)
{
   /*todo Parse enb.conf*/
}


static int parse_cell_cfg(all_args_t* args_, srsran_cell_t* cell)
{
	cell->frame_type = SRSRAN_FDD;
	cell->cp		 = args_->phy.extended_cp ? SRSRAN_CP_EXT : SRSRAN_CP_NORM;
	cell->nof_ports  = args_->enb.nof_ports;
	cell->nof_prb	 = args_->enb.n_prb;
	// PCI not configured yet
	// phich no need
    return OSET_OK;
}

static bool sib_is_present(struct sib_type1_lte_s* sib1, enum sib_type_opts sib_num)
{
  for (uint32_t i = 0; i < sib1.nof_sched_info; i++) {
    for (uint32_t j = 0; j < sib1.sched_info_list[i].nof_sib_map; j++) {
      if (sib1.sched_info_list[i].sib_map_info[j] == sib_num) {
        return true;
      }
    }
  }
  return false;
}

static void parse_sib3(char* filename, struct sib_type3_lte_s* data)
{
	// CellReselectionInfoCommon
	data->cell_resel_info_common.q_hyst = (enum q_hyst_opts)db2 ;//db2
	data->cell_resel_info_common.speed_state_resel_pars_present = false;
    //struct speed_state_resel_pars_s_* resel_pars = &data->cell_resel_info_common.speed_state_resel_pars;

	// CellReselectionServingFreqInfo
	struct cell_resel_serving_freq_info_s_* freqinfo = &data->cell_resel_serving_freq_info;
	freqinfo->s_non_intra_search_present = true;
	freqinfo->s_non_intra_search = 3;
	freqinfo->thresh_serving_low = 2;
	freqinfo->cell_resel_prio = 6;
	
    // intraFreqCellReselectionInfo	
	struct intra_freq_cell_resel_info_s_* intrafreq = &data->intra_freq_cell_resel_info;
	intrafreq->q_rx_lev_min = -61;
	intrafreq->p_max_present = true;
	intrafreq->p_max  = 23;
	intrafreq->s_intra_search_present = true;
	intrafreq->s_intra_search = 5;
	intrafreq->presence_ant_port1 = true;
	intrafreq->neigh_cell_cfg = 1;	
	intrafreq->t_resel_eutra = 1;
}

static void parse_sib2(char* filename, struct sib_type2_lte_s* data)
{
	data->time_align_timer_common = infinity;
	data->mbsfn_sf_cfg_list_present = false;

	data->freq_info.add_spec_emission = 1;
	data->freq_info.ul_carrier_freq_present = true;
	data->freq_info.ul_bw_present = true;

	// AC barring configuration
	data->ac_barr_info_present = false;

	// UE timers and constants
	data->ue_timers_and_consts.t300 = ms2000;
	data->ue_timers_and_consts.t301 = ms100;
	data->ue_timers_and_consts.t310 = ms200;
	data->ue_timers_and_consts.n310 = (enum n310_opts)n1;//n1
	data->ue_timers_and_consts.t311 = ms10000;
	data->ue_timers_and_consts.n311 = (enum n311_opts)n1;//n1

	// Radio-resource configuration section
	struct rr_cfg_common_sib_s* rr_cfg_common = &data->rr_cfg_common;
	rr_cfg_common->ul_cp_len = len1;
	// RACH configuration
	rr_cfg_common->rach_cfg_common.preamb_info.nof_ra_preambs = n52;
	rr_cfg_common->rach_cfg_common.pwr_ramp_params.preamb_init_rx_target_pwr = dbm_minus104;
	rr_cfg_common->rach_cfg_common.pwr_ramp_params.pwr_ramp_step = db6;
	rr_cfg_common->rach_cfg_common.ra_supervision_info.preamb_trans_max = (enum preamb_trans_max_opts)n10;//n10
	rr_cfg_common->rach_cfg_common.ra_supervision_info.ra_resp_win_size = sf10;
	rr_cfg_common->rach_cfg_common.ra_supervision_info.mac_contention_resolution_timer = sf64;
	rr_cfg_common->rach_cfg_common.max_harq_msg3_tx = 4;
	rr_cfg_common->rach_cfg_common.preamb_info.preambs_group_a_cfg_present = false;

	// BCCH configuration
	rr_cfg_common->bcch_cfg.mod_period_coeff = (enum mod_period_coeff_opts)n16;//n16;

	// PCCH configuration
	rr_cfg_common->pcch_cfg.default_paging_cycle = rf32;
	rr_cfg_common->pcch_cfg.nb = one_t;

	// PRACH configuration
	rr_cfg_common->prach_cfg.root_seq_idx = 128;
	rr_cfg_common->prach_cfg.prach_cfg_info.high_speed_flag = false;
	rr_cfg_common->prach_cfg.prach_cfg_info.prach_cfg_idx = 3;
	rr_cfg_common->prach_cfg.prach_cfg_info.prach_freq_offset = 4;
	rr_cfg_common->prach_cfg.prach_cfg_info.zero_correlation_zone_cfg = 5;

	// PDSCH configuration
	rr_cfg_common->pdsch_cfg_common.p_b = 1;
	rr_cfg_common->pdsch_cfg_common.ref_sig_pwr = 0;

	// PUSCH configuration
	rr_cfg_common->pusch_cfg_common.pusch_cfg_basic.n_sb = 1;
	rr_cfg_common->pusch_cfg_common.pusch_cfg_basic.hop_mode = inter_sub_frame;
	rr_cfg_common->pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset = 2;
	rr_cfg_common->pusch_cfg_common.pusch_cfg_basic.enable64_qam = false;
	// PUSCH-ULRS configuration
	rr_cfg_common->pusch_cfg_common.ul_ref_sigs_pusch.cyclic_shift = 0;
	rr_cfg_common->pusch_cfg_common.ul_ref_sigs_pusch.group_assign_pusch = 0;
	rr_cfg_common->pusch_cfg_common.ul_ref_sigs_pusch.group_hop_enabled = false;
	rr_cfg_common->pusch_cfg_common.ul_ref_sigs_pusch.seq_hop_enabled = false;

	// PUCCH configuration
	rr_cfg_common->pucch_cfg_common.delta_pucch_shift = ds1;
	rr_cfg_common->pucch_cfg_common.nrb_cqi = 1;
	rr_cfg_common->pucch_cfg_common.ncs_an = 0;
	rr_cfg_common->pucch_cfg_common.n1_pucch_an = 12;

	// UL PWR Ctrl configuration
	rr_cfg_common->ul_pwr_ctrl_common.p0_nominal_pusch = -85;
	rr_cfg_common->ul_pwr_ctrl_common.alpha = al07;
	rr_cfg_common->ul_pwr_ctrl_common.p0_nominal_pucch = -107;
	rr_cfg_common->ul_pwr_ctrl_common.delta_preamb_msg3 =  6;
	// Delta Flist PUCCH
	rr_cfg_common->ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format1 = (enum delta_f_pucch_format1_opts)delta_f0;//delta_f0
	rr_cfg_common->ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format1b = (enum delta_f_pucch_format1b_opts)delta_f3;//delta_f3
	rr_cfg_common->ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2 = (enum delta_f_pucch_format2_opts)delta_f1;//delta_f1
	rr_cfg_common->ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2a = (enum delta_f_pucch_format2a_opts)delta_f2;//delta_f2
	rr_cfg_common->ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2b = (enum delta_f_pucch_format2b_opts)delta_f2;//delta_f2

}


static void parse_sib1(char* filename, struct sib_type1_lte_s* data)
{
    //ASN_RRC_SIB1_t
	/*data->cellSelectionInfo = oset_core_alloc(gnb_manager_self()->app_pool, sizeof(struct ASN_RRC_SIB1__cellSelectionInfo));
	data->cellSelectionInfo->q_RxLevMin = -65;
	data->si_SchedulingInfo= oset_core_alloc(gnb_manager_self()->app_pool, sizeof(ASN_RRC_SI_SchedulingInfo_t));
	data->si_SchedulingInfo.si_WindowLength = ASN_RRC_SI_SchedulingInfo__si_WindowLength_s20;
	
	struct ASN_RRC_SchedulingInfo *schedulingInfo = oset_core_alloc(gnb_manager_self()->app_pool, sizeof(ASN_RRC_SchedulingInfo_t));
	//schedulingInfo->si_BroadcastStatus = ASN_RRC_SchedulingInfo__si_BroadcastStatus_broadcasting;
	schedulingInfo->si_Periodicity = ASN_RRC_SchedulingInfo__si_Periodicity_rf16;

	asn_set_empty(&schedulingInfo->sib_MappingInfo.list);
	ASN_RRC_SIB_TypeInfo *sib_type3 = oset_core_alloc(gnb_manager_self()->app_pool, sizeof(ASN_RRC_SIB_TypeInfo_t));
	sib_type3->type = ASN_RRC_SIB_TypeInfo__type_sibType3;
	ASN_SEQUENCE_ADD(&schedulingInfo->sib_MappingInfo.list,sib_type3);*/
	
    data->cell_access_related_info.intra_freq_resel = (enum intra_freq_resel_opts)allowed;
	data->cell_sel_info.q_rx_lev_min = -65;
	data->cell_access_related_info.cell_barred = (enum cell_barred_opts)not_barred;
	data->si_win_len = (enum si_win_len_opts)ms20;
	data->sys_info_value_tag = 0;
	data->nof_sched_info = 1;//At least one
	data->sched_info_list[0].si_periodicity = (enum si_periodicity_r12_opts)rf16;//At least one
	data->sched_info_list[0].nof_sib_map = 1;
	data->sched_info_list[0].sib_map_info[0] = (enum sib_type_opts)sib_type2;//Optional
}

static int parse_sibs(all_args_t* args_, rrc_cfg_t* rrc_cfg_, phy_cfg_t* phy_config_common)
{
    /***********sib1****************/
	struct sib_type1_lte_s* sib1 = &rrc_cfg_->sib1;
	struct sib_type2_lte_s*	sib2 = &rrc_cfg_->sib2;
	struct sib_type3_lte_s*	sib3 = &rrc_cfg_->sib3;
	if (parse_sib1(args_->enb_files.sib_config, sib1) != OSET_OK) {
	  return OSET_ERROR;
	}

	struct cell_access_related_info_s_* cell_access = &sib1->cell_access_related_info;
    mcc_to_bytes(args_->nr_stack.ngap.mcc, &cell_access->plmn_id_list[0].plmn_id.mcc);
    mnc_to_bytes(args_->nr_stack.ngap.mnc, &cell_access->plmn_id_list[0].plmn_id.mnc,&cell_access->plmn_id_list[0].plmn_id.nof_mnc_digits);
	cell_access->plmn_id_list[0].cell_reserved_for_oper = (enum cell_reserved_for_oper_opts)not_reserved;
	sib1->cell_sel_info.q_rx_lev_min_offset 			= 0;

    /***********sib2****************/
	if (parse_sib2(args_->enb_files.sib_config, sib2) != OSET_OK) {
	  return OSET_ERROR;
	}
	sib2->rr_cfg_common.srs_ul_cfg_common.types = release;
	if (sib2->freq_info.ul_bw_present) {
	   sib2->freq_info.ul_bw = (enum ul_bw_opts)n50;//n50 //args_->enb.n_prb = 50
	}

	//if (not args_->nr_stack.embms.enable) {
	//  sib2->mbsfn_sf_cfg_list_present = false;
	//}

    /***********si****************/
	// Generate SIB3 if defined in mapping info
	if (sib_is_present(sib1,sib_type3)) {
	  if (parse_sib3(args_->enb_files.sib_config, sib3) != OSET_OK) {
		return OSET_ERROR;
	  }
	}

	// Copy PHY common configuration
	//phy_config_common->prach_cnfg  = sib2->rr_cfg_common.prach_cfg;
	//phy_config_common->pdsch_cnfg  = sib2->rr_cfg_common.pdsch_cfg_common;
	//phy_config_common->pusch_cnfg  = sib2->rr_cfg_common.pusch_cfg_common;
	//phy_config_common->pucch_cnfg  = sib2->rr_cfg_common.pucch_cfg_common;
	//phy_config_common->srs_ul_cnfg = sib2->rr_cfg_common.srs_ul_cfg_common;

	return OSET_OK;
}

static void make_default_coreset(uint8_t coreset_id, uint32_t nof_prb, struct ctrl_res_set_s *coreset)
{
  coreset->ctrl_res_set_id = coreset_id;
  // Generate frequency resources for the full BW
  memset(&coreset->freq_domain_res[0], 0, 6);
  for (uint32_t i = 0; i < SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE; i++) {
    bitstring_set(&coreset->freq_domain_res[0], SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE - i - 1, i < SRSRAN_FLOOR(nof_prb, 6));
  }
  coreset->dur = 1;
  coreset->cce_reg_map_type.types = non_interleaved;
  coreset->precoder_granularity = same_as_reg_bundle;
}

static uint32_t coreset_get_bw(struct ctrl_res_set_s* coreset)
{
  uint32_t prb_count = 0;

  // Iterate all the frequency domain resources bit-map...
  for (uint32_t i = 0; i < SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE; i++) {
    // ... and count 6 PRB for every frequency domain resource that it is enabled
    if (bitstring_get(&coreset->freq_domain_res[0],i)) {
      prb_count += 6;
    }
  }
  // Return the total count of physical resource blocks
  return prb_count;
}


static int coreset_get_pdcch_nr_max_candidates(struct ctrl_res_set_s* coreset, uint32_t aggregation_level)
{
  uint32_t coreset_bw = coreset_get_bw(coreset);
  uint32_t nof_cce    = (coreset_bw * coreset->dur) / 6;

  uint32_t L              = 1U << aggregation_level;
  uint32_t nof_candidates = nof_cce / L;

  return SRSRAN_MIN(nof_candidates, SRSRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR);
}

static void make_default_common_search_space(uint8_t ss_id, struct ctrl_res_set_s* cs, struct search_space_s *ss)
{
  ss->search_space_id                                = ss_id;
  ss->ctrl_res_set_id_present                        = true;
  ss->ctrl_res_set_id                                = cs->ctrl_res_set_id;
  ss->dur_present                                    = false; // false for duration=1
  ss->monitoring_slot_periodicity_and_offset_present = true;
  ss->monitoring_slot_periodicity_and_offset.types = sl1;
  ss->monitoring_symbols_within_slot_present = true;
  memset(&ss->monitoring_symbols_within_slot[0], 0, 2);
  bitstring_from_number(&ss->monitoring_symbols_within_slot[0], 0b10000000000000, 14);
  ss->search_space_type_present                                                = true;
  ss->search_space_type = (enum search_space_types_opts)common;
  ss->search_space_type.c.common.dci_format0_minus0_and_format1_minus0_present = true;
  ss->nrof_candidates_present                                                  = true;
  uint32_t nof_cand = SRSRAN_MIN(coreset_get_pdcch_nr_max_candidates(cs, 0), 2);
  ss->nrof_candidates.aggregation_level1 = (enum aggregation_level1_opts)nof_cand;
  nof_cand = SRSRAN_MIN(coreset_get_pdcch_nr_max_candidates(cs, 1), 2);
  ss->nrof_candidates.aggregation_level2 = (enum aggregation_level2_opts)nof_cand;
  nof_cand = SRSRAN_MIN(coreset_get_pdcch_nr_max_candidates(cs, 2), 2);
  ss->nrof_candidates.aggregation_level4 = (enum aggregation_level4_opts)nof_cand;
  nof_cand = SRSRAN_MIN(coreset_get_pdcch_nr_max_candidates(cs, 3), 2);
  ss->nrof_candidates.aggregation_level8 = (enum aggregation_level8_opts)nof_cand;
  nof_cand = SRSRAN_MIN(coreset_get_pdcch_nr_max_candidates(cs, 4), 2);
  ss->nrof_candidates.aggregation_level16 = (enum aggregation_level16_opts)nof_cand;
}


/// Generate default rrc nr cell configuration
static void generate_default_nr_cell(rrc_cell_cfg_nr_t* cell)
{
  cell->coreset0_idx            = 7;
  cell->ssb_absolute_freq_point = 0; // auto derived
  cell->num_ra_preambles        = 8;
  cell->phy_cell.carrier.scs    = srsran_subcarrier_spacing_15kHz;
  cell->phy_cell.carrier.nof_prb   = 52;
  cell->phy_cell.carrier.max_mimo_layers = 1;

  // PDCCH
  // - Add CORESET#2 as UE-specific
  //cell->pdcch_cfg_ded.nof_ctrl_res_set_to_add_mod = 1;
  struct ctrl_res_set_s coreset2 = {0};
  make_default_coreset(2, cell->phy_cell.carrier.nof_prb, &coreset2);
  cvector_push_back(cell->pdcch_cfg_ded.ctrl_res_set_to_add_mod_list, coreset2)

  // - Add SearchSpace#2 as UE-specific -> CORESET#2
  //cell->pdcch_cfg_ded.nof_search_spaces_to_add_mod = 1;
  struct search_space_s ss2 = {0};
  make_default_common_search_space(2, coreset2, &ss2);
  ss2->search_space_type.types = ue_specific;
  ss2->search_space_type.c.ue_spec.dci_formats = formats0_minus0_and_minus1_minus0;
  cvector_push_back(cell->pdcch_cfg_ded.search_spaces_to_add_mod_list, ss2)
}


int parse_rr(all_args_t* args_, rrc_cfg_t* rrc_cfg_, rrc_nr_cfg_t* rrc_nr_cfg_)
{
	/* Transmission mode config section */
	if (args_->enb.transmission_mode < 1 || args_->enb.transmission_mode > 4) {
	  oset_error("Invalid transmission mode (%d). Only indexes 1-4 are implemented.", args_->enb.transmission_mode);
	  return OSET_ERROR;
	}
	if (args_->enb.transmission_mode == 1 && args_->enb.nof_ports > 1) {
	  oset_error("Invalid number of ports (%d) for transmission mode (%d). Only one antenna port is allowed.",
			      args_->enb.nof_ports,
			      args_->enb.transmission_mode);
	  return OSET_ERROR;
	}
	if (args_->enb.transmission_mode > 1 && args_->enb.nof_ports != 2) {
	  oset_error("The selected number of ports (%d) are insufficient for the selected transmission mode (%d).",
			     args_->enb.nof_ports,
			     args_->enb.transmission_mode);
	  return OSET_ERROR;
	}

	rrc_cfg_->antenna_info.tx_mode = (enum tx_mode_opts)(args_->enb.transmission_mode - 1);
	switch (rrc_cfg_->antenna_info.tx_mode) {
	  case tm1:
	  case tm2:
		rrc_cfg_->antenna_info.ue_tx_ant_sel.type = (enum setup_e)release;//release
		rrc_cfg_->antenna_info.codebook_subset_restrict_present = false;
		break;
	  case tm3:
		rrc_cfg_->antenna_info.ue_tx_ant_sel.type = (enum setup_e)setup;//setup
		rrc_cfg_->antenna_info.ue_tx_ant_sel.c = (enum ue_tx_ant_sel_setup_opts)open_loop;//open_loop
	
		rrc_cfg_->antenna_info.codebook_subset_restrict_present = true;
		rrc_cfg_->antenna_info.codebook_subset_restrict.types = n2_tx_ant_tm3;
		rrc_cfg_->antenna_info.codebook_subset_restrict.c = 0b11;
		break;
	  case tm4:
		rrc_cfg_->antenna_info.ue_tx_ant_sel.type = (enum setup_e)setup;//setup
		rrc_cfg_->antenna_info.ue_tx_ant_sel.c = (enum ue_tx_ant_sel_setup_opts)closed_loop;//closed_loop

		rrc_cfg_->antenna_info.codebook_subset_restrict_present = true;
		rrc_cfg_->antenna_info.codebook_subset_restrict.types = n2_tx_ant_tm4;
		rrc_cfg_->antenna_info.codebook_subset_restrict.c = 0b111111;
		break;
	  default:
		oset_error("Unsupported transmission mode %d", rrc_cfg_->antenna_info.tx_mode);
		return OSET_ERROR;
	}

	rrc_cfg_->pdsch_cfg = db_minus6; //args_->enb.p_a = 0

	/* MAC config section */
	rrc_cfg_->mac_cnfg.phr_cfg.types = (enum setup_e)release;//release // default is release if "phr_cnfg" is not found

	rrc_cfg_->mac_cnfg.ul_sch_cfg.tti_bundling = false;
	rrc_cfg_->mac_cnfg.ul_sch_cfg.max_harq_tx_present = true;
	rrc_cfg_->mac_cnfg.ul_sch_cfg.max_harq_tx = 4;
	rrc_cfg_->mac_cnfg.ul_sch_cfg.periodic_bsr_timer_present = true;
	rrc_cfg_->mac_cnfg.ul_sch_cfg.periodic_bsr_timer = (enum periodic_bsr_timer_r12_opts)sf20;//sf20
    rrc_cfg_->mac_cnfg.ul_sch_cfg.retx_bsr_timer = (enum retx_bsr_timer_r12_opts)sf320;//sf320

	rrc_cfg_->mac_cnfg.time_align_timer_ded = (enum time_align_timer_opts)infinity;//infinity

	/* PHY config section */
	//pusch_cnfg_ded
	rrc_cfg_->pusch_cfg.beta_offset_ack_idx = 6;
	rrc_cfg_->pusch_cfg.beta_offset_ri_idx = 6;
	rrc_cfg_->pusch_cfg.beta_offset_cqi_idx = 6;

    //sched_request_cnfg
	rrc_cfg_->sr_cfg.dsr_max = (enum dsr_trans_max_opts)n64;//n64
	rrc_cfg_->sr_cfg.period = 20;
	rrc_cfg_->sr_cfg.nof_prb = 1;

	//cqi_report_cnfg
	rrc_cfg_->cqi_cfg.mode = RRC_CFG_CQI_MODE_PERIODIC;//periodic
	rrc_cfg_->cqi_cfg.period = 40;
	rrc_cfg_->cqi_cfg.m_ri = 8;//RI period in CQI period
	rrc_cfg_->cqi_cfg.simultaneousAckCQI = true;

	// NR RRC and cell config section(list)
    for(int i =0 ; i < 1; ++i){
		rrc_cell_cfg_nr_t cell_cfg = {0};
		cell_cfg.cell_idx = i;
		generate_default_nr_cell(&cell_cfg);
	    cell_cfg.phy_cell.rf_port = 0;
		cell_cfg.phy_cell.carrier.pci = 500;
		cell_cfg.phy_cell.carrier.pci = cell_cfg.phy_cell.carrier.pci % SRSRAN_NOF_NID_NR;

		cell_cfg.phy_cell.cell_id = 2;
		cell_cfg.coreset0_idx = 6;
		cell_cfg.prach_root_seq_idx = 1;
		cell_cfg.tac = 7;

		cell_cfg.dl_arfcn = 368500;
		//cell_cfg.ul_arfcn = 0;
		cell_cfg.band = 3;
		cvector_push_back(rrc_nr_cfg_->cell_list, cell_cfg);
	}

    /*todo */
	// Configuration check

    return OSET_OK;	
}

static void parse_srb(struct rlc_cfg_c* rlc_cfg)
{
	rlc_cfg->types = (enum rlc_types_opts)am;
	struct ul_am_rlc_s* ul_am_rlc = &rlc_cfg->c.am.ul_am_rlc;
	ul_am_rlc->sn_field_len = (enum sn_field_len_am_opts)size12;//size12
	ul_am_rlc->sn_field_len_present = true;
	ul_am_rlc->t_poll_retx = (enum t_poll_retx_opts)ms45;//ms45
	ul_am_rlc->poll_pdu = pinfinity;
	ul_am_rlc->poll_byte = kbinfinity;
	ul_am_rlc->max_retx_thres = (enum max_retx_thres_opts)t8;//t8

	struct dl_am_rlc_s* dl_am_rlc = &rlc_cfg->c.am.dl_am_rlc;
	dl_am_rlc->sn_field_len = (enum sn_field_len_am_opts)size12;//size12
	dl_am_rlc->sn_field_len_present = true;
	dl_am_rlc->t_reassembly = (enum t_reassembly_opts)ms35;//ms35
	dl_am_rlc.t_status_prohibit = (enum t_status_prohibit_opts)ms10;//ms10
}

static void parse_5qi(oset_list2_t     *five_qi_cfg_list)
{
    /********************7**************************/
	rrc_nr_cfg_five_qi_t *five_qi_cfg1 = oset_core_alloc(gnb_manager_self()->app_pool, sizeof(rrc_nr_cfg_five_qi_t));
	five_qi_cfg1->five_qi = 7;

	//pdcp_nr_config
	struct pdcp_cfg_s* pdcp_cfg = &five_qi_cfg1->pdcp_cfg;
	pdcp_cfg->drb_present = true;
	struct drb_s_ *drb_cfg = &pdcp_cfg->drb
	drb_cfg->pdcp_sn_size_ul = (enum pdcp_sn_size_ul_opts)len18bits;//len18bits
	drb_cfg->pdcp_sn_size_ul_present = true;
	drb_cfg->pdcp_sn_size_dl = (enum pdcp_sn_size_dl_opts)len18bits//len18bits
	drb_cfg->pdcp_sn_size_dl_present = true;
	drb_cfg->discard_timer = (enum discard_timer_opts)ms50;//ms50
	drb_cfg->discard_timer_present = true;
	drb_cfg->status_report_required_present = false;
	drb_cfg->integrity_protection_present = false;
	drb_cfg->hdr_compress.types = not_used;
	pdcp_cfg->t_reordering = (enum t_reordering_opts)ms50;//ms50
	pdcp_cfg->t_reordering_present = true;

	//rlc_config
	struct rlc_cfg_c* rlc_cfg = &five_qi_cfg1->rlc_cfg;
	rlc_cfg->types = um_bi_dir;
	struct ul_um_rlc_s *ul_um_cfg = &rlc_cfg->c.um_bi_dir.ul_um_rlc;
	struct dl_um_rlc_s *dl_um_cfg = &rlc_cfg->c.um_bi_dir.dl_um_rlc;
	// RLC UM UL
	ul_um_cfg->sn_field_len_present = true;
	ul_um_cfg->sn_field_len = (enum sn_field_len_um_opts)size12;//size12
	// RLC UM DL
	dl_um_cfg->sn_field_len_present = true;
	dl_um_cfg->sn_field_len = (enum sn_field_len_um_opts)size12;//size12;
	dl_um_cfg->t_reassembly = (enum t_reassembly_opts)ms50;//ms50
	oset_list2_add(five_qi_cfg_list, five_qi_cfg1);

    /********************9**************************/
	rrc_nr_cfg_five_qi_t *five_qi_cfg2 = oset_core_alloc(gnb_manager_self()->app_pool, sizeof(rrc_nr_cfg_five_qi_t));
	five_qi_cfg2->five_qi = 9;

	//pdcp_nr_config
	struct pdcp_cfg_s* pdcp_cfg = &five_qi_cfg2->pdcp_cfg;
	pdcp_cfg->drb_present = true;
	struct drb_s_ *drb_cfg = &pdcp_cfg->drb
	drb_cfg->pdcp_sn_size_ul = (enum pdcp_sn_size_ul_opts)len18bits;//len18bits
	drb_cfg->pdcp_sn_size_ul_present = true;
	drb_cfg->pdcp_sn_size_dl = (enum pdcp_sn_size_dl_opts)len18bits//len18bits
	drb_cfg->pdcp_sn_size_dl_present = true;
	drb_cfg->discard_timer = (enum discard_timer_opts)ms50;//ms50
	drb_cfg->discard_timer_present = true;
	drb_cfg->status_report_required_present = false;
	drb_cfg->integrity_protection_present = false;
	drb_cfg->hdr_compress.types = not_used;
	pdcp_cfg->t_reordering = (enum t_reordering_opts)ms50;//ms50
	pdcp_cfg->t_reordering_present = true;

	//rlc_config
	struct rlc_cfg_c* rlc_cfg = &five_qi_cfg2->rlc_cfg;
	rlc_cfg->types = am;
	struct ul_am_rlc_s* ul_am_rlc = &rlc_cfg->c.am.ul_am_rlc;
	ul_am_rlc->sn_field_len = (enum sn_field_len_am_opts)size12;//size12
	ul_am_rlc->sn_field_len_present = true;
	ul_am_rlc->t_poll_retx = (enum t_poll_retx_opts)ms45;//ms45
	ul_am_rlc->poll_pdu = (enum poll_pdu_opts)p4;//p4
	ul_am_rlc->poll_byte = (enum poll_byte_opts)kb3000;//kb3000
	ul_am_rlc->max_retx_thres = (enum max_retx_thres_opts)t4;//t4

	struct dl_am_rlc_s* dl_am_rlc = &rlc_cfg->c.am.dl_am_rlc;
	dl_am_rlc->sn_field_len = (enum sn_field_len_am_opts)size12;//size12
	dl_am_rlc->sn_field_len_present = true;
	dl_am_rlc->t_reassembly = (enum t_reassembly_opts)ms50;//ms50
	dl_am_rlc.t_status_prohibit = (enum t_status_prohibit_opts)ms50;//ms50
	oset_list2_add(five_qi_cfg_list, five_qi_cfg2);
}

static int parse_rb(all_args_t* args_, rrc_nr_cfg_t* rrc_nr_cfg_)
{
	//srb1_5g_config
    //rlc_config
	parse_srb(&rrc_nr_cfg_->srb1_cfg);
	rrc_nr_cfg_->srb1_cfg.present = true;

	//srb2_5g_config
    //rlc_config
	parse_srb(&rrc_nr_cfg_->srb2_cfg);
	rrc_nr_cfg_->srb2_cfg.present = true;

    //five_qi_config
	rrc_nr_cfg_->five_qi_cfg = oset_list2_create();
	parse_5qi(rrc_nr_cfg_->five_qi_cfg);
   return OSET_OK; 
}

static int derive_phy_cell_freq_params(band_helper_t *band_helper, uint32_t dl_arfcn, uint32_t ul_arfcn, phy_cell_cfg_nr_t* phy_cell)
{
  // Verify essential parameters are specified and valid
  ERROR_IF_NOT(dl_arfcn > 0, "DL ARFCN is a mandatory parameter");

  // derive DL freq from ARFCN
  if (phy_cell->carrier.dl_center_frequency_hz == 0) {
    phy_cell->carrier.dl_center_frequency_hz = nr_arfcn_to_freq_2c(band_helper, dl_arfcn);
  }

  // derive UL freq from ARFCN
  if (phy_cell->carrier.ul_center_frequency_hz == 0) {
    // auto-detect UL frequency
    if (ul_arfcn == 0) {
      // derive UL ARFCN from given DL ARFCN
      ul_arfcn = get_ul_arfcn_from_dl_arfcn_2c(band_helper, dl_arfcn);
      ERROR_IF_NOT(ul_arfcn > 0, "Can't derive UL ARFCN from DL ARFCN %d", dl_arfcn);
    }
    phy_cell->carrier.ul_center_frequency_hz = nr_arfcn_to_freq_2c(band_helper, ul_arfcn);
  }
  return OSET_OK;
}

static int derive_ssb_params(bool                        is_sa,
                      uint32_t                    dl_arfcn,
                      uint32_t                    band,
                      srsran_subcarrier_spacing_t pdcch_scs,
                      uint32_t                    coreset0_idx,
                      uint32_t                    nof_prb,
                      rrc_cell_cfg_nr_t           *cell)
{
  // Verify essential parameters are specified and valid
  ERROR_IF_NOT(dl_arfcn > 0, "Invalid DL ARFCN=%d", dl_arfcn);
  ERROR_IF_NOT(band > 0, "Band is a mandatory parameter");
  ERROR_IF_NOT(pdcch_scs < srsran_subcarrier_spacing_invalid, "Invalid carrier SCS");
  ERROR_IF_NOT(coreset0_idx < 15, "Invalid controlResourceSetZero");
  ERROR_IF_NOT(nof_prb > 0, "Invalid DL number of PRBS=%d", nof_prb);

  band_helper_t *band_helper = gnb_manager_self()->band_helper;

  // 下行中心频率
  double   dl_freq_hz               = nr_arfcn_to_freq_2c(band_helper, dl_arfcn);
  // 下行pointA
  uint32_t dl_absolute_freq_point_a = get_abs_freq_point_a_arfcn_2c(band_helper, nof_prb, dl_arfcn);

  // derive SSB pattern and scs
  // 根据band和scs查表获取ssb参数 nr_band_ss_raster_table
  cell->ssb_pattern = get_ssb_pattern_2c(band_helper, band, srsran_subcarrier_spacing_15kHz);
  if (cell->ssb_pattern == SRSRAN_SSB_PATTERN_A) {
    // 15kHz SSB SCS
    cell->ssb_scs = srsran_subcarrier_spacing_15kHz;
  } else {
    // try to optain SSB pattern for same band with 30kHz SCS
    cell->ssb_pattern = get_ssb_pattern_2c(band_helper, band, srsran_subcarrier_spacing_30kHz);
    if (cell->ssb_pattern == SRSRAN_SSB_PATTERN_B || cell->ssb_pattern == SRSRAN_SSB_PATTERN_C) {
      // SSB SCS is 30 kHz
      cell->ssb_scs = srsran_subcarrier_spacing_30kHz;
    } else {
      oset_error("Can't derive SSB pattern from band %d", band);
	  oset_abort(); 
    }
  }

  // derive SSB position
  int coreset0_rb_offset = 0;
  if (is_sa) {
    // Get offset in RBs between CORESET#0 and SSB
    // 根据coreset_id 查表coreset_zero_15_15获取 offset (ssb和coreset偏移量 = kssb+offset)
    coreset0_rb_offset = srsran_coreset0_ssb_offset(coreset0_idx, cell->ssb_scs, pdcch_scs);
    ERROR_IF_NOT(coreset0_rb_offset >= 0, "Failed to compute RB offset between CORESET#0 and SSB");
  } else {
    // TODO: Verify if specified SSB frequency is valid
  }

  // 根据offset和pointA和scs和同步栅格，可以推算出ssb中心频点(全局栅格)
  uint32_t ssb_abs_freq_point = get_abs_freq_ssb_arfcn_2c(band_helper, band, cell->ssb_scs, dl_absolute_freq_point_a, coreset0_rb_offset);
  ERROR_IF_NOT(ssb_abs_freq_point > 0,
               "Can't derive SSB freq point for dl_arfcn=%d and band %d",
               freq_to_nr_arfcn_2c(band_helper, dl_freq_hz),
               band);

  // Convert to frequency for PHY
  // ssb中心频点
  cell->ssb_absolute_freq_point = ssb_abs_freq_point;
  // ssb中心频率
  cell->ssb_freq_hz             = nr_arfcn_to_freq_2c(band_helper, ssb_abs_freq_point);

  
  // pointA频率
  double   pointA_abs_freq_Hz = dl_freq_hz - nof_prb * SRSRAN_NRE * SRSRAN_SUBC_SPACING_NR(pdcch_scs) / 2;

  //ssb中心频率和ponitA之间的频率偏移量
  uint32_t ssb_pointA_freq_offset_Hz =
      (cell->ssb_freq_hz > pointA_abs_freq_Hz) ? (uint32_t)(cell->ssb_freq_hz - pointA_abs_freq_Hz) : 0;

  // 通过频率偏移量取余(ssb_pointA_freq_offset_Hz)可以算出子载波的偏移量  =Kssb
  cell->ssb_offset = (uint32_t)(ssb_pointA_freq_offset_Hz / SRSRAN_SUBC_SPACING_NR(pdcch_scs)) % SRSRAN_NRE;

  // Validate Coreset0 has space
  srsran_coreset_t coreset0 = {0};
  ERROR_IF_NOT(
      srsran_coreset_zero(
          cell->phy_cell.cell_id, ssb_pointA_freq_offset_Hz, cell->ssb_scs, pdcch_scs, coreset0_idx, &coreset0) == 0,
      "Deriving parameters for coreset0: index=%d, ssb_pointA_offset=%d kHz\n",
      coreset0_idx,
      ssb_pointA_freq_offset_Hz / 1000);

  ERROR_IF_NOT(srsran_coreset_start_rb(&coreset0) + srsran_coreset_get_bw(&coreset0) <= cell->phy_cell.carrier.nof_prb,
               "Coreset0 index=%d is not compatible with DL ARFCN %d in band %d\n",
               coreset0_idx,
               cell->dl_arfcn,
               cell->band);

  // Validate Coreset0 has less than 3 symbols
  ERROR_IF_NOT(coreset0.duration < 3,
               "Coreset0 index=%d is not supported due to overlap with SSB. Select a coreset0 index from 38.213 Table "
               "13-1 such that N_symb_coreset < 3\n",
               coreset0_idx);

  // Validate Coreset0 has more than 24 RB
  ERROR_IF_NOT(srsran_coreset_get_bw(&coreset0) > 24,
               "Coreset0 configuration index=%d has only %d RB. A coreset0 index >= 6 is required such as N_rb >= 48\n",
               srsran_coreset_get_bw(&coreset0),
               coreset0_idx);

  return OSET_OK;
}


static int set_derived_nr_cell_params(bool is_sa, rrc_cell_cfg_nr_t *cell)
{
  // Verify essential parameters are specified and valid
  ERROR_IF_NOT(cell->dl_arfcn > 0, "DL ARFCN is a mandatory parameter");
  ERROR_IF_NOT(cell->band > 0, "Band is a mandatory parameter");
  ERROR_IF_NOT(cell->phy_cell.carrier.nof_prb > 0, "Number of PRBs is a mandatory parameter");

  // Use helper class to derive NR carrier parameters
  band_helper_t *band_helper = gnb_manager_self()->band_helper;

  if (cell->ul_arfcn == 0) {
    // derive UL ARFCN from given DL ARFCN
    // 中心频点
    cell->ul_arfcn = get_ul_arfcn_from_dl_arfcn_2c(band_helper, cell.dl_arfcn);
    ERROR_IF_NOT(cell->ul_arfcn > 0, "Can't derive UL ARFCN from DL ARFCN %d", cell.dl_arfcn);
  }

  // duplex mode
  cell->duplex_mode = get_duplex_mode_2c(band_helper, cell->band);

  // PointA
  // 根据中心频点推算pointA = (中心频点-prb/2/15khz*12)
  cell->dl_absolute_freq_point_a = get_abs_freq_point_a_arfcn_2c(band_helper, cell->phy_cell.carrier.nof_prb, cell->dl_arfcn);
  cell->ul_absolute_freq_point_a = get_abs_freq_point_a_arfcn_2c(band_helper, cell->phy_cell.carrier.nof_prb, cell->ul_arfcn);

  // Derive phy_cell parameters that depend on ARFCNs
  derive_phy_cell_freq_params(band_helper, cell->dl_arfcn, cell->ul_arfcn, cell->phy_cell);

  // Derive SSB params
  // 计算ssb和coreset时频位置
  oset_expect_or_return_val(derive_ssb_params(is_sa,
                                 cell->dl_arfcn,
                                 cell->band,
                                 cell->phy_cell.carrier.scs,
                                 cell->coreset0_idx,
                                 cell->phy_cell.carrier.nof_prb,
                                 cell) == 0, OSET_ERROR);

  cell->phy_cell.carrier.ssb_center_freq_hz = cell->ssb_freq_hz;

  // Derive remaining config params
  if (!is_sa) {
    // Configure CORESET#1
    cell->pdcch_cfg_common.common_ctrl_res_set_present = true;
    make_default_coreset(1, cell->phy_cell.carrier.nof_prb, &cell->pdcch_cfg_common.common_ctrl_res_set);
  }

  // Configure SearchSpace#1
  //cell->pdcch_cfg_common.nof_common_search_space = 1;
  struct search_space_s ss1 = {0};

  if (is_sa) {
    // Configure SearchSpace#1 -> CORESET#0
    struct ctrl_res_set_s dummy_coreset = {0};//?????????
    make_default_coreset(0, cell->phy_cell.carrier.nof_prb , &dummy_coreset);
    make_default_common_search_space(1, &dummy_coreset, &ss1);
    ss1.nrof_candidates.aggregation_level1  = (enum aggregation_level1_opts)n0//n0;
    ss1.nrof_candidates.aggregation_level2  = (enum aggregation_level2_opts)n0//n0;
    ss1.nrof_candidates.aggregation_level4  = (enum aggregation_level4_opts)n1//n1;
    ss1.nrof_candidates.aggregation_level8  = (enum aggregation_level8_opts)n0//n0;
    ss1.nrof_candidates.aggregation_level16 = (enum aggregation_level16_opts)n0//n0;

  } else {
    // Configure SearchSpace#1 -> CORESET#1
    make_default_common_search_space(1, &cell->pdcch_cfg_common.common_ctrl_res_set, &ss1);
    //cell.phy_cell.pdcch.search_space[1].type       = srsran_search_space_type_common_3;
  }
  cell->pdcch_cfg_common.ra_search_space_present = true;
  cell->pdcch_cfg_common.ra_search_space         = ss1.search_space_id;

  cvector_push_back(cell->pdcch_cfg_common.common_search_space_list, ss1);
  return OSET_OK;
}


static int set_derived_nr_rrc_params(rrc_nr_cfg_t* rrc_cfg)
{
	rrc_cell_cfg_nr_t *cell = NULL;
	cvector_for_each_in(cell, rrc_cfg->cell_list){
		HANDLE_ERROR(set_derived_nr_cell_params(rrc_cfg.is_standalone, cell));
	}
	
    return OSET_OK;
}


static int set_derived_args_nr(all_args_t* args_, rrc_cfg_t* rrc_cfg_, rrc_nr_cfg_t* rrc_nr_cfg_, phy_cfg_t* phy_cfg_)
{
	rrc_cell_cfg_nr_t *cfg = NULL;

	// we only support one NR cell
	if (cvector_size(rrc_nr_cfg_->cell_list) > 1) {
		oset_error("Only a single NR cell supported.");
		return OSET_ERROR;
	}

	rrc_nr_cfg_->inactivity_timeout_ms = args_->general.rrc_inactivity_timer;

	cvector_for_each_in(cfg, rrc_nr_cfg_->cell_list){
	   cfg->phy_cell.carrier.max_mimo_layers = args_->enb.nof_ports;
	   // NR cells have the same bandwidth as EUTRA cells, adjust PRB sizes  //4g prb change to 5g
	   switch (args_->enb.n_prb) {
		 case 25:
		   cfg->phy_cell.carrier.nof_prb = 25;
		   break;
		 case 50:
		   cfg->phy_cell.carrier.nof_prb = 52;
		   break;
		 case 100:
		   cfg->phy_cell.carrier.nof_prb = 106;
		   break;
		 default:
		   oset_error("The only accepted number of PRB is: 25, 50, 100");
		   return OSET_ERROR;
	   }
	   // phy_cell_cfg.root_seq_idx = cfg->root_seq_idx;
	   
	   // PDSCH
	   cfg->pdsch_rs_power = 0;//phy_cfg_->pdsch_cnfg.ref_sig_pwr
	}
	rrc_nr_cfg_->enb_id = args_->enb.enb_id;
	rrc_nr_cfg_->mcc	= args_->nr_stack.ngap.mcc;
	rrc_nr_cfg_->mnc	= args_->nr_stack.ngap.mnc;

	// Derive cross-dependent cell params
	if (set_derived_nr_rrc_params(rrc_nr_cfg_) != OSET_OK) {
	  oset_error("Failed to derive NR cell params");
	  return OSET_ERROR;
	}
	
	// Update PHY with RRC cell configs
	cvector_for_each_in(cfg, rrc_nr_cfg_->cell_list){
		cvector_push_back(phy_cfg_->phy_cell_cfg_nr, cfg->phy_cell);
	}

	cvector_for_each_in(cfg, rrc_nr_cfg_->cell_list){
		if (cfg->phy_cell.carrier.nof_prb != 52) {
		  oset_error("Only 10 MHz bandwidth supported.");
		  return OSET_ERROR;
		}
	}

	 /*gnb_manager_self()->args.general.eea_pref_list = "EEA0, EEA2, EEA1";
	 gnb_manager_self()->args.general.eia_pref_list = "EIA2, EIA1, EIA0";*/
	
	 // Parse NIA/NEA preference list
	rrc_nr_cfg_->nea_preference_list[0] = CIPHERING_ALGORITHM_ID_EEA0;
	rrc_nr_cfg_->nea_preference_list[1] = CIPHERING_ALGORITHM_ID_128_EEA2;
	rrc_nr_cfg_->nea_preference_list[2] = CIPHERING_ALGORITHM_ID_128_EEA1;
	//rrc_nr_cfg_->nea_preference_list[3] = CIPHERING_ALGORITHM_ID_128_EEA3;
	
	rrc_nr_cfg_->nia_preference_list[0] = INTEGRITY_ALGORITHM_ID_NR_128_NIA2;
	rrc_nr_cfg_->nia_preference_list[1] = INTEGRITY_ALGORITHM_ID_NR_128_NIA1;
	rrc_nr_cfg_->nia_preference_list[2] = INTEGRITY_ALGORITHM_ID_NR_NIA0;
	//rrc_nr_cfg_->nia_preference_list[3] = INTEGRITY_ALGORITHM_ID_NR_128_NIA3;

	
   /*********************set_derived_args*************************/
	/*if (args_->enb.transmission_mode == 1) {
	  phy_cfg_->pdsch_cnfg.p_b									  = 0; // Default TM1
	  rrc_cfg_->sib2.rr_cfg_common.pdsch_cfg_common.p_b = 0;
	} else {
	  phy_cfg_->pdsch_cnfg.p_b									  = 1; // Default TM2,3,4
	  rrc_cfg_->sib2.rr_cfg_common.pdsch_cfg_common.p_b = 1;
	}

	uint32_t t310 = 200;//rrc_cfg_->sib2.ue_timers_and_consts.t310;
	uint32_t t311 = 1;//rrc_cfg_->sib2.ue_timers_and_consts.t311;
	uint32_t n310 = 10000;//rrc_cfg_->sib2.ue_timers_and_consts.n310;
	uint32_t min_rrc_inactivity_timer = t310 + t311 + n310 + 50;
	if (args_->general.rrc_inactivity_timer < min_rrc_inactivity_timer) {
	  oset_error("rrc_inactivity_timer=%d is too low. Consider setting it to a value equal or above %d",
			args_->general.rrc_inactivity_timer,
			min_rrc_inactivity_timer);
	}

	// Check PUCCH and PRACH configuration
	uint32_t nrb_pucch =
		max(rrc_cfg_->sr_cfg.nof_prb, (uint32_t)rrc_cfg_->sib2.rr_cfg_common.pucch_cfg_common.nrb_cqi);
	uint32_t prach_freq_offset = rrc_cfg_->sib2.rr_cfg_common.prach_cfg.prach_cfg_info.prach_freq_offset;
	if (args_->enb.n_prb > 6) {
	  uint32_t lower_bound = nrb_pucch;
	  uint32_t upper_bound = args_->enb.n_prb - nrb_pucch;
	  if (prach_freq_offset + 6 > upper_bound or prach_freq_offset < lower_bound) {
		oset_error("ERROR: Invalid PRACH configuration - prach_freq_offset=%d collides with PUCCH", prach_freq_offset);
		oset_error("		Consider changing \"prach_freq_offset\" in sib.conf to a value between %d and %d", lower_bound, upper_bound);
		return OSET_ERROR;
	  }
	} else { // 6 PRB case
	  if (prach_freq_offset + 6 > args_->enb.n_prb) {
		oset_error("ERROR: Invalid PRACH configuration - prach=(%d, %d) does not fit into the eNB PRBs=(0, %d).\n",
				    prach_freq_offset,
				    prach_freq_offset + 6,
				    args_->enb.n_prb);
		oset_error("		Consider changing the \"prach_freq_offset\" value to 0 in the sib.conf file when using 6 PRBs");
		// patch PRACH config for PHY and in RRC for SIB2
		rrc_cfg_->sib2.rr_cfg_common.prach_cfg.prach_cfg_info.prach_freq_offset = 0;
		phy_cfg_->prach_cnfg.prach_cfg_info.prach_freq_offset = 0;
	  }
	}*/

	// Patch certain args that are not exposed yet
	args_->rf.nof_antennas = args_->enb.nof_ports;

	// Set max number of KOs
	rrc_cfg_->max_mac_dl_kos	   = args_->general.max_mac_dl_kos;
	rrc_cfg_->max_mac_ul_kos	   = args_->general.max_mac_ul_kos;
	rrc_cfg_->rlf_release_timer_ms = args_->general.rlf_release_timer_ms;

    return OSET_OK; 
}

/*#####################################################################
# CFR configuration options
#
# The CFR module provides crest factor reduction for the transmitted signal.
#
# enable:           Enable or disable the CFR. Default: disabled
#
# mode:             manual:   CFR threshold is set by cfr_manual_thres (default).
#                   auto_ema: CFR threshold is adaptive based on the signal PAPR. Power avg. with Exponential Moving Average.
#                             The time constant of the averaging can be tweaked with the ema_alpha parameter.
#                   auto_cma: CFR threshold is adaptive based on the signal PAPR. Power avg. with Cumulative Moving Average.
#                             Use with care, as CMA's increasingly slow response may be unsuitable for most use cases.
#
# strength:         Ratio between amplitude-limited vs unprocessed signal (0 to 1). Default: 1
# manual_thres:     Fixed manual clipping threshold for CFR manual mode. Default: 0.5
# auto_target_papr: Signal PAPR target (in dB) in CFR auto modes. output PAPR can be higher due to peak smoothing. Default: 8
# ema_alpha:        Alpha coefficient for the power average in auto_ema mode. Default: 1/7
#
#####################################################################*/

static int parse_cfr_args(all_args_t* args, srsran_cfr_cfg_t* cfr_config)
{
  cfr_config->cfr_enable  = args->phy.cfr_args.enable;
  cfr_config->cfr_mode    = args->phy.cfr_args.mode;
  cfr_config->alpha       = args->phy.cfr_args.strength;
  cfr_config->manual_thr  = args->phy.cfr_args.manual_thres;
  cfr_config->max_papr_db = args->phy.cfr_args.auto_target_papr;
  cfr_config->ema_alpha   = args->phy.cfr_args.ema_alpha;

  if (!srsran_cfr_params_valid(cfr_config)) {
    oset_error("Invalid CFR parameters: cfr_mode=%d, alpha=%.2f, manual_thr=%.2f, \n "
               "max_papr_db=%.2f, ema_alpha=%.2f",
               cfr_config->cfr_mode,
               cfr_config->alpha,
               cfr_config->manual_thr,
               cfr_config->max_papr_db,
               cfr_config->ema_alpha);
    return OSET_ERROR;
  }

  return OSET_OK;
}

int parse_cfg_files(all_args_t* args_, rrc_cfg_t* rrc_cfg_, rrc_nr_cfg_t* rrc_nr_cfg_, phy_cfg_t* phy_cfg_)
{
    // Parse config files
	/*srsran_cell_t cell_common_cfg = {0};
    if (parse_cell_cfg(args_, &cell_common_cfg) != OSET_OK) {
	    oset_error("Error parsing Cell configuration");
        return OSET_ERROR;
    }*/

	//sib for 4G
    /*if (parse_sibs(args_, rrc_cfg_, phy_cfg_) != OSET_OK) {
	  oset_error("Error parsing SIB configuration");
      return OSET_ERROR;
    }*/

    if (parse_rr(args_, rrc_cfg_, rrc_nr_cfg_) != OSET_OK) {
      oset_error("Error parsing Radio Resources configuration");
      return OSET_ERROR;
    }

    if (parse_rb(args_, rrc_nr_cfg_) != OSET_OK) {
      oset_error("Error parsing RB configuration");
      return OSET_ERROR;
    }

	rrc_cfg_->num_nr_cells = cvector_size(rrc_nr_cfg_->cell_list);
	args_->rf.nof_carriers = cvector_size(rrc_nr_cfg_->cell_list);
    oset_assert(args_->rf.nof_carriers > 0);
    if (rrc_cfg_->num_nr_cells) {
      // SA mode.
      rrc_nr_cfg_->is_standalone = true;
    }


	// Set fields derived from others, and check for correctness of the parsed configuration
	if (set_derived_args_nr(args_, rrc_cfg_, rrc_nr_cfg_, phy_cfg_) != OSET_OK) {
	  oset_error("Error deriving NR cell parameters");
	  return OSET_ERROR;
	}

	// update number of NR cells
	  // NR cells available.
	  if (cvector_size(rrc_nr_cfg_->cell_list > 0) && (rrc_nr_cfg_->is_standalone)) {
		  // SA mode. Update NGAP args
	      args_->nr_stack.ngap.cell_id = rrc_nr_cfg_->cell_list[0].phy_cell.cell_id;
		  args_->nr_stack.ngap.tac	 = rrc_nr_cfg_->cell_list[0].tac;
		  //update NGAP params
		  args_->nr_stack.ngap.gnb_id = args_->enb.enb_id;
		  
		  // MAC needs to know the cell bandwidth to dimension softbuffers
		  args_->nr_stack.mac_nr.nof_prb = args_->enb.n_prb;
	  }

	  // Parse CFR args
	  if (parse_cfr_args(args_, &phy_cfg_->cfr_config) != OSET_OK) {
		oset_error("Error parsing CFR configuration\n");
		return OSET_ERROR;
	  }
    return OSET_OK;	
}


