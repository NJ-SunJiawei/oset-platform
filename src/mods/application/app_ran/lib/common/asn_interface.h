/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef ASN_INTERFACE_H_
#define ASN_INTERFACE_H_

#include "lib/common/asn_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

enum setup_release_e {nothing, release, setup, nulltype } ;
#define setup_release_c(T) \
			struct { \
				enum setup_release_e type_;\
				T	  c;\
			}
/**********************/
enum reg_bundle_size_opts { n2, n3, n6, nulltype };
enum interleaver_size_opts { n2, n3, n6, nulltype };
struct interleaved_s_ {

  bool				  shift_idx_present;
  enum reg_bundle_size_opts   reg_bundle_size;
  enum interleaver_size_opts interleaver_size;
  uint16_t			  shift_idx;
};

enum cce_reg_map_types_opts {nothing, interleaved, non_interleaved, nulltype };
struct cce_reg_map_type_c_ {
  enum cce_reg_map_types_opts types;
  struct interleaved_s_ c;
};

enum precoder_granularity_opts { same_as_reg_bundle, all_contiguous_rbs, nulltype };
// ControlResourceSet
struct ctrl_res_set_s {
  bool								  tci_present_in_dci_present;
  bool								  pdcch_dmrs_scrambling_id_present;
  uint8_t							  ctrl_res_set_id;
  uint8_t							  freq_domain_res[6];//fixed_bitstring<45>
  uint8_t							  dur;//1
  struct cce_reg_map_type_c_		  cce_reg_map_type;
  enum precoder_granularity_opts	  precoder_granularity;
  cvector_vector_t(uint8_t)		  tci_states_pdcch_to_add_list;//dyn_array<uint8_t>
  cvector_vector_t(uint8_t)		  tci_states_pdcch_to_release_list;//dyn_array<uint8_t>
  uint32_t							  pdcch_dmrs_scrambling_id;
};

enum monitoring_slot_periodicity_and_offset_types_opts{
  nothing,
  sl1,
  sl2,
  sl4,
  sl5,
  sl8,
  sl10,
  sl16,
  sl20,
  sl40,
  sl80,
  sl160,
  sl320,
  sl640,
  sl1280,
  sl2560,
  nulltype
};

struct monitoring_slot_periodicity_and_offset_c_ {
	enum monitoring_slot_periodicity_and_offset_types_opts types;
	void	*c;//pod_choice_buffer_t
};


enum aggregation_level1_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
enum aggregation_level2_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
enum aggregation_level4_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
enum aggregation_level8_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
enum aggregation_level16_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
struct nrof_candidates_s_ {
  enum aggregation_level1_opts	aggregation_level1;
  enum aggregation_level2_opts	aggregation_level2;
  enum aggregation_level4_opts	aggregation_level4;
  enum aggregation_level8_opts	aggregation_level8;
  enum aggregation_level16_opts aggregation_level16;
};

struct dci_format0_minus0_and_format1_minus0_s_ {
};

enum aggregation_level1_opts2 { n1, n2, nulltype };
enum aggregation_level2_opts2 { n1, n2, nulltype };
enum aggregation_level4_opts2 { n1, n2, nulltype };
enum aggregation_level8_opts2 { n1, n2, nulltype };
enum aggregation_level16_opts2 { n1, n2, nulltype };
struct nrof_candidates_sfi_s_ {
  bool					 aggregation_level1_present;
  bool					 aggregation_level2_present;
  bool					 aggregation_level4_present;
  bool					 aggregation_level8_present;
  bool					 aggregation_level16_present;
  enum aggregation_level1_opts2	aggregation_level1;
  enum aggregation_level2_opts2	aggregation_level2;
  enum aggregation_level4_opts2	aggregation_level4;
  enum aggregation_level8_opts2	aggregation_level8;
  enum aggregation_level16_opts2 aggregation_level16;
};

struct dci_format2_minus0_s_ {
  struct nrof_candidates_sfi_s_ nrof_candidates_sfi;
  // ...
};

struct dci_format2_minus1_s_ {
  // ...
};
struct dci_format2_minus2_s_ {
  // ...
};

enum dummy1_opts { sl1, sl2, sl4, sl5, sl8, sl10, sl16, sl20, nulltype };
enum dummy2_opts { n1, n2, nulltype };
struct dci_format2_minus3_s_ {
  bool		dummy1_present;
  enum dummy1_opts dummy1;
  enum dummy2_opts dummy2;
  // ...
};

struct common_s_ {
  bool									   dci_format0_minus0_and_format1_minus0_present;
  bool									   dci_format2_minus0_present;
  bool									   dci_format2_minus1_present;
  bool									   dci_format2_minus2_present;
  bool									   dci_format2_minus3_present;
  struct dci_format0_minus0_and_format1_minus0_s_ dci_format0_minus0_and_format1_minus0;
  struct dci_format2_minus0_s_				   dci_format2_minus0;
  struct dci_format2_minus1_s_				   dci_format2_minus1;
  struct dci_format2_minus2_s_				   dci_format2_minus2;
  struct dci_format2_minus3_s_				   dci_format2_minus3;
};

enum dci_formats_opts { formats0_minus0_and_minus1_minus0, formats0_minus1_and_minus1_minus1, nulltype };
struct ue_specific_s_ {
  enum dci_formats_opts dci_formats;
  // ...
};

enum search_space_types_opts {nothing, common, ue_specific, nulltype };
struct search_space_type_c_ {
  enum search_space_types_opts types ;
  union{
	 struct common_s_		common;
	 struct ue_specific_s_	ue_spec;
  }c;
};

struct search_space_s{
  bool										ctrl_res_set_id_present;
  bool										monitoring_slot_periodicity_and_offset_present;
  bool										dur_present;
  bool										monitoring_symbols_within_slot_present;
  bool										nrof_candidates_present;
  bool										search_space_type_present;
  uint8_t									search_space_id;
  uint8_t									ctrl_res_set_id;
  struct monitoring_slot_periodicity_and_offset_c_ monitoring_slot_periodicity_and_offset;
  uint16_t									dur;//2
  uint8_t									monitoring_symbols_within_slot[2];//fixed_bitstring<14>
  struct nrof_candidates_s_ 				nrof_candidates;
  struct search_space_type_c_				search_space_type;
};


// PDCCH-ConfigCommon
struct pdcch_cfg_common_s {
  bool						  ctrl_res_set_zero_present;
  bool						  common_ctrl_res_set_present;
  bool						  search_space_zero_present;
  bool						  search_space_sib1_present;
  bool						  search_space_other_sys_info_present;
  bool						  paging_search_space_present;
  bool						  ra_search_space_present;
  uint8_t					  ctrl_res_set_zero;
  struct ctrl_res_set_s 	  common_ctrl_res_set;
  uint8_t					  search_space_zero;
  uint8_t					  nof_common_search_space;
  cvector_vector_t(struct search_space_s)  common_search_space_list;//dyn_array<search_space_s>
  uint8_t					  search_space_sib1;
  uint8_t					  search_space_other_sys_info;
  uint8_t					  paging_search_space;
  uint8_t					  ra_search_space;
};


// PDCCH-Config
struct pdcch_cfg_s {
  bool								   dl_preemption_present;
  bool								   tpc_pusch_present;
  bool								   tpc_pucch_present;
  bool								   tpc_srs_present;
  int								   nof_ctrl_res_set_to_add_mod;
  cvector_vector_t(struct ctrl_res_set_s) ctrl_res_set_to_add_mod_list;//dyn_array<ctrl_res_set_s>
  uint8_t								ctrl_res_set_to_release_list[3];
  int									nof_search_spaces_to_add_mod;
  cvector_vector_t(struct search_space_s) search_spaces_to_add_mod_list;//dyn_array<ctrl_res_set_s>
  uint8_t								search_spaces_to_release_list[10];
};

// PDSCH-ConfigCommon
struct pdsch_cfg_common_lte_s {
  int8_t  ref_sig_pwr;//-60
  uint8_t p_b;//0
};

// PUSCH-ConfigCommon
enum hop_mode_opts { inter_sub_frame, intra_and_inter_sub_frame, nulltype };

struct pusch_cfg_basic_s_ {
  uint8_t	  n_sb;// 1
  enum hop_mode_opts hop_mode;
  uint8_t	  pusch_hop_offset;//0
  bool		  enable64_qam;//false
};

struct ul_ref_sigs_pusch_s {
  bool	  group_hop_enabled;//false
  uint8_t group_assign_pusch;//0
  bool	  seq_hop_enabled;//false
  uint8_t cyclic_shift;//0
};

struct pusch_cfg_common_lte_s {
  struct pusch_cfg_basic_s_  pusch_cfg_basic;
  struct ul_ref_sigs_pusch_s ul_ref_sigs_pusch;
};

// PUCCH-ConfigCommon
enum delta_pucch_shift_opts { ds1, ds2, ds3, nulltype };
struct pucch_cfg_common_lte_s {
  enum delta_pucch_shift_opts delta_pucch_shift;
  uint8_t			   nrb_cqi;
  uint8_t			   ncs_an;
  uint16_t			   n1_pucch_an;
};

// PRACH-ConfigSIB
struct prach_cfg_info_s {
  uint8_t prach_cfg_idx;
  bool	  high_speed_flag;
  uint8_t zero_correlation_zone_cfg;
  uint8_t prach_freq_offset;
};

struct prach_cfg_sib_lte_s {
  uint16_t		   root_seq_idx;
  struct prach_cfg_info_s prach_cfg_info;
};

// SoundingRS-UL-ConfigCommon
enum srs_bw_cfg_opts { bw0, bw1, bw2, bw3, bw4, bw5, bw6, bw7, nulltype };
enum srs_sf_cfg_opts {
  sc0,
  sc1,
  sc2,
  sc3,
  sc4,
  sc5,
  sc6,
  sc7,
  sc8,
  sc9,
  sc10,
  sc11,
  sc12,
  sc13,
  sc14,
  sc15,
  nulltype
};

struct srs_ul_cfg_common_lte_c {
  struct setup_s_ {
	bool		  srs_max_up_pts_present;
	enum srs_bw_cfg_opts srs_bw_cfg;
	enum srs_sf_cfg_opts srs_sf_cfg;
	bool		  ack_nack_srs_simul_tx;
  }c;
  enum setup_release_e		types;
};

/************/
enum nof_ra_preambs_opts { n4, n8, n12, n16, n20, n24, n28, n32, n36, n40, n44, n48, n52, n56, n60, n64, nulltype };
enum size_of_ra_preambs_group_a_opts { n4, n8, n12, n16, n20, n24, n28, n32, n36, n40, n44, n48, n52, n56, n60, nulltype };
enum msg_size_group_a_opts { b56, b144, b208, b256, nulltype };
enum msg_pwr_offset_group_b_opts { minusinfinity, db0, db5, db8, db10, db12, db15, db18, nulltype };
enum ra_resp_win_size_opts { sf2, sf3, sf4, sf5, sf6, sf7, sf8, sf10, nulltype };
enum mac_contention_resolution_timer_opts { sf8, sf16, sf24, sf32, sf40, sf48, sf56, sf64, nulltype };
enum preamb_trans_max_opts { n3, n4, n5, n6, n7, n8, n10, n20, n50, n100, n200, nulltype };
enum pwr_ramp_step_opts { db0, db2, db4, db6, nulltype };
enum preamb_init_rx_target_pwr_opts {
  dbm_minus120,
  dbm_minus118,
  dbm_minus116,
  dbm_minus114,
  dbm_minus112,
  dbm_minus110,
  dbm_minus108,
  dbm_minus106,
  dbm_minus104,
  dbm_minus102,
  dbm_minus100,
  dbm_minus98,
  dbm_minus96,
  dbm_minus94,
  dbm_minus92,
  dbm_minus90,
  nulltype
};

struct preambs_group_a_cfg_s_ {
  enum size_of_ra_preambs_group_a_opts	size_of_ra_preambs_group_a;
  enum msg_size_group_a_opts		   msg_size_group_a;
  enum msg_pwr_offset_group_b_opts	   msg_pwr_offset_group_b;
};

struct preamb_info_s_ {
  bool					 preambs_group_a_cfg_present;
  enum nof_ra_preambs_opts		nof_ra_preambs;
  struct preambs_group_a_cfg_s_ preambs_group_a_cfg;
};

struct ra_supervision_info_s_ {
  enum preamb_trans_max_opts		 preamb_trans_max;
  enum ra_resp_win_size_opts		 ra_resp_win_size;
  enum mac_contention_resolution_timer_opts  mac_contention_resolution_timer;
};

struct pwr_ramp_params_s {
  enum pwr_ramp_step_opts			  pwr_ramp_step;
  enum preamb_init_rx_target_pwr_opts preamb_init_rx_target_pwr;
};

struct rach_cfg_common_lte_s {
  struct preamb_info_s_  preamb_info;
  struct pwr_ramp_params_s		pwr_ramp_params;
  struct ra_supervision_info_s_ ra_supervision_info;
  uint8_t				 max_harq_msg3_tx;//1
};

enum mod_period_coeff_opts { n2, n4, n8, n16, nulltype };
struct bcch_cfg_s {
  enum mod_period_coeff_opts mod_period_coeff;
};

enum default_paging_cycle_opts { rf32, rf64, rf128, rf256, nulltype };
enum nb_opts {
  four_t,
  two_t,
  one_t,
  half_t,
  quarter_t,
  one_eighth_t,
  one_sixteenth_t,
  one_thirty_second_t,
  nulltype
};

struct pcch_cfg_lte_s {
  enum default_paging_cycle_opts  default_paging_cycle;
  enum nb_opts					  nb;
};


// UplinkPowerControlCommon
enum alpha_r12_opts { al0, al04, al05, al06, al07, al08, al09, al1, nulltype };
enum delta_f_pucch_format1_opts { delta_f_minus2, delta_f0, delta_f2, nulltype };
enum delta_f_pucch_format1b_opts { delta_f1, delta_f3, delta_f5, nulltype };
enum delta_f_pucch_format2_opts { delta_f_minus2, delta_f0, delta_f1, delta_f2, nulltype };
enum delta_f_pucch_format2a_opts { delta_f_minus2, delta_f0, delta_f2, nulltype };
enum delta_f_pucch_format2b_opts { delta_f_minus2, delta_f0, delta_f2, nulltype };

struct delta_flist_pucch_s {
  enum delta_f_pucch_format1_opts  delta_f_pucch_format1;
  enum delta_f_pucch_format1b_opts delta_f_pucch_format1b;
  enum delta_f_pucch_format2_opts  delta_f_pucch_format2;
  enum delta_f_pucch_format2a_opts delta_f_pucch_format2a;
  enum delta_f_pucch_format2b_opts delta_f_pucch_format2b;
};

struct ul_pwr_ctrl_common_lte_s {
  int8_t			  p0_nominal_pusch;//-126
  enum alpha_r12_opts alpha;
  int8_t			  p0_nominal_pucch;//-127
  struct delta_flist_pucch_s delta_flist_pucch;
  int8_t			  delta_preamb_msg3;//-1
};


//PDSCH-ConfigDedicated
enum p_a_opts { db_minus6, db_minus4dot77, db_minus3, db_minus1dot77, db0, db1, db2, db3, nulltype };

// PUSCH-ConfigDedicated
struct pusch_cfg_ded_s {
  uint8_t beta_offset_ack_idx;
  uint8_t beta_offset_ri_idx;
  uint8_t beta_offset_cqi_idx;
};

/******************/
enum max_harq_tx_opts { n1, n2, n3, n4, n5, n6, n7, n8, n10, n12, n16, n20, n24, n28, spare2, spare1, nulltype };
enum periodic_bsr_timer_r12_opts {
  sf5,
  sf10,
  sf16,
  sf20,
  sf32,
  sf40,
  sf64,
  sf80,
  sf128,
  sf160,
  sf320,
  sf640,
  sf1280,
  sf2560,
  infinity,
  spare1,
  nulltype
};
enum retx_bsr_timer_r12_opts { sf320, sf640, sf1280, sf2560, sf5120, sf10240, spare2, spare1, nulltype };

struct ul_sch_cfg_s_ {

  bool					   max_harq_tx_present;
  bool					   periodic_bsr_timer_present;
  enum max_harq_tx_opts    max_harq_tx;
  enum periodic_bsr_timer_r12_opts periodic_bsr_timer;
  enum retx_bsr_timer_r12_opts	   retx_bsr_timer;
  bool					   tti_bundling;
};


enum periodic_phr_timer_opts { sf10, sf20, sf50, sf100, sf200, sf500, sf1000, infinity, nulltype };
enum prohibit_phr_timer_opts { sf0, sf10, sf20, sf50, sf100, sf200, sf500, sf1000, nulltype };
enum dl_pathloss_change_opts { db1, db3, db6, infinity, nulltype };
struct phr_cfg_c_ {
  struct setup_s_ {
	enum periodic_phr_timer_opts periodic_phr_timer;
	enum prohibit_phr_timer_opts prohibit_phr_timer;
	enum dl_pathloss_change_opts dl_pathloss_change;
  }c;
  enum setup_release_e types;
};

// TimeAlignmentTimer
enum time_align_timer_opts { sf500, sf750, sf1280, sf1920, sf2560, sf5120, sf10240, infinity, nulltype };

// MAC-MainConfig
struct mac_main_cfg_s {
  bool				 ul_sch_cfg_present;
  bool				 drx_cfg_present;
  bool				 phr_cfg_present;
  struct ul_sch_cfg_s_		ul_sch_cfg;
  enum time_align_timer_opts time_align_timer_ded;
  struct phr_cfg_c_ 		phr_cfg;
};


/**********************************************/
// RLC-Config
enum rlc_types_opts {nothing, am, um_bi_dir, um_uni_dir_ul, um_uni_dir_dl, /*...*/ nulltype };
enum max_retx_thres_opts { t1, t2, t3, t4, t6, t8, t16, t32, nulltype };
enum poll_pdu_opts {     
	p4,
    p8,
    p16,
    p32,
    p64,
    p128,
    p256,
    p512,
    p1024,
    p2048,
    p4096,
    p6144,
    p8192,
    p12288,
    p16384,
    p20480,
    p24576,
    p28672,
    p32768,
    p40960,
    p49152,
    p57344,
    p65536,
    infinity,
    spare8,
    spare7,
    spare6,
    spare5,
    spare4,
    spare3,
    spare2,
    spare1,
    nulltype};

enum t_poll_retx_opts {
    ms5,
    ms10,
    ms15,
    ms20,
    ms25,
    ms30,
    ms35,
    ms40,
    ms45,
    ms50,
    ms55,
    ms60,
    ms65,
    ms70,
    ms75,
    ms80,
    ms85,
    ms90,
    ms95,
    ms100,
    ms105,
    ms110,
    ms115,
    ms120,
    ms125,
    ms130,
    ms135,
    ms140,
    ms145,
    ms150,
    ms155,
    ms160,
    ms165,
    ms170,
    ms175,
    ms180,
    ms185,
    ms190,
    ms195,
    ms200,
    ms205,
    ms210,
    ms215,
    ms220,
    ms225,
    ms230,
    ms235,
    ms240,
    ms245,
    ms250,
    ms300,
    ms350,
    ms400,
    ms450,
    ms500,
    ms800,
    ms1000,
    ms2000,
    ms4000,
    spare5,
    spare4,
    spare3,
    spare2,
    spare1,
    nulltype

};
enum poll_byte_opts {
    kb1,
    kb2,
    kb5,
    kb8,
    kb10,
    kb15,
    kb25,
    kb50,
    kb75,
    kb100,
    kb125,
    kb250,
    kb375,
    kb500,
    kb750,
    kb1000,
    kb1250,
    kb1500,
    kb2000,
    kb3000,
    kb4000,
    kb4500,
    kb5000,
    kb5500,
    kb6000,
    kb6500,
    kb7000,
    kb7500,
    mb8,
    mb9,
    mb10,
    mb11,
    mb12,
    mb13,
    mb14,
    mb15,
    mb16,
    mb17,
    mb18,
    mb20,
    mb25,
    mb30,
    mb40,
    infinity,
    spare20,
    spare19,
    spare18,
    spare17,
    spare16,
    spare15,
    spare14,
    spare13,
    spare12,
    spare11,
    spare10,
    spare9,
    spare8,
    spare7,
    spare6,
    spare5,
    spare4,
    spare3,
    spare2,
    spare1,
    nulltype

};

enum t_reassembly_opts {
    ms0,
    ms5,
    ms10,
    ms15,
    ms20,
    ms25,
    ms30,
    ms35,
    ms40,
    ms45,
    ms50,
    ms55,
    ms60,
    ms65,
    ms70,
    ms75,
    ms80,
    ms85,
    ms90,
    ms95,
    ms100,
    ms110,
    ms120,
    ms130,
    ms140,
    ms150,
    ms160,
    ms170,
    ms180,
    ms190,
    ms200,
    spare1,
    nulltype
};

enum t_status_prohibit_opts {
    ms0,
    ms5,
    ms10,
    ms15,
    ms20,
    ms25,
    ms30,
    ms35,
    ms40,
    ms45,
    ms50,
    ms55,
    ms60,
    ms65,
    ms70,
    ms75,
    ms80,
    ms85,
    ms90,
    ms95,
    ms100,
    ms105,
    ms110,
    ms115,
    ms120,
    ms125,
    ms130,
    ms135,
    ms140,
    ms145,
    ms150,
    ms155,
    ms160,
    ms165,
    ms170,
    ms175,
    ms180,
    ms185,
    ms190,
    ms195,
    ms200,
    ms205,
    ms210,
    ms215,
    ms220,
    ms225,
    ms230,
    ms235,
    ms240,
    ms245,
    ms250,
    ms300,
    ms350,
    ms400,
    ms450,
    ms500,
    ms800,
    ms1000,
    ms1200,
    ms1600,
    ms2000,
    ms2400,
    spare2,
    spare1,
    nulltype
};
	
enum sn_field_len_am_opts { size12, size18, nulltype };
enum sn_field_len_um_opts { size6, size12, nulltype };

struct ul_am_rlc_s {
  bool			  sn_field_len_present;
  enum sn_field_len_am_opts   sn_field_len;
  enum t_poll_retx_opts    t_poll_retx;
  enum poll_pdu_opts       poll_pdu;
  enum poll_byte_opts      poll_byte;
  enum max_retx_thres_opts max_retx_thres;
};

struct dl_am_rlc_s {
  bool			sn_field_len_present;
  enum sn_field_len_am_opts	sn_field_len;
  enum t_reassembly_opts     t_reassembly;
  enum t_status_prohibit_opts  t_status_prohibit;
};

struct ul_um_rlc_s {
  bool			  sn_field_len_present;
  enum sn_field_len_um_opts sn_field_len;
};

struct dl_um_rlc_s {
  bool			  sn_field_len_present;
  enum sn_field_len_um_opts sn_field_len;
  enum t_reassembly_opts    t_reassembly;
};

struct am_s_ {
  struct ul_am_rlc_s ul_am_rlc;
  struct dl_am_rlc_s dl_am_rlc;
};
struct um_bi_dir_s_ {
  struct ul_um_rlc_s ul_um_rlc;
  struct dl_um_rlc_s dl_um_rlc;
};
struct um_uni_dir_ul_s_ {
  struct ul_um_rlc_s ul_um_rlc;
};
struct um_uni_dir_dl_s_ {
  struct dl_um_rlc_s dl_um_rlc;
};

struct rlc_cfg_c {
  enum rlc_types_opts   types;
  union{
    struct am_s_ am;
	struct um_bi_dir_s_ um_bi_dir;
	struct um_uni_dir_ul_s_ um_uni_dir_ul;
	struct um_uni_dir_dl_s_ um_uni_dir_dl;
  }c;
};


// PDCP-Config
enum discard_timer_opts {
  ms10,
  ms20,
  ms30,
  ms40,
  ms50,
  ms60,
  ms75,
  ms100,
  ms150,
  ms200,
  ms250,
  ms300,
  ms500,
  ms750,
  ms1500,
  infinity,
  nulltype
};
enum t_reordering_opts {
  ms0,
  ms1,
  ms2,
  ms4,
  ms5,
  ms8,
  ms10,
  ms15,
  ms20,
  ms30,
  ms40,
  ms50,
  ms60,
  ms80,
  ms100,
  ms120,
  ms140,
  ms160,
  ms180,
  ms200,
  ms220,
  ms240,
  ms260,
  ms280,
  ms300,
  ms500,
  ms750,
  ms1000,
  ms1250,
  ms1500,
  ms1750,
  ms2000,
  ms2250,
  ms2500,
  ms2750,
  ms3000,
  spare28,
  spare27,
  spare26,
  spare25,
  spare24,
  spare23,
  spare22,
  spare21,
  spare20,
  spare19,
  spare18,
  spare17,
  spare16,
  spare15,
  spare14,
  spare13,
  spare12,
  spare11,
  spare10,
  spare09,
  spare08,
  spare07,
  spare06,
  spare05,
  spare04,
  spare03,
  spare02,
  spare01,
  nulltype
}; 

enum ul_data_split_thres_opts {
  b0,
  b100,
  b200,
  b400,
  b800,
  b1600,
  b3200,
  b6400,
  b12800,
  b25600,
  b51200,
  b102400,
  b204800,
  b409600,
  b819200,
  b1228800,
  b1638400,
  b2457600,
  b3276800,
  b4096000,
  b4915200,
  b5734400,
  b6553600,
  infinity,
  spare8,
  spare7,
  spare6,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
} ;

enum pdcp_sn_size_ul_opts { len12bits, len18bits, nulltype };
enum pdcp_sn_size_dl_opts { len12bits, len18bits, nulltype };

struct profiles_s_ {
  bool profile0x0001;
  bool profile0x0002;
  bool profile0x0003;
  bool profile0x0004;
  bool profile0x0006;
  bool profile0x0101;
  bool profile0x0102;
  bool profile0x0103;
  bool profile0x0104;
};

struct rohc_s_ {
  bool		  max_cid_present;
  bool		  drb_continue_rohc_present;
  uint16_t	  max_cid;//1
  struct profiles_s_ profiles;
};

struct profiles_s_ {
  bool profile0x0006;
};

struct ul_only_rohc_s_ {
  bool		  max_cid_present;
  bool		  drb_continue_rohc_present;
  uint16_t	  max_cid;//1
  struct profiles_s_ profiles;
};

struct rohc_s_ {
  struct profiles_s_ {
	bool profile0x0001;
	bool profile0x0002;
	bool profile0x0003;
	bool profile0x0004;
	bool profile0x0006;
	bool profile0x0101;
	bool profile0x0102;
	bool profile0x0103;
	bool profile0x0104;
  };

  // member variables
  bool		  max_cid_present;
  bool		  drb_continue_rohc_present;
  uint16_t	  max_cid;//1
  profiles_s_ profiles;
};


enum hdr_compress_types_opts { not_used, rohc, ul_only_rohc, /*...*/ nulltype } value;
struct hdr_compress_c_ {
	enum hdr_compress_types_opts  types;
	union
	{
	    struct rohc_s_         rohc;
	    struct ul_only_rohc_s_ ul_only_rohc;
	}c;

};

struct drb_s_ {
	bool               discard_timer_present;
	bool               pdcp_sn_size_ul_present;
	bool               pdcp_sn_size_dl_present;
	bool               integrity_protection_present;
	bool               status_report_required_present;
	bool               out_of_order_delivery_present;
	enum discard_timer_opts   discard_timer;
	enum pdcp_sn_size_ul_opts pdcp_sn_size_ul;
	enum pdcp_sn_size_dl_opts pdcp_sn_size_dl;
	struct hdr_compress_c_    hdr_compress;
};

struct primary_path_s_ {
  bool	  cell_group_present;
  bool	  lc_ch_present;
  uint8_t cell_group;
  uint8_t lc_ch;//1
};

struct more_than_one_rlc_s_ {
  bool					ul_data_split_thres_present;
  bool					pdcp_dupl_present;
  struct primary_path_s_ primary_path;
  enum ul_data_split_thres_opts  ul_data_split_thres;
  bool					 pdcp_dupl;
};

struct pdcp_cfg_s {
  bool                 drb_present;
  bool                 more_than_one_rlc_present;
  bool                 t_reordering_present;
  struct drb_s_        drb;
  struct more_than_one_rlc_s_ more_than_one_rlc;
  enum t_reordering_opts  t_reordering;
};

typedef struct srb_5g_cfg_s{
  bool                    present;
  struct rlc_cfg_c        rlc_cfg;//ASN_RRC_RLC_Config_t
}srb_5g_cfg_t;

typedef struct rrc_nr_cfg_five_qi_s{
  bool           configured;
  uint32_t       five_qi;
  struct pdcp_cfg_s    pdcp_cfg;//ASN_RRC_PDCP_Config_t
  struct rlc_cfg_c     rlc_cfg;//ASN_RRC_RLC_Config_t
}rrc_nr_cfg_five_qi_t;

/***********************************************************/
/***********************************************************/
/***********************************************************/
enum dsr_trans_max_opts { n4, n8, n16, n32, n64, spare3, spare2, spare1, nulltype };
typedef struct rrc_cfg_sr_s{
  uint32_t         period;
  enum dsr_trans_max_opts dsr_max;//asn1::rrc::sched_request_cfg_c::setup_s_::dsr_trans_max_e_
  uint32_t          nof_prb;
  uint32_t          sf_mapping[80];
  uint32_t          nof_subframes;
}rrc_cfg_sr_t;


enum rrc_cfg_cqi_mode_t { RRC_CFG_CQI_MODE_PERIODIC = 0, RRC_CFG_CQI_MODE_APERIODIC, RRC_CFG_CQI_MODE_N_ITEMS };
typedef struct  rrc_cfg_cqi_s {
  uint32_t           sf_mapping[80];
  uint32_t           nof_subframes;
  uint32_t           period;
  uint32_t           m_ri;
  bool               is_subband_enabled;
  uint32_t           subband_k;
  bool               simultaneousAckCQI;
  enum rrc_cfg_cqi_mode_t mode;
}rrc_cfg_cqi_t;

enum prioritised_bit_rate_opts {
  kbps0,
  kbps8,
  kbps16,
  kbps32,
  kbps64,
  kbps128,
  kbps256,
  infinity,
  kbps512_v1020,
  kbps1024_v1020,
  kbps2048_v1020,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
};
enum bucket_size_dur_opts { ms50, ms100, ms150, ms300, ms500, ms1000, spare2, spare1, nulltype };
struct ul_specific_params_lte_s_ {
  bool					  lc_ch_group_present;
  uint8_t				  prio;//1
  enum prioritised_bit_rate_opts  prioritised_bit_rate;
  enum bucket_size_dur_opts	  bucket_size_dur;
  uint8_t				  lc_ch_group ;
};

typedef struct rrc_cfg_qci_s{
  bool                          configured;//false
  int                           enb_dl_max_retx_thres;//-1
  struct ul_specific_params_lte_s_  lc_cfg;//ASN_RRC_LogicalChannelConfig_t
  struct pdcp_cfg_s             pdcp_cfg;
  struct rlc_cfg_c              rlc_cfg;
}rrc_cfg_qci_t;


// SystemInformationBlockType1
enum si_win_len_opts { ms1, ms2, ms5, ms10, ms15, ms20, ms40, nulltype };
enum cell_barred_opts { barred, not_barred, nulltype };
enum intra_freq_resel_opts { allowed, not_allowed, nulltype };
enum cell_reserved_for_oper_opts { reserved, not_reserved, nulltype };
enum si_periodicity_r12_opts { rf8, rf16, rf32, rf64, rf128, rf256, rf512, nulltype };
enum sf_assign_opts { sa0, sa1, sa2, sa3, sa4, sa5, sa6, nulltype };
enum special_sf_patterns_opts { ssp0, ssp1, ssp2, ssp3, ssp4, ssp5, ssp6, ssp7, ssp8, nulltype };
enum sib_type_opts {
  sib_type2,
  sib_type3,
  sib_type4,
  sib_type5,
  sib_type6,
  sib_type7,
  sib_type8,
  sib_type9,
  sib_type10,
  sib_type11,
  sib_type12_v920,
  sib_type13_v920,
  sib_type14_v1130,
  sib_type15_v1130,
  sib_type16_v1130,
  sib_type17_v1250,
  sib_type18_v1250,
  // ...
  sib_type19_v1250,
  sib_type20_v1310,
  sib_type21_v1430,
  sib_type24_v1530,
  sib_type25_v1530,
  sib_type26_v1530,
  nulltype
};

enum sib_info_types_opts {
  sib2,
  sib3,
  sib4,
  sib5,
  sib6,
  sib7,
  sib8,
  sib9,
  sib10,
  sib11,
  // ...
  sib12_v920,
  sib13_v920,
  sib14_v1130,
  sib15_v1130,
  sib16_v1130,
  sib17_v1250,
  sib18_v1250,
  sib19_v1250,
  sib20_v1310,
  sib21_v1430,
  sib24_v1530,
  sib25_v1530,
  sib26_v1530,
  nulltype
} ;

typedef uint8_t mcc_l[3];
typedef uint8_t mnc_l[3];

struct cell_sel_info_s_ {
  bool	  q_rx_lev_min_offset_present;//false
  int8_t  q_rx_lev_min;//-70
  uint8_t q_rx_lev_min_offset;//1
};

struct plmn_id_s {
  bool  mcc_present;
  mcc_l mcc;
  mnc_l mnc;
  uint8_t nof_mnc_digits;
};

struct plmn_id_info_lte_s {
  struct plmn_id_s                 plmn_id;
  enum cell_reserved_for_oper_opts cell_reserved_for_oper;
};

struct cell_access_related_info_s_ {
  bool				  csg_id_present;
  struct plmn_id_info_lte_s      plmn_id_list[6]; //PLMN-IdentityList ::= SEQUENCE (SIZE (1..6)) OF PLMN-IdentityInfo
  uint8_t             tac[2];//fixed_bitstring<16>
  uint8_t             cell_id[4];//fixed_bitstring<28>
  enum cell_barred_opts	  cell_barred;
  enum intra_freq_resel_opts intra_freq_resel;
  bool				  csg_ind;
  uint8_t             csg_id[4];//fixed_bitstring<27>
};

struct sched_info_lte_s {
  enum si_periodicity_r12_opts si_periodicity;
  int                          nof_sib_map;//0
  enum sib_type_opts           sib_map_info[32];//SIB-MappingInfo ::= SEQUENCE (SIZE (0..31)) OF SIB-Type
};

struct tdd_cfg_s {
  enum sf_assign_opts           sf_assign;
  enum special_sf_patterns_opts special_sf_patterns;
};
// SystemInformationBlockType
struct sib_type1_lte_s {
  bool                        p_max_present;//false
  bool                        tdd_cfg_present;//false
  bool                        non_crit_ext_present;//false
  struct cell_access_related_info_s_ cell_access_related_info;
  struct cell_sel_info_s_     cell_sel_info;
  int8_t                      p_max;//-30
  uint8_t                     freq_band_ind;//1
  uint8_t                     nof_sched_info;//1
  struct sched_info_lte_s     sched_info_list[32];//dyn_array<sched_info_s>;//SchedulingInfoList ::= SEQUENCE (SIZE (1..32)) OF SchedulingInfo
  struct tdd_cfg_s            tdd_cfg;
  enum si_win_len_opts        si_win_len;
  uint8_t                     sys_info_value_tag;//0
  //struct sib_type1_v890_ies_s non_crit_ext;
};


enum ac_barr_factor_opts { p00, p05, p10, p15, p20, p25, p30, p40, p50, p60, p70, p75, p80, p85, p90, p95, nulltype };
enum ac_barr_time_opts { s4, s8, s16, s32, s64, s128, s256, s512, nulltype };
enum ul_bw_opts { n6, n15, n25, n50, n75, n100, nulltype };
//enum time_align_timer_opts { sf500, sf750, sf1280, sf1920, sf2560, sf5120, sf10240, infinity, nulltype };

struct ac_barr_cfg_s {
  enum ac_barr_factor_opts  ac_barr_factor;
  enum ac_barr_time_opts    ac_barr_time;
  uint8_t ac_barr_for_special_ac;//fixed_bitstring<5>
};

struct ac_barr_info_s_ {
  bool			ac_barr_for_mo_sig_present;
  bool			ac_barr_for_mo_data_present;
  bool			ac_barr_for_emergency;
  struct ac_barr_cfg_s ac_barr_for_mo_sig;
  struct ac_barr_cfg_s ac_barr_for_mo_data;
};

struct freq_info_s_ {
  bool	   ul_carrier_freq_present;
  bool	   ul_bw_present;
  uint32_t ul_carrier_freq;
  enum ul_bw_opts ul_bw;
  uint8_t  add_spec_emission;//1
};

enum ul_cp_len_opts { len1, len2, nulltype };
//RadioResourceConfigCommonSIB
struct rr_cfg_common_sib_s {
  struct rach_cfg_common_lte_s    rach_cfg_common;
  struct bcch_cfg_s           bcch_cfg;
  struct pcch_cfg_lte_s           pcch_cfg;
  struct prach_cfg_sib_lte_s      prach_cfg;
  struct pdsch_cfg_common_lte_s   pdsch_cfg_common;
  struct pusch_cfg_common_lte_s   pusch_cfg_common;
  struct pucch_cfg_common_lte_s   pucch_cfg_common;
  struct srs_ul_cfg_common_lte_c  srs_ul_cfg_common;
  struct ul_pwr_ctrl_common_lte_s ul_pwr_ctrl_common;
  enum ul_cp_len_opts         ul_cp_len;
};

enum t300_opts { ms100, ms200, ms300, ms400, ms600, ms1000, ms1500, ms2000, nulltype };
enum t301_opts { ms100, ms200, ms300, ms400, ms600, ms1000, ms1500, ms2000, nulltype };
enum t310_opts { ms0, ms50, ms100, ms200, ms500, ms1000, ms2000, nulltype };
enum n310_opts { n1, n2, n3, n4, n6, n8, n10, n20, nulltype };
enum t311_opts { ms1000, ms3000, ms5000, ms10000, ms15000, ms20000, ms30000, nulltype };
enum n311_opts { n1, n2, n3, n4, n5, n6, n8, n10, nulltype };
enum t300_v1310_opts { ms2500, ms3000, ms3500, ms4000, ms5000, ms6000, ms8000, ms10000, nulltype };
enum t301_v1310_opts { ms2500, ms3000, ms3500, ms4000, ms5000, ms6000, ms8000, ms10000, nulltype };
enum t310_v1330_opts { ms4000, ms6000, nulltype };
enum t300_r15_opts { ms4000, ms6000, ms8000, ms10000, ms15000, ms25000, ms40000, ms60000, nulltype };
struct ue_timers_and_consts_lte_s {
  enum t300_opts t300;
  enum t301_opts t301;
  enum t310_opts t310;
  enum n310_opts n310;
  enum t311_opts t311;
  enum n311_opts n311;
  // ...
  // group 0
  bool          t300_v1310_present;
  bool          t301_v1310_present;
  enum t300_v1310_opts t300_v1310;
  enum t301_v1310_opts t301_v1310;
  // group 1
  bool          t310_v1330_present;
  enum t310_v1330_opts t310_v1330;
  // group 2
  bool        t300_r15_present;
  enum t300_r15_opts t300_r15;
};

enum radioframe_alloc_period_opts { n1, n2, n4, n8, n16, n32, nulltype };
// MBSFN-SubframeConfig
struct mbsfn_sf_cfg_s {
  enum radioframe_alloc_period_opts  radioframe_alloc_period;
  uint8_t                            radioframe_alloc_offset;
};

// SystemInformationBlockType2
struct sib_type2_lte_s {
  bool                   ac_barr_info_present;
  bool                   mbsfn_sf_cfg_list_present;
  struct ac_barr_info_s_ ac_barr_info;
  struct rr_cfg_common_sib_s  rr_cfg_common;
  struct ue_timers_and_consts_lte_s ue_timers_and_consts;
  struct freq_info_s_    freq_info;
  struct mbsfn_sf_cfg_s    mbsfn_sf_cfg_list[8];// MBSFN-SubframeConfigList ::= SEQUENCE (SIZE (1..8)) OF MBSFN-SubframeConfig
  enum time_align_timer_opts    time_align_timer_common;
};

enum t_eval_opts { s30, s60, s120, s180, s240, spare3, spare2, spare1, nulltype };
enum t_hyst_normal_opts { s30, s60, s120, s180, s240, spare3, spare2, spare1, nulltype };
// MobilityStateParameters
struct mob_state_params_s {
  enum t_eval_opts        t_eval;
  enum t_hyst_normal_opts t_hyst_normal;
  uint8_t          n_cell_change_medium;//1
  uint8_t          n_cell_change_high;//1
};

enum q_hyst_opts {
  db0,
  db1,
  db2,
  db3,
  db4,
  db5,
  db6,
  db8,
  db10,
  db12,
  db14,
  db16,
  db18,
  db20,
  db22,
  db24,
  nulltype
};
enum sf_medium_opts { db_minus6, db_minus4, db_minus2, db0, nulltype };
enum sf_high_opts { db_minus6, db_minus4, db_minus2, db0, nulltype };

struct q_hyst_sf_s_ {
  enum sf_medium_opts sf_medium;
  enum sf_high_opts   sf_high;
};

struct speed_state_resel_pars_s_ {
  struct mob_state_params_s mob_state_params;
  struct q_hyst_sf_s_	q_hyst_sf;
};

struct cell_resel_info_common_lte_s_ {
  bool						speed_state_resel_pars_present;
  enum q_hyst_opts			q_hyst;
  struct speed_state_resel_pars_s_ speed_state_resel_pars;
};

struct cell_resel_serving_freq_info_s_ {
  bool	  s_non_intra_search_present;
  uint8_t s_non_intra_search;
  uint8_t thresh_serving_low;
  uint8_t cell_resel_prio;
};

enum sf_medium_opts { odot25, odot5, odot75, ldot0, nulltype };
enum sf_high_opts { odot25, odot5, odot75, ldot0, nulltype };
// SpeedStateScaleFactors
struct speed_state_scale_factors_s {
  enum sf_medium_opts sf_medium;
  enum sf_high_opts   sf_high;
};

enum allowed_meas_bw_opts { mbw6, mbw15, mbw25, mbw50, mbw75, mbw100, nulltype };
struct intra_freq_cell_resel_info_s_ {
  bool						  p_max_present;
  bool						  s_intra_search_present;
  bool						  allowed_meas_bw_present;
  bool						  t_resel_eutra_sf_present;
  int8_t					  q_rx_lev_min;//-70
  int8_t					  p_max;//-30
  uint8_t					  s_intra_search;
  enum allowed_meas_bw_opts   allowed_meas_bw;
  bool						  presence_ant_port1;
  uint8_t					  neigh_cell_cfg;//fixed_bitstring<2>
  uint8_t					  t_resel_eutra;
  speed_state_scale_factors_s t_resel_eutra_sf;
};

// SystemInformationBlockType3
struct sib_type3_lte_s {
  struct cell_resel_info_common_lte_s_       cell_resel_info_common;
  struct cell_resel_serving_freq_info_s_ cell_resel_serving_freq_info;
  struct intra_freq_cell_resel_info_s_   intra_freq_cell_resel_info;
};


enum  codebook_subset_restrict_opts {
  n2_tx_ant_tm3,
  n4_tx_ant_tm3,
  n2_tx_ant_tm4,
  n4_tx_ant_tm4,
  n2_tx_ant_tm5,
  n4_tx_ant_tm5,
  n2_tx_ant_tm6,
  n4_tx_ant_tm6,
  nulltype
};
struct codebook_subset_restrict_c_ {
    enum  codebook_subset_restrict_opts     types;
	uint64_t c;

};

enum ue_tx_ant_sel_setup_opts { closed_loop, open_loop, nulltype };
struct ue_tx_ant_sel_c_ {
  enum setup_release_e    type;
  enum ue_tx_ant_sel_setup_opts    c;
};

enum tx_mode_opts { tm1, tm2, tm3, tm4, tm5, tm6, tm7, tm8_v920, nulltype };
// AntennaInfoDedicated
struct ant_info_ded_s {
  bool                        codebook_subset_restrict_present;
  enum tx_mode_opts           tx_mode;
  struct codebook_subset_restrict_c_ codebook_subset_restrict;
  struct ue_tx_ant_sel_c_            ue_tx_ant_sel;
};


enum subcarrier_spacing_e { khz15, khz30, khz60, khz120, khz240, spare3, spare2, spare1, nulltype };

// SCS-SpecificCarrier ::= SEQUENCE
struct scs_specific_carrier_s {
  uint16_t             offset_to_carrier;
  enum subcarrier_spacing_e subcarrier_spacing;
  uint16_t             carrier_bw;//1
  // ...
  // group 0
  bool     tx_direct_current_location_present;
  uint16_t tx_direct_current_location;
};

// NR-NS-PmaxValue ::= SEQUENCE
struct nr_ns_pmax_value_s {
  bool    add_pmax_present;
  int8_t  add_pmax;//-30
  uint8_t add_spec_emission;
};

// NR-MultiBandInfo ::= SEQUENCE
struct nr_multi_band_info_s {
  bool              freq_band_ind_nr_present;
  uint16_t          freq_band_ind_nr;//1
  struct nr_ns_pmax_value_s nr_ns_pmax_list[8];// NR-NS-PmaxList ::= SEQUENCE (SIZE (1..8)) OF NR-NS-PmaxValue
};


// FrequencyInfoDL-SIB ::= SEQUENCE
struct freq_info_dl_sib_s {
  // member variables
  cvector_vector_t(struct nr_multi_band_info_s) freq_band_list;//// MultiFrequencyBandListNR-SIB ::= SEQUENCE (SIZE (1..8)) OF NR-MultiBandInfo
  uint16_t                                    offset_to_point_a;
  cvector_vector_t(struct scs_specific_carrier_s)  scs_specific_carrier_list;// dyn_array
};

// BWP ::= SEQUENCE
struct bwp_s {
  bool                 cp_present;
  uint16_t             location_and_bw;
  enum subcarrier_spacing_e subcarrier_spacing;
};

// PDSCH-ConfigCommon ::= SEQUENCE
struct pdsch_cfg_common_s {
  cvector_vector_t(struct pdsch_time_domain_res_alloc_s) pdsch_time_domain_alloc_list;//dyn_array<pdsch_time_domain_res_alloc_s>
};

// BWP-DownlinkCommon ::= SEQUENCE
struct bwp_dl_common_s {
  bool                                pdcch_cfg_common_present;
  bool                                pdsch_cfg_common_present;
  struct bwp_s                        generic_params;
  setup_release_c(struct pdcch_cfg_common_s) pdcch_cfg_common;
  setup_release_c(struct pdsch_cfg_common_s) pdsch_cfg_common;
};

struct nand_paging_frame_offset_c_ {
  enum { one_t, half_t, quarter_t, one_eighth_t, one_sixteenth_t, nulltype }	 type_;
  int c;//pod_choice_buffer_t
};

enum ns_e_ { four, two, one, nulltype };

typedef uint8_t  scs15_kh_zone_t_l_[4];
typedef uint16_t scs30_kh_zone_t_scs15_kh_zhalf_t_l_[4];
typedef uint16_t scs60_kh_zone_t_scs30_kh_zhalf_t_scs15_kh_zquarter_t_l_[4];
typedef uint16_t scs120_kh_zone_t_scs60_kh_zhalf_t_scs30_kh_zquarter_t_scs15_kh_zone_eighth_t_l_[4];
typedef uint16_t scs120_kh_zhalf_t_scs60_kh_zquarter_t_scs30_kh_zone_eighth_t_scs15_kh_zone_sixteenth_t_l_[4];
typedef uint16_t scs120_kh_zquarter_t_scs60_kh_zone_eighth_t_scs30_kh_zone_sixteenth_t_l_[4];
typedef uint16_t scs120_kh_zone_eighth_t_scs60_kh_zone_sixteenth_t_l_[4];
typedef uint16_t scs120_kh_zone_sixteenth_t_l_[4];
struct first_pdcch_monitoring_occasion_of_po_c_ {
	enum types {
	  scs15_kh_zone_t,
	  scs30_kh_zone_t_scs15_kh_zhalf_t,
	  scs60_kh_zone_t_scs30_kh_zhalf_t_scs15_kh_zquarter_t,
	  scs120_kh_zone_t_scs60_kh_zhalf_t_scs30_kh_zquarter_t_scs15_kh_zone_eighth_t,
	  scs120_kh_zhalf_t_scs60_kh_zquarter_t_scs30_kh_zone_eighth_t_scs15_kh_zone_sixteenth_t,
	  scs120_kh_zquarter_t_scs60_kh_zone_eighth_t_scs30_kh_zone_sixteenth_t,
	  scs120_kh_zone_eighth_t_scs60_kh_zone_sixteenth_t,
	  scs120_kh_zone_sixteenth_t,
	  nulltype
	} type_;
  union {
		  scs120_kh_zhalf_t_scs60_kh_zquarter_t_scs30_kh_zone_eighth_t_scs15_kh_zone_sixteenth_t_l_ scs15_kh_zone_t;
		  scs120_kh_zone_eighth_t_scs60_kh_zone_sixteenth_t_l_ scs30_kh_zone_t_scs15_kh_zhalf;
		  scs120_kh_zone_sixteenth_t_l_ scs60_kh_zone_t_scs30_kh_zhalf_t_scs15_kh_zquarter;
		  scs120_kh_zone_t_scs60_kh_zhalf_t_scs30_kh_zquarter_t_scs15_kh_zone_eighth_t_l_ scs120_kh_zone_t_scs60_kh_zhalf_t_scs30_kh_zquarter_t_scs15_kh_zone_eighth;
		  scs120_kh_zquarter_t_scs60_kh_zone_eighth_t_scs30_kh_zone_sixteenth_t_l_ scs120_kh_zhalf_t_scs60_kh_zquarter_t_scs30_kh_zone_eighth_t_scs15_kh_zone_sixteenth;
		  scs15_kh_zone_t_l_ scs120_kh_zquarter_t_scs60_kh_zone_eighth_t_scs30_kh_zone_sixteenth;
		  scs30_kh_zone_t_scs15_kh_zhalf_t_l_ scs120_kh_zone_eighth_t_scs60_kh_zone_sixteenth;
		  scs60_kh_zone_t_scs30_kh_zhalf_t_scs15_kh_zquarter_t_l_ scs120_kh_zone_sixteenth;
	  }c;//choice_buffer_t
};

// PagingCycle ::= ENUMERATED
enum paging_cycle_e { rf32, rf64, rf128, rf256, nulltype };

// PCCH-Config ::= SEQUENCE
struct pcch_cfg_s {
  // member variables
  bool                                           first_pdcch_monitoring_occasion_of_po_present;
  enum  paging_cycle_e                           default_paging_cycle;
  struct nand_paging_frame_offset_c_             nand_paging_frame_offset;
  enum ns_e_                                     ns;
  struct first_pdcch_monitoring_occasion_of_po_c_ first_pdcch_monitoring_occasion_of_po;
};


// DownlinkConfigCommonSIB ::= SEQUENCE
struct dl_cfg_common_sib_s {
  struct freq_info_dl_sib_s freq_info_dl;
  struct bwp_dl_common_s    init_dl_bwp;
  struct bcch_cfg_s         bcch_cfg;
  struct pcch_cfg_s         pcch_cfg;
};

// FrequencyInfoUL-SIB ::= SEQUENCE
struct freq_info_ul_sib_s {
  // member variables
  bool                          absolute_freq_point_a_present;
  bool                          p_max_present;
  bool                          freq_shift7p5khz_present;
  cvector_vector_t(struct nr_multi_band_info_s)   freq_band_list;
  uint32_t                      absolute_freq_point_a;
  cvector_vector_t(struct scs_specific_carrier_s) scs_specific_carrier_list;//???5
  int8_t                        p_max;//-30
};

enum one_eighth_e_ { n4, n8, n12, n16, n20, n24, n28, n32, n36, n40, n44, n48, n52, n56, n60, n64, nulltype };
enum one_fourth_e_ { n4, n8, n12, n16, n20, n24, n28, n32, n36, n40, n44, n48, n52, n56, n60, n64, nulltype };
enum one_half_e_ { n4, n8, n12, n16, n20, n24, n28, n32, n36, n40, n44, n48, n52, n56, n60, n64, nulltype };
enum one_e_ { n4, n8, n12, n16, n20, n24, n28, n32, n36, n40, n44, n48, n52, n56, n60, n64, nulltype };
enum two_e_ { n4, n8, n12, n16, n20, n24, n28, n32, nulltype };
struct ssb_per_rach_occasion_and_cb_preambs_per_ssb_c_ {
  enum types { one_eighth, one_fourth, one_half, one, two, four, eight, sixteen, nulltype }    type_;
  int		c;//pod_choice_buffer_t
};

enum ra_msg3_size_group_a_e_ {
  b56,
  b144,
  b208,
  b256,
  b282,
  b480,
  b640,
  b800,
  b1000,
  b72,
  spare6,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
};
enum msg_pwr_offset_group_b_e_ { minusinfinity, db0, db5, db8, db10, db12, db15, db18, nulltype };
struct group_bcfgured_s_ {
  // member variables
  enum ra_msg3_size_group_a_e_	 ra_msg3_size_group_a;
  enum msg_pwr_offset_group_b_e_ msg_pwr_offset_group_b;
  uint8_t					nof_ra_preambs_group_a;// = 1
};

enum ra_contention_resolution_timer_e_ { sf8, sf16, sf24, sf32, sf40, sf48, sf56, sf64, nulltype };
enum prach_root_seq_idx_e_ { l839, l139, nulltype };
struct prach_root_seq_idx_c_ {
  enum prach_root_seq_idx_e_ type_;
  uint32_t  c;
};

enum restricted_set_cfg_e_ { unrestricted_set, restricted_set_type_a, restricted_set_type_b, nulltype };

// RACH-ConfigCommon ::= SEQUENCE
struct rach_cfg_common_s {
  // member variables
  bool                                            total_nof_ra_preambs_present;
  bool                                            ssb_per_rach_occasion_and_cb_preambs_per_ssb_present;
  bool                                            group_bcfgured_present;
  bool                                            rsrp_thres_ssb_present;
  bool                                            rsrp_thres_ssb_sul_present;
  bool                                            msg1_subcarrier_spacing_present;
  bool                                            msg3_transform_precoder_present;
  struct rach_cfg_generic_s                       rach_cfg_generic;
  uint8_t                                         total_nof_ra_preambs;// = 1
  struct ssb_per_rach_occasion_and_cb_preambs_per_ssb_c_ ssb_per_rach_occasion_and_cb_preambs_per_ssb;
  struct group_bcfgured_s_                          group_bcfgured;
  enum ra_contention_resolution_timer_e_            ra_contention_resolution_timer;
  uint8_t                                           rsrp_thres_ssb;
  uint8_t                                           rsrp_thres_ssb_sul;
  struct prach_root_seq_idx_c_                      prach_root_seq_idx;
  enum subcarrier_spacing_e                         msg1_subcarrier_spacing;
  enum restricted_set_cfg_e_                        restricted_set_cfg;
};

// PUSCH-ConfigCommon ::= SEQUENCE
struct pusch_cfg_common_s {
  bool                               group_hop_enabled_transform_precoding_present;
  bool                               msg3_delta_preamb_present;
  bool                               p0_nominal_with_grant_present;
  cvector_vector_t(struct pusch_time_domain_res_alloc_s) pusch_time_domain_alloc_list;//dyn_array<pusch_time_domain_res_alloc_s>;
  int8_t                             msg3_delta_preamb;
  int16_t                            p0_nominal_with_grant;
};

enum pucch_group_hop_e_ { neither, enable, disable, nulltype };
// PUCCH-ConfigCommon ::= SEQUENCE
struct pucch_cfg_common_s {
  // member variables
  bool               pucch_res_common_present;
  bool               hop_id_present;
  bool               p0_nominal_present;
  uint8_t            pucch_res_common;
  enum pucch_group_hop_e_ pucch_group_hop;
  uint16_t           hop_id;
  int16_t            p0_nominal;
};

// BWP-UplinkCommon ::= SEQUENCE
struct bwp_ul_common_s {
  bool                       rach_cfg_common_present;
  bool                       pusch_cfg_common_present;
  bool                       pucch_cfg_common_present;
  struct bwp_s               generic_params;
  setup_release_c(struct rach_cfg_common_s)   rach_cfg_common;
  setup_release_c(struct pusch_cfg_common_s)  pusch_cfg_common;
  setup_release_c(struct pucch_cfg_common_s)  pucch_cfg_common;
};

enum time_align_timer_e { ms500, ms750, ms1280, ms1920, ms2560, ms5120, ms10240, infinity, nulltype };

// UplinkConfigCommonSIB ::= SEQUENCE
struct ul_cfg_common_sib_s {
  struct freq_info_ul_sib_s freq_info_ul;
  struct bwp_ul_common_s    init_ul_bwp;
  enum time_align_timer_e time_align_timer_common;
};

enum milli_seconds_e_ {
	ms1,
	ms2,
	ms3,
	ms4,
	ms5,
	ms6,
	ms8,
	ms10,
	ms20,
	ms30,
	ms40,
	ms50,
	ms60,
	ms80,
	ms100,
	ms200,
	ms300,
	ms400,
	ms500,
	ms600,
	ms800,
	ms1000,
	ms1200,
	ms1600,
	spare8,
	spare7,
	spare6,
	spare5,
	spare4,
	spare3,
	spare2,
	spare1,
	nulltype
};

struct drx_on_dur_timer_c_ {
  enum types { sub_milli_seconds, milli_seconds, nulltype } type_;
  void      *c;//pod_choice_buffer_t
};

enum drx_inactivity_timer_e_ {
	ms0,
	ms1,
	ms2,
	ms3,
	ms4,
	ms5,
	ms6,
	ms8,
	ms10,
	ms20,
	ms30,
	ms40,
	ms50,
	ms60,
	ms80,
	ms100,
	ms200,
	ms300,
	ms500,
	ms750,
	ms1280,
	ms1920,
	ms2560,
	spare9,
	spare8,
	spare7,
	spare6,
	spare5,
	spare4,
	spare3,
	spare2,
	spare1,
	nulltype
};

enum drx_retx_timer_dl_e_ {
	sl0,
	sl1,
	sl2,
	sl4,
	sl6,
	sl8,
	sl16,
	sl24,
	sl33,
	sl40,
	sl64,
	sl80,
	sl96,
	sl112,
	sl128,
	sl160,
	sl320,
	spare15,
	spare14,
	spare13,
	spare12,
	spare11,
	spare10,
	spare9,
	spare8,
	spare7,
	spare6,
	spare5,
	spare4,
	spare3,
	spare2,
	spare1,
	nulltype
};

enum drx_retx_timer_ul_e_ {
	sl0,
	sl1,
	sl2,
	sl4,
	sl6,
	sl8,
	sl16,
	sl24,
	sl33,
	sl40,
	sl64,
	sl80,
	sl96,
	sl112,
	sl128,
	sl160,
	sl320,
	spare15,
	spare14,
	spare13,
	spare12,
	spare11,
	spare10,
	spare9,
	spare8,
	spare7,
	spare6,
	spare5,
	spare4,
	spare3,
	spare2,
	spare1,
	nulltype
};

struct drx_long_cycle_start_offset_c_ {
	enum types {
	  ms10,
	  ms20,
	  ms32,
	  ms40,
	  ms60,
	  ms64,
	  ms70,
	  ms80,
	  ms128,
	  ms160,
	  ms256,
	  ms320,
	  ms512,
	  ms640,
	  ms1024,
	  ms1280,
	  ms2048,
	  ms2560,
	  ms5120,
	  ms10240,
	  nulltype
	}type_;
   void *c;//pod_choice_buffer_t
};

enum drx_short_cycle_e_ {
  ms2,
  ms3,
  ms4,
  ms5,
  ms6,
  ms7,
  ms8,
  ms10,
  ms14,
  ms16,
  ms20,
  ms30,
  ms32,
  ms35,
  ms40,
  ms64,
  ms80,
  ms128,
  ms160,
  ms256,
  ms320,
  ms512,
  ms640,
  spare9,
  spare8,
  spare7,
  spare6,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
};

struct short_drx_s_ {

  // member variables
  enum drx_short_cycle_e_ drx_short_cycle;
  uint8_t			      drx_short_cycle_timer;//1
};


// DRX-Config ::= SEQUENCE
struct drx_cfg_s{
  // member variables
  bool                           short_drx_present;
  struct drx_on_dur_timer_c_     drx_on_dur_timer;
  enum drx_inactivity_timer_e_   drx_inactivity_timer;
  uint8_t                        drx_harq_rtt_timer_dl;
  uint8_t                        drx_harq_rtt_timer_ul;
  enum drx_retx_timer_dl_e_             drx_retx_timer_dl;
  enum drx_retx_timer_ul_e_             drx_retx_timer_ul;
  struct drx_long_cycle_start_offset_c_ drx_long_cycle_start_offset;
  struct short_drx_s_                   short_drx;
  uint8_t                               drx_slot_offset;
};

enum sr_prohibit_timer_e_ { ms1, ms2, ms4, ms8, ms16, ms32, ms64, ms128, nulltype };
enum sr_trans_max_e_ { n4, n8, n16, n32, n64, spare3, spare2, spare1, nulltype };

// SchedulingRequestToAddMod ::= SEQUENCE
struct sched_request_to_add_mod_s {
  // member variables
  bool                 sr_prohibit_timer_present;
  uint8_t              sched_request_id;
  enum sr_prohibit_timer_e_ sr_prohibit_timer;
  enum sr_trans_max_e_      sr_trans_max;
};


// SchedulingRequestConfig ::= SEQUENCE
struct sched_request_cfg_s {
  // member variables
  cvector_vector_t(struct sched_request_to_add_mod_s) sched_request_to_add_mod_list;//dyn_array<sched_request_to_add_mod_s>
  cvector_vector_t(uint8_t) sched_request_to_release_list;//bounded_array<uint8_t, 8>
};

enum periodic_bsr_timer_e_ {
  sf1,
  sf5,
  sf10,
  sf16,
  sf20,
  sf32,
  sf40,
  sf64,
  sf80,
  sf128,
  sf160,
  sf320,
  sf640,
  sf1280,
  sf2560,
  infinity,
  nulltype
};

enum retx_bsr_timer_e_ {
  sf10,
  sf20,
  sf40,
  sf80,
  sf160,
  sf320,
  sf640,
  sf1280,
  sf2560,
  sf5120,
  sf10240,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
};

enum lc_ch_sr_delay_timer_e_ { sf20, sf40, sf64, sf128, sf512, sf1024, sf2560, spare1, nulltype };

// BSR-Config ::= SEQUENCE
struct bsr_cfg_s {
  // member variables
  bool                         lc_ch_sr_delay_timer_present;
  enum periodic_bsr_timer_e_   periodic_bsr_timer;
  enum retx_bsr_timer_e_       retx_bsr_timer;
  enum lc_ch_sr_delay_timer_e_ lc_ch_sr_delay_timer;
};


// TAG ::= SEQUENCE
struct tag_s {
  uint8_t            tag_id;
  enum time_align_timer_e time_align_timer;
};

// TAG-Config ::= SEQUENCE
struct tag_cfg_s {
  // member variables
  cvector_vector_t(uint8_t)        tag_to_release_list;//bounded_array<uint8_t, 4>
  cvector_vector_t(struct tag_s)   tag_to_add_mod_list;// dyn_array<tag_s>
};


enum phr_periodic_timer_e_ { sf10, sf20, sf50, sf100, sf200, sf500, sf1000, infinity, nulltype };
enum phr_prohibit_timer_e_ { sf0, sf10, sf20, sf50, sf100, sf200, sf500, sf1000, nulltype };
enum phr_tx_pwr_factor_change_e_ { db1, db3, db6, infinity, nulltype };
enum phr_mode_other_cg_e_ { real, virtual_value, nulltype };

// PHR-Config ::= SEQUENCE
struct phr_cfg_s {
  // member variables
  enum  phr_periodic_timer_e_       phr_periodic_timer;
  enum  phr_prohibit_timer_e_       phr_prohibit_timer;
  enum  phr_tx_pwr_factor_change_e_ phr_tx_pwr_factor_change;
  bool                              multiple_phr;
  bool                              dummy;
  bool                              phr_type2_other_cell;
  enum  phr_mode_other_cg_e_        phr_mode_other_cg;
};


enum data_inactivity_timer_e { s1, s2, s3, s5, s7, s10, s15, s20, s40, s50, s60, s80, s100, s120, s150, s180, nulltype };

// MAC-CellGroupConfig ::= SEQUENCE
struct mac_cell_group_cfg_s {
  bool                       drx_cfg_present;
  bool                       sched_request_cfg_present;
  bool                       bsr_cfg_present;
  bool                       tag_cfg_present;
  bool                       phr_cfg_present;
  setup_release_c(struct drx_cfg_s)  drx_cfg;
  struct sched_request_cfg_s sched_request_cfg;
  struct bsr_cfg_s           bsr_cfg;
  struct tag_cfg_s           tag_cfg;
  setup_release_c(struct phr_cfg_s)   phr_cfg;
  bool                       skip_ul_tx_dynamic;
  // ...
  // group 0
  bool                                                csi_mask_present;
  bool                                                csi_mask;
  setup_release_c(enum data_inactivity_timer_e)       data_inactivity_timer;
};

enum pdsch_harq_ack_codebook_e_ { semi_static, dynamic_value, nulltype };
enum xscale_e_ { db0, db6, spare2, spare1, nulltype };

// PhysicalCellGroupConfig ::= SEQUENCE
struct phys_cell_group_cfg_s {
  // member variables
  bool                                          harq_ack_spatial_bundling_pucch_present;
  bool                                          harq_ack_spatial_bundling_pusch_present;
  bool                                          p_nr_fr1_present;
  bool                                          tpc_srs_rnti_present;
  bool                                          tpc_pucch_rnti_present;
  bool                                          tpc_pusch_rnti_present;
  bool                                          sp_csi_rnti_present;
  bool                                          cs_rnti_present;
  int8_t                                        p_nr_fr1;//=-30
  enum pdsch_harq_ack_codebook_e_               pdsch_harq_ack_codebook;
  uint32_t                                      tpc_srs_rnti;
  uint32_t                                      tpc_pucch_rnti;
  uint32_t                                      tpc_pusch_rnti;
  uint32_t                                      sp_csi_rnti;
  setup_release_c(uint32_t) cs_rnti;//0~65535
  // ...
  // group 0
  bool     mcs_c_rnti_present;
  bool     p_ue_fr1_present;
  uint32_t mcs_c_rnti;
  int8_t   p_ue_fr1;
  // group 1
  bool      xscale_present;
  enum xscale_e_ xscale;
  // group 2
  setup_release_c(uint8_t) pdcch_blind_detection;//1~15
};

enum t304_e_ { ms50, ms100, ms150, ms200, ms500, ms1000, ms2000, ms10000, nulltype };

enum msg1_fdm_e_ { one, two, four, eight, nulltype };
enum preamb_trans_max_e_ { n3, n4, n5, n6, n7, n8, n10, n20, n50, n100, n200, nulltype };
enum pwr_ramp_step_e_ { db0, db2, db4, db6, nulltype };
enum ra_resp_win_e_ { sl1, sl2, sl4, sl8, sl10, sl20, sl40, sl80, nulltype };

// RACH-ConfigGeneric ::= SEQUENCE
struct rach_cfg_generic_s {
  // member variables
  uint16_t            prach_cfg_idx;
  enum msg1_fdm_e_    msg1_fdm;
  uint16_t            msg1_freq_start;
  uint8_t             zero_correlation_zone_cfg;
  int16_t             preamb_rx_target_pwr;//-202
  enum preamb_trans_max_e_ preamb_trans_max;
  enum pwr_ramp_step_e_    pwr_ramp_step;
  enum ra_resp_win_e_      ra_resp_win;
};

enum ssb_per_rach_occasion_e_ { one_eighth, one_fourth, one_half, one, two, four, eight, sixteen, nulltype };
struct occasions_s_ {
  // member variables
  bool					   ssb_per_rach_occasion_present;
  struct rach_cfg_generic_s	   rach_cfg_generic;
  enum ssb_per_rach_occasion_e_ ssb_per_rach_occasion;
};

// CFRA-SSB-Resource ::= SEQUENCE
struct cfra_ssb_res_s {
  uint8_t ssb;
  uint8_t ra_preamb_idx;
};


struct ssb_s_ {
	// member variables
	cvector_vector_t(struct cfra_ssb_res_s)  ssb_res_list;//dyn_array<cfra_ssb_res_s>
	uint8_t 		ra_ssb_occasion_mask_idx;
};

struct cfra_csirs_res_s {
  // member variables
  uint8_t             csi_rs;
  cvector_vector_t(uint16_t) ra_occasion_list;//dyn_array<uint16_t>
  uint8_t                  ra_preamb_idx;
};

struct csirs_s_ {
	// member variables
	cvector_vector_t(struct cfra_csirs_res_s)  csirs_res_list;//dyn_array<cfra_csirs_res_s>
	uint8_t 		  rsrp_thres_csi_rs;
};

struct cfra_s {
	enum types { ssb, csirs, nulltype } type_;
	union {struct csirs_s_ csirs; struct ssb_s_ ssb;} c;
};


// CFRA ::= SEQUENCE
struct res_c_ {
  // member variables
  bool         occasions_present;
  struct occasions_s_ occasions;
  struct res_c_       res;
  // ...
  // group 0
  bool    total_nof_ra_preambs_present;
  uint8_t total_nof_ra_preambs;//1
};

enum pwr_ramp_step_high_prio_e_ { db0, db2, db4, db6, nulltype };
enum scaling_factor_bi_e_ { zero, dot25, dot5, dot75, nulltype };

// RA-Prioritization ::= SEQUENCE
struct ra_prioritization_s {
  // member variables
  bool                       scaling_factor_bi_present;
  enum pwr_ramp_step_high_prio_e_ pwr_ramp_step_high_prio;
  enum scaling_factor_bi_e_       scaling_factor_bi;
};

// RACH-ConfigDedicated ::= SEQUENCE
struct rach_cfg_ded_s {
  bool                cfra_present;
  bool                ra_prioritization_present;
  struct cfra_s       cfra;
  enum ra_prioritization_s ra_prioritization;
};

struct rach_cfg_ded_c_ {
  enum types { ul, supplementary_ul, nulltype } type_;
  struct rach_cfg_ded_s  c;
};

enum n_timing_advance_offset_e_ { n0, n25600, n39936, nulltype };
enum types { short_bitmap, medium_bitmap, long_bitmap, nulltype };
struct ssb_positions_in_burst_c_ {
  enum types { short_bitmap, medium_bitmap, long_bitmap, nulltype }  type_;
  uint8_t  c[8];//fixed_bitstring<64>
};

enum ssb_periodicity_serving_cell_e_ { ms5, ms10, ms20, ms40, ms80, ms160, spare2, spare1, nulltype };
enum dmrs_type_a_position_e_ { pos2, pos3, nulltype };

// FrequencyInfoDL ::= SEQUENCE
struct freq_info_dl_s {
  // member variables
  bool                         absolute_freq_ssb_present;
  uint32_t                     absolute_freq_ssb;
  cvector_vector_t(uint16_t)     freq_band_list;//bounded_array<uint16_t, 8>//// MultiFrequencyBandListNR ::= SEQUENCE (SIZE (1..8)) OF INTEGER (1..1024)
  uint32_t                     absolute_freq_point_a;
  cvector_vector_t(struct scs_specific_carrier_s)  scs_specific_carrier_list;//dyn_array<scs_specific_carrier_s>
};

// DownlinkConfigCommon ::= SEQUENCE
struct dl_cfg_common_s {
  bool            freq_info_dl_present;
  bool            init_dl_bwp_present;
  struct freq_info_dl_s  freq_info_dl;
  struct bwp_dl_common_s init_dl_bwp;
};

struct symbols_in_res_block_c_ {
  enum types { one_slot, two_slots, nulltype }	   type_;
  uint8_t	   c[4];//fixed_bitstring<28>
};

struct periodicity_and_pattern_c_ {
  enum types { n2, n4, n5, n8, n10, n20, n40, nulltype } type_;
  uint8_t c[5];//fixed_bitstring<40>
};

struct bitmaps_s_ {
  // member variables
  bool						 periodicity_and_pattern_present;
  uint8_t					 res_blocks[35];//fixed_bitstring<275>
  struct symbols_in_res_block_c_	symbols_in_res_block;
  struct periodicity_and_pattern_c_ periodicity_and_pattern;
};

struct pattern_type_c_ {
  enum types { bitmaps, ctrl_res_set, nulltype } type_;
  bitmaps_s_ c;
};

enum dummy_e_ { dynamic_value, semi_static, nulltype };

// RateMatchPattern ::= SEQUENCE
struct rate_match_pattern_s {
  // member variables
  bool                 subcarrier_spacing_present;
  uint8_t              rate_match_pattern_id;
  struct pattern_type_c_      pattern_type;
  enum subcarrier_spacing_e   subcarrier_spacing;
  enum dummy_e_               dummy;
};

// FrequencyInfoUL ::= SEQUENCE
struct freq_info_ul_s {
  // member variables
  bool                         absolute_freq_point_a_present;
  bool                         add_spec_emission_present;
  bool                         p_max_present;
  bool                         freq_shift7p5khz_present;
  cvector_vector_t(uint16_t)        freq_band_list;//bounded_array<uint16_t, 8>//// MultiFrequencyBandListNR ::= SEQUENCE (SIZE (1..8)) OF INTEGER (1..1024)
  uint32_t                        absolute_freq_point_a;
  cvector_vector_t(struct scs_specific_carrier_s)   scs_specific_carrier_list;//dyn_array<scs_specific_carrier_s>
  uint8_t                         add_spec_emission;
  int8_t                          p_max;//-30
};


// UplinkConfigCommon ::= SEQUENCE
struct ul_cfg_common_s {
  bool               freq_info_ul_present;
  bool               init_ul_bwp_present;
  struct freq_info_ul_s     freq_info_ul;
  struct bwp_ul_common_s    init_ul_bwp;
  enum time_align_timer_e   dummy;
};

enum dl_ul_tx_periodicity_e_ { ms0p5, ms0p625, ms1, ms1p25, ms2, ms2p5, ms5, ms10, nulltype };
enum dl_ul_tx_periodicity_v1530_e_ { ms3, ms4, nulltype };

// TDD-UL-DL-Pattern ::= SEQUENCE
struct tdd_ul_dl_pattern_s {
  // member variables
  enum dl_ul_tx_periodicity_e_ dl_ul_tx_periodicity;
  uint16_t                nrof_dl_slots;
  uint8_t                 nrof_dl_symbols;
  uint16_t                nrof_ul_slots;
  uint8_t                 nrof_ul_symbols;
  // group 0
  bool                               dl_ul_tx_periodicity_v1530_present;
  enum dl_ul_tx_periodicity_v1530_e_ dl_ul_tx_periodicity_v1530;
};

// TDD-UL-DL-ConfigCommon ::= SEQUENCE
struct tdd_ul_dl_cfg_common_s {
  bool                 pattern2_present;
  enum subcarrier_spacing_e   ref_subcarrier_spacing;
  struct tdd_ul_dl_pattern_s  pattern1;
  struct tdd_ul_dl_pattern_s  pattern2;
};

enum radioframe_alloc_period_e_ { n1, n2, n4, n8, n16, n32, nulltype };
struct sf_alloc1_c_ {
  enum types { one_frame, four_frames, nulltype } type_;
  uint8_t c[3];//fixed_bitstring<24>
};

struct sf_alloc2_c_ {
  enum types { one_frame, four_frames, nulltype }  type_;
  uint8_t c;//fixed_bitstring<8>
};

// EUTRA-MBSFN-SubframeConfig ::= SEQUENCE
struct eutra_mbsfn_sf_cfg_s {
  // member variables
  bool                       sf_alloc2_present;
  enum radioframe_alloc_period_e_ radioframe_alloc_period;
  uint8_t                         radioframe_alloc_offset;
  struct sf_alloc1_c_             sf_alloc1;
  struct sf_alloc2_c_             sf_alloc2;
};


enum carrier_bw_dl_e_ { n6, n15, n25, n50, n75, n100, spare2, spare1, nulltype };
enum nrof_crs_ports_e_ { n1, n2, n4, nulltype };
enum v_shift_e_ { n0, n1, n2, n3, n4, n5, nulltype };


// RateMatchPatternLTE-CRS ::= SEQUENCE
struct rate_match_pattern_lte_crs_s {
  // member variables
  uint16_t                  carrier_freq_dl;
  enum carrier_bw_dl_e_          carrier_bw_dl;
  cvector_vector_t(struct eutra_mbsfn_sf_cfg_s)    mbsfn_sf_cfg_list;//dyn_array<eutra_mbsfn_sf_cfg_s>//// EUTRA-MBSFN-SubframeConfigList ::= SEQUENCE (SIZE (1..8)) OF EUTRA-MBSFN-SubframeConfig
  enum nrof_crs_ports_e_         nrof_crs_ports;
  enum v_shift_e_                v_shift;
};

// ServingCellConfigCommon ::= SEQUENCE
struct serving_cell_cfg_common_s {
  // member variables
  bool                                          pci_present;
  bool                                          dl_cfg_common_present;
  bool                                          ul_cfg_common_present;
  bool                                          supplementary_ul_cfg_present;
  bool                                          n_timing_advance_offset_present;
  bool                                          ssb_positions_in_burst_present;
  bool                                          ssb_periodicity_serving_cell_present;
  bool                                          lte_crs_to_match_around_present;
  bool                                          ssb_subcarrier_spacing_present;
  bool                                          tdd_ul_dl_cfg_common_present;
  uint16_t                                      pci;
  struct dl_cfg_common_s                        dl_cfg_common;
  struct ul_cfg_common_s                        ul_cfg_common;
  struct ul_cfg_common_s                        supplementary_ul_cfg;
  enum n_timing_advance_offset_e_               n_timing_advance_offset;
  struct ssb_positions_in_burst_c_              ssb_positions_in_burst;
  enum ssb_periodicity_serving_cell_e_          ssb_periodicity_serving_cell;
  enum dmrs_type_a_position_e_                  dmrs_type_a_position;
  setup_release_c(struct rate_match_pattern_lte_crs_s) lte_crs_to_match_around;
  cvector_vector_t(struct rate_match_pattern_s)    rate_match_pattern_to_add_mod_list;//dyn_array<rate_match_pattern_s>
  cvector_vector_t(uint8_t)                       rate_match_pattern_to_release_list;//bounded_array<uint8_t, 4>
  enum subcarrier_spacing_e                     ssb_subcarrier_spacing;
  struct tdd_ul_dl_cfg_common_s                 tdd_ul_dl_cfg_common;
  int8_t                                        ss_pbch_block_pwr;//-60
};


struct periodicity_and_offset_c_ {
  enum types { sf5, sf10, sf20, sf40, sf80, sf160, nulltype }	type_;
  void *c;//pod_choice_buffer_t
};

enum dur_e_ { sf1, sf2, sf3, sf4, sf5, nulltype };

// SSB-MTC ::= SEQUENCE
struct ssb_mtc_s {
  // member variables
  struct periodicity_and_offset_c_ periodicity_and_offset;
  enum dur_e_                      dur;
};

// ReconfigurationWithSync ::= SEQUENCE
struct recfg_with_sync_s {
  // member variables
  bool                      sp_cell_cfg_common_present;
  bool                      rach_cfg_ded_present;
  struct serving_cell_cfg_common_s sp_cell_cfg_common;
  uint32_t                  new_ue_id;//0
  enum t304_e_              t304;
  struct rach_cfg_ded_c_    rach_cfg_ded;
  // ...
  // group 0
  struct ssb_mtc_s          smtc;
};

enum t310_e_ { ms0, ms50, ms100, ms200, ms500, ms1000, ms2000, ms4000, ms6000, nulltype };
enum n310_e_ { n1, n2, n3, n4, n6, n8, n10, n20, nulltype };
enum n311_e_ { n1, n2, n3, n4, n5, n6, n8, n10, nulltype };
enum t311_e_ { ms1000, ms3000, ms5000, ms10000, ms15000, ms20000, ms30000, nulltype };

// RLF-TimersAndConstants ::= SEQUENCE
struct rlf_timers_and_consts_s {
  // member variables
  enum t310_e_ t310;
  enum n310_e_ n310;
  enum n311_e_ n311;
  // group 0
  enum t311_e_ t311;
};

enum bwp_inactivity_timer_e_ {
  ms2,
  ms3,
  ms4,
  ms5,
  ms6,
  ms8,
  ms10,
  ms20,
  ms30,
  ms40,
  ms50,
  ms60,
  ms80,
  ms100,
  ms200,
  ms300,
  ms500,
  ms750,
  ms1280,
  ms1920,
  ms2560,
  spare10,
  spare9,
  spare8,
  spare7,
  spare6,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
} ;

enum scell_deactivation_timer_e_ {
  ms20,
  ms40,
  ms80,
  ms160,
  ms200,
  ms240,
  ms320,
  ms400,
  ms480,
  ms520,
  ms640,
  ms720,
  ms840,
  ms1280,
  spare2,
  spare1,
  nulltype
};

struct explicit_s_ {
  bool	  nrof_dl_symbols_present;
  bool	  nrof_ul_symbols_present;
  uint8_t nrof_dl_symbols;//1
  uint8_t nrof_ul_symbols;//1
};

struct symbols_c_ {
  enum types { all_dl, all_ul, explicit_type, nulltype }	   type_;
  struct explicit_s_ c;
};

// TDD-UL-DL-SlotConfig ::= SEQUENCE
struct tdd_ul_dl_slot_cfg_s {
  // member variables
  uint16_t   slot_idx;
  struct symbols_c_ symbols;
};


// TDD-UL-DL-ConfigDedicated ::= SEQUENCE
struct tdd_ul_dl_cfg_ded_s {
  // member variables
  cvector_vector_t(struct tdd_ul_dl_slot_cfg_s)   slot_specific_cfgs_to_add_mod_list;//dyn_array<tdd_ul_dl_slot_cfg_s>
  cvector_vector_t(struct uint16_t)               slot_specific_cfgs_to_release_list;//dyn_array<uint16_t>
};

enum vrb_to_prb_interleaver_e_ { n2, n4, nulltype };
enum res_alloc_e_ { res_alloc_type0, res_alloc_type1, dynamic_switch, nulltype };
enum pdsch_aggregation_factor_e_ { n2, n4, n8, nulltype };

enum rbg_size_e_ { cfg1, cfg2, nulltype };
enum mcs_table_e_ { qam256, qam64_low_se, nulltype };
enum max_nrof_code_words_sched_by_dci_e_ { n1, n2, nulltype };
enum bundle_size_e_ { n4, wideband, nulltype };

struct static_bundling_s_ {
  // member variables
  bool			 bundle_size_present;
  enum bundle_size_e_ bundle_size;
};

enum bundle_size_set1_e_ { n4, wideband, n2_wideband, n4_wideband, nulltype };
enum bundle_size_set2_e_ { n4, wideband, nulltype };

struct dynamic_bundling_s_ {
  // member variables
  bool				  bundle_size_set1_present;
  bool				  bundle_size_set2_present;
  enum bundle_size_set1_e_ bundle_size_set1;
  enum bundle_size_set2_e_ bundle_size_set2;
};

enum prb_bundling_types { static_bundling, dynamic_bundling, nulltype };
struct prb_bundling_type_c_ {
  enum prb_bundling_types type_;
  union {
		 struct dynamic_bundling_s_ dynamic;
		 struct static_bundling_s_ static;
		 } c;
};



enum res_elem_offset_e_ { offset01, offset10, offset11, nulltype };
// PTRS-DownlinkConfig ::= SEQUENCE
struct ptrs_dl_cfg_s {
  // member variables
  bool               freq_density_present;
  bool               time_density_present;
  bool               epre_ratio_present;
  bool               res_elem_offset_present;
  uint16_t           freq_density[2];//std::array<uint16_t, 2>
  uint8_t            time_density[3];//std::array<uint8_t, 3>
  uint8_t                   epre_ratio;
  enum res_elem_offset_e_   res_elem_offset;
};

enum dmrs_add_position_e_ { pos0, pos1, pos3, nulltype };

// DMRS-DownlinkConfig ::= SEQUENCE
struct dmrs_dl_cfg_s {
  // member variables
  bool                           dmrs_type_present;
  bool                           dmrs_add_position_present;
  bool                           max_len_present;
  bool                           scrambling_id0_present;
  bool                           scrambling_id1_present;
  bool                           phase_tracking_rs_present;
  enum dmrs_add_position_e_      dmrs_add_position;
  uint32_t                       scrambling_id0;
  uint32_t                       scrambling_id1;
  setup_release_c(struct ptrs_dl_cfg_s)  phase_tracking_rs;
};

enum qcl_ref_sig_types {nothing, csi_rs, ssb, nulltype };
struct qcl_ref_sig_c_ {
	enum qcl_ref_sig_types type_;
 	int  c;//pod_choice_buffer_t
};

enum qcl_type_e_{ type_a, type_b, type_c, type_d, nulltype };

// QCL-Info ::= SEQUENCE
struct qcl_info_s {
  // member variables
  bool        cell_present;
  bool        bwp_id_present;
  uint8_t     cell;
  uint8_t     bwp_id;
  struct qcl_ref_sig_c_  ref_sig;
  enum qcl_type_e_   qcl_type;
};


// TCI-State ::= SEQUENCE
struct tci_state_s {
  bool       qcl_type2_present;
  uint8_t    tci_state_id;
  struct qcl_info_s qcl_type1;
  struct qcl_info_s qcl_type2;
};

enum map_type_e_ { type_a, type_b, nulltype };
// PDSCH-TimeDomainResourceAllocation ::= SEQUENCE
struct pdsch_time_domain_res_alloc_s {
  bool        k0_present;
  uint8_t     k0;
  enum map_type_e_ map_type;
  uint8_t     start_symbol_and_len;
};

struct rate_match_pattern_group_item_c_ {
  enum types { cell_level, bwp_level, nulltype }   type_;
  void *c;//pod_choice_buffer_t
};

enum freq_domain_alloc_e_ {nothing, row1, row2, row4, other, nulltype };
struct freq_domain_alloc_c_ {
  enum freq_domain_alloc_e_  type_;
  uint8_t c[2];//choice_buffer_t//fixed_bitstring<12>
};
//fixed_bitstring<4>  set_row1();
//fixed_bitstring<12> set_row2();
//fixed_bitstring<3>  set_row4();
//fixed_bitstring<6>  set_other();

enum nrof_ports_e_ { p1, p2, p4, p8, p12, p16, p24, p32, nulltype };
enum cdm_type_e_ { no_cdm, fd_cdm2, cdm4_fd2_td2, cdm8_fd2_td4, nulltype };
enum dot5_e_ { even_prbs, odd_prbs, nulltype };
enum density_e_ { nothing, dot5, one, three, spare, nulltype };

struct density_c_ {
  enum density_e_ type_;
  enum dot5_e_ c;
};

// CSI-FrequencyOccupation ::= SEQUENCE
struct csi_freq_occupation_s {
  uint16_t start_rb;//0
  uint16_t nrof_rbs;//24
};


// CSI-RS-ResourceMapping ::= SEQUENCE
struct csi_rs_res_map_s {
  // member variables
  bool                  first_ofdm_symbol_in_time_domain2_present;
  struct freq_domain_alloc_c_  freq_domain_alloc;
  enum nrof_ports_e_           nrof_ports;
  uint8_t                      first_ofdm_symbol_in_time_domain;//0
  uint8_t                      first_ofdm_symbol_in_time_domain2;//2
  enum cdm_type_e_             cdm_type;
  struct density_c_            density;
  struct csi_freq_occupation_s freq_band;
};

enum csi_res_periodicity_and_offset_type_e {
  nothing,
  slots4,
  slots5,
  slots8,
  slots10,
  slots16,
  slots20,
  slots32,
  slots40,
  slots64,
  slots80,
  slots160,
  slots320,
  slots640,
  nulltype
};

// CSI-ResourcePeriodicityAndOffset ::= CHOICE
struct csi_res_periodicity_and_offset_c {
    enum csi_res_periodicity_and_offset_type_e type_;
    int   c;//pod_choice_buffer_t
};

// ZP-CSI-RS-Resource ::= SEQUENCE
struct zp_csi_rs_res_s {
  bool                             periodicity_and_offset_present;
  uint8_t                          zp_csi_rs_res_id;
  struct csi_rs_res_map_s          res_map;
  csi_res_periodicity_and_offset_c periodicity_and_offset;
};


// ZP-CSI-RS-ResourceSet ::= SEQUENCE
struct zp_csi_rs_res_set_s {
  // member variables
  uint8_t                  zp_csi_rs_res_set_id;
  cvector_vector_t(uint8_t)  zp_csi_rs_res_id_list;//bounded_array<uint8_t, 16>
};

// PDSCH-Config ::= SEQUENCE
struct pdsch_cfg_s {
  // member variables
  bool                                                               data_scrambling_id_pdsch_present;
  bool                                                               dmrs_dl_for_pdsch_map_type_a_present;
  bool                                                               dmrs_dl_for_pdsch_map_type_b_present;
  bool                                                               vrb_to_prb_interleaver_present;
  bool                                                               pdsch_time_domain_alloc_list_present;
  bool                                                               pdsch_aggregation_factor_present;
  bool                                                               mcs_table_present;
  bool                                                               max_nrof_code_words_sched_by_dci_present;
  bool                                                               p_zp_csi_rs_res_set_present;
  uint16_t                                                           data_scrambling_id_pdsch;
  setup_release_c(struct dmrs_dl_cfg_s) dmrs_dl_for_pdsch_map_type_a;
  setup_release_c(struct dmrs_dl_cfg_s) dmrs_dl_for_pdsch_map_type_b;
  cvector_vector_t(struct tci_state_s)                                 tci_states_to_add_mod_list;
  cvector_vector_t(uint8_t)                                            tci_states_to_release_list;
  enum vrb_to_prb_interleaver_e_                                     vrb_to_prb_interleaver;
  enum res_alloc_e_                                                  res_alloc;
  setup_release_c(cvector_vector_t(struct pdsch_time_domain_res_alloc_s))  pdsch_time_domain_alloc_list;//dyn_seq_of<pdsch_time_domain_res_alloc_s, 1, 16>
  pdsch_aggregation_factor_e_                                        pdsch_aggregation_factor;
  cvector_vector_t(struct rate_match_pattern_s)                        rate_match_pattern_to_add_mod_list;
  cvector_vector_t(uint8_t)                                            rate_match_pattern_to_release_list;//bounded_array<uint8_t, 4>
  cvector_vector_t(struct rate_match_pattern_group_item_c_)            rate_match_pattern_group1;//dyn_array<rate_match_pattern_group_item_c_>//// RateMatchPatternGroup ::= SEQUENCE (SIZE (1..8)) OF RateMatchPatternGroup-item
  cvector_vector_t(struct rate_match_pattern_group_item_c_)            rate_match_pattern_group2;
  enum rbg_size_e_                                                   rbg_size;
  enum mcs_table_e_                                                  mcs_table;
  enum max_nrof_code_words_sched_by_dci_e_                           max_nrof_code_words_sched_by_dci;
  struct prb_bundling_type_c_                                        prb_bundling_type;
  cvector_vector_t(struct zp_csi_rs_res_s)                             zp_csi_rs_res_to_add_mod_list;
  cvector_vector_t(uint8_t)                                            zp_csi_rs_res_to_release_list;//bounded_array<uint8_t, 32>
  cvector_vector_t(struct zp_csi_rs_res_set_s)                         aperiodic_zp_csi_rs_res_sets_to_add_mod_list;
  cvector_vector_t(uint8_t)                                            aperiodic_zp_csi_rs_res_sets_to_release_list;//bounded_array<uint8_t, 16>
  cvector_vector_t(struct zp_csi_rs_res_set_s)                         sp_zp_csi_rs_res_sets_to_add_mod_list;
  cvector_vector_t(uint8_t)                                            sp_zp_csi_rs_res_sets_to_release_list;//bounded_array<uint8_t, 16>
  setup_release_c(struct zp_csi_rs_res_set_s)  p_zp_csi_rs_res_set;
};

enum periodicity_e_ {
  ms10,
  ms20,
  ms32,
  ms40,
  ms64,
  ms80,
  ms128,
  ms160,
  ms320,
  ms640,
  spare6,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
};

// SPS-Config ::= SEQUENCE
struct sps_cfg_s {
  // member variables
  bool           n1_pucch_an_present;
  bool           mcs_table_present;
  enum periodicity_e_ periodicity;
  uint8_t        nrof_harq_processes;//1
  uint8_t        n1_pucch_an;//0
};

enum purpose_e_ { beam_fail, rlf, both, nulltype };

struct detection_res_c_ {
  enum types { ssb_idx, csi_rs_idx, nulltype }	   type_;
  void *c;//pod_choice_buffer_t
};

// RadioLinkMonitoringRS ::= SEQUENCE
struct radio_link_monitoring_rs_s {
  // member variables
  uint8_t               radio_link_monitoring_rs_id;
  enum purpose_e_       purpose;
  struct detection_res_c_ detection_res;
};


enum beam_fail_instance_max_count_e_ { n1, n2, n3, n4, n5, n6, n8, n10, nulltype };
enum beam_fail_detection_timer_e_ { pbfd1, pbfd2, pbfd3, pbfd4, pbfd5, pbfd6, pbfd8, pbfd10, nulltype };

// RadioLinkMonitoringConfig ::= SEQUENCE
struct radio_link_monitoring_cfg_s {
  // member variables
  bool                                  beam_fail_instance_max_count_present;
  bool                                  beam_fail_detection_timer_present;
  cvector_vector_t(struct radio_link_monitoring_rs_s) fail_detection_res_to_add_mod_list;//dyn_array<radio_link_monitoring_rs_s>
  cvector_vector_t(uint8_t)                           fail_detection_res_to_release_list;//bounded_array<uint8_t, 10>
  enum beam_fail_instance_max_count_e_       beam_fail_instance_max_count;
  enum beam_fail_detection_timer_e_          beam_fail_detection_timer;
};


// BWP-DownlinkDedicated ::= SEQUENCE
struct bwp_dl_ded_s {
  bool                                         pdcch_cfg_present;
  bool                                         pdsch_cfg_present;
  bool                                         sps_cfg_present;
  bool                                         radio_link_monitoring_cfg_present;
  setup_release_c(struct pdcch_cfg_s)          pdcch_cfg;
  setup_release_c(struct pdsch_cfg_s)          pdsch_cfg;
  setup_release_c(struct sps_cfg_s)            sps_cfg;
  setup_release_c(struct radio_link_monitoring_cfg_s) radio_link_monitoring_cfg;
};

// BWP-Downlink ::= SEQUENCE
struct bwp_dl_s {
  bool            bwp_common_present;
  bool            bwp_ded_present;
  uint8_t         bwp_id;
  struct bwp_dl_common_s bwp_common;
  struct bwp_dl_ded_s    bwp_ded;
};

// PUCCH-ResourceSet ::= SEQUENCE
struct pucch_res_set_s {
  // member variables
  bool        max_payload_size_present;
  uint8_t     pucch_res_set_id;
  cvector_vector_t(uint8_t) res_list;//bounded_array<uint8_t, 32>
  uint16_t    max_payload_size;//4
};

// PUCCH-format0 ::= SEQUENCE
struct pucch_format0_s {
  uint8_t init_cyclic_shift;
  uint8_t nrof_symbols;//1
  uint8_t start_symbol_idx;
};

// PUCCH-format1 ::= SEQUENCE
struct pucch_format1_s {
  uint8_t init_cyclic_shift ;
  uint8_t nrof_symbols;//4
  uint8_t start_symbol_idx;
  uint8_t time_domain_occ;
};

// PUCCH-format2 ::= SEQUENCE
struct pucch_format2_s {
  uint8_t nrof_prbs;//1
  uint8_t nrof_symbols;//1
  uint8_t start_symbol_idx;
};

// PUCCH-format3 ::= SEQUENCE
struct pucch_format3_s {
  uint8_t nrof_prbs;//1
  uint8_t nrof_symbols;//4
  uint8_t start_symbol_idx;
};

enum occ_len_e_ { n2, n4, nulltype };
enum occ_idx_e_ { n0, n1, n2, n3, nulltype };

// PUCCH-format4 ::= SEQUENCE
struct pucch_format4_s {
  // member variables
  uint8_t    nrof_symbols;//4
  enum occ_len_e_ occ_len;
  enum occ_idx_e_ occ_idx;
  uint8_t    start_symbol_idx;
};

enum pucch_format_types {nothing, format0, format1, format2, format3, format4, nulltype };

struct format_c_ {
  enum pucch_format_types type_;
  union{struct pucch_format0_s f0; struct pucch_format1_s f1; struct pucch_format2_s f2; struct pucch_format3_s f3; struct  pucch_format4_s f4;} c;//choice_buffer_t
};

// PUCCH-Resource ::= SEQUENCE
struct pucch_res_s {
  // member variables
  bool      intra_slot_freq_hop_present;
  bool      second_hop_prb_present;
  uint8_t   pucch_res_id;
  uint16_t  start_prb;
  uint16_t  second_hop_prb;
  struct format_c_ format;
};

enum periodicity_and_offset_e_ {
   nothing,
   sym2,
   sym6or7,
   sl1,
   sl2,
   sl4,
   sl5,
   sl8,
   sl10,
   sl16,
   sl20,
   sl40,
   sl80,
   sl160,
   sl320,
   sl640,
   nulltype
 };

struct periodicity_and_offset_c_ {
    enum periodicity_and_offset_e_ type_;
    int  c;//pod_choice_buffer_t
};

// SchedulingRequestResourceConfig ::= SEQUENCE
struct sched_request_res_cfg_s {
	// member variables
	bool					  periodicity_and_offset_present;
	bool					  res_present;
	uint8_t 				  sched_request_res_id;//1
	uint8_t 				  sched_request_id;
	struct periodicity_and_offset_c_ periodicity_and_offset;
	uint8_t 				  res;
};

struct srs_s_ {
  uint8_t res;
  uint8_t ul_bwp;
};

struct ref_sig_c_ {
  enum types { ssb_idx, csi_rs_idx, srs, nulltype }  type_;
  struct srs_s_ c;//choice_buffer_t
};

enum closed_loop_idx_e_ { i0, i1, nulltype };

// PUCCH-SpatialRelationInfo ::= SEQUENCE
struct pucch_spatial_relation_info_s {
  // member variables
  bool               serving_cell_id_present;
  uint8_t            pucch_spatial_relation_info_id;//1
  uint8_t            serving_cell_id;
  struct ref_sig_c_  ref_sig;
  uint8_t            pucch_pathloss_ref_rs_id;
  uint8_t            p0_pucch_id;//1
  enum closed_loop_idx_e_ closed_loop_idx;
};

enum nrof_slots_e_ { n2, n4, n8, nulltype };
enum pucch_max_code_rate_e { zero_dot08, zero_dot15, zero_dot25, zero_dot35, zero_dot45, zero_dot60, zero_dot80, nulltype };

// PUCCH-FormatConfig ::= SEQUENCE
struct pucch_format_cfg_s {
  // member variables
  bool                  interslot_freq_hop_present;
  bool                  add_dmrs_present;
  bool                  max_code_rate_present;
  bool                  nrof_slots_present;
  bool                  pi2_bpsk_present;
  bool                  simul_harq_ack_csi_present;
  enum pucch_max_code_rate_e max_code_rate;
  enum nrof_slots_e_         nrof_slots;
};

// P0-PUCCH ::= SEQUENCE
struct p0_pucch_s {
  uint8_t p0_pucch_id;//1
  int8_t  p0_pucch_value;//-16
};

struct ref_sig_c_ {
  enum types { ssb_idx, csi_rs_idx, nulltype }	type_;
  void *c;//pod_choice_buffer_t
};

// PUCCH-PathlossReferenceRS ::= SEQUENCE
struct pucch_pathloss_ref_rs_s {
  // member variables
  uint8_t    pucch_pathloss_ref_rs_id;
  struct     ref_sig_c_ ref_sig;
};

// PUCCH-PowerControl ::= SEQUENCE
struct pucch_pwr_ctrl_s {
  // member variables
  bool                delta_f_pucch_f0_present;
  bool                delta_f_pucch_f1_present;
  bool                delta_f_pucch_f2_present;
  bool                delta_f_pucch_f3_present;
  bool                delta_f_pucch_f4_present;
  bool                two_pucch_pc_adjustment_states_present;
  int8_t              delta_f_pucch_f0;//-16
  int8_t              delta_f_pucch_f1;//-16
  int8_t              delta_f_pucch_f2;//-16
  int8_t              delta_f_pucch_f3;//-16
  int8_t              delta_f_pucch_f4;//-16
  cvector_vector_t(struct p0_pucch_s) p0_set;//dyn_array<p0_pucch_s>
  cvector_vector_t(struct pucch_pathloss_ref_rs_s) pathloss_ref_rss;//dyn_array<pucch_pathloss_ref_rs_s>
};


// PUCCH-Config ::= SEQUENCE
struct pucch_cfg_s {
  // member variables
  bool                                     format1_present;
  bool                                     format2_present;
  bool                                     format3_present;
  bool                                     format4_present;
  bool                                     pucch_pwr_ctrl_present;
  cvector_vector_t(struct pucch_res_set_s)   res_set_to_add_mod_list;//dyn_array<pucch_res_set_s>
  cvector_vector_t(uint8_t)                  res_set_to_release_list;//bounded_array<uint8_t, 4>
  cvector_vector_t(struct pucch_res_s)       res_to_add_mod_list;//dyn_array<pucch_res_s>
  cvector_vector_t(struct uint8_t)           res_to_release_list;//dyn_array<uint8_t>
  setup_release_c(pucch_format_cfg_s)      format1;
  setup_release_c(pucch_format_cfg_s)      format2;
  setup_release_c(pucch_format_cfg_s)      format3;
  setup_release_c(pucch_format_cfg_s)      format4;
  cvector_vector_t(struct sched_request_res_cfg_s)   sched_request_res_to_add_mod_list;//dyn_array<sched_request_res_cfg_s>
  cvector_vector_t(uint8_t)                          sched_request_res_to_release_list;//bounded_array<uint8_t, 8>
  cvector_vector_t(uint8_t)                          multi_csi_pucch_res_list;//bounded_array<uint8_t, 2>
  cvector_vector_t(uint8_t)                          dl_data_to_ul_ack;//bounded_array<uint8_t, 8>
  cvector_vector_t(struct pucch_spatial_relation_info_s)   spatial_relation_info_to_add_mod_list;//dyn_array<pucch_spatial_relation_info_s>
  cvector_vector_t(uint8_t)                                spatial_relation_info_to_release_list;//bounded_array<uint8_t, 8>
  struct pucch_pwr_ctrl_s                                pucch_pwr_ctrl;
};

enum tx_cfg_e_ { codebook, non_codebook, nulltype };
enum freq_hop_e_ { intra_slot, inter_slot, nulltype };
enum pusch_aggregation_factor_e_ { n2, n4, n8, nulltype };
//enum mcs_table_e_ { qam256, qam64_low_se, nulltype };
enum mcs_table_transform_precoder_e_ { qam256, qam64_low_se, nulltype };
enum transform_precoder_e_ { enabled, disabled, nulltype };
enum codebook_subset_e_ { fully_and_partial_and_non_coherent, partial_and_non_coherent, non_coherent, nulltype };


//enum dmrs_add_position_e_ { pos0, pos1, pos3, nulltype };
struct transform_precoding_disabled_s_ {
  bool	   scrambling_id0_present;
  bool	   scrambling_id1_present;
  uint32_t scrambling_id0;
  uint32_t scrambling_id1;
};

struct transform_precoding_enabled_s_ {
  bool	   npusch_id_present;
  bool	   seq_group_hop_present;
  bool	   seq_hop_present;
  uint16_t npusch_id;
};

enum max_nrof_ports_e_ { n1, n2, nulltype };
enum res_elem_offset_e_ { offset01, offset10, offset11, nulltype };
enum ptrs_pwr_e_ { p00, p01, p10, p11, nulltype };

struct transform_precoder_disabled_s_ {
  // member variables
  bool				 freq_density_present;
  bool				 time_density_present;
  bool				 res_elem_offset_present;
  uint16_t	         freq_density[2];//std::array<uint16_t, 2>
  uint8_t	         time_density[3];//std::array<uint8_t, 3>
  enum max_nrof_ports_e_  max_nrof_ports;
  enum res_elem_offset_e_ res_elem_offset;
  enum ptrs_pwr_e_		  ptrs_pwr;
};

struct transform_precoder_enabled_s_ {
  // member variables
  bool	   time_density_transform_precoding_present;
  uint16_t sample_density[5];//std::array<uint16_t, 5>
};

// PTRS-UplinkConfig ::= SEQUENCE
struct ptrs_ul_cfg_s {
  // member variables
  bool                           transform_precoder_disabled_present;
  bool                           transform_precoder_enabled_present;
  struct transform_precoder_disabled_s_ transform_precoder_disabled;
  struct transform_precoder_enabled_s_  transform_precoder_enabled;
};


// DMRS-UplinkConfig ::= SEQUENCE
struct dmrs_ul_cfg_s {
  // member variables
  bool                            dmrs_type_present;
  bool                            dmrs_add_position_present;
  bool                            phase_tracking_rs_present;
  bool                            max_len_present;
  bool                            transform_precoding_disabled_present;
  bool                            transform_precoding_enabled_present;
  enum dmrs_add_position_e_              dmrs_add_position;
  setup_release_c(struct ptrs_ul_cfg_s)  phase_tracking_rs;
  struct transform_precoding_disabled_s_ transform_precoding_disabled;
  struct transform_precoding_enabled_s_  transform_precoding_enabled;
};

enum sri_pusch_closed_loop_idx_e_ { i0, i1, nulltype };

// SRI-PUSCH-PowerControl ::= SEQUENCE
struct sri_pusch_pwr_ctrl_s {
  // member variables
  uint8_t                      sri_pusch_pwr_ctrl_id;
  uint8_t                      sri_pusch_pathloss_ref_rs_id;
  uint8_t                      sri_p0_pusch_alpha_set_id;
  enum sri_pusch_closed_loop_idx_e_ sri_pusch_closed_loop_idx;
};

enum alpha_e { alpha0, alpha04, alpha05, alpha06, alpha07, alpha08, alpha09, alpha1, nulltype };

// P0-PUSCH-AlphaSet ::= SEQUENCE
struct p0_pusch_alpha_set_s {
  bool    p0_present;
  bool    alpha_present;
  uint8_t p0_pusch_alpha_set_id;
  int8_t  p0;//-16
  enum alpha_e alpha;
};

// PUSCH-PathlossReferenceRS ::= SEQUENCE
struct pusch_pathloss_ref_rs_s {
  // member variables
  uint8_t           pusch_pathloss_ref_rs_id;
  struct ref_sig_c_ ref_sig;
};


// PUSCH-PowerControl ::= SEQUENCE
struct pusch_pwr_ctrl_s {
  // member variables
  bool                               tpc_accumulation_present;
  bool                               msg3_alpha_present;
  bool                               p0_nominal_without_grant_present;
  bool                               two_pusch_pc_adjustment_states_present;
  bool                               delta_mcs_present;
  alpha_e                            msg3_alpha;
  int16_t                            p0_nominal_without_grant;//-202
  cvector_vector_t(struct p0_pusch_alpha_set_s)    p0_alpha_sets;//dyn_array<p0_pusch_alpha_set_s>
  cvector_vector_t(struct pusch_pathloss_ref_rs_s) pathloss_ref_rs_to_add_mod_list;//dyn_array<pusch_pathloss_ref_rs_s>
  cvector_vector_t(uint8_t)                        pathloss_ref_rs_to_release_list;//bounded_array<uint8_t, 4>
  cvector_vector_t(struct sri_pusch_pwr_ctrl_s)    sri_pusch_map_to_add_mod_list;//dyn_array<sri_pusch_pwr_ctrl_s>
  cvector_vector_t(uint8_t)                        sri_pusch_map_to_release_list;//bounded_array<uint8_t, 16>
};

enum map_type_e_ { type_a, type_b, nulltype };

// PUSCH-TimeDomainResourceAllocation ::= SEQUENCE
struct pusch_time_domain_res_alloc_s {
  // member variables
  bool        k2_present;
  uint8_t     k2;
  enum map_type_e_ map_type;
  uint8_t     start_symbol_and_len;
};

// BetaOffsets ::= SEQUENCE
struct beta_offsets_s {
  bool    beta_offset_ack_idx1_present;
  bool    beta_offset_ack_idx2_present;
  bool    beta_offset_ack_idx3_present;
  bool    beta_offset_csi_part1_idx1_present;
  bool    beta_offset_csi_part1_idx2_present;
  bool    beta_offset_csi_part2_idx1_present;
  bool    beta_offset_csi_part2_idx2_present;
  uint8_t beta_offset_ack_idx1;
  uint8_t beta_offset_ack_idx2;
  uint8_t beta_offset_ack_idx3;
  uint8_t beta_offset_csi_part1_idx1;
  uint8_t beta_offset_csi_part1_idx2;
  uint8_t beta_offset_csi_part2_idx1;
  uint8_t beta_offset_csi_part2_idx2;
};

enum beta_offsets_types {nothing, dynamic_type, semi_static, nulltype };
struct beta_offsets_c_ {
  enum beta_offsets_types 	 type_;
  union{
		struct beta_offsets_s semi;
		struct beta_offsets_s dynamic[4];//std::array<beta_offsets_s, 4>
	   }c;//choice_buffer_t
};

enum scaling_e_ { f0p5, f0p65, f0p8, f1, nulltype };

// UCI-OnPUSCH ::= SEQUENCE
struct uci_on_pusch_s {
  // member variables
  bool            beta_offsets_present;
  struct beta_offsets_c_ beta_offsets;
  enum scaling_e_        scaling;
};

// PUSCH-Config ::= SEQUENCE
struct pusch_cfg_s {
  // member variables
  bool                                                               data_scrambling_id_pusch_present;
  bool                                                               tx_cfg_present;
  bool                                                               dmrs_ul_for_pusch_map_type_a_present;
  bool                                                               dmrs_ul_for_pusch_map_type_b_present;
  bool                                                               pusch_pwr_ctrl_present;
  bool                                                               freq_hop_present;
  bool                                                               pusch_time_domain_alloc_list_present;
  bool                                                               pusch_aggregation_factor_present;
  bool                                                               mcs_table_present;
  bool                                                               mcs_table_transform_precoder_present;
  bool                                                               transform_precoder_present;
  bool                                                               codebook_subset_present;
  bool                                                               max_rank_present;
  bool                                                               rbg_size_present;
  bool                                                               uci_on_pusch_present;
  bool                                                               tp_pi2_bpsk_present;
  uint16_t                                                           data_scrambling_id_pusch;
  enum tx_cfg_e_                                                        tx_cfg;
  setup_release_c(struct dmrs_ul_cfg_s)                                 dmrs_ul_for_pusch_map_type_a;
  setup_release_c(struct dmrs_ul_cfg_s)                                 dmrs_ul_for_pusch_map_type_b;
  struct pusch_pwr_ctrl_s                                               pusch_pwr_ctrl;
  enum freq_hop_e_                                                      freq_hop;
  cvector_vector_t(uint16_t)                                              freq_hop_offset_lists;//bounded_array<uint16_t, 4>
  enum res_alloc_e_                                                     res_alloc;
  setup_release_c(cvector_vector_t(struct pusch_time_domain_res_alloc_s)) pusch_time_domain_alloc_list;//dyn_seq_of<pusch_time_domain_res_alloc_s, 1, 16>
  enum pusch_aggregation_factor_e_                                      pusch_aggregation_factor;
  enum mcs_table_e_                                                     mcs_table;
  enum mcs_table_transform_precoder_e_                                  mcs_table_transform_precoder;
  enum transform_precoder_e_                                            transform_precoder;
  enum codebook_subset_e_                                               codebook_subset;
  uint8_t                                                               max_rank;//1
  setup_release_c(struct uci_on_pusch_s)                                uci_on_pusch;
};


enum freq_hop_e_ { intra_slot, inter_slot, nulltype };
//enum mcs_table_e_ { qam256, qam64_low_se, nulltype };
//enum mcs_table_transform_precoder_e_ { qam256, qam64_low_se, nulltype };
enum pwr_ctrl_loop_to_use_e_ { n0, n1, nulltype };
enum transform_precoder_e_ { enabled, disabled, nulltype };
enum rep_k_e_ { n1, n2, n4, n8, nulltype };
enum rep_k_rv_e_ { s1_minus0231, s2_minus0303, s3_minus0000, nulltype };
enum periodicity_e_ {
  sym2,
  sym7,
  sym1x14,
  sym2x14,
  sym4x14,
  sym5x14,
  sym8x14,
  sym10x14,
  sym16x14,
  sym20x14,
  sym32x14,
  sym40x14,
  sym64x14,
  sym80x14,
  sym128x14,
  sym160x14,
  sym256x14,
  sym320x14,
  sym512x14,
  sym640x14,
  sym1024x14,
  sym1280x14,
  sym2560x14,
  sym5120x14,
  sym6,
  sym1x12,
  sym2x12,
  sym4x12,
  sym5x12,
  sym8x12,
  sym10x12,
  sym16x12,
  sym20x12,
  sym32x12,
  sym40x12,
  sym64x12,
  sym80x12,
  sym128x12,
  sym160x12,
  sym256x12,
  sym320x12,
  sym512x12,
  sym640x12,
  sym1280x12,
  sym2560x12,
  nulltype
};

struct rrc_cfgured_ul_grant_s_ {
  bool				  dmrs_seq_initization_present;
  bool				  srs_res_ind_present;
  bool				  freq_hop_offset_present;
  uint16_t			  time_domain_offset;
  uint8_t			  time_domain_alloc;
  uint8_t             freq_domain_alloc[3];//fixed_bitstring<18>
  uint8_t			  ant_port;
  uint8_t			  dmrs_seq_initization;
  uint8_t			  precoding_and_nof_layers;
  uint8_t			  srs_res_ind;
  uint8_t			  mcs_and_tbs;
  uint16_t			  freq_hop_offset;//1
  uint8_t			  pathloss_ref_idx;
};

// CG-UCI-OnPUSCH ::= CHOICE
struct cg_uci_on_pusch_c {
  enum types { dynamic_type, semi_static, nulltype }   type_;
  union{
  	      struct beta_offsets_s semi;
		  cvector_vector_t(struct beta_offsets_s) dynamic;//dyn_array<beta_offsets_s>
       } c;//choice_buffer_t
};

// ConfiguredGrantConfig ::= SEQUENCE
struct cfgured_grant_cfg_s {
  // member variables
  bool                               freq_hop_present;
  bool                               mcs_table_present;
  bool                               mcs_table_transform_precoder_present;
  bool                               uci_on_pusch_present;
  bool                               rbg_size_present;
  bool                               transform_precoder_present;
  bool                               rep_k_rv_present;
  bool                               cfgured_grant_timer_present;
  bool                               rrc_cfgured_ul_grant_present;
  enum freq_hop_e_                   freq_hop;
  struct dmrs_ul_cfg_s               cg_dmrs_cfg;
  enum mcs_table_e_                  mcs_table;
  enum mcs_table_transform_precoder_e_      mcs_table_transform_precoder;
  setup_release_c(struct cg_uci_on_pusch_c) uci_on_pusch;
  enum res_alloc_e_                  res_alloc;
  enum pwr_ctrl_loop_to_use_e_       pwr_ctrl_loop_to_use;
  uint8_t                            p0_pusch_alpha;
  enum transform_precoder_e_         transform_precoder;
  uint8_t                            nrof_harq_processes;//1
  enum rep_k_e_                      rep_k;
  enum rep_k_rv_e_                   rep_k_rv;
  enum periodicity_e_                periodicity;
  uint8_t                            cfgured_grant_timer;//1
  struct rrc_cfgured_ul_grant_s_     rrc_cfgured_ul_grant;
};

struct aperiodic_s_ {
  // member variables
  bool	  csi_rs_present;
  bool	  slot_offset_present;
  uint8_t aperiodic_srs_res_trigger;//1
  uint8_t csi_rs;
  uint8_t slot_offset;//1
  cvector_vector_t(uint8_t) aperiodic_srs_res_trigger_list;//bounded_array<uint8_t, 2>
};

struct semi_persistent_s_ {
  bool	  associated_csi_rs_present;
  uint8_t associated_csi_rs;
};
struct periodic_s_ {
  bool	  associated_csi_rs_present;
  uint8_t associated_csi_rs;
};

struct res_type_c_ {
  enum types { aperiodic, semi_persistent, periodic, nulltype }    type_;
  union{struct aperiodic_s_ aperiodic; struct periodic_s_ periodic; struct semi_persistent_s_ semi_persistent;} c;//choice_buffer_t
};

enum usage_e_ { beam_management, codebook, non_codebook, ant_switching, nulltype };

struct pathloss_ref_rs_c_ {
  enum types { ssb_idx, csi_rs_idx, nulltype }	 type_;
  void	*c;//pod_choice_buffer_t
};

enum srs_pwr_ctrl_adjustment_states_e_ { same_as_fci2, separate_closed_loop, nulltype };

// SRS-ResourceSet ::= SEQUENCE
struct srs_res_set_s {
  // member variables
  bool                              alpha_present;
  bool                              p0_present;
  bool                              pathloss_ref_rs_present;
  bool                              srs_pwr_ctrl_adjustment_states_present;
  uint8_t                           srs_res_set_id;
  cvector_vector_t(uint8_t)           srs_res_id_list;//bounded_array<uint8_t, 16>
  struct res_type_c_                res_type;
  enum usage_e_                     usage;
  enum alpha_e                      alpha;
  int16_t                           p0;//-202
  struct pathloss_ref_rs_c_         pathloss_ref_rs;
  enum srs_pwr_ctrl_adjustment_states_e_ srs_pwr_ctrl_adjustment_states;
};

// SRS-PeriodicityAndOffset ::= CHOICE
struct srs_periodicity_and_offset_c {
      enum types {
      sl1,
      sl2,
      sl4,
      sl5,
      sl8,
      sl10,
      sl16,
      sl20,
      sl32,
      sl40,
      sl64,
      sl80,
      sl160,
      sl320,
      sl640,
      sl1280,
      sl2560,
      nulltype
    }               type_;
    void *c;//pod_choice_buffer_t
};

enum nrof_srs_ports_e_ { port1, ports2, ports4, nulltype };
enum ptrs_port_idx_e_ { n0, n1, nulltype };
struct n2_s_ {
  uint8_t comb_offset_n2;
  uint8_t cyclic_shift_n2;
};
struct n4_s_ {
  uint8_t comb_offset_n4;
  uint8_t cyclic_shift_n4;
};

struct tx_comb_c_ {
  enum types { n2, n4, nulltype }	 type_;
  union{struct n2_s_ n2; struct n4_s_ n4;} c;//choice_buffer_t
};

enum nrof_symbols_e_ { n1, n2, n4, nulltype };
enum repeat_factor_e_ { n1, n2, n4, nulltype };

struct res_map_s_ {
  // member variables
  uint8_t				start_position;
  enum nrof_symbols_e_	nrof_symbols;
  enum repeat_factor_e_ repeat_factor;
};

struct freq_hop_s_ {
  uint8_t c_srs;
  uint8_t b_srs;
  uint8_t b_hop;
};

enum group_or_seq_hop_e_ { neither, group_hop, seq_hop, nulltype };

struct aperiodic_s_ {
};

struct semi_persistent_s_ {
  struct srs_periodicity_and_offset_c periodicity_and_offset_sp;
};

struct periodic_s_ {
  struct srs_periodicity_and_offset_c periodicity_and_offset_p;
  // ...
};

struct srs_s_ {
  uint8_t res_id;
  uint8_t ul_bwp;
};

// SRS-SpatialRelationInfo ::= SEQUENCE
struct srs_spatial_relation_info_s {
  // member variables
  bool       serving_cell_id_present;
  uint8_t    serving_cell_id;
  struct ref_sig_c_ ref_sig;
};


// SRS-Resource ::= SEQUENCE
struct srs_res_s {
  // member variables
  bool                               ptrs_port_idx_present;
  bool                               spatial_relation_info_present;
  uint8_t                            srs_res_id;
  enum nrof_srs_ports_e_             nrof_srs_ports;
  enum ptrs_port_idx_e_              ptrs_port_idx;
  struct tx_comb_c_                  tx_comb;
  struct res_map_s_                  res_map;
  uint8_t                            freq_domain_position;
  uint16_t                           freq_domain_shift;
  struct freq_hop_s_                 freq_hop;
  enum group_or_seq_hop_e_           group_or_seq_hop;
  struct res_type_c_                 res_type;
  uint16_t                           seq_id;
  struct srs_spatial_relation_info_s spatial_relation_info;
};


// SRS-Config ::= SEQUENCE
struct srs_cfg_s {
  // member variables
  bool                           tpc_accumulation_present;
  cvector_vector_t(uint8_t)        srs_res_set_to_release_list;//bounded_array<uint8_t, 16>
  cvector_vector_t(struct srs_res_set_s) srs_res_set_to_add_mod_list;//dyn_array<srs_res_set_s>
  cvector_vector_t(uint8_t)              srs_res_to_release_list;//dyn_array<uint8_t>
  cvector_vector_t(struct srs_res_s)     srs_res_to_add_mod_list;//dyn_array<srs_res_s>
};

enum ssb_per_rach_occasion_e_ { one_eighth, one_fourth, one_half, one, two, four, eight, sixteen, nulltype };
enum beam_fail_recovery_timer_e_ { ms10, ms20, ms40, ms60, ms80, ms100, ms150, ms200, nulltype };

// BFR-CSIRS-Resource ::= SEQUENCE
struct bfr_csirs_res_s {
  // member variables
  bool                ra_preamb_idx_present;
  uint8_t             csi_rs;
  cvector_vector_t(uint16_t) ra_occasion_list;//dyn_array<uint16_t>
  uint8_t             ra_preamb_idx;
};
// BFR-SSB-Resource ::= SEQUENCE
struct bfr_ssb_res_s {
  uint8_t ssb;
  uint8_t ra_preamb_idx;
};


// PRACH-ResourceDedicatedBFR ::= CHOICE
struct prach_res_ded_bfr_c {
  enum types { ssb, csi_rs, nulltype }  type_;
  union{struct bfr_csirs_res_s csi_rs; struct bfr_ssb_res_s ssb;} c;//choice_buffer_t
};


// BeamFailureRecoveryConfig ::= SEQUENCE
struct beam_fail_recovery_cfg_s {
  // member variables
  bool                        root_seq_idx_bfr_present;
  bool                        rach_cfg_bfr_present;
  bool                        rsrp_thres_ssb_present;
  bool                        ssb_per_rach_occasion_present;
  bool                        ra_ssb_occasion_mask_idx_present;
  bool                        recovery_search_space_id_present;
  bool                        ra_prioritization_present;
  bool                        beam_fail_recovery_timer_present;
  uint8_t                     root_seq_idx_bfr;
  struct rach_cfg_generic_s   rach_cfg_bfr;
  uint8_t                     rsrp_thres_ssb;
  cvector_vector_t(struct prach_res_ded_bfr_c)   candidate_beam_rs_list;//dyn_array<prach_res_ded_bfr_c>
  enum ssb_per_rach_occasion_e_  ssb_per_rach_occasion;
  uint8_t                          ra_ssb_occasion_mask_idx;
  uint8_t                          recovery_search_space_id;
  struct ra_prioritization_s       ra_prioritization;
  enum beam_fail_recovery_timer_e_ beam_fail_recovery_timer;
  // ...
  // group 0
  bool                      msg1_subcarrier_spacing_present;
  enum subcarrier_spacing_e msg1_subcarrier_spacing;
};


// BWP-UplinkDedicated ::= SEQUENCE
struct bwp_ul_ded_s {
  bool                                      pucch_cfg_present;
  bool                                      pusch_cfg_present;
  bool                                      cfgured_grant_cfg_present;
  bool                                      srs_cfg_present;
  bool                                      beam_fail_recovery_cfg_present;
  setup_release_c(struct pucch_cfg_s)       pucch_cfg;
  setup_release_c(struct pusch_cfg_s)              pusch_cfg;
  setup_release_c(struct cfgured_grant_cfg_s)      cfgured_grant_cfg;
  setup_release_c(struct srs_cfg_s)                srs_cfg;
  setup_release_c(struct beam_fail_recovery_cfg_s) beam_fail_recovery_cfg;
};

// BWP-Uplink ::= SEQUENCE
struct bwp_ul_s {
  bool            bwp_common_present;
  bool            bwp_ded_present;
  uint8_t         bwp_id;
  struct bwp_ul_common_s bwp_common;
  struct bwp_ul_ded_s    bwp_ded;
};

enum max_code_block_groups_per_transport_block_e_ { n2, n4, n6, n8, nulltype };

// PUSCH-CodeBlockGroupTransmission ::= SEQUENCE
struct pusch_code_block_group_tx_s {
  // member variables
  enum max_code_block_groups_per_transport_block_e_ max_code_block_groups_per_transport_block;
};

enum xoverhead_e_ { xoh6, xoh12, xoh18, nulltype };
// PUSCH-ServingCellConfig ::= SEQUENCE
struct pusch_serving_cell_cfg_s {
  // member variables
  bool                                         code_block_group_tx_present;
  bool                                         rate_matching_present;
  bool                                         xoverhead_present;
  setup_release_c(struct pusch_code_block_group_tx_s) code_block_group_tx;
  enum xoverhead_e_                            xoverhead;
  // ...
  // group 0
  bool    max_mimo_layers_present;
  bool    processing_type2_enabled_present;
  uint8_t max_mimo_layers;//1
  bool    processing_type2_enabled;
};

enum srs_switch_from_carrier_e_ { sul, nul, nulltype };

// SRS-CC-SetIndex ::= SEQUENCE
struct srs_cc_set_idx_s {
  bool    cc_set_idx_present;
  bool    cc_idx_in_one_cc_set_present;
  uint8_t cc_set_idx;
  uint8_t cc_idx_in_one_cc_set;
};

// SRS-TPC-PDCCH-Config ::= SEQUENCE
struct srs_tpc_pdcch_cfg_s {
  // member variables
  cvector_vector_t(struct srs_cc_set_idx_s) srs_cc_set_idxlist;//dyn_array<srs_cc_set_idx_s>
};

struct srs_tpc_pdcch_group_c_ {
  enum types { type_a, type_b, nulltype }	 type_;
  union{
  	    struct srs_tpc_pdcch_cfg_s type_b;
		cvector_vector_t(struct srs_tpc_pdcch_cfg_s) type_a;//dyn_array<srs_tpc_pdcch_cfg_s>
		} c;//choice_buffer_t
};

// SRS-CarrierSwitching ::= SEQUENCE
struct srs_carrier_switching_s {
  // member variables
  bool                       srs_switch_from_serv_cell_idx_present;
  bool                       srs_tpc_pdcch_group_present;
  uint8_t                    srs_switch_from_serv_cell_idx;
  enum srs_switch_from_carrier_e_   srs_switch_from_carrier;
  struct srs_tpc_pdcch_group_c_     srs_tpc_pdcch_group;
  cvector_vector_t(uint8_t)           monitoring_cells;//bounded_array<uint8_t, 32>
};

// UplinkConfig ::= SEQUENCE
struct ul_cfg_s {
  // member variables
  bool                                      init_ul_bwp_present;
  bool                                      first_active_ul_bwp_id_present;
  bool                                      pusch_serving_cell_cfg_present;
  bool                                      carrier_switching_present;
  struct bwp_ul_ded_s                       init_ul_bwp;
  cvector_vector_t(uint8_t)                   ul_bwp_to_release_list;//bounded_array<uint8_t, 4>
  cvector_vector_t(struct bwp_ul_s)           ul_bwp_to_add_mod_list;//dyn_array<bwp_ul_s>
  uint8_t                                   first_active_ul_bwp_id;
  setup_release_c(struct pusch_serving_cell_cfg_s) pusch_serving_cell_cfg;
  setup_release_c(struct srs_carrier_switching_s)  carrier_switching;
  // ...
  // group 0
  bool                               pwr_boost_pi2_bpsk_present;
  bool                               pwr_boost_pi2_bpsk;
  cvector_vector_t(struct scs_specific_carrier_s) ul_ch_bw_per_scs_list;//dyn_array<scs_specific_carrier_s>
};

// SlotFormatCombination ::= SEQUENCE
struct slot_format_combination_s {
  // member variables
  uint16_t        slot_format_combination_id;
  cvector_vector_t(uint16_t) slot_formats;//dyn_array<uint16_t>
};

// SlotFormatCombinationsPerCell ::= SEQUENCE
struct slot_format_combinations_per_cell_s {
  // member variables
  bool                        subcarrier_spacing2_present;
  bool                        position_in_dci_present;
  uint8_t                     serving_cell_id;
  enum subcarrier_spacing_e        subcarrier_spacing;
  enum subcarrier_spacing_e        subcarrier_spacing2;
  cvector_vector_t(struct slot_format_combination_s) slot_format_combinations;//dyn_array<slot_format_combination_s>
  uint8_t                     position_in_dci;
};

// SlotFormatIndicator ::= SEQUENCE
struct slot_format_ind_s {
  // member variables
  uint32_t                            sfi_rnti;
  uint8_t                             dci_payload_size;//1
  cvector_vector_t(struct slot_format_combinations_per_cell_s) slot_format_comb_to_add_mod_list;//dyn_array<slot_format_combinations_per_cell_s>
  cvector_vector_t(uint8_t) slot_format_comb_to_release_list;//bounded_array<uint8_t, 16>
};

// PDCCH-ServingCellConfig ::= SEQUENCE
struct pdcch_serving_cell_cfg_s {
  bool                               slot_format_ind_present;
  setup_release_c(struct slot_format_ind_s) slot_format_ind;
};

enum max_code_block_groups_per_transport_block_e_ { n2, n4, n6, n8, nulltype };

// PDSCH-CodeBlockGroupTransmission ::= SEQUENCE
struct pdsch_code_block_group_tx_s {
  // member variables
  enum max_code_block_groups_per_transport_block_e_ max_code_block_groups_per_transport_block;
  bool                                         code_block_group_flush_ind;
};

enum xoverhead_e_ { xoh6, xoh12, xoh18, nulltype };
enum nrof_harq_processes_for_pdsch_e_ { n2, n4, n6, n10, n12, n16, nulltype };

// PDSCH-ServingCellConfig ::= SEQUENCE
struct pdsch_serving_cell_cfg_s {
  // member variables
  bool                                         code_block_group_tx_present;
  bool                                         xoverhead_present;
  bool                                         nrof_harq_processes_for_pdsch_present;
  bool                                         pucch_cell_present;
  setup_release_c(struct pdsch_code_block_group_tx_s) code_block_group_tx;
  enum xoverhead_e_                            xoverhead;
  enum nrof_harq_processes_for_pdsch_e_        nrof_harq_processes_for_pdsch;
  uint8_t                                      pucch_cell;
  // ...
  // group 0
  bool    max_mimo_layers_present;
  bool    processing_type2_enabled_present;
  uint8_t max_mimo_layers;//1
  bool    processing_type2_enabled;
};

enum pwr_ctrl_offset_ss_e_ { db_minus3, db0, db3, db6, nulltype };

// NZP-CSI-RS-Resource ::= SEQUENCE
struct nzp_csi_rs_res_s {
  // member variables
  bool                             pwr_ctrl_offset_ss_present;
  bool                             periodicity_and_offset_present;
  bool                             qcl_info_periodic_csi_rs_present;
  uint8_t                          nzp_csi_rs_res_id;
  struct csi_rs_res_map_s          res_map;
  int8_t                           pwr_ctrl_offset;//-8
  enum pwr_ctrl_offset_ss_e_       pwr_ctrl_offset_ss;
  uint16_t                         scrambling_id;
  struct csi_res_periodicity_and_offset_c periodicity_and_offset;
  uint8_t                          qcl_info_periodic_csi_rs;
};

enum repeat_e_ { on, off, nulltype };
// NZP-CSI-RS-ResourceSet ::= SEQUENCE
struct nzp_csi_rs_res_set_s {
  // member variables
  bool              repeat_present;
  bool              aperiodic_trigger_offset_present;
  bool              trs_info_present;
  uint8_t           nzp_csi_res_set_id;
  cvector_vector_t(uint8_t) nzp_csi_rs_res;//dyn_array<uint8_t>
  enum repeat_e_    repeat;
  uint8_t           aperiodic_trigger_offset;
};

enum subcarrier_location_p0_e_ { s0, s2, s4, s6, s8, s10, nulltype };
struct pattern0_s_ {
  // member variables
  enum subcarrier_location_p0_e_ subcarrier_location_p0;
  uint8_t					symbol_location_p0;
};

enum subcarrier_location_p1_e_ { s0, s4, s8, nulltype };
struct pattern1_s_ {
  // member variables
  enum subcarrier_location_p1_e_ subcarrier_location_p1;
  uint8_t					symbol_location_p1;
};

enum csi_im_res_elem_pattern_types {nothing, pattern0, pattern1, nulltype };
struct csi_im_res_elem_pattern_c_ {
  enum csi_im_res_elem_pattern_types type_;
  union{
  	    struct pattern0_s_ pattern0;
		struct pattern1_s_ pattern1;
	   }c;//choice_buffer_t
};

// CSI-IM-Resource ::= SEQUENCE
struct csi_im_res_s {
  // member variables
  bool                             csi_im_res_elem_pattern_present;
  bool                             freq_band_present;
  bool                             periodicity_and_offset_present;
  uint8_t                          csi_im_res_id;
  struct csi_im_res_elem_pattern_c_       csi_im_res_elem_pattern;
  struct csi_freq_occupation_s            freq_band;
  struct csi_res_periodicity_and_offset_c periodicity_and_offset;
};

// CSI-IM-ResourceSet ::= SEQUENCE
struct csi_im_res_set_s {
  // member variables
  uint8_t       csi_im_res_set_id;
  cvector_vector_t(uint8_t) csi_im_res;//bounded_array<uint8_t, 8>
};

// CSI-SSB-ResourceSet ::= SEQUENCE
struct csi_ssb_res_set_s {
  // member variables
  uint8_t             csi_ssb_res_set_id;
  cvector_vector_t(uint8_t) csi_ssb_res_list;//dyn_array<uint8_t>
};


struct nzp_csi_rs_ssb_s_ {
  // member variables
  bool		 csi_ssb_res_set_list_present;
  cvector_vector_t(uint8_t)	 nzp_csi_rs_res_set_list;//bounded_array<uint8_t, 16>
  uint8_t	 csi_ssb_res_set_list;//std::array<uint8_t, 1>
};

struct csi_rs_res_set_list_c_ {
  enum types {nothing ,nzp_csi_rs_ssb, csi_im_res_set_list, nulltype } type_;
  union{
		  cvector_vector_t(uint8_t) csi_im_res_set_list;//bounded_array<uint8_t, 16>
		  struct nzp_csi_rs_ssb_s_ nzp_csi_rs_ssb;
		} c;//choice_buffer_t
};

enum res_type_e_ { aperiodic, semi_persistent, periodic, nulltype };

// CSI-ResourceConfig ::= SEQUENCE
struct csi_res_cfg_s {
  // member variables
  uint8_t                csi_res_cfg_id;
  struct csi_rs_res_set_list_c_ csi_rs_res_set_list;
  uint8_t                bwp_id;
  enum res_type_e_       res_type;
};

// PUCCH-CSI-Resource ::= SEQUENCE
struct pucch_csi_res_s {
  uint8_t ul_bw_part_id;
  uint8_t pucch_res;
};

  enum csi_report_periodicity_and_offset_e_ {
  nothing,
  slots4,
  slots5,
  slots8,
  slots10,
  slots16,
  slots20,
  slots40,
  slots80,
  slots160,
  slots320,
  nulltype
};

// CSI-ReportPeriodicityAndOffset ::= CHOICE
struct csi_report_periodicity_and_offset_c {
     enum csi_report_periodicity_and_offset_e_ type_;
     int c;//pod_choice_buffer_t
};

struct periodic_s_ {
  // member variables
  struct csi_report_periodicity_and_offset_c  report_slot_cfg;
  cvector_vector_t(struct pucch_csi_res_s) 	  pucch_csi_res_list;//dyn_array<pucch_csi_res_s>
};
struct semi_persistent_on_pucch_s_ {
  // member variables
  struct csi_report_periodicity_and_offset_c report_slot_cfg;
  cvector_vector_t(struct pucch_csi_res_s) 	 pucch_csi_res_list;//dyn_array<pucch_csi_res_s>
};

enum report_slot_cfg_e_ { sl5, sl10, sl20, sl40, sl80, sl160, sl320, nulltype };
struct semi_persistent_on_pusch_s_ {
  // member variables
  enum report_slot_cfg_e_    report_slot_cfg;
  cvector_vector_t(uint8_t)    report_slot_offset_list;//bounded_array<uint8_t, 16>
  uint8_t					 p0alpha;
};

struct aperiodic_s_ {
  // member variables
  cvector_vector_t(uint8_t)   report_slot_offset_list;//bounded_array<uint8_t, 16>
};

enum report_cfg_type_e_ {nothing, periodic, semi_persistent_on_pucch, semi_persistent_on_pusch, aperiodic, nulltype };

struct report_cfg_type_c_ {
  enum report_cfg_type_e_ type_;
  union{
		  struct aperiodic_s_ aperiodic;
		  struct periodic_s_ periodic;
		  struct semi_persistent_on_pucch_s_ semi_persistent_on_pucch;
		  struct semi_persistent_on_pusch_s_ semi_persistent_on_pusch;
	   } c;//choice_buffer_t
};

enum pdsch_bundle_size_for_csi_e_ { n2, n4, nulltype };
struct cri_ri_i1_cqi_s_ {
  // member variables
  bool						   pdsch_bundle_size_for_csi_present;
  enum pdsch_bundle_size_for_csi_e_ pdsch_bundle_size_for_csi;
};

enum report_quant_e_ {
	nothing,
	none,
	cri_ri_pmi_cqi,
	cri_ri_i1,
	cri_ri_i1_cqi,
	cri_ri_cqi,
	cri_rsrp,
	ssb_idx_rsrp,
	cri_ri_li_pmi_cqi,
	nulltype
};

struct report_quant_c_ {
  enum report_quant_e_ type_;
  struct cri_ri_i1_cqi_s_ c;
};

enum cqi_format_ind_e_ { wideband_cqi, subband_cqi, nulltype };
enum pmi_format_ind_e_ { wideband_pmi, subband_pmi, nulltype };

struct csi_report_band_c_ {
  enum types {
	  subbands3,
	  subbands4,
	  subbands5,
	  subbands6,
	  subbands7,
	  subbands8,
	  subbands9,
	  subbands10,
	  subbands11,
	  subbands12,
	  subbands13,
	  subbands14,
	  subbands15,
	  subbands16,
	  subbands17,
	  subbands18,
	  subbands19_v1530,
	  nulltype
	}  type_;
  uint8_t c[3];//choice_buffer_t//fixed_bitstring<19>
};

struct report_freq_cfg_s_ {
  // member variables
  bool				 cqi_format_ind_present;
  bool				 pmi_format_ind_present;
  bool				 csi_report_band_present;
  enum cqi_format_ind_e_  cqi_format_ind;
  enum pmi_format_ind_e_  pmi_format_ind;
  struct csi_report_band_c_ csi_report_band;
};

enum time_restrict_for_ch_meass_e_ { cfgured, not_cfgured, nulltype };
enum time_restrict_for_interference_meass_e_ { cfgured, not_cfgured, nulltype };
enum dummy_e_ { n1, n2, nulltype };
enum nrof_reported_rs_e_ { n1, n2, n3, n4, nulltype };
struct disabled_s_ {
  // member variables
  bool				  nrof_reported_rs_present;
  enum nrof_reported_rs_e_ nrof_reported_rs;
};

enum group_based_beam_report_types {nothing, enabled, disabled, nulltype };
struct group_based_beam_report_c_ {
  enum group_based_beam_report_types type_;
  struct disabled_s_ c;
};

enum cqi_table_e_ { table1, table2, table3, spare1, nulltype };
enum subband_size_e_ { value1, value2, nulltype };
enum report_slot_cfg_v1530_e_ { sl4, sl8, sl16, nulltype };
struct semi_persistent_on_pusch_v1530_s_ {
	// member variables
	enum report_slot_cfg_v1530_e_ report_slot_cfg_v1530;
};

struct port_idx8_s_ {
  // member variables
  bool			  rank1_minus8_present;
  bool			  rank2_minus8_present;
  bool			  rank3_minus8_present;
  bool			  rank4_minus8_present;
  bool			  rank5_minus8_present;
  bool			  rank6_minus8_present;
  bool			  rank7_minus8_present;
  bool			  rank8_minus8_present;
  uint8_t		  rank1_minus8;
  uint8_t		  rank2_minus8[2];
  uint8_t		  rank3_minus8[3];
  uint8_t		  rank4_minus8[4];
  uint8_t		  rank5_minus8[5];
  uint8_t		  rank6_minus8[6];
  uint8_t		  rank7_minus8[7];
  uint8_t		  rank8_minus8[8];
};

struct port_idx4_s_ {
  // member variables
  bool			  rank1_minus4_present;
  bool			  rank2_minus4_present;
  bool			  rank3_minus4_present;
  bool			  rank4_minus4_present;
  uint8_t		  rank1_minus4;
  uint8_t         rank2_minus4[2];
  uint8_t         rank3_minus4[3];
  uint8_t         rank4_minus4[4];
};

struct port_idx2_s_ {
  // member variables
  bool			  rank1_minus2_present;
  bool			  rank2_minus2_present;
  uint8_t		  rank1_minus2;
  uint8_t		  rank2_minus2[2];
};

// PortIndexFor8Ranks ::= CHOICE
struct port_idx_for8_ranks_c {
  enum types { port_idx8, port_idx4, port_idx2, port_idx1, nulltype }      type_;
  union{struct port_idx2_s_ port_idx2; struct port_idx4_s_ port_idx4; struct port_idx8_s_ port_idx8;}  c;//choice_buffer_t
};

struct two_s_ {
  uint8_t two_tx_codebook_subset_restrict;//fixed_bitstring<6>
};

struct n1_n2_c_ {
  enum types {
	  two_one_type_i_single_panel_restrict,
	  two_two_type_i_single_panel_restrict,
	  four_one_type_i_single_panel_restrict,
	  three_two_type_i_single_panel_restrict,
	  six_one_type_i_single_panel_restrict,
	  four_two_type_i_single_panel_restrict,
	  eight_one_type_i_single_panel_restrict,
	  four_three_type_i_single_panel_restrict,
	  six_two_type_i_single_panel_restrict,
	  twelve_one_type_i_single_panel_restrict,
	  four_four_type_i_single_panel_restrict,
	  eight_two_type_i_single_panel_restrict,
	  sixteen_one_type_i_single_panel_restrict,
	  nulltype
	}		   type_;
  uint8_t c[32];//choice_buffer_t<fixed_bitstring<256> >
};

struct more_than_two_s_ {
  // member variables
  bool				  type_i_single_panel_codebook_subset_restrict_i2_present;
  struct n1_n2_c_	  n1_n2;
  uint8_t             type_i_single_panel_codebook_subset_restrict_i2[2];//fixed_bitstring<16>
};

enum nr_of_ant_ports_types {nothing, two, more_than_two, nulltype };
struct nr_of_ant_ports_c_ {
  enum nr_of_ant_ports_types 	  type_;
  union{struct more_than_two_s_ more_than_two; struct two_s_ two;}c;//choice_buffer_t
};

struct type_i_single_panel_s_ {
  // member variables
  struct nr_of_ant_ports_c_ nr_of_ant_ports;
  uint8_t type_i_single_panel_ri_restrict;//fixed_bitstring<8>
};

struct ng_n1_n2_c_ {
  enum types {
	  two_two_one_type_i_multi_panel_restrict,
	  two_four_one_type_i_multi_panel_restrict,
	  four_two_one_type_i_multi_panel_restrict,
	  two_two_two_type_i_multi_panel_restrict,
	  two_eight_one_type_i_multi_panel_restrict,
	  four_four_one_type_i_multi_panel_restrict,
	  two_four_two_type_i_multi_panel_restrict,
	  four_two_two_type_i_multi_panel_restrict,
	  nulltype
	}	 type_;
  uint8_t c[16];//choice_buffer_t<fixed_bitstring<128>>
};

struct type_i_multi_panel_s_ {
  // member variables
  ng_n1_n2_c_  ng_n1_n2;
  uint8_t	   ri_restrict;//fixed_bitstring<4>
};


enum sub_types {nothing, type_i_single_panel, type_i_multi_panel, nulltype };

struct type1_s_ {
  // member variables
  struct sub_type_c_ {
	  enum sub_types type_;
	  union{struct type_i_multi_panel_s_ type_i_multi_panel; struct type_i_single_panel_s_ type_i_single_panel;} c;//choice_buffer_t
  } sub_type;
  uint8_t	  codebook_mode;//1
};

struct n1_n2_codebook_subset_restrict_c_ {
  enum types {
	  two_one,
	  two_two,
	  four_one,
	  three_two,
	  six_one,
	  four_two,
	  eight_one,
	  four_three,
	  six_two,
	  twelve_one,
	  four_four,
	  eight_two,
	  sixteen_one,
	  nulltype
	}  type_;
  uint8_t c[18];//choice_buffer_t<fixed_bitstring<139> >
};

struct type_ii_s_ {
  // member variables
  struct n1_n2_codebook_subset_restrict_c_ n1_n2_codebook_subset_restrict;
  uint8_t				 type_ii_ri_restrict;//fixed_bitstring<2>
};

enum port_sel_sampling_size_e_ { n1, n2, n3, n4, nulltype };
struct type_ii_port_sel_s_ {
  // member variables
  bool						port_sel_sampling_size_present;
  enum port_sel_sampling_size_e_ port_sel_sampling_size;
  uint8_t					type_ii_port_sel_ri_restrict;//fixed_bitstring<2> 
};

enum phase_alphabet_size_e_ { n4, n8, nulltype };
enum nof_beams_e_ { two, three, four, nulltype };

struct type2_s_ {
  // member variables
  struct sub_type_c_ {
	  enum types { type_ii, type_ii_port_sel, nulltype }  type_;
	  union{struct type_ii_port_sel_s_ type_ii_port_sel; struct type_ii_s_ type_ii;} c;//choice_buffer_t
  } sub_type;
  enum phase_alphabet_size_e_ phase_alphabet_size;
  bool					 subband_amplitude;
  enum nof_beams_e_			 nof_beams;
};

enum codebook_types {nothing, type1, type2, nulltype };

struct codebook_type_c_ {
  enum codebook_types type_;
  union{struct type1_s_ type1; struct type2_s_ type2;} c;//choice_buffer_t
};

// CodebookConfig ::= SEQUENCE
struct codebook_cfg_s {
  // member variables
  struct codebook_type_c_ codebook_type;
};

// CSI-ReportConfig ::= SEQUENCE
struct csi_report_cfg_s {
  // member variables
  bool                                    carrier_present;
  bool                                    csi_im_res_for_interference_present;
  bool                                    nzp_csi_rs_res_for_interference_present;
  bool                                    report_freq_cfg_present;
  bool                                    codebook_cfg_present;
  bool                                    dummy_present;
  bool                                    cqi_table_present;
  uint8_t                                 report_cfg_id;
  uint8_t                                 carrier;
  uint8_t                                 res_for_ch_meas;
  uint8_t                                 csi_im_res_for_interference;
  uint8_t                                 nzp_csi_rs_res_for_interference;
  struct report_cfg_type_c_                    report_cfg_type;
  struct report_quant_c_                       report_quant;
  struct report_freq_cfg_s_                    report_freq_cfg;
  enum time_restrict_for_ch_meass_e_           time_restrict_for_ch_meass;
  enum time_restrict_for_interference_meass_e_ time_restrict_for_interference_meass;
  struct codebook_cfg_s                        codebook_cfg;
  enum dummy_e_                                dummy;
  struct group_based_beam_report_c_            group_based_beam_report;
  enum cqi_table_e_                            cqi_table;
  enum subband_size_e_                         subband_size;
  cvector_vector_t(struct port_idx_for8_ranks_c) non_pmi_port_ind;//dyn_array<port_idx_for8_ranks_c>
  // ...
  // group 0
  struct semi_persistent_on_pusch_v1530_s_ semi_persistent_on_pusch_v1530;
};

struct nzp_csi_rs_s_ {
  // member variables
  uint8_t	  res_set;//1
  cvector_vector_t(uint8_t) qcl_info;//bounded_array<uint8_t, 16>
};

struct res_for_ch_c_ {
  enum types { nzp_csi_rs, csi_ssb_res_set, nulltype }	type_;
  struct nzp_csi_rs_s_	c;//choice_buffer_t
};

// CSI-AssociatedReportConfigInfo ::= SEQUENCE
struct csi_associated_report_cfg_info_s {
  // member variables
  bool          csi_im_res_for_interference_present;
  bool          nzp_csi_rs_res_for_interference_present;
  uint8_t       report_cfg_id;
  struct res_for_ch_c_ res_for_ch;
  uint8_t       csi_im_res_for_interference;//1
  uint8_t       nzp_csi_rs_res_for_interference;//1
};

// CSI-AperiodicTriggerState ::= SEQUENCE
struct csi_aperiodic_trigger_state_s {
  cvector_vector_t(struct csi_associated_report_cfg_info_s) associated_report_cfg_info_list;//dyn_array<csi_associated_report_cfg_info_s>
};

// CSI-SemiPersistentOnPUSCH-TriggerState ::= SEQUENCE
struct csi_semi_persistent_on_pusch_trigger_state_s {
  uint8_t associated_report_cfg_info;
};

// CSI-MeasConfig ::= SEQUENCE
struct csi_meas_cfg_s {
  int	                                duplex_mode;

  // member variables
  bool                                  report_trigger_size_present;
  bool                                  aperiodic_trigger_state_list_present;
  bool                                  semi_persistent_on_pusch_trigger_state_list_present;
  cvector_vector_t(struct nzp_csi_rs_res_s)  nzp_csi_rs_res_to_add_mod_list;//dyn_array<nzp_csi_rs_res_s>
  cvector_vector_t(uint8_t)                  nzp_csi_rs_res_to_release_list;//dyn_array<uint8_t>
  cvector_vector_t(struct nzp_csi_rs_res_set_s)  nzp_csi_rs_res_set_to_add_mod_list;//dyn_array<nzp_csi_rs_res_set_s>
  cvector_vector_t(uint8_t)                      nzp_csi_rs_res_set_to_release_list;//dyn_array<uint8_t>
  cvector_vector_t(struct csi_im_res_s)          csi_im_res_to_add_mod_list;//dyn_array<csi_im_res_s>
  cvector_vector_t(uint8_t)                      csi_im_res_to_release_list;//bounded_array<uint8_t, 32>
  cvector_vector_t(struct csi_im_res_set_s)      csi_im_res_set_to_add_mod_list;//dyn_array<csi_im_res_set_s>
  cvector_vector_t(uint8_t)                      csi_im_res_set_to_release_list;//dyn_array<uint8_t>
  cvector_vector_t(struct csi_ssb_res_set_s)     csi_ssb_res_set_to_add_mod_list;//dyn_array<csi_ssb_res_set_s>
  cvector_vector_t(uint8_t)                      csi_ssb_res_set_to_release_list;//dyn_array<uint8_t>
  cvector_vector_t(struct csi_res_cfg_s)         csi_res_cfg_to_add_mod_list;//dyn_array<csi_res_cfg_s>
  cvector_vector_t(uint8_t)                      csi_res_cfg_to_release_list;//dyn_array<uint8_t>
  cvector_vector_t(struct csi_report_cfg_s)      csi_report_cfg_to_add_mod_list;//dyn_array<csi_report_cfg_s>
  cvector_vector_t(uint8_t)                      csi_report_cfg_to_release_list;//dyn_array<uint8_t>
  uint8_t                               report_trigger_size;
  setup_release_c(cvector_vector_t(struct csi_aperiodic_trigger_state_s))  aperiodic_trigger_state_list;//dyn_seq_of<csi_aperiodic_trigger_state_s, 1, 128> 
  setup_release_c(cvector_vector_t(struct csi_semi_persistent_on_pusch_trigger_state_s))  semi_persistent_on_pusch_trigger_state_list;//dyn_seq_of<csi_semi_persistent_on_pusch_trigger_state_s, 1, 64>
};

struct own_s_ {
  bool cif_presence;
};
struct other_s_ {
  uint8_t sched_cell_id;//0
  uint8_t cif_in_sched_cell;//1
};

struct sched_cell_info_c_ {
  enum types { own, other, nulltype }  type_;
  union{struct other_s_ other; struct own_s_ own;} c;//choice_buffer_t
};

// CrossCarrierSchedulingConfig ::= SEQUENCE
struct cross_carrier_sched_cfg_s {
  // member variables
  struct sched_cell_info_c_ sched_cell_info;
};

enum pathloss_ref_linking_e_ { sp_cell, scell, nulltype };

// ServingCellConfig ::= SEQUENCE
struct serving_cell_cfg_s {
  int                                       duplex_mode;
  // member variables
  bool                                      tdd_ul_dl_cfg_ded_present;
  bool                                      init_dl_bwp_present;
  bool                                      first_active_dl_bwp_id_present;
  bool                                      bwp_inactivity_timer_present;
  bool                                      default_dl_bwp_id_present;
  bool                                      ul_cfg_present;
  bool                                      supplementary_ul_present;
  bool                                      pdcch_serving_cell_cfg_present;
  bool                                      pdsch_serving_cell_cfg_present;
  bool                                      csi_meas_cfg_present;
  bool                                      scell_deactivation_timer_present;
  bool                                      cross_carrier_sched_cfg_present;
  bool                                      dummy_present;
  bool                                      pathloss_ref_linking_present;
  bool                                      serving_cell_mo_present;
  struct tdd_ul_dl_cfg_ded_s                tdd_ul_dl_cfg_ded;
  struct bwp_dl_ded_s                       init_dl_bwp;
  cvector_vector_t(uint8_t)                   dl_bwp_to_release_list;//bounded_array<uint8_t, 4>
  cvector_vector_t(struct bwp_dl_s)           dl_bwp_to_add_mod_list;
  uint8_t                                   first_active_dl_bwp_id;
  enum bwp_inactivity_timer_e_              bwp_inactivity_timer;
  uint8_t                                   default_dl_bwp_id;
  struct ul_cfg_s                                  ul_cfg;
  struct ul_cfg_s                                  supplementary_ul;
  setup_release_c(struct pdcch_serving_cell_cfg_s) pdcch_serving_cell_cfg;
  setup_release_c(struct pdsch_serving_cell_cfg_s) pdsch_serving_cell_cfg;
  setup_release_c(struct csi_meas_cfg_s)           csi_meas_cfg;
  enum scell_deactivation_timer_e_                 scell_deactivation_timer;
  struct cross_carrier_sched_cfg_s                 cross_carrier_sched_cfg;
  uint8_t                                          tag_id;
  enum pathloss_ref_linking_e_                     pathloss_ref_linking;
  uint8_t                                          serving_cell_mo;//1
  // ...
  // group 0
  setup_release_c(struct rate_match_pattern_lte_crs_s) lte_crs_to_match_around;
  cvector_vector_t(struct rate_match_pattern_s)     rate_match_pattern_to_add_mod_list;//dyn_array<rate_match_pattern_s>
  cvector_vector_t(uint8_t)                         rate_match_pattern_to_release_list;//bounded_array<uint8_t, 4>
  cvector_vector_t(struct scs_specific_carrier_s)   dl_ch_bw_per_scs_list;//dyn_array<scs_specific_carrier_s>
};

// SpCellConfig ::= SEQUENCE
struct sp_cell_cfg_s {
  bool                                     serv_cell_idx_present;
  bool                                     recfg_with_sync_present;
  bool                                     rlf_timers_and_consts_present;
  bool                                     rlm_in_sync_out_of_sync_thres_present;
  bool                                     sp_cell_cfg_ded_present;
  uint8_t                                  serv_cell_idx;
  struct recfg_with_sync_s                 recfg_with_sync;
  setup_release_c(struct rlf_timers_and_consts_s) rlf_timers_and_consts;
  struct serving_cell_cfg_s                       sp_cell_cfg_ded;
};

struct ssb_positions_in_burst_s_ {
  bool				 group_presence_present;
  uint8_t            in_one_group;//fixed_bitstring<8>
  uint8_t            group_presence;//fixed_bitstring<8>
};

// PDCCH-ConfigSIB1 ::= SEQUENCE
struct pdcch_cfg_sib1_s {
  uint8_t ctrl_res_set_zero;
  uint8_t search_space_zero;
};

enum q_hyst_e_ {
  db0,
  db1,
  db2,
  db3,
  db4,
  db5,
  db6,
  db8,
  db10,
  db12,
  db14,
  db16,
  db18,
  db20,
  db22,
  db24,
  nulltype
};
enum sf_medium_e_ { db_minus6, db_minus4, db_minus2, db0, nulltype };
enum sf_high_e_ { db_minus6, db_minus4, db_minus2, db0, nulltype };
struct q_hyst_sf_s_ {
  // member variables
  enum sf_medium_e_ sf_medium;
  enum sf_high_e_   sf_high;
};

struct speed_state_resel_pars_s_ {
  // member variables
  struct mob_state_params_s mob_state_params;
  struct q_hyst_sf_s_		 q_hyst_sf;
};

// ThresholdNR ::= SEQUENCE
struct thres_nr_s {
  bool    thres_rsrp_present;
  bool    thres_rsrq_present;
  bool    thres_sinr_present;
  uint8_t thres_rsrp;
  uint8_t thres_rsrq;
  uint8_t thres_sinr;
};

// Q-OffsetRange ::= ENUMERATED
enum q_offset_range_e {
	db_minus24,
	db_minus22,
	db_minus20,
	db_minus18,
	db_minus16,
	db_minus14,
	db_minus12,
	db_minus10,
	db_minus8,
	db_minus6,
	db_minus5,
	db_minus4,
	db_minus3,
	db_minus2,
	db_minus1,
	db0,
	db1,
	db2,
	db3,
	db4,
	db5,
	db6,
	db8,
	db10,
	db12,
	db14,
	db16,
	db18,
	db20,
	db22,
	db24,
	nulltype
};

// RangeToBestCell ::= Q-OffsetRange
typedef  q_offset_range_e  range_to_best_cell_e;

struct cell_resel_info_common_s_ {
  // member variables
  bool						nrof_ss_blocks_to_average_present;
  bool						abs_thresh_ss_blocks_consolidation_present;
  bool						range_to_best_cell_present;
  bool						speed_state_resel_pars_present;
  uint8_t					nrof_ss_blocks_to_average;//2
  struct thres_nr_s			abs_thresh_ss_blocks_consolidation;
  enum range_to_best_cell_e	range_to_best_cell;
  enum q_hyst_e_ 			q_hyst;
  struct speed_state_resel_pars_s_ speed_state_resel_pars;
  // ...
};

// CellReselectionSubPriority ::= ENUMERATED
enum cell_resel_sub_prio_e { odot2, odot4, odot6, odot8, nulltype };

struct cell_resel_serving_freq_info_s_ {
  bool					s_non_intra_search_p_present;
  bool					s_non_intra_search_q_present;
  bool					thresh_serving_low_q_present;
  bool					cell_resel_sub_prio_present;
  uint8_t				s_non_intra_search_p;
  uint8_t				s_non_intra_search_q;
  uint8_t				thresh_serving_low_p;
  uint8_t				thresh_serving_low_q;
  uint8_t				cell_resel_prio;
  enum cell_resel_sub_prio_e cell_resel_sub_prio;
  // ...
};

// SS-RSSI-Measurement ::= SEQUENCE
struct ss_rssi_meas_s {
  uint8_t    meas_slots[10];//bounded_bitstring<1, 80>
  uint8_t    end_symbol;
};

// SSB-ToMeasure ::= CHOICE
struct ssb_to_measure_c {
  enum types { short_bitmap, medium_bitmap, long_bitmap, nulltype } type_;
  uint8_t c[8];//choice_buffer_t<fixed_bitstring<64> >
};

struct intra_freq_cell_resel_info_s_ {
  bool							q_rx_lev_min_sul_present;
  bool							q_qual_min_present;
  bool							s_intra_search_q_present;
  bool							p_max_present;
  bool							smtc_present;
  bool							ss_rssi_meas_present;
  bool							ssb_to_measure_present;
  int8_t						q_rx_lev_min;// = -70
  int8_t						q_rx_lev_min_sul;// = -70
  int8_t						q_qual_min;// = -43
  uint8_t						s_intra_search_p;
  uint8_t						s_intra_search_q;
  uint8_t						t_resel_nr;
  cvector_vector_t(struct nr_multi_band_info_s) freq_band_list;//dyn_array<nr_multi_band_info_s>// MultiFrequencyBandListNR-SIB ::= SEQUENCE (SIZE (1..8)) OF NR-MultiBandInfo
  cvector_vector_t(struct nr_multi_band_info_s) freq_band_list_sul;
  int8_t						p_max;// = -30
  struct ssb_mtc_s 				smtc;
  struct ss_rssi_meas_s			ss_rssi_meas;
  struct ssb_to_measure_c		ssb_to_measure;
  bool							derive_ssb_idx_from_cell;
  // ...
  // group 0
  struct speed_state_scale_factors_s   *t_resel_nr_sf;
};

struct thresh_x_q_r9_s_ {
  uint8_t thresh_x_high_q_r9;
  uint8_t thresh_x_low_q_r9;
};

// Q-OffsetRange ::= ENUMERATED
enum q_offset_range_e {
	db_minus24,
	db_minus22,
	db_minus20,
	db_minus18,
	db_minus16,
	db_minus14,
	db_minus12,
	db_minus10,
	db_minus8,
	db_minus6,
	db_minus5,
	db_minus4,
	db_minus3,
	db_minus2,
	db_minus1,
	db0,
	db1,
	db2,
	db3,
	db4,
	db5,
	db6,
	db8,
	db10,
	db12,
	db14,
	db16,
	db18,
	db20,
	db22,
	db24,
	nulltype
};

enum allowed_meas_bw_e { mbw6, mbw15, mbw25, mbw50, mbw75, mbw100, nulltype };

// InterFreqNeighCellInfo ::= SEQUENCE
struct inter_freq_neigh_cell_info_s {
  uint16_t         pci;
  enum q_offset_range_e q_offset_cell;
};

enum range_e_ {
  n4,
  n8,
  n12,
  n16,
  n24,
  n32,
  n48,
  n64,
  n84,
  n96,
  n128,
  n168,
  n252,
  n504,
  spare2,
  spare1,
  nulltype
};

// PhysCellIdRange ::= SEQUENCE
struct pci_range_s {
  // member variables
  bool     range_present;
  uint16_t start;
  enum range_e_ range;
};

// InterFreqCarrierFreqInfo ::= SEQUENCE
struct inter_freq_carrier_freq_info_s {
  // member variables
  bool                         p_max_present;
  bool                         t_resel_eutra_sf_present;
  bool                         cell_resel_prio_present;
  bool                         q_offset_freq_present;
  bool                         inter_freq_neigh_cell_list_present;
  bool                         inter_freq_black_cell_list_present;
  uint32_t                     dl_carrier_freq;
  int8_t                       q_rx_lev_min;//                       = -70
  int8_t                       p_max;//                              = -30
  uint8_t                      t_resel_eutra;
  struct speed_state_scale_factors_s  t_resel_eutra_sf;
  uint8_t                      thresh_x_high;
  uint8_t                      thresh_x_low;
  enum allowed_meas_bw_e       allowed_meas_bw;
  bool                         presence_ant_port1;
  uint8_t                      cell_resel_prio;
  uint8_t                      neigh_cell_cfg;//fixed_bitstring<2>
  enum q_offset_range_e        q_offset_freq;
  cvector_vector_t(struct inter_freq_neigh_cell_info_s) inter_freq_neigh_cell_list;// InterFreqNeighCellList ::= SEQUENCE (SIZE (1..16)) OF InterFreqNeighCellInfo
  cvector_vector_t(struct pci_range_s) inter_freq_black_cell_list;// InterFreqBlackCellList ::= SEQUENCE (SIZE (1..16)) OF PhysCellIdRange
  // ...
  // group 0
  bool                       q_qual_min_r9_present;
  int8_t                     q_qual_min_r9;//         = -34
  thresh_x_q_r9_s_           *thresh_x_q_r9;
  // group 1
  bool   q_qual_min_wb_r11_present;
  int8_t q_qual_min_wb_r11;//         = -34
};

// IntraFreqNeighCellInfo ::= SEQUENCE
struct intra_freq_neigh_cell_info_s {
  bool             q_rx_lev_min_offset_cell_present;
  bool             q_rx_lev_min_offset_cell_sul_present;
  bool             q_qual_min_offset_cell_present;
  uint16_t         pci;
  enum q_offset_range_e q_offset_cell;
  uint8_t          q_rx_lev_min_offset_cell1;//     = 1
  uint8_t          q_rx_lev_min_offset_cell_sul;//  = 1
  uint8_t          q_qual_min_offset_cell;//        = 1
};

// SIB2 ::= SEQUENCE
struct sib2_s {
  // member variables
  struct cell_resel_info_common_s_       cell_resel_info_common;
  struct cell_resel_serving_freq_info_s_ cell_resel_serving_freq_info;
  struct intra_freq_cell_resel_info_s_   intra_freq_cell_resel_info;
};

// SIB3 ::= SEQUENCE
struct sib3_s {
  bool                         exte;
  cvector_vector_t(struct intra_freq_neigh_cell_info_s) intra_freq_neigh_cell_list;// IntraFreqNeighCellList ::= SEQUENCE (SIZE (1..16)) OF IntraFreqNeighCellInfo
  cvector_vector_t(struct pci_range_s) intra_freq_black_cell_list;// IntraFreqBlackCellList ::= SEQUENCE (SIZE (1..16)) OF PCI-Range
  dyn_octstring                late_non_crit_ext;
};

// SIB4 ::= SEQUENCE
struct sib4_s {
  cvector_vector_t(struct inter_freq_carrier_freq_info_s) inter_freq_carrier_freq_list;// InterFreqCarrierFreqList ::= SEQUENCE (SIZE (1..8)) OF InterFreqCarrierFreqInfo
  dyn_octstring                  late_non_crit_ext;
};

struct thresh_x_q_s_ {
  uint8_t thresh_x_high_q;
  uint8_t thresh_x_low_q;
};

// EUTRA-NS-PmaxValue ::= SEQUENCE
struct eutra_ns_pmax_value_s {
  bool     add_pmax_present;
  bool     add_spec_emission_present;
  int8_t   add_pmax;//                  = -30
  uint16_t add_spec_emission;//         = 1
};

// EUTRA-MultiBandInfo ::= SEQUENCE
struct eutra_multi_band_info_s {
  uint16_t             eutra_freq_band_ind;// = 1
  cvector_vector_t(struct eutra_ns_pmax_value_s) eutra_ns_pmax_list;//// EUTRA-NS-PmaxList ::= SEQUENCE (SIZE (1..8)) OF EUTRA-NS-PmaxValue
};

enum eutra_q_offset_range_e {
  db_minus24,
  db_minus22,
  db_minus20,
  db_minus18,
  db_minus16,
  db_minus14,
  db_minus12,
  db_minus10,
  db_minus8,
  db_minus6,
  db_minus5,
  db_minus4,
  db_minus3,
  db_minus2,
  db_minus1,
  db0,
  db1,
  db2,
  db3,
  db4,
  db5,
  db6,
  db8,
  db10,
  db12,
  db14,
  db16,
  db18,
  db20,
  db22,
  db24,
  nulltype
};

// EUTRA-FreqNeighCellInfo ::= SEQUENCE
struct eutra_freq_neigh_cell_info_s {
  bool                   q_rx_lev_min_offset_cell_present;
  bool                   q_qual_min_offset_cell_present;
  uint16_t               pci;
  enum eutra_q_offset_range_e dummy;
  uint8_t                q_rx_lev_min_offset_cell;// = 1
  uint8_t                q_qual_min_offset_cell;//   = 1
};

enum range_e_ {
  n4,
  n8,
  n12,
  n16,
  n24,
  n32,
  n48,
  n64,
  n84,
  n96,
  n128,
  n168,
  n252,
  n504,
  spare2,
  spare1,
  nulltype
};

// EUTRA-PhysCellIdRange ::= SEQUENCE
struct eutra_pci_range_s {
  // member variables
  bool     range_present;
  uint16_t start;
  range_e_ range;
};

enum eutra_allowed_meas_bw_e { mbw6, mbw15, mbw25, mbw50, mbw75, mbw100, nulltype };

// CarrierFreqEUTRA ::= SEQUENCE
struct carrier_freq_eutra_s {
  // member variables
  bool                         cell_resel_prio_present;
  bool                         cell_resel_sub_prio_present;
  bool                         thresh_x_q_present;
  uint32_t                     carrier_freq;
  cvector_vector_t(struct eutra_multi_band_info_s) eutra_multi_band_info_list;// EUTRA-MultiBandInfoList ::= SEQUENCE (SIZE (1..8)) OF EUTRA-MultiBandInfo
  cvector_vector_t(struct eutra_freq_neigh_cell_info_s)  eutra_freq_neigh_cell_list;// EUTRA-FreqNeighCellList ::= SEQUENCE (SIZE (1..8)) OF EUTRA-FreqNeighCellInfo
  cvector_vector_t(struct eutra_pci_range_s) eutra_black_cell_list;// EUTRA-FreqBlackCellList ::= SEQUENCE (SIZE (1..16)) OF EUTRA-PhysCellIdRange
  enum eutra_allowed_meas_bw_e     allowed_meas_bw;
  bool                         presence_ant_port1;
  uint8_t                      cell_resel_prio;
  cell_resel_sub_prio_e        cell_resel_sub_prio;
  uint8_t                      thresh_x_high;
  uint8_t                      thresh_x_low;
  int8_t                       q_rx_lev_min;//  = -70
  int8_t                       q_qual_min;//    = -34
  int8_t                       p_max_eutra;//   = -30
  struct thresh_x_q_s_         thresh_x_q;
};

// SIB5 ::= SEQUENCE
struct sib5_s {
  bool                        t_resel_eutra_sf_present;
  cvector_vector_t(struct carrier_freq_eutra_s)   carrier_freq_list_eutra;//// CarrierFreqListEUTRA ::= SEQUENCE (SIZE (1..8)) OF CarrierFreqEUTRA
  uint8_t                     t_resel_eutra;
  struct speed_state_scale_factors_s t_resel_eutra_sf;
  dyn_octstring               late_non_crit_ext;
};

// SIB6 ::= SEQUENCE
struct sib6_s {
  uint8_t             msg_id[2];//fixed_bitstring<16>
  uint8_t             serial_num[2];//fixed_bitstring<16>
  fixed_octstring     warning_type[2];//fixed_octstring<2>
  dyn_octstring       late_non_crit_ext;
};

enum warning_msg_segment_type_e_ { not_last_segment, last_segment, nulltype };

// SIB7 ::= SEQUENCE
struct sib7_s {
  // member variables
  bool                        data_coding_scheme_present;
  uint8_t                     msg_id[2];//fixed_bitstring<16>
  uint8_t                     serial_num[2];//fixed_bitstring<16>
  enum warning_msg_segment_type_e_ warning_msg_segment_type;
  uint8_t                     warning_msg_segment_num;
  dyn_octstring               warning_msg_segment;
  fixed_octstring             data_coding_scheme;//fixed_octstring<1>
  dyn_octstring               late_non_crit_ext;
};

// SIB8 ::= SEQUENCE
struct sib8_s {
  // member variables
  bool                        data_coding_scheme_present;
  uint8_t                     msg_id[2];//fixed_bitstring<16>
  uint8_t                     serial_num[2];//fixed_bitstring<16>
  enum warning_msg_segment_type_e_ warning_msg_segment_type;
  uint8_t                     warning_msg_segment_num;
  dyn_octstring               warning_msg_segment;
  fixed_octstring             data_coding_scheme;//fixed_octstring<1>
  dyn_octstring               warning_area_coordinates_segment;
  dyn_octstring               late_non_crit_ext;
};


struct time_info_s_ {
  bool				 day_light_saving_time_present;
  bool				 leap_seconds_present;
  bool				 local_time_offset_present;
  uint64_t			 time_info_utc;
  uint8_t			 day_light_saving_time;//fixed_bitstring<2>
  int16_t			 leap_seconds;// = -127
  int8_t			 local_time_offset;// = -63
};

// SIB9 ::= SEQUENCE
struct sib9_s {
  // member variables
  bool                 time_info_present;
  struct time_info_s_  time_info;
  dyn_octstring late_non_crit_ext;
};

enum sib_type_and_info_item_e_ { sib2, sib3, sib4, sib5, sib6, sib7, sib8, sib9, /*...*/ nulltype };
struct sib_type_and_info_item_c_ {
  enum sib_type_and_info_item_e_  type_;
  union{
		struct sib2_s sib2;
		struct sib3_s sib3;
		struct sib4_s sib4;
		struct sib5_s sib5;
		struct sib6_s sib6;
		struct sib7_s sib7;
		struct sib8_s sib8;
		struct sib9_s sib9;
	   }c;
};

enum sub_carrier_spacing_common_e_ { scs15or60, scs30or120, nulltype };
enum cell_barred_e_ { barred, not_barred, nulltype };
enum intra_freq_resel_e_ { allowed, not_allowed, nulltype };

// MIB ::= SEQUENCE
struct mib_s {

  // member variables
  uint8_t                            sys_frame_num;//fixed_bitstring<6>
  enum sub_carrier_spacing_common_e_ sub_carrier_spacing_common;
  uint8_t                            ssb_subcarrier_offset;
  enum dmrs_type_a_position_e_       dmrs_type_a_position;
  struct pdcch_cfg_sib1_s            pdcch_cfg_sib1;
  enum cell_barred_e_                cell_barred;
  enum intra_freq_resel_e_           intra_freq_resel;
  uint8_t                            spare;//fixed_bitstring<1>
};

struct cell_sel_info_s_ {
  bool	  q_rx_lev_min_offset_present;
  bool	  q_rx_lev_min_sul_present;
  bool	  q_qual_min_present;
  bool	  q_qual_min_offset_present;
  int8_t  q_rx_lev_min;//        = -70
  uint8_t q_rx_lev_min_offset;// = 1
  int8_t  q_rx_lev_min_sul;//    = -70
  int8_t  q_qual_min;//			 = -43
  uint8_t q_qual_min_offset;//   = 1
};

// UAC-AccessCategory1-SelectionAssistanceInfo ::= ENUMERATED
enum uac_access_category1_sel_assist_info_e { a, b, c, nulltype };

struct uac_access_category1_sel_assist_info_c_ {
  enum types { plmn_common, individual_plmn_list, nulltype }  type_;
  cvector_vector_t(struct uac_access_category1_sel_assist_info_e) c;//choice_buffer_t//bounded_array<uac_access_category1_sel_assist_info_e, 12>
};

// UAC-BarringPerCat ::= SEQUENCE
struct uac_barr_per_cat_s {
  uint8_t access_category;//       = 1
  uint8_t uac_barr_info_set_idx;// = 1
};

//dyn_array<uac_barr_per_cat_s>//// UAC-BarringPerCatList ::= SEQUENCE (SIZE (1..63)) OF UAC-BarringPerCat
struct uac_ac_barr_list_type_c_ {
  enum types { uac_implicit_ac_barr_list, uac_explicit_ac_barr_list, nulltype }    type_;
  union {cvector_vector_t(struct uac_barr_per_cat_s) uac_implicit_ac_barr_l;uint8_t uac_implicit_ac_barr_l[63];} c;//choice_buffer_t<uac_barr_per_cat_list_l, uac_implicit_ac_barr_list_l_>
};

// UAC-BarringPerPLMN ::= SEQUENCE
struct uac_barr_per_plmn_s {
  // member variables
  bool                     uac_ac_barr_list_type_present;
  uint8_t                  plmn_id_idx;// = 1
  struct uac_ac_barr_list_type_c_ uac_ac_barr_list_type;
};

enum uac_barr_factor_e_ { p00, p05, p10, p15, p20, p25, p30, p40, p50, p60, p70, p75, p80, p85, p90, p95, nulltype };
enum uac_barr_time_e_ { s4, s8, s16, s32, s64, s128, s256, s512, nulltype };

// UAC-BarringInfoSet ::= SEQUENCE
struct uac_barr_info_set_s {
  // member variables
  enum uac_barr_factor_e_ uac_barr_factor;
  enum uac_barr_time_e_   uac_barr_time;
  uint8_t uac_barr_for_access_id;//fixed_bitstring<7>
};

struct uac_barr_info_s_ {
  // member variables
  bool									  uac_access_category1_sel_assist_info_present;
  cvector_vector_t(struct uac_barr_per_cat_s)	 uac_barr_for_common;// UAC-BarringPerCatList ::= SEQUENCE (SIZE (1..63)) OF UAC-BarringPerCat
  cvector_vector_t(struct uac_barr_per_plmn_s) uac_barr_per_plmn_list;// UAC-BarringPerPLMN-List ::= SEQUENCE (SIZE (1..12)) OF UAC-BarringPerPLMN
  cvector_vector_t(struct uac_barr_info_set_s) uac_barr_info_set_list;// UAC-BarringInfoSetList ::= SEQUENCE (SIZE (1..8)) OF UAC-BarringInfoSet
  struct uac_access_category1_sel_assist_info_c_ uac_access_category1_sel_assist_info;
};

enum cell_reserved_for_oper_e_ { reserved, not_reserved, nulltype };
// PLMN-IdentityInfo ::= SEQUENCE
struct plmn_id_info_s {
  // member variables
  bool                      tac_present;
  bool                      ranac_present;
  cvector_vector_t(struct plmn_id_s)   plmn_id_list;//dyn_array<plmn_id_s>
  uint8_t                   tac[3];//fixed_bitstring<24>
  uint16_t                  ranac;
  uint8_t                   cell_id[5];//fixed_bitstring<36>
  enum cell_reserved_for_oper_e_ cell_reserved_for_oper;
};


// CellAccessRelatedInfo ::= SEQUENCE
struct cell_access_related_info_s {
  bool                cell_reserved_for_other_use_present;
  cvector_vector_t(struct plmn_id_info_s) plmn_id_list;// PLMN-IdentityInfoList ::= SEQUENCE (SIZE (1..12)) OF PLMN-IdentityInfo
};

enum conn_est_fail_count_e_ { n1, n2, n3, n4, nulltype };
enum conn_est_fail_offset_validity_e_ { s30, s60, s120, s240, s300, s420, s600, s900, nulltype };

// ConnEstFailureControl ::= SEQUENCE
struct conn_est_fail_ctrl_s {
  // member variables
  bool                                  conn_est_fail_offset_present;
  enum conn_est_fail_count_e_           conn_est_fail_count;
  enum conn_est_fail_offset_validity_e_ conn_est_fail_offset_validity;
  uint8_t                               conn_est_fail_offset;
};

enum ssb_per_rach_occasion_e_ { one_eighth, one_fourth, one_half, one, two, four, eight, sixteen, nulltype };
struct rach_occasions_si_s_ {
  // member variables
  struct rach_cfg_generic_s	   rach_cfg_si;
  enum ssb_per_rach_occasion_e_ ssb_per_rach_occasion;
};

// SI-RequestResources ::= SEQUENCE
struct si_request_res_s {
  bool    ra_assoc_period_idx_present;
  bool    ra_ssb_occasion_mask_idx_present;
  uint8_t ra_preamb_start_idx;
  uint8_t ra_assoc_period_idx;
  uint8_t ra_ssb_occasion_mask_idx;
};

enum si_request_period_e_ { one, two, four, six, eight, ten, twelve, sixteen, nulltype };

// SI-RequestConfig ::= SEQUENCE
struct si_request_cfg_s {
  // member variables
  bool                 rach_occasions_si_present;
  bool                 si_request_period_present;
  struct rach_occasions_si_s_ rach_occasions_si;
  enum si_request_period_e_   si_request_period;
  cvector_vector_t(struct si_request_res_s)    si_request_res;//dyn_array<si_request_res_s>
};

enum si_win_len_e_ { s5, s10, s20, s40, s80, s160, s320, s640, s1280, nulltype };

enum type_e_ {
  sib_type2,
  sib_type3,
  sib_type4,
  sib_type5,
  sib_type6,
  sib_type7,
  sib_type8,
  sib_type9,
  spare8,
  spare7,
  spare6,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  // ...
  nulltype
};

// SIB-TypeInfo ::= SEQUENCE
struct sib_type_info_s {
  // member variables
  bool    value_tag_present;
  bool    area_scope_present;
  enum type_e_ type;
  uint8_t value_tag;
};

enum si_broadcast_status_e_ { broadcasting, not_broadcasting, nulltype };
enum si_periodicity_e_ { rf8, rf16, rf32, rf64, rf128, rf256, rf512, nulltype };

// SchedulingInfo ::= SEQUENCE
struct sched_info_s {
  // member variables
  enum si_broadcast_status_e_ si_broadcast_status;
  enum si_periodicity_e_      si_periodicity;
  cvector_vector_t(struct sib_type_info_s)   sib_map_info;//dyn_array<sib_type_info_s>
};

// SI-SchedulingInfo ::= SEQUENCE
struct si_sched_info_s {
  // member variables
  bool                si_request_cfg_present;
  bool                si_request_cfg_sul_present;
  bool                sys_info_area_id_present;
  cvector_vector_t(struct sched_info_s)  sched_info_list;//dyn_array<sched_info_s>
  enum si_win_len_e_              si_win_len;
  struct si_request_cfg_s    si_request_cfg;
  struct si_request_cfg_s    si_request_cfg_sul;
  uint8_t                    sys_info_area_id[3];//fixed_bitstring<24>
};

enum n_timing_advance_offset_e_ { n0, n25600, n39936, nulltype };
/*struct ssb_positions_in_burst_s_ {
  bool		group_presence_present;
  uint8_t   in_one_group;//fixed_bitstring<8>
  uint8_t   group_presence;//fixed_bitstring<8>
};*/

enum ssb_periodicity_serving_cell_sib_e_ { ms5, ms10, ms20, ms40, ms80, ms160, nulltype };

// ServingCellConfigCommonSIB ::= SEQUENCE
struct serving_cell_cfg_common_sib_s {
  // member variables
  bool                            ul_cfg_common_present;
  bool                            supplementary_ul_present;
  bool                            n_timing_advance_offset_present;
  bool                            tdd_ul_dl_cfg_common_present;
  struct dl_cfg_common_sib_s             dl_cfg_common;
  struct ul_cfg_common_sib_s             ul_cfg_common;
  struct ul_cfg_common_sib_s             supplementary_ul;
  enum n_timing_advance_offset_e_        n_timing_advance_offset;
  struct ssb_positions_in_burst_s_       ssb_positions_in_burst;
  enum ssb_periodicity_serving_cell_sib_e_   ssb_periodicity_serving_cell;
  struct tdd_ul_dl_cfg_common_s          tdd_ul_dl_cfg_common;
  int8_t                                 ss_pbch_block_pwr;// = -60
};

enum t300_e_ { ms100, ms200, ms300, ms400, ms600, ms1000, ms1500, ms2000, nulltype };
enum t301_e_ { ms100, ms200, ms300, ms400, ms600, ms1000, ms1500, ms2000, nulltype };
enum t319_e_ { ms100, ms200, ms300, ms400, ms600, ms1000, ms1500, ms2000, nulltype };

// UE-TimersAndConstants ::= SEQUENCE
struct ue_timers_and_consts_s {
  enum t300_e_ t300;
  enum t301_e_ t301;
  enum t310_e_ t310;
  enum n310_e_ n310;
  enum t311_e_ t311;
  enum n311_e_ n311;
  enum t319_e_ t319;
};

// SIB1 ::= SEQUENCE
struct sib1_s {
  // member variables
  bool                          cell_sel_info_present;
  bool                          conn_est_fail_ctrl_present;
  bool                          si_sched_info_present;
  bool                          serving_cell_cfg_common_present;
  bool                          ims_emergency_support_present;
  bool                          ecall_over_ims_support_present;
  bool                          ue_timers_and_consts_present;
  bool                          uac_barr_info_present;
  bool                          use_full_resume_id_present;
  bool                          non_crit_ext_present;
  struct cell_sel_info_s_              cell_sel_info;
  struct cell_access_related_info_s    cell_access_related_info;
  struct conn_est_fail_ctrl_s          conn_est_fail_ctrl;
  struct si_sched_info_s               si_sched_info;
  struct serving_cell_cfg_common_sib_s serving_cell_cfg_common;
  struct ue_timers_and_consts_s        ue_timers_and_consts;
  struct uac_barr_info_s_              uac_barr_info;
  dyn_octstring                 late_non_crit_ext;
};

enum prioritised_bit_rate_e_ {
  kbps0,
  kbps8,
  kbps16,
  kbps32,
  kbps64,
  kbps128,
  kbps256,
  kbps512,
  kbps1024,
  kbps2048,
  kbps4096,
  kbps8192,
  kbps16384,
  kbps32768,
  kbps65536,
  infinity,
  nulltype
};
enum bucket_size_dur_e_ {
  ms5,
  ms10,
  ms20,
  ms50,
  ms100,
  ms150,
  ms300,
  ms500,
  ms1000,
  spare7,
  spare6,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
};

enum max_pusch_dur_e_ { ms0p02, ms0p04, ms0p0625, ms0p125, ms0p25, ms0p5, spare2, spare1, nulltype };
enum bit_rate_query_prohibit_timer_e_ { s0, s0dot4, s0dot8, s1dot6, s3, s6, s12, s30, nulltype };

struct ul_specific_params_s_ {
  // member variables
  bool					   max_pusch_dur_present;
  bool					   cfgured_grant_type1_allowed_present;
  bool					   lc_ch_group_present;
  bool					   sched_request_id_present;
  uint8_t				   prio;//	 = 1
  enum prioritised_bit_rate_e_  prioritised_bit_rate;
  enum bucket_size_dur_e_	   bucket_size_dur;
  cvector_vector_t(uint8_t)  allowed_serving_cells;//bounded_array<uint8_t, 31>
  cvector_vector_t(struct subcarrier_spacing_e)  allowed_scs_list;//bounded_array<subcarrier_spacing_e, 5>
  enum max_pusch_dur_e_		   max_pusch_dur;
  uint8_t				   lc_ch_group;
  uint8_t				   sched_request_id;
  bool					   lc_ch_sr_mask;
  bool					   lc_ch_sr_delay_timer_applied;
  // ...
  bool							   bit_rate_query_prohibit_timer_present;
  enum bit_rate_query_prohibit_timer_e_ bit_rate_query_prohibit_timer;
};

// LogicalChannelConfig ::= SEQUENCE
struct lc_ch_cfg_s {

  // member variables
  bool                  ul_specific_params_present;
  struct ul_specific_params_s_ ul_specific_params;
};


enum served_radio_bearer_types { srb_id, drb_id, nulltype };

struct served_radio_bearer_c_ {
  enum served_radio_bearer_types type_;
  uint8_t c;//pod_choice_buffer_t
};

// RLC-BearerConfig ::= SEQUENCE
struct rlc_bearer_cfg_s {
  // member variables
  bool                   served_radio_bearer_present;
  bool                   reestablish_rlc_present;
  bool                   rlc_cfg_present;
  bool                   mac_lc_ch_cfg_present;
  uint8_t                lc_ch_id;//   = 1
  struct served_radio_bearer_c_ served_radio_bearer;
  struct rlc_cfg_c       rlc_cfg;
  struct lc_ch_cfg_s     mac_lc_ch_cfg;
  // ...
};

// SCellConfig ::= SEQUENCE
struct scell_cfg_s {
  bool                      scell_cfg_common_present;
  bool                      scell_cfg_ded_present;
  uint8_t                   scell_idx;// = 1
  struct serving_cell_cfg_common_s scell_cfg_common;
  struct serving_cell_cfg_s        scell_cfg_ded;
  // ...
};

// CellGroupConfig ::= SEQUENCE
struct cell_group_cfg_s {
  // member variables
  bool                          mac_cell_group_cfg_present;
  bool                          phys_cell_group_cfg_present;
  bool                          sp_cell_cfg_present;
  uint8_t                       cell_group_id;
  cvector_vector_t(struct rlc_bearer_cfg_s) rlc_bearer_to_add_mod_list;//dyn_array<rlc_bearer_cfg_s>
  cvector_vector_t(uint8_t)       rlc_bearer_to_release_list;//bounded_array<uint8_t, 32>
  struct mac_cell_group_cfg_s     mac_cell_group_cfg;
  struct phys_cell_group_cfg_s    phys_cell_group_cfg;
  struct sp_cell_cfg_s            sp_cell_cfg;
  cvector_vector_t(struct scell_cfg_s)      scell_to_add_mod_list;//dyn_array<scell_cfg_s>
  cvector_vector_t(uint8_t)       scell_to_release_list;//bounded_array<uint8_t, 31>
};

// EstablishmentCause ::= ENUMERATED
enum establishment_cause_e {
	emergency,
	high_prio_access,
	mt_access,
	mo_sig,
	mo_data,
	mo_voice_call,
	mo_video_call,
	mo_sms,
	mps_prio_access,
	mcs_prio_access,
	spare6,
	spare5,
	spare4,
	spare3,
	spare2,
	spare1,
	nulltype
};

// SRB-ToAddMod ::= SEQUENCE
struct srb_to_add_mod_s {
  bool       reestablish_pdcp_present;
  bool       discard_on_pdcp_present;
  bool       pdcp_cfg_present;
  uint8_t    srb_id;//1
  struct pdcp_cfg_s pdcp_cfg;
};


enum sdap_hdr_dl_e_ { present, absent, nulltype };
enum sdap_hdr_ul_e_ { present, absent, nulltype };
// SDAP-Config ::= SEQUENCE
struct sdap_cfg_s {
  // member variables
  uint16_t                       pdu_session;
  enum sdap_hdr_dl_e_            sdap_hdr_dl;
  enum sdap_hdr_ul_e_            sdap_hdr_ul;
  bool                           default_drb;
  cvector_vector_t(uint8_t)        mapped_qos_flows_to_add;//dyn_array<uint8_t>
  cvector_vector_t(uint8_t)        mapped_qos_flows_to_release;//dyn_array<uint8_t>
};

struct cn_assoc_c_ {
  enum types { eps_bearer_id, sdap_cfg, nulltype } type_;
  struct sdap_cfg_s c;//choice_buffer_t
};

// DRB-ToAddMod ::= SEQUENCE
struct drb_to_add_mod_s {
  // member variables
  bool        cn_assoc_present;
  bool        reestablish_pdcp_present;
  bool        recover_pdcp_present;
  bool        pdcp_cfg_present;
  struct cn_assoc_c_ cn_assoc;
  uint8_t            drb_id;//1
  struct pdcp_cfg_s  pdcp_cfg;
};

// CipheringAlgorithm ::= ENUMERATED
enum ciphering_algorithm_e { nea0, nea1, nea2, nea3, spare4, spare3, spare2, spare1, /*...*/ nulltype };
// IntegrityProtAlgorithm ::= ENUMERATED
enum integrity_prot_algorithm_e { nia0, nia1, nia2, nia3, spare4, spare3, spare2, spare1, /*...*/ nulltype };
// SecurityAlgorithmConfig ::= SEQUENCE
struct security_algorithm_cfg_s {
  bool                       integrity_prot_algorithm_present;
  enum  ciphering_algorithm_e  ciphering_algorithm;
  enum  integrity_prot_algorithm_e integrity_prot_algorithm;
};

enum key_to_use_e_ { master, secondary, nulltype };
// SecurityConfig ::= SEQUENCE
struct security_cfg_s {
  // member variables;
  bool                     security_algorithm_cfg_present;
  bool                     key_to_use_present;
  struct  security_algorithm_cfg_s security_algorithm_cfg;
  enum key_to_use_e_       key_to_use;
};


// RadioBearerConfig ::= SEQUENCE
struct radio_bearer_cfg_s {
  bool                  srb3_to_release_present;
  bool                  security_cfg_present;
  cvector_vector_t(struct srb_to_add_mod_s) srb_to_add_mod_list;// SRB-ToAddModList ::= SEQUENCE (SIZE (1..2)) OF SRB-ToAddMod
  cvector_vector_t(struct drb_to_add_mod_s) drb_to_add_mod_list;// DRB-ToAddModList ::= SEQUENCE (SIZE (1..29)) OF DRB-ToAddMod
  cvector_vector_t(uint8_t) drb_to_release_list;// DRB-ToReleaseList ::= SEQUENCE (SIZE (1..29)) OF INTEGER (1..32)
  struct security_cfg_s   security_cfg;
};

// UESecurityCapabilities ::= SEQUENCE
struct ue_security_cap_s {
  uint8_t       nrencryption_algorithms[2];//fixed_bitstring<16, true, true>
  uint8_t       nrintegrity_protection_algorithms[2];//fixed_bitstring<16, true, true>
  uint8_t       eutr_aencryption_algorithms[2];//fixed_bitstring<16, true, true>
  uint8_t       eutr_aintegrity_protection_algorithms[2];//fixed_bitstring<16, true, true>
};

#ifdef __cplusplus
}
#endif

#endif
