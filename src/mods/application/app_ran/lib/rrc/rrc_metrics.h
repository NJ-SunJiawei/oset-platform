/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef RRC_METRICS_H_
#define RRC_METRICS_H_

#include "oset-core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  RRC_STATE_IDLE = 0,
  RRC_STATE_WAIT_FOR_CON_SETUP_COMPLETE,
  RRC_STATE_WAIT_FOR_CON_REEST_COMPLETE,
  RRC_STATE_WAIT_FOR_SECURITY_MODE_COMPLETE,
  RRC_STATE_WAIT_FOR_UE_CAP_INFO,
  RRC_STATE_WAIT_FOR_UE_CAP_INFO_ENDC, /* only entered for UEs with NSA support */
  RRC_STATE_WAIT_FOR_CON_RECONF_COMPLETE,
  RRC_STATE_REESTABLISHMENT_COMPLETE,
  RRC_STATE_REGISTERED,
  RRC_STATE_RELEASE_REQUEST,
  RRC_STATE_N_ITEMS,
} rrc_state_t;

typedef struct {
  rrc_state_t                     state;
  cvector_vector_t(oset_hash_t *) drb_qci_map;
}rrc_ue_metrics_t;

typedef struct  {
  cvector_vector_t(rrc_ue_metrics_t) ues;
}rrc_metrics_t;


#ifdef __cplusplus
}
#endif

#endif
