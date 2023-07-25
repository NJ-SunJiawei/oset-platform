/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#ifndef RRC_INTERFACE_TYPES_H
#define RRC_INTERFACE_TYPES_H

#include "lib/common/bcd_interface.h"
#include "lib/common/common.h"
#include "lib/srsran/config.h"
#include "lib/srsran/srsran.h"

/***************************
 *   Establishment Cause
 **************************/
typedef enum {
  emergency,
  high_prio_access,
  mt_access,
  mo_sig,
  mo_data,
  delay_tolerant_access_v1020,
  mo_voice_call_v1280,
  spare1,
  nulltype
}establishment_cause_t;

inline char* establishment_cause_to_string(establishment_cause_t  cause)
{
   static const char* options[] = {"emergency",
	                                "highPriorityAccess",
	                                "mt-Access",
	                                "mo-Signalling",
	                                "mo-Data",
	                                "delayTolerantAccess-v1020",
	                                "mo-VoiceCall-v1280",
	                                "spare1"};
  return enum_to_text(options, (establishment_cause_t)nulltype, (uint32_t)cause);
}

typedef enum {
  t310_expiry,
  random_access_problem,
  rlc_max_num_retx,
  synch_recfg_fail_scg,
  scg_recfg_fail,
  srb3_integrity_fail,
  nulltype
}scg_failure_cause_t;

inline char* scg_failure_cause_to_string(scg_failure_cause_t cause)
{
   static const char* options[] = {"t310_expiry",
                                    "random_access_problem",
                                    "rlc_max_num_retx",
                                    "synch_recfg_fail_scg",
                                    "scg_recfg_fail",
                                    "srb3_integrity_fail",
                                    "nulltype"};
  return enum_to_text(options, (scg_failure_cause_t)nulltype, (uint32_t)cause);
}

typedef enum {
  emergency,
  highPriorityAccess,
  mt_Access,
  mo_Signalling,
  mo_Data,
  mo_VoiceCall,
  mo_VideoCall,
  mo_SMS,
  mps_PriorityAccess,
  mcs_PriorityAccess,
  spare6,
  spare5,
  spare4,
  spare3,
  spare2,
  spare1,
  nulltype
}nr_establishment_cause_t;
  
inline char* nr_establishment_cause_to_string(nr_establishment_cause_t cause)
{
   static const char* options[] = {
							      "emergency",
							      "highPriorityAccess",
							      "mt_Access",
							      "mo_Signalling",
							      "mo_Data",
							      "mo_VoiceCall",
							      "mo_VideoCall",
							      "mo_SMS",
							      "mps_PriorityAccess",
							      "mcs_PriorityAccess",
							      "spare6",
							      "spare5",
							      "spare4",
							      "spare3",
							      "spare2",
							      "spare1",
							  };
  return enum_to_text(options, (nr_establishment_cause_t)nulltype, (uint32_t)cause);
}

/***************************
 *      PHY Config
 **************************/

typedef struct {
  srsran_dl_cfg_t dl_cfg;
  srsran_ul_cfg_t ul_cfg;
  bool               prach_cfg_present;
  srsran_prach_cfg_t prach_cfg;
}rrc_phy_cfg_t;

typedef enum { n1, n2, n4, n8, n16, n32, nulltype } alloc_period_t;
typedef enum { one_frame, four_frames, nulltype }sf_alloc_type_t;
typedef struct {
  alloc_period_t radioframe_alloc_period;
  uint8_t        radioframe_alloc_offset;
  sf_alloc_type_t nof_alloc_subfrs;
  uint32_t        sf_alloc;
}mbsfn_sf_cfg_t;

inline uint16_t alloc_period_enum_to_number(alloc_period_t radioframe_period)
{
  static uint16_t options[] = {1, 2, 4, 8, 16, 32};
  return enum_to_number(options, (alloc_period_t)nulltype, (uint32_t)radioframe_period);
}

typedef enum { n2, n4 } coeff_t;
typedef struct  {
  coeff_t notif_repeat_coeff;//(coeff_t)n2
  uint8_t notif_offset;//       = 0
  uint8_t notif_sf_idx;//       = 1
}mbms_notif_cfg_t;

typedef enum { rf32, rf64, rf128, rf256, nulltype } repeat_period_t;
typedef enum { rf512, rf1024 } mod_period_t;
typedef enum { n2, n7, n13, n19, nulltype } sig_mcs_t;
typedef struct  {
  repeat_period_t mcch_repeat_period;
  uint8_t mcch_offset;
  mod_period_t mcch_mod_period;
  uint8_t sf_alloc_info;
  sig_mcs_t sig_mcs;
} mcch_cfg_t;
typedef enum { s1, s2, nulltype } region_len_t; 
// MBSFN-AreaInfo-r9 ::= SEQUENCE
typedef struct {
  uint8_t mbsfn_area_id;
  region_len_t non_mbsfn_region_len;
  uint8_t notif_ind;
  mcch_cfg_t   mcch_cfg;
}mbsfn_area_info_t;
inline uint16_t region_lenenum_to_number(region_len_t region_len)
{
  static uint16_t options[] = {1, 2};
  return enum_to_number(options, (region_len_t)nulltype, (uint32_t)region_len);
}
inline uint16_t repeat_period_enum_to_number(repeat_period_t repeat_period)
{
  static uint16_t options[] = {32, 64, 128, 256};
  return enum_to_number(options, (repeat_period_t)nulltype, (uint32_t)repeat_period);
}
inline uint16_t sig_mcs_enum_to_number(sig_mcs_t sig_mcs)
{
  static uint16_t options[] = {2, 7, 13, 19};
  return enum_to_number(options, (sig_mcs_t)nulltype, (uint32_t)sig_mcs);
}

// TMGI-r9
typedef enum { plmn_idx, explicit_value } plmn_id_type_t;
typedef struct  {
  plmn_id_type_t plmn_id_type;
  union choice {
    uint8_t   plmn_idx;
    plmn_id_t explicit_value;
  } plmn_id;
  uint8_t serviced_id[3];
}tmgi_t;

typedef enum  { rf8, rf16, rf32, rf64, rf128, rf256, rf512, rf1024, nulltype } mch_sched_period_t;
static const uint32_t max_session_per_pmch = 29;

// mbms_session_info_list
typedef struct  {
  bool	  session_id_present;
  tmgi_t  tmgi;
  uint8_t session_id;
  uint8_t lc_ch_id;
}mbms_session_info_t;
typedef struct  {
  // pmch_cfg_t
  uint16_t sf_alloc_end;
  uint8_t  data_mcs;
  mch_sched_period_t mch_sched_period;
  uint32_t              nof_mbms_session_info;
  mbms_session_info_t   mbms_session_info_list[max_session_per_pmch];
}pmch_info_t;
inline uint16_t mch_period_enum_to_number(mch_sched_period_t mch_period)
{
  static uint16_t options[] = {8, 16, 32, 64, 128, 256, 512, 1024};
  return enum_to_number(options, (mch_sched_period_t)nulltype, (uint32_t)mch_period);
}

typedef enum { rf4, rf8, rf16, rf32, rf64, rf128, rf256, nulltype } common_sf_alloc_period_t;
typedef struct {
  uint32_t       nof_common_sf_alloc;
  mbsfn_sf_cfg_t common_sf_alloc[8];
  common_sf_alloc_period_t common_sf_alloc_period;
  uint32_t    nof_pmch_info;
  pmch_info_t pmch_info_list[15];
  // mbsfn_area_cfg_v930_ies non crit ext OPTIONAL
}mcch_msg_t;
inline uint16_t common_sf_alloc_period_enum_to_number(common_sf_alloc_period_t alloc_period)
{
  static uint16_t options[] = {4, 8, 16, 32, 64, 128, 256};
  return enum_to_number(options, (common_sf_alloc_period_t)nulltype, (uint32_t)alloc_period);
}

typedef struct {
  mbsfn_sf_cfg_t    mbsfn_subfr_cnfg;
  mbms_notif_cfg_t  mbsfn_notification_cnfg;
  mbsfn_area_info_t mbsfn_area_info;
  mcch_msg_t        mcch;
}phy_cfg_mbsfn_t;

// SystemInformationBlockType13-r9
static const uint32_t max_mbsfn_area	  = 8;
typedef struct {
  uint32_t              nof_mbsfn_area_info;
  mbsfn_area_info_t     mbsfn_area_info_list[max_mbsfn_area];
  mbms_notif_cfg_t      notif_cfg;
}sib13_t;

static const uint32_t max_nof_mbsfn_sf_cfg = 8;
typedef struct {
  bool                  mbsfn_sf_cfg_list_present;
  int                   nof_mbsfn_sf_cfg;
  mbsfn_sf_cfg_t        mbsfn_sf_cfg_list[max_nof_mbsfn_sf_cfg];
}sib2_mbms_t;

typedef enum  { none = 0, mo_data, mo_signalling, mt, all } barring_t;
inline char* barring_to_string(barring_t b)
{
  static const char* options[] = {"none", "mo-data", "mo-signalling", "mt", "all"};
  return enum_to_text(options, 5u, (uint32_t)b);
}

/**
 * Flat UE capabilities
 */
typedef struct {
  uint8_t release;//           = 8
  uint8_t category;//          = 4
  uint8_t category_dl;//       = 0
  uint8_t category_ul;//       = 0
  bool    support_dl_256qam;// = false
  bool    support_ul_64qam;//  = false
}rrc_ue_capabilities_t;

#endif // RRC_INTERFACE_TYPES_H
