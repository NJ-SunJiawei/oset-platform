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
#include "lib/common/time.h"
#include "lib/mac/sched_nr_interface.h"
#include "rrc/rrc_security_context.h"
#include "rrc/rrc_nr_config.h"


#ifdef __cplusplus
extern "C" {
#endif

// ACTIVE (CONNECTED):    UE 和NG-RAN—connected     NG-RAN和5GC—connected
// IDLE:                  UE 和NG-RAN—released      NG-RAN和5GC—released
// INACTIVE:              UE 和NG-RAN—suspend       NG-RAN和5GC—connected
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
  oset_lnode_t          lnode;
  oset_apr_memory_pool_t *usepool;
  uint16_t              rnti; // = SRSRAN_INVALID_RNTI
  // state
  rrc_nr_state_t        state; //= (rrc_nr_state_t)RRC_IDLE
  uint8_t               transaction_id;
  gnb_timer_t	        *activity_timer; // for basic DL/UL activity timeout//srsran::unique_timer
  uint32_t              activity_timer_deadline_ms;

  // RRC configs for UEs
  struct cell_group_cfg_s              cell_group_cfg;
  struct cell_group_cfg_s              next_cell_group_cfg;
  struct radio_bearer_cfg_s            radio_bearer_cfg;
  struct radio_bearer_cfg_s            next_radio_bearer_cfg;

  cvector_vector_t(byte_buffer_t)      nas_pdu_queue;//std::vector<srsran::unique_byte_buffer_t>

  // MAC controller
  sched_nr_ue_cfg_t  uecfg;//API_mac_rrc_api_ue_cfg()

  uint32_t           drb1_five_qi; /// selected by 5GC

  // Security helper
  nr_security_context sec_ctx;

  // SA specific variables
  ctxt_t   ctxt;

  // NSA specific variables
  bool     endc;//       = false
  uint16_t eutra_rnti;// = SRSRAN_INVALID_RNTI
  activity_timeout_type_t type;
}rrc_nr_ue;

void rrc_rem_user_info(uint16_t rnti);
void activity_timer_expired(rrc_nr_ue *ue);
void rrc_nr_ue_deactivate_bearers(rrc_nr_ue *ue);
void rrc_nr_ue_set_activity(rrc_nr_ue *ue, bool enabled);
void rrc_nr_ue_add(uint16_t rnti_, uint32_t pcell_cc_idx, bool start_msg3_timer);
void rrc_nr_ue_get_metrics(rrc_ue_metrics_t *metrics);

#ifdef __cplusplus
}
#endif

#endif
