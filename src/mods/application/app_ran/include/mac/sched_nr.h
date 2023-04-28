/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef SCHED_NR_H_
#define SCHED_NR_H_

#include "mac/sched_nr_cfg.h"
#include "mac/sched_nr_ue.h"
#include "mac/sched_nr_worker.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*event_callback)(void *argv);
typedef int (*ue_event_callback)(void *argv);
typedef int (*ue_cc_event_callback)(void *argv);

enum struct {
	DL_RACH_INFO,
	UE_CFG,
	UE_REM,
}event_e;

typedef struct{
	rar_info_t  rar_info;
}dl_rach_info_argv_t;

typedef struct{
	uint16_t rnti;
	sched_nr_ue_cfg_t *uecfg;
}ue_cfg_argv_t;

typedef struct{
	sched_nr_ue *u;
}ue_rem_argv_t;

typedef struct {
  event_e         event_name;
  event_callback  callback;
  union{
	  dl_rach_info_argv_t dl_rach_info_argv; 
	  ue_cfg_argv_t       ue_cfg_argv;
	  ue_rem_argv_t       ue_rem_argv;
  }u;
}event_t;

typedef struct {
	uint16_t		   rnti;
	event_e            event_name;
	ue_event_callback  callback;
	union{

	}u;
}ue_event_t;

struct ue_cc_event_t {
	uint16_t             rnti;
	uint32_t			 cc;
	event_e              event_name;
	ue_cc_event_callback callback;
	union{

	}u;
}ue_cc_event_t;

typedef struct {
	oset_apr_thread_rwlock_t	       *event_cc_mutex;
	cvector_vector_t(ue_cc_event_t)    next_slot_ue_events;
	cvector_vector_t(ue_cc_event_t)    current_slot_ue_events;//srsran::deque<ue_cc_event_t>
}cc_events;

typedef struct {
	oset_apr_thread_rwlock_t     *event_mutex;
	cvector_vector_t(event_t)    next_slot_events;
	cvector_vector_t(event_t)    current_slot_events;//srsran::deque<event_t>
	cvector_vector_t(ue_event_t) next_slot_ue_events;
	cvector_vector_t(ue_event_t) current_slot_ue_events;//srsran::deque<ue_event_t>

	cvector_vector_t(cc_events)  carriers;//std::vector<cc_events>
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
  slot_point                  current_slot_tx;
  int                         worker_count;

  // args
  sched_params_t              cfg;

  cvector_vector_t(cc_worker) cc_workers; //std::vector<std::unique_ptr<sched_nr_impl::cc_worker> >
  // UE Database
  OSET_POOL(ue_pool, sched_nr_ue);//sched rnti context 
  oset_list_t	              sched_ue_list;
  oset_hash_t			      *ue_db;//static_circular_map<uint16_t, std::unique_ptr<sched_nr_ue>, SRSENB_MAX_UES>
    // Feedback management
  event_manager               pending_events;
  // metrics extraction
  ue_metrics_manager          metrics_handler;
}sched_nr;

void sched_nr_get_metrics(ue_metrics_manager *manager, mac_metrics_t *metrics);
//////////////////////////////////////////////////////////////////////
void sched_nr_init(sched_nr *scheluder);
void sched_nr_destory(sched_nr *scheluder);
int sched_nr_config(sched_nr *scheluder, sched_args_t *sched_cfg, cvector_vector_t(sched_nr_cell_cfg_t) sched_cells);
void sched_nr_slot_indication(sched_nr *scheluder, slot_point slot_tx);
//////////////////////////////////////////////////////////////////////
void dl_rach_info_callback(void *argv);
void sched_nr_ue_remove_callback(void *argv);
void sched_nr_ue_cfg_impl_callback(void *argv);

int sched_nr_dl_rach_info(sched_nr *scheluder, rar_info_t *rar_info);
void sched_nr_ue_rem(sched_nr *scheluder, uint16_t rnti);
void sched_nr_ue_cfg(sched_nr *scheluder, uint16_t rnti, sched_nr_ue_cfg_t *uecfg);

#ifdef __cplusplus
}
#endif

#endif
