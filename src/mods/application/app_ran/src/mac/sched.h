/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef SCHED_H_
#define SCHED_H_

#include "mac/sched_cfg.h"
#include "mac/sched_ue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*event_callback)(uint16_t rnti, void *argv);
typedef int (*ue_event_callback)(sched_nr_ue *ue, void *argv);
typedef int (*ue_cc_event_callback)(ue_carrier *carriers, void *argv);

typedef enum {
	EVENT_NULL,
	DL_RACH_INFO,
	UE_CFG,
	UE_REM,
	EVENT_MAX,
}event_e;

typedef struct{
	rar_info_t  rar_info;
}dl_rach_info_argv_t;

typedef struct{
	sched_nr_ue_cfg_t *uecfg;
}ue_cfg_argv_t;

typedef struct {
  uint16_t        rnti;
  event_e         event_name;
  event_callback  callback;
  union{
	  dl_rach_info_argv_t dl_rach_info_argv; 
	  ue_cfg_argv_t       ue_cfg_argv;
  }u;
}event_t;
///////////////////////////////////////////////////
typedef enum {
	UE_EVENT_NULL,
	UL_SR_INFO,
	UL_BSR,
	DL_MAC_CE,
	DL_BUFFER_STATE,
	UE_EVENT_MAX,
}ue_event_e;

typedef struct{
	uint32_t lcg_id;
	uint32_t bsr;
}ul_bsr_argv_t;

typedef struct{
	uint32_t ce_lcid;
}dl_mac_ce_argv_t;

typedef struct{
	uint32_t lcid;
	uint32_t newtx;
	uint32_t retx;
}dl_buffer_state_argv_t;

typedef struct {
	uint16_t		   rnti;
	ue_event_e         event_name;
	ue_event_callback  callback;
	union{
		ul_bsr_argv_t          ul_bsr_argv;
		dl_mac_ce_argv_t       dl_mac_ce_argv;
		dl_buffer_state_argv_t dl_buffer_state_argv;
	}u;
}ue_event_t;
///////////////////////////////////////////////////
typedef enum {
	UE_CC_EVENT_NULL,
	DL_ACK_INFO,
	UL_CRC_INFO,
	DL_CQI_INFO,
	UE_CC_EVENT_MAX,
}ue_cc_event_e;

typedef struct{
	uint32_t pid;
	uint32_t tb_idx;
	bool ack;
}dl_ack_info_argv_t;

typedef struct{
	uint32_t pid;
	bool crc;
}ul_crc_info_argv_t;

typedef struct{
	uint32_t cqi_value;
}dl_cqi_info_argv_t;

typedef struct ue_cc_event_t {
	uint16_t             rnti;
	uint32_t			 cc;
	ue_cc_event_e        event_name;
	ue_cc_event_callback callback;
	union{
		dl_ack_info_argv_t     dl_ack_info_argv;
		ul_crc_info_argv_t     ul_crc_info_argv;
		dl_cqi_info_argv_t     dl_cqi_info_argv;
	}u;
}ue_cc_event_t;

typedef struct {
	oset_apr_thread_rwlock_t	       *event_cc_mutex;
	cvector_vector_t(ue_cc_event_t)    next_slot_ue_events;
	cvector_vector_t(ue_cc_event_t)    current_slot_ue_events;
}cc_events;

typedef struct {
	oset_apr_thread_rwlock_t     *event_mutex;
	cvector_vector_t(event_t)    next_slot_events;
	cvector_vector_t(event_t)    current_slot_events;
	cvector_vector_t(ue_event_t) next_slot_ue_events;//has ca
	cvector_vector_t(ue_event_t) current_slot_ue_events;

	cvector_vector_t(cc_events)  carriers;
}event_manager;
/*****************************************************/

typedef struct {
  //oset_hash_t			   *ues;//ue_map_t
  oset_apr_mutex_t		   *mutex;
  oset_apr_thread_cond_t   *cvar;
  mac_metrics_t            *pending_metrics;
  bool                     stopped;
}ue_metrics_manager;
/*****************************************************/


typedef struct {
  // slot-specific todo atomic
  //slot_point                  current_slot_tx;
  //int                         worker_count;
  oset_apr_mutex_t		     *mutex;

  // args
  sched_params_t              cfg;

  cvector_vector_t(cc_worker) cc_workers; //std::vector<std::unique_ptr<sched_nr_impl::cc_worker> >
  // UE Database
  oset_list_t	              sched_ue_list;
  oset_hash_t			      *ue_db;//static_circular_map<uint16_t, std::unique_ptr<sched_nr_ue>, SRSENB_MAX_UES>
    // Feedback management
  event_manager               pending_events;
  // metrics extraction
  ue_metrics_manager          metrics_handler;
}sched_nr;

void dl_cqi_info_callback(ue_carrier *carriers, void *argv);
void ul_crc_info_callback(ue_carrier *carriers, void *argv);
void dl_ack_info_callback(ue_carrier *carriers, void *argv);

void sched_nr_dl_cqi_info(sched_nr *scheluder, uint16_t rnti, uint32_t cc, uint32_t cqi_value);
void sched_nr_ul_crc_info(sched_nr *scheluder, uint16_t rnti, uint32_t cc, uint32_t pid, bool crc);
void sched_nr_dl_ack_info(sched_nr *scheluder, uint16_t rnti, uint32_t cc, uint32_t pid, uint32_t tb_idx, bool ack);
//////////////////////////////////////////////////////////////////////
void dl_buffer_state_callback(sched_nr_ue *ue, void *argv);
void dl_mac_ce_callback(sched_nr_ue *ue, void *argv);
void ul_bsr_callback(sched_nr_ue *ue, void *argv);
void ul_sr_info_callback(sched_nr_ue *ue, void *argv);

void sched_nr_dl_buffer_state(sched_nr *scheluder, uint16_t rnti, uint32_t lcid, uint32_t newtx, uint32_t retx);
void sched_nr_dl_mac_ce(sched_nr *scheluder, uint16_t rnti, uint32_t ce_lcid);
void sched_nr_ul_bsr(sched_nr *scheluder, uint16_t rnti, uint32_t lcg_id, uint32_t bsr);
void sched_nr_ul_sr_info(sched_nr *scheluder, uint16_t rnti);
//////////////////////////////////////////////////////////////////////
void dl_rach_info_callback(uint16_t rnti, void *argv);
void ue_remove_callback(uint16_t rnti, void *argv);
void ue_cfg_impl_callback(uint16_t rnti, void *argv);

int sched_nr_dl_rach_info(sched_nr *scheluder, rar_info_t *rar_info);
void sched_nr_ue_rem(sched_nr *scheluder, uint16_t rnti);
void sched_nr_ue_cfg(sched_nr *scheluder, uint16_t rnti, sched_nr_ue_cfg_t *uecfg);
//////////////////////////////////////////////////////////////////////
void sched_nr_get_metrics(ue_metrics_manager *manager, mac_metrics_t *metrics);
//////////////////////////////////////////////////////////////////////
void sched_nr_init(sched_nr *scheluder);
void sched_nr_destory(sched_nr *scheluder);
int sched_nr_config(sched_nr *scheluder, sched_args_t *sched_cfg, cvector_vector_t(sched_nr_cell_cfg_t) sched_cells);
void sched_nr_slot_indication(sched_nr *scheluder, slot_point slot_tx);
dl_res_t* sched_nr_get_dl_sched(sched_nr *scheluder, slot_point pdsch_tti, uint32_t cc);
ul_sched_t* sched_nr_get_ul_sched(sched_nr *scheluder, slot_point slot_ul, uint32_t cc);

#ifdef __cplusplus
}
#endif

#endif
