/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_UE_H_
#define SCHED_UE_H_

#include "mac/ue_cfg_manager.h"
#include "mac/sched_harq.h"
#include "mac/base_ue_buffer_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t lcid;
	uint32_t cc;
}ce_t;

typedef struct {
	base_ue_buffer_manager        base_ue;
	cvector_vector_t(ce_t)        pending_ces;//srsran::deque<ce_t>
}ue_buffer_manager;

typedef struct  {
  uint32_t			 cc;//SRSRAN_MAX_CARRIERS
  ue_buffer_manager  *parent;//sched_nr_ue->buffers
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
  // common context
  ue_context_common   *common_ctxt;

  ue_carrier_params_t  bwp_cfg;//每个ue对应小区的参数，初始从rrc获取，随后rrc高层更新
  // metrics
  mac_ue_metrics_t     metrics;
} ue_carrier;


typedef struct {
    oset_lnode_t lnode;
	oset_apr_memory_pool_t *usepool;
	ue_carrier   *carriers[SCHED_NR_MAX_CARRIERS];//对应每个ue的小区配置和资源
	uint16_t     rnti;

	sched_params_t    *sched_cfg;
	ue_cfg_manager    ue_cfg;
	slot_point        last_tx_slot;// tx slot
	slot_point        last_sr_slot;// sr rx slot
	ue_context_common common_ctxt;
	ue_buffer_manager buffers;
}sched_nr_ue;

typedef struct{
  // UE parameters common to all sectors
  uint32_t      dl_bytes;
  uint32_t      ul_bytes;
  // UE parameters that are sector specific
  bool          dl_active;
  bool          ul_active;
  slot_point    pdcch_slot;
  slot_point    pdsch_slot;
  slot_point    pusch_slot;
  slot_point    uci_slot;
  dl_harq_proc  *h_dl;
  ul_harq_proc  *h_ul;
  ue_carrier    *ue;
}slot_ue;

//////////////////////////////////////////////////////////////
int ue_carrier_dl_ack_info(ue_carrier *carrier, uint32_t pid, uint32_t tb_idx, bool ack);
int ue_carrier_ul_crc_info(ue_carrier *carrier, uint32_t pid, bool crc);
//////////////////////////////////////////////////////////////
void sched_nr_ue_remove(sched_nr_ue *u);
void sched_nr_ue_remove_all(void);
sched_nr_ue *sched_nr_ue_find_by_rnti(uint16_t rnti);
void sched_nr_add_ue_impl(sched_nr_ue *u, uint32_t cc);
sched_nr_ue *sched_nr_ue_add(uint16_t rnti_, uint32_t cc, sched_params_t *sched_cfg_);
sched_nr_ue *sched_nr_ue_add_inner(uint16_t rnti_, sched_nr_ue_cfg_t *uecfg, sched_params_t *sched_cfg_);
bool sched_nr_ue_has_ca(sched_nr_ue *u);
void sched_nr_ue_new_slot(sched_nr_ue *ue, slot_point pdcch_slot);
void sched_nr_ue_ul_sr_info(sched_nr_ue *ue);
void sched_nr_ue_add_dl_mac_ce(sched_nr_ue *ue, uint32_t ce_lcid, uint32_t nof_cmds);
void sched_nr_ue_rlc_buffer_state(sched_nr_ue *ue, uint32_t lcid, uint32_t newtx, uint32_t priotx);
void sched_nr_ue_ul_bsr(sched_nr_ue *ue, uint32_t lcg, uint32_t bsr_val);
////////////////////////////////////////////////////////////////////////////////////////////
void slot_ue_alloc(sched_nr_ue *ue, slot_point pdcch_slot, uint32_t cc);
void slot_ue_clear(uint32_t cc);
void slot_ue_destory(uint32_t cc);
slot_ue* slot_ue_find_by_rnti(uint16_t rnti, uint32_t cc);
///////////////////////////////////////////////////////////////////////////////////////////
uint32_t dl_cqi(slot_ue *slot_u);
uint32_t ul_cqi(slot_ue *slot_u);
bool get_pending_bytes(slot_ue *slot_u, uint32_t lcid);
bool build_pdu(slot_ue *slot_u, uint32_t rem_bytes, dl_pdu_t *pdu);


#ifdef __cplusplus
}
#endif

#endif
