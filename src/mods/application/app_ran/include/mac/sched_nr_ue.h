/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_UE_H_
#define SCHED_NR_UE_H_

#include "mac/ue_cfg_manager.h"
#include "mac/sched_nr_harq.h"
#include "mac/base_ue_buffer_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	oset_lnode_t lnode;
	uint32_t lcid;
	uint32_t cc;
}ce_t;

typedef struct {
   oset_list_t  pending_ces;//srsran::deque<ce_t>
}ue_buffer_manager;


typedef struct  {
  uint32_t			 cc;//SRSRAN_MAX_CARRIERS
  ue_buffer_manager  *parent;//???
}pdu_builder;


///containing context of UE that is common to all carriers
typedef struct {
  uint32_t pending_dl_bytes;
  uint32_t pending_ul_bytes;
}ue_context_common;

typedef struct {
  uint16_t             rnti;
  uint32_t             cc;
  cell_config_manager  *cell_params;
  // Channel state
  uint32_t            dl_cqi;//1
  uint32_t            ul_cqi;//0
  harq_entity         harq_ent;
  pdu_builder         pdu_builders;
  // metrics
  mac_ue_metrics_t metrics;
  // common context
  ue_context_common *common_ctxt;

  ue_carrier_params_t   bwp_cfg;
} ue_carrier;


typedef struct {
	ue_carrier  *carriers[SCHED_NR_MAX_CARRIERS];//对应每个ue的小区配置
	uint16_t     rnti;

	sched_params_t    *sched_cfg;
	ue_cfg_manager    ue_cfg;
	slot_point        last_tx_slot;
	slot_point        last_sr_slot;
	ue_context_common common_ctxt;
	ue_buffer_manager buffers;
}ue;



#ifdef __cplusplus
}
#endif

#endif
