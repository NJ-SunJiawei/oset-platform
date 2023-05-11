/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_INTERFACE_H_
#define SCHED_NR_INTERFACE_H_

#include "gnb_interface.h"
#include "lib/common/phy_cfg_nr.h"
#include "lib/common/common_nr.h"
#include "lib/common/slot_point.h"
#include "lib/common/slot_interval.h"
#include "lib/common/asn_interface.h"
#include "lib/common/buffer_interface.h"
#include "lib/common/time.h"


#ifdef __cplusplus
extern "C" {
#endif

#define expected(T, E) \
			struct { \
				bool has_val;\
				union {\
				  T val;\
				  E unexpected;\
				}res;\
			}
/////////////////////////////////////////////////////////


typedef enum  { IDLE = 0, UL, DL, BOTH } direction_t;

typedef struct mac_lc_ch_cfg_s {
  direction_t direction;//IDLE
  int         priority;// channel priority (1 is highest) = 1
  uint32_t    bsd; // msec = 1000
  uint32_t    pbr;// prioritised bit rate = -1   
  int         group;// logical channel group = 0//lcid分组
}mac_lc_ch_cfg_t;


#define   SCHED_NR_MAX_CARRIERS      4
#define   SCHED_NR_INVALID_RNTI      0
#define   SCHED_NR_MAX_NOF_RBGS      18
#define   SCHED_NR_MAX_TB            1
#define   SCHED_NR_MAX_HARQ          SRSRAN_DEFAULT_HARQ_PROC_DL_NR
#define   SCHED_NR_MAX_BWP_PER_CELL  2
#define   SCHED_NR_MAX_LCID          MAX_NR_NOF_BEARERS;
#define   SCHED_NR_MAX_LC_GROUP      7

typedef struct sched_nr_ue_cc_cfg_s {
  bool     active ;//false
  uint32_t cc ;//0
}sched_nr_ue_cc_cfg_t;

typedef struct sched_nr_ue_lc_ch_cfg_s {
  uint32_t        lcid; // 1..32
  mac_lc_ch_cfg_t cfg;
}sched_nr_ue_lc_ch_cfg_t;

typedef struct  sched_nr_ue_cfg_s{
  uint32_t                                  maxharq_tx;//=4
  cvector_vector_t(sched_nr_ue_cc_cfg_t)    carriers;//bounded_vector<sched_nr_ue_cc_cfg_t, SCHED_NR_MAX_CARRIERS> 
  phy_cfg_nr_t                              phy_cfg;//更新并传递给mac/sched更新
  struct  mac_cell_group_cfg_s              *mac_cell_group_cfg;
  struct  phys_cell_group_cfg_s             *phy_cell_group_cfg;
  struct  sp_cell_cfg_s                     *sp_cell_cfg;
  cvector_vector_t(sched_nr_ue_lc_ch_cfg_t) lc_ch_to_add;//sched_nr_ue_lc_ch_cfg_t
  cvector_vector_t(uint32_t)                lc_ch_to_rem;//uint32_t
}sched_nr_ue_cfg_t;

typedef struct  {
  uint32_t                 start_rb;
  uint32_t                 rb_width;//100
  srsran_pdcch_cfg_nr_t    pdcch;
  srsran_sch_hl_cfg_nr_t   pdsch;
  srsran_sch_hl_cfg_nr_t   pusch;
  srsran_pucch_nr_hl_cfg_t pucch;
  srsran_harq_ack_cfg_hl_t harq_ack;
  uint32_t                 rar_window_size;//10 // See TS 38.331, ra-ResponseWindow: {1, 2, 4, 8, 10, 20, 40, 80}
  uint32_t                 numerology_idx;
}sched_nr_bwp_cfg_t;

typedef struct  {
  uint32_t len;
  uint32_t period_rf;
  uint32_t si_window_slots;
}sched_nr_cell_cfg_sib_t;

#define MAX_SIBS  2

typedef struct sched_nr_cell_cfg_s{
  uint32_t                                               nof_layers;
  uint32_t                                               pci;
  uint32_t                                               ssb_offset;
  uint32_t                                               dl_cell_nof_prb;
  uint32_t                                               ul_cell_nof_prb;
  struct dl_cfg_common_sib_s                             *dl_cfg_common;
  struct ul_cfg_common_sib_s                             *ul_cfg_common;
  struct tdd_ul_dl_cfg_common_s                          *tdd_ul_dl_cfg_common;
  struct ssb_positions_in_burst_s_                       *ssb_positions_in_burst;
  uint32_t                                               ssb_periodicity_ms;
  enum dmrs_type_a_position_e_                           dmrs_type_a_position;
  enum subcarrier_spacing_e                              ssb_scs;
  struct pdcch_cfg_sib1_s                                pdcch_cfg_sib1;
  int                                                    ss_pbch_block_power;
  // Extras
  cvector_vector_t(sched_nr_bwp_cfg_t)        bwps; //std::vector<sched_nr_bwp_cfg_t> bwps{1}// idx0 for BWP-common
  cvector_vector_t(sched_nr_cell_cfg_sib_t)   sibs;// std::vector<sched_nr_cell_cfg_sib_t> //from rrc
  double                               dl_center_frequency_hz;
  double                               ul_center_frequency_hz;
  double                               ssb_center_freq_hz;
}sched_nr_cell_cfg_t;

////// RA signalling //////

#define MAX_SUBPDUS  8

typedef struct rar_info_s {
  uint32_t	 msg3_size;//7
  uint32_t	 cc;
  uint16_t	 temp_crnti;
  slot_point prach_slot;
  uint32_t	 ofdm_symbol_idx;
  uint32_t	 freq_idx;
  uint32_t	 preamble_idx;
  uint32_t	 ta_cmd;//ta time
}rar_info_t;

typedef struct msg3_grant_s {
  rar_info_t		 data;
  srsran_dci_ul_nr_t msg3_dci;
}msg3_grant_t;

typedef struct rar_s {
  cvector_vector_t(msg3_grant_t) grants;//srsran::bounded_vector<msg3_grant_t, MAX_GRANTS>
}rar_t;


////// DL data signalling //////
typedef struct dl_pdu_s {
   cvector_vector_t(uint32_t) subpdus;//srsran::bounded_vector<uint32_t, MAX_SUBPDUS>
}dl_pdu_t;


/***sched_nr_interface***/
typedef struct dl_res_s{
  dl_sched_t		         phy;
  cvector_vector_t(dl_pdu_t) data; //bounded_vector<dl_pdu_t, MAX_GRANTS>
  cvector_vector_t(rar_t)	 rar; //bounsed_vector<rar_t, MAX_GRANTS>;
  cvector_vector_t(uint32_t) sib_idxs;//bounded_vector<uint32_t, MAX_GRANTS>//list of SI indexes
}dl_res_t;

#ifdef __cplusplus
}
#endif

#endif
