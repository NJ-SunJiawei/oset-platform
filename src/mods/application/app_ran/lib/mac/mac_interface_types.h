/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#ifndef MAC_INTERFACE_TYPES_H
#define MAC_INTERFACE_TYPES_H

//#include "lib/rrc/rrc_interface_types.h"
#include <stdint.h>

/***************************
 *      MAC Config
 **************************/

/* Logical Channel Multiplexing and Prioritization + Msg3 Buffer */
typedef struct {
  uint8_t  lcid;
  uint8_t  lcg;
  int32_t  Bj;
  int32_t  PBR; // in kByte/s, -1 sets to infinity
  uint32_t bucket_size;
  uint32_t BSD;
  uint32_t priority;
  int      sched_len;  // scheduled upper layer payload for this LCID
  int      buffer_len; // outstanding bytes for this LCID
}logical_channel_config_t;

typedef struct {
  int periodic_timer;//-1
  int retx_timer;//2560
}bsr_cfg_t;

typedef struct {
  bool enabled;
  int  periodic_timer;
  int  prohibit_timer;
  int  db_pathloss_change;
  bool extended;
}phr_cfg_t;

typedef struct {
  bool enabled;
  int  dsr_transmax;
}sr_cfg_t;

typedef struct  {
  uint32_t max_harq_msg3_tx;//5
  uint32_t max_harq_tx;//5
}ul_harq_cfg_t;

/// NR specific config for DL HARQ with configurable number of processes
typedef struct {
  uint8_t nof_procs; // Number of HARQ processes used in the DL
}dl_harq_cfg_nr_t;

typedef struct {
  bool     enabled;
  uint32_t nof_preambles;
  uint32_t nof_groupA_preambles;
  int32_t  messagePowerOffsetGroupB;
  uint32_t messageSizeGroupA;
  uint32_t responseWindowSize;
  uint32_t powerRampingStep;
  uint32_t preambleTransMax;
  int32_t  iniReceivedTargetPower;
  uint32_t contentionResolutionTimer;
  uint32_t new_ra_msg_len;
}rach_cfg_t;

// 38.321 5.1.1 Not complete yet
typedef struct {
  uint32_t prach_ConfigurationIndex;
  int      PreambleReceivedTargetPower;
  uint32_t preambleTransMax;
  uint32_t powerRampingStep;
  uint32_t ra_responseWindow;
  uint32_t ra_ContentionResolutionTimer;
}rach_cfg_nr_t;

// 38.321 Section 5.4.4 (only one config supported right now)
typedef struct {
  uint8_t sched_request_id;
  uint8_t prohibit_timer;
  uint8_t trans_max;
}sr_cfg_item_nr_t;

#define SRSRAN_MAX_MAX_NR_OF_SR_CFG_PER_CELL_GROUP (8)
typedef struct {
  bool             enabled;
  uint8_t          num_items;
  sr_cfg_item_nr_t item[SRSRAN_MAX_MAX_NR_OF_SR_CFG_PER_CELL_GROUP];
}sr_cfg_nr_t;

typedef struct {
  uint8_t  tag_id;
  uint32_t time_align_timer;
}tag_cfg_nr_t;

typedef struct {
  int  periodic_timer;
  int  prohibit_timer;
  int  tx_pwr_factor_change;
  bool extended;
}phr_cfg_nr_t;

typedef struct {
  // mandatory BSR config
  int periodic_timer;
  int retx_timer;

  // SR specific configs for logical channel
  bool sr_delay_timer_enabled;
  int  sr_delay_timer;
  bool sr_mask; // Indicates whether SR masking is configured for this logical channel
}bsr_cfg_nr_t;

typedef struct {
  // Default constructor with default values as in 36.331 9.2.2
  bsr_cfg_t     bsr_cfg;
  phr_cfg_t     phr_cfg;
  sr_cfg_t      sr_cfg;
  rach_cfg_t    rach_cfg;
  ul_harq_cfg_t harq_cfg;
  int           time_alignment_timer;
}mac_cfg_t;

typedef struct {
  // Default constructor with default values as in 36.331 9.2.2
  bsr_cfg_t     bsr_cfg;
  phr_cfg_nr_t  phr_cfg;
  sr_cfg_t      sr_cfg;
  rach_cfg_nr_t rach_cfg;
  ul_harq_cfg_t harq_cfg;
  int           time_alignment_timer;
}mac_cfg_nr_t;

#endif
