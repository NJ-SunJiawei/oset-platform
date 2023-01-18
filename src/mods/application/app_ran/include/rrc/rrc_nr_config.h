/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef RRC_NR_CONFIG_H_
#define RRC_NR_CONFIG_H_

#include "phy/phy_nr_config.h"
#include "asn/rrc/ASN_RRC_RLC-Config.h"
#include "asn/rrc/ASN_RRC_PDCP-Config.h"
#include "asn/rrc/ASN_RRC_PDCCH-ConfigCommon.h"
#include "asn/rrc/ASN_RRC_PDCCH-Config.h"
#include "asn/rrc/ASN_RRC_SchedulingRequestToAddMod.h"
#include "asn/rrc/ASN_RRC_LogicalChannelConfig.h"
#include "asn/rrc/ASN_RRC_SIB-TypeInfo.h"
#include "asn/rrc/ASN_RRC_MIB.h"
#include "asn/rrc/ASN_RRC_SIB1.h"
#include "asn/rrc/ASN_RRC_SIB2.h"
#include "asn/rrc/ASN_RRC_SIB3.h"
#include "asn/rrc/ASN_RRC_SIB4.h"
#include "asn/rrc/ASN_RRC_SIB5.h"
#include "asn/rrc/ASN_RRC_SIB6.h"
#include "asn/rrc/ASN_RRC_SIB7.h"
#include "asn/rrc/ASN_RRC_SIB8.h"
#include "asn/rrc/ASN_RRC_SIB9.h"

#ifdef __cplusplus
extern "C" {
#endif

// RLC-Config
enum rlc_types_opts { am, um_bi_dir, um_uni_dir_ul, um_uni_dir_dl, /*...*/ nulltype };
enum max_retx_thres_opts { t1, t2, t3, t4, t6, t8, t16, t32, nulltype };
enum poll_pdu_opts { p4, p8, p16, p32, p64, p128, p256, pinfinity, nulltype };

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
  ms800_v1310,
  ms1000_v1310,
  ms2000_v1310,
  ms4000_v1310,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
};
enum poll_byte_opts {
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
	kbinfinity,
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
  ms800_v1310,
  ms1000_v1310,
  ms1200_v1310,
  ms1600_v1310,
  ms2000_v1310,
  ms2400_v1310,
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
	bool profile0x0001 = false;
	bool profile0x0002 = false;
	bool profile0x0003 = false;
	bool profile0x0004 = false;
	bool profile0x0006 = false;
	bool profile0x0101 = false;
	bool profile0x0102 = false;
	bool profile0x0103 = false;
	bool profile0x0104 = false;
  };

  // member variables
  bool		  max_cid_present			= false;
  bool		  drb_continue_rohc_present = false;
  uint16_t	  max_cid					= 1;
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
  bool                 ext;
  bool                 drb_present;
  bool                 more_than_one_rlc_present;
  bool                 t_reordering_present;
  struct drb_s_        drb;
  struct more_than_one_rlc_s_ more_than_one_rlc;
  enum t_reordering_opts  t_reordering;
};

typedef struct srb_5g_cfg_s{
  bool                    present;false
  struct rlc_cfg_c        rlc_cfg;//ASN_RRC_RLC_Config_t
}srb_5g_cfg_t;

typedef struct rrc_nr_cfg_five_qi_s{
  bool           configured = false;
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
struct ul_specific_params_s_ {
  bool					  lc_ch_group_present;
  uint8_t				  prio;//1
  enum prioritised_bit_rate_opts  prioritised_bit_rate;
  enum bucket_size_dur_opts	  bucket_size_dur;
  uint8_t				  lc_ch_group ;
};

typedef struct rrc_cfg_qci_s{
  bool                          configured;//false
  int                           enb_dl_max_retx_thres;//-1
  struct ul_specific_params_s_  lc_cfg;//ASN_RRC_LogicalChannelConfig_t
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

struct plmn_id_info_s {
  struct plmn_id_s                 plmn_id;
  enum cell_reserved_for_oper_opts cell_reserved_for_oper;
};

struct cell_access_related_info_s_ {
  bool				  csg_id_present;
  plmn_id_info_s      plmn_id_list[6]; //PLMN-IdentityList ::= SEQUENCE (SIZE (1..6)) OF PLMN-IdentityInfo
  uint8_t             tac[2];//fixed_bitstring<16>
  uint8_t             cell_id[4];//fixed_bitstring<28>
  enum cell_barred_opts	  cell_barred;
  enum intra_freq_resel_opts intra_freq_resel;
  bool				  csg_ind;
  uint8_t             csg_id[4];//fixed_bitstring<27>
};

struct sched_info_s {
  enum si_periodicity_r12_opts si_periodicity;
  int                          nof_sib_map;//0
  enum sib_type_opts           sib_map_info[32];//SIB-MappingInfo ::= SEQUENCE (SIZE (0..31)) OF SIB-Type
};

struct tdd_cfg_s {
  enum sf_assign_opts           sf_assign;
  enum special_sf_patterns_opts special_sf_patterns;
};
// SystemInformationBlockType
struct sib_type1_s {
  bool                        p_max_present;//false
  bool                        tdd_cfg_present;//false
  bool                        non_crit_ext_present;//false
  struct cell_access_related_info_s_ cell_access_related_info;
  struct cell_sel_info_s_     cell_sel_info;
  int8_t                      p_max;//-30
  uint8_t                     freq_band_ind;//1
  uint8_t                     nof_sched_info;//1
  sched_info_s                sched_info_list[32];//SchedulingInfoList ::= SEQUENCE (SIZE (1..32)) OF SchedulingInfo
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
  bool                 ext;
  struct rach_cfg_common_s    rach_cfg_common;
  struct bcch_cfg_s           bcch_cfg;
  struct pcch_cfg_s           pcch_cfg;
  struct prach_cfg_sib_s      prach_cfg;
  struct pdsch_cfg_common_s   pdsch_cfg_common;
  struct pusch_cfg_common_s   pusch_cfg_common;
  struct pucch_cfg_common_s   pucch_cfg_common;
  struct srs_ul_cfg_common_c  srs_ul_cfg_common;
  struct ul_pwr_ctrl_common_s ul_pwr_ctrl_common;
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
struct ue_timers_and_consts_s {
  bool    ext;
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
struct sib_type2_s {
  bool                   ext;
  bool                   ac_barr_info_present;
  bool                   mbsfn_sf_cfg_list_present;
  struct ac_barr_info_s_ ac_barr_info;
  struct rr_cfg_common_sib_s  rr_cfg_common;
  struct ue_timers_and_consts_s ue_timers_and_consts;
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

struct cell_resel_info_common_s_ {
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
struct sib_type3_s {
  bool                                   ext;
  struct cell_resel_info_common_s_       cell_resel_info_common;
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
  enum setup_opts    type;
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
  bool                 ext;
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
  struct nr_multi_band_info_s       freq_band_list[8];//// MultiFrequencyBandListNR-SIB ::= SEQUENCE (SIZE (1..8)) OF NR-MultiBandInfo
  uint16_t                          offset_to_point_a;
  struct scs_specific_carrier_s     scs_specific_carrier_list[5];//???5 dyn_array
};

// BWP ::= SEQUENCE
struct bwp_s {
  bool                 cp_present;
  uint16_t             location_and_bw;
  subcarrier_spacing_e subcarrier_spacing;
};

// BWP-DownlinkCommon ::= SEQUENCE
struct bwp_dl_common_s {
  bool                                ext;
  bool                                pdcch_cfg_common_present;
  bool                                pdsch_cfg_common_present;
  struct bwp_s                        generic_params;
  struct pdcch_cfg_common_s           pdcch_cfg_common;
  struct pdsch_cfg_common_s           pdsch_cfg_common;
};

// DownlinkConfigCommonSIB ::= SEQUENCE
struct dl_cfg_common_sib_s {
  bool               ext;
  struct freq_info_dl_sib_s freq_info_dl;
  struct bwp_dl_common_s    init_dl_bwp;
  struct bcch_cfg_s         bcch_cfg;
  struct pcch_cfg_s         pcch_cfg;
};


// FrequencyInfoUL-SIB ::= SEQUENCE
struct freq_info_ul_sib_s {
  // member variables
  bool                          ext;
  bool                          absolute_freq_point_a_present;
  bool                          p_max_present;
  bool                          freq_shift7p5khz_present;
  struct nr_multi_band_info_s   freq_band_list;
  uint32_t                      absolute_freq_point_a;
  struct scs_specific_carrier_s scs_specific_carrier_list[5];//???5
  int8_t                        p_max;//-30
};

// BWP-UplinkCommon ::= SEQUENCE
struct bwp_ul_common_s {
  bool                       ext;
  bool                       rach_cfg_common_present;
  bool                       pusch_cfg_common_present;
  bool                       pucch_cfg_common_present;
  struct bwp_s               generic_params;
  struct rach_cfg_common_s   rach_cfg_common;
  struct pusch_cfg_common_s  pusch_cfg_common;
  struct pucch_cfg_common_s  pucch_cfg_common;
};

enum time_align_timer_e { ms500, ms750, ms1280, ms1920, ms2560, ms5120, ms10240, infinity, nulltype };

// UplinkConfigCommonSIB ::= SEQUENCE
struct ul_cfg_common_sib_s {
  freq_info_ul_sib_s freq_info_ul;
  bwp_ul_common_s    init_ul_bwp;
  enum time_align_timer_e time_align_timer_common;
};

/***************rrc_cell_cfg_nr_t****************************/
// Cell/Sector configuration for NR cells
typedef struct rrc_cell_cfg_nr_s{
  phy_cell_cfg_nr_t                phy_cell; // already contains all PHY-related parameters (i.e. RF port, PCI, etc.)
  uint32_t                         tac;      // Tracking area code
  uint32_t                         dl_arfcn; // DL freq already included in carrier
  uint32_t                         ul_arfcn; // UL freq also in carrier
  uint32_t                         dl_absolute_freq_point_a; // derived from DL ARFCN
  uint32_t                         ul_absolute_freq_point_a; // derived from UL ARFCN
  uint32_t                         band;
  uint32_t                         prach_root_seq_idx;
  uint32_t                         num_ra_preambles;
  uint32_t                         coreset0_idx; // Table 13-{1,...15} row index
  srsran_duplex_mode_t             duplex_mode;
  double                           ssb_freq_hz;
  uint32_t                         ssb_absolute_freq_point; // derived from DL ARFCN (SSB arfcn)
  uint32_t                         ssb_offset;
  srsran_subcarrier_spacing_t      ssb_scs;
  srsran_ssb_pattern_t             ssb_pattern;
  struct  pdcch_cfg_common_s       pdcch_cfg_common;//ASN_RRC_PDCCH_ConfigCommon_t
  struct  pdcch_cfg_s              pdcch_cfg_ded;//ASN_RRC_PDCCH_Config_t
  int8_t                           pdsch_rs_power;
}rrc_cell_cfg_nr_t;

/***************rrc_cfg_t****************************/
typedef struct rrc_cfg_s{
  uint32_t enb_id; ///< Required to pack SIB1
  // Per eNB SIBs
  srsran_mib_nr_t          mib;
  struct sib_type1_s       sib1;//ASN_RRC_SIB1_t
  enum sib_info_types_opts sibs_type;//ASN_RRC_SIB_TypeInfo_t 
  struct sib_type2_s       sib2;//ASN_RRC_SIB2_t
  struct sib_type3_s       sib3;//ASN_RRC_SIB3_t
  struct mac_main_cfg_s    mac_cnfg;
  struct pusch_cfg_ded_s   pusch_cfg;
  struct ant_info_ded_s    antenna_info;
  enum p_a_opts			   pdsch_cfg;
  rrc_cfg_sr_t             sr_cfg;
  rrc_cfg_cqi_t            cqi_cfg;
  //oset_hash_t              *qci_cfg;//std::map<uint32_t, rrc_cfg_qci_t>
  srsran_cell_t            cell;
  uint32_t       num_nr_cells; /// number of configured NR cells (used to configure RF)
  uint32_t       max_mac_dl_kos;
  uint32_t       max_mac_ul_kos;
  uint32_t       rlf_release_timer_ms;
}rrc_cfg_t;

/***************rrc_nr_cfg_t****************************/
typedef struct rrc_nr_cfg_s{
  oset_list2_t       *cell_list; //rrc_cell_cfg_nr_t
  uint32_t           inactivity_timeout_ms;//100000
  uint32_t           enb_id;
  uint16_t           mcc;
  uint16_t           mnc;
  bool               is_standalone;// SA mode.
  srb_5g_cfg_t       srb1_cfg;
  srb_5g_cfg_t       srb2_cfg;
  oset_list2_t       *five_qi_cfg; //std::map<uint32_t, rrc_nr_cfg_five_qi_t>;
  uint8_t nea_preference_list[4];//CIPHERING_ALGORITHM_ID_NR_ENUM
  uint8_t nia_preference_list[4];//INTEGRITY_ALGORITHM_ID_NR_ENUM
}rrc_nr_cfg_t;

#ifdef __cplusplus
}
#endif

#endif
