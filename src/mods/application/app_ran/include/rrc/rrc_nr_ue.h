/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_NR_UE_H_
#define RRC_NR_UE_H_

#include "lib/common/buffer_interface.h"
#include "lib/mac/sched_nr_interface.h"
#include "rrc/rrc_nr_security_context.h"
#include "rrc/rrc_nr_config.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum  { RRC_IDLE, RRC_INACTIVE, RRC_CONNECTED } rrc_nr_state_t;

typedef enum activity_timeout_type_e {
  MSG3_RX_TIMEOUT = 0,	 ///< Msg3 has its own timeout to quickly remove fake UEs from random PRACHs
  UE_INACTIVITY_TIMEOUT, ///< (currently unused) UE inactivity timeout (usually bigger than reestablishment timeout)
  MSG5_RX_TIMEOUT,		 ///< (currently unused) for receiving RRCConnectionSetupComplete/RRCReestablishmentComplete
  nulltype
}activity_timeout_type_t;

typedef struct {
  uint64_t								 setup_ue_id;
  enum establishment_cause_e		     connection_cause;
} ctxt_t;


const uint32_t drb1_lcid = 4;

typedef struct {
  uint16_t              rnti;// = SRSRAN_INVALID_RNTI
  // state
  rrc_nr_state_t        state;//          = (rrc_nr_state_t)RRC_IDLE
  uint8_t               transaction_id;
  oset_time_t           activity_timer; /// for basic DL/UL activity timeout//srsran::unique_timer

  // RRC configs for UEs
  struct cell_group_cfg_s              cell_group_cfg, next_cell_group_cfg;
  struct radio_bearer_cfg_s            radio_bearer_cfg, next_radio_bearer_cfg;
  A_DYN_ARRAY_OF(struct byte_buffer_t) nas_pdu_queue;//std::vector<srsran::unique_byte_buffer_t>

  // MAC controller
  sched_nr_ue_cfg_t uecfg;

  uint32_t       drb1_five_qi; /// selected by 5GC

  // Security helper
  nr_security_context sec_ctx;

  // SA specific variables
  ctxt_t   ctxt;

  // NSA specific variables
  bool     endc;//       = false
  uint16_t eutra_rnti;// = SRSRAN_INVALID_RNTI
}rrc_nr_ue;


#ifdef __cplusplus
}
#endif

#endif
