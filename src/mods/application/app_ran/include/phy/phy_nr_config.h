/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef PHY_NR_CONFIG_H_
#define PHY_NR_CONFIG_H_

#include "lib/srsran/srsran.h"
#include "ASN_RRC_PDSCH-ConfigCommon.h"
#include "ASN_RRC_PUSCH-ConfigCommon.h"
#include "ASN_RRC_PUCCH-ConfigCommon.h"


#ifdef __cplusplus
extern "C" {
#endif
enum setup_opts { release, setup, nulltype };

/**********************/
enum reg_bundle_size_opts { n2, n3, n6, nulltype };
enum interleaver_size_opts { n2, n3, n6, nulltype };
struct interleaved_s_ {

  bool				  shift_idx_present;
  enum reg_bundle_size_opts   reg_bundle_size;
  enum interleaver_size_opts interleaver_size;
  uint16_t			  shift_idx;
};

enum types_opts { interleaved, non_interleaved, nulltype };
struct cce_reg_map_type_c_ {
  enum types_opts types;
  struct interleaved_s_ c;
};

enum precoder_granularity_opts { same_as_reg_bundle, all_contiguous_rbs, nulltype };
// ControlResourceSet
struct ctrl_res_set_s {
  bool                                ext;
  bool                                tci_present_in_dci_present;
  bool                                pdcch_dmrs_scrambling_id_present;
  uint8_t                             ctrl_res_set_id;
  uint8_t                             freq_domain_res[6];//fixed_bitstring<45>
  uint8_t                             dur;//1
  struct cce_reg_map_type_c_          cce_reg_map_type;
  enum precoder_granularity_opts      precoder_granularity;
  A_DYN_ARRAY_OF(uint8_t)             tci_states_pdcch_to_add_list;//dyn_array<uint8_t>
  A_DYN_ARRAY_OF(uint8_t)             tci_states_pdcch_to_release_list;//dyn_array<uint8_t>
  uint32_t                            pdcch_dmrs_scrambling_id;
};

struct monitoring_slot_periodicity_and_offset_c_ {
	enum types_opts {
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
	} types;
    //pod_choice_buffer_t c;//???
};


enum aggregation_level1_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
enum aggregation_level2_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
enum aggregation_level4_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
enum aggregation_level8_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
enum aggregation_level16_opts { n0, n1, n2, n3, n4, n5, n6, n8, nulltype };
struct nrof_candidates_s_ {
  enum aggregation_level1_opts  aggregation_level1;
  enum aggregation_level2_opts  aggregation_level2;
  enum aggregation_level4_opts  aggregation_level4;
  enum aggregation_level8_opts  aggregation_level8;
  enum aggregation_level16_opts aggregation_level16;
};

struct dci_format0_minus0_and_format1_minus0_s_ {
  bool ext;
};

enum aggregation_level1_opts { n1, n2, nulltype };
enum aggregation_level2_opts { n1, n2, nulltype };
enum aggregation_level4_opts { n1, n2, nulltype };
enum aggregation_level8_opts { n1, n2, nulltype };
enum aggregation_level16_opts { n1, n2, nulltype };
struct nrof_candidates_sfi_s_ {
  bool					 aggregation_level1_present;
  bool					 aggregation_level2_present;
  bool					 aggregation_level4_present;
  bool					 aggregation_level8_present;
  bool					 aggregation_level16_present;
  enum aggregation_level1_opts	aggregation_level1;
  enum aggregation_level2_opts	aggregation_level2;
  enum aggregation_level4_opts	aggregation_level4;
  enum aggregation_level8_opts	aggregation_level8;
  enum aggregation_level16_opts aggregation_level16;
};

struct dci_format2_minus0_s_ {
  bool					 ext;
  struct nrof_candidates_sfi_s_ nrof_candidates_sfi;
  // ...
};

struct dci_format2_minus1_s_ {
  bool ext;
  // ...
};
struct dci_format2_minus2_s_ {
  bool ext;
  // ...
};

enum dummy1_opts { sl1, sl2, sl4, sl5, sl8, sl10, sl16, sl20, nulltype };
enum dummy2_opts { n1, n2, nulltype };
struct dci_format2_minus3_s_ {
  bool		ext;
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
  struct dci_format2_minus0_s_ 				   dci_format2_minus0;
  struct dci_format2_minus1_s_ 				   dci_format2_minus1;
  struct dci_format2_minus2_s_ 				   dci_format2_minus2;
  struct dci_format2_minus3_s_ 				   dci_format2_minus3;
};

enum dci_formats_opts { formats0_minus0_and_minus1_minus0, formats0_minus1_and_minus1_minus1, nulltype };
struct ue_specific_s_ {
  bool			 ext;
  enum dci_formats_opts dci_formats;
  // ...
};

struct search_space_type_c_ {
  enum types_opts { common, ue_specific, nulltype } types ;
  union{
     struct common_s_       common;
     struct ue_specific_s_  ue_spec;
  }c;
};

struct search_space_s{
  bool                                      ctrl_res_set_id_present;
  bool                                      monitoring_slot_periodicity_and_offset_present;
  bool                                      dur_present;
  bool                                      monitoring_symbols_within_slot_present;
  bool                                      nrof_candidates_present;
  bool                                      search_space_type_present;
  uint8_t                                   search_space_id;
  uint8_t                                   ctrl_res_set_id;
  struct monitoring_slot_periodicity_and_offset_c_ monitoring_slot_periodicity_and_offset;
  uint16_t                                  dur;//2
  uint8_t                                   monitoring_symbols_within_slot[2];//fixed_bitstring<14>
  struct nrof_candidates_s_                 nrof_candidates;
  struct search_space_type_c_               search_space_type;
};


// PDCCH-ConfigCommon
struct pdcch_cfg_common_s {
  bool                        ext;
  bool                        ctrl_res_set_zero_present;
  bool                        common_ctrl_res_set_present;
  bool                        search_space_zero_present;
  bool                        search_space_sib1_present;
  bool                        search_space_other_sys_info_present;
  bool                        paging_search_space_present;
  bool                        ra_search_space_present;
  uint8_t                     ctrl_res_set_zero;
  struct ctrl_res_set_s       common_ctrl_res_set;
  uint8_t                     search_space_zero;
  uint8_t                     nof_common_search_space;
  A_DYN_ARRAY_OF(struct search_space_s)  common_search_space_list;//dyn_array<search_space_s>
  uint8_t                     search_space_sib1;
  uint8_t                     search_space_other_sys_info;
  uint8_t                     paging_search_space;
  uint8_t                     ra_search_space;
};


// PDCCH-Config
struct pdcch_cfg_s {
  bool                                 ext;
  bool                                 dl_preemption_present;
  bool                                 tpc_pusch_present;
  bool                                 tpc_pucch_present;
  bool                                 tpc_srs_present;
  int                                  nof_ctrl_res_set_to_add_mod;
  A_DYN_ARRAY_OF(struct ctrl_res_set_s) ctrl_res_set_to_add_mod_list;//dyn_array<ctrl_res_set_s>
  uint8_t                               ctrl_res_set_to_release_list[3];
  int                                   nof_search_spaces_to_add_mod;
  A_DYN_ARRAY_OF(struct search_space_s) search_spaces_to_add_mod_list;//dyn_array<ctrl_res_set_s>
  uint8_t                               search_spaces_to_release_list[10];
};

// PDSCH-ConfigCommon
struct pdsch_cfg_common_s {
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
  bool    group_hop_enabled;//false
  uint8_t group_assign_pusch;//0
  bool    seq_hop_enabled;//false
  uint8_t cyclic_shift;//0
};

struct pusch_cfg_common_s {
  struct pusch_cfg_basic_s_  pusch_cfg_basic;
  struct ul_ref_sigs_pusch_s ul_ref_sigs_pusch;
};

// PUCCH-ConfigCommon
enum delta_pucch_shift_opts { ds1, ds2, ds3, nulltype };
struct pucch_cfg_common_s {
  enum delta_pucch_shift_opts delta_pucch_shift;
  uint8_t              nrb_cqi;
  uint8_t              ncs_an;
  uint16_t             n1_pucch_an;
};

// PRACH-ConfigSIB
struct prach_cfg_info_s {
  uint8_t prach_cfg_idx;
  bool    high_speed_flag;
  uint8_t zero_correlation_zone_cfg;
  uint8_t prach_freq_offset;
};

struct prach_cfg_sib_s {
  uint16_t         root_seq_idx;
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

struct srs_ul_cfg_common_c {
  struct setup_s_ {
    bool          srs_max_up_pts_present;
    enum srs_bw_cfg_opts srs_bw_cfg;
    enum srs_sf_cfg_opts srs_sf_cfg;
    bool          ack_nack_srs_simul_tx;
  }c;
  enum setup_opts       types;
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
  bool							ext;
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
  enum preamb_trans_max_opts	     preamb_trans_max;
  enum ra_resp_win_size_opts		 ra_resp_win_size;
  enum mac_contention_resolution_timer_opts  mac_contention_resolution_timer;
};

struct pwr_ramp_params_s {
  enum pwr_ramp_step_opts             pwr_ramp_step;
  enum preamb_init_rx_target_pwr_opts preamb_init_rx_target_pwr;
};

struct rach_cfg_common_s {
  bool                   ext;
  struct preamb_info_s_  preamb_info;
  struct pwr_ramp_params_s      pwr_ramp_params;
  struct ra_supervision_info_s_ ra_supervision_info;
  uint8_t                max_harq_msg3_tx;//1
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

struct pcch_cfg_s {
  enum default_paging_cycle_opts  default_paging_cycle;
  enum nb_opts                    nb;
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

struct ul_pwr_ctrl_common_s {
  int8_t              p0_nominal_pusch;//-126
  enum alpha_r12_opts alpha;
  int8_t              p0_nominal_pucch;//-127
  struct delta_flist_pucch_s delta_flist_pucch;
  int8_t              delta_preamb_msg3;//-1
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
  enum max_harq_tx_opts	   max_harq_tx;
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
  enum setup_opts types;
};

// TimeAlignmentTimer
enum time_align_timer_opts { sf500, sf750, sf1280, sf1920, sf2560, sf5120, sf10240, infinity, nulltype };

// MAC-MainConfig
struct mac_main_cfg_s {
  bool               ext;
  bool               ul_sch_cfg_present;
  bool               drx_cfg_present;
  bool               phr_cfg_present;
  struct ul_sch_cfg_s_      ul_sch_cfg;
  enum time_align_timer_opts time_align_timer_ded;
  struct phr_cfg_c_         phr_cfg;
};

/*******************phy_cell_cfg_nr_t************************/
typedef struct phy_cell_cfg_nr_s {
  srsran_carrier_nr_t carrier;//phy cell
  uint32_t            rf_port;
  uint32_t            cell_id;
  float               gain_db;
  bool                dl_measure;
}phy_cell_cfg_nr_t;


/*******************phy_cfg_t************************/
typedef oset_list2_t phy_cell_cfg_list_nr_t;

typedef struct phy_cfg_s {
  // Individual cell/sector configuration list
  phy_cell_cfg_list_nr_t          *phy_cell_cfg_nr; //std::vector<phy_cell_cfg_nr_t>

  // Common configuration for all cells
  struct prach_cfg_sib_s          prach_cnfg;//4G??
  struct pdsch_cfg_common_s       pdsch_cnfg;//ASN_RRC_PDSCH_ConfigCommon_t
  struct pusch_cfg_common_s       pusch_cnfg;//ASN_RRC_PUSCH_ConfigCommon_t
  struct pucch_cfg_common_s       pucch_cnfg;//ASN_RRC_PUCCH_ConfigCommon_t
  struct srs_ul_cfg_common_c      srs_ul_cnfg;//4G??
  srsran_cfr_cfg_t cfr_config;
}phy_cfg_t;

#ifdef __cplusplus
}
#endif

#endif
