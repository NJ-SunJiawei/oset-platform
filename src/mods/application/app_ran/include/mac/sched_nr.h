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

typedef int (*event_callback)(void *data);
typedef int (*ue_event_callback)(void *data);
typedef int (*ue_cc_event_callback)(void *data);

typedef struct {
  char            *event_name;
  event_callback  callback;
  //void (*callback)(void *)
}event_t;

typedef struct {
  oset_lnode_t       next_slot_events_node;
  oset_lnode_t       current_slot_events_node;
  oset_lnode_t       next_slot_ue_events_node;
  oset_lnode_t       current_slot_ue_events_node;

  uint16_t			 rnti;
  char               *event_name;
  ue_event_callback  callback;
}ue_event_t;

struct ue_cc_event_t {
	oset_lnode_t	     next_slot_ue_events_node;
	oset_lnode_t	     current_slot_ue_events_node;
	uint16_t             rnti;
	uint32_t			 cc;
	char                 *event_name;
	ue_cc_event_callback callback;
}ue_cc_event_t;

typedef struct {
  oset_apr_mutex_t	   *event_cc_mutex;
  oset_list_t          next_slot_ue_events, current_slot_ue_events;//srsran::deque<ue_cc_event_t>
}cc_events;

typedef struct {
  oset_apr_mutex_t          *event_mutex;
  oset_list_t               next_slot_events, current_slot_events;//srsran::deque<event_t>
  oset_list_t               next_slot_ue_events, current_slot_ue_events;//srsran::deque<ue_event_t>
  cvector_vector_t(cc_events) carriers;//std::vector<cc_events>
}event_manager;
/*****************************************************/


typedef struct {
  oset_hash_t			   *ues;//ue_map_t
  oset_apr_mutex_t		   *mutex;
  oset_apr_thread_cond_t   *cvar;
  mac_metrics_t            pending_metrics;
  bool                     stopped;
}ue_metrics_manager;
/*****************************************************/


typedef struct {
  // args
  sched_params_t              cfg;
  // slot-specific
  slot_point                  current_slot_tx;
  int                         worker_count;
  cvector_vector_t(cc_worker)   cc_workers; //std::vector<std::unique_ptr<sched_nr_impl::cc_worker> >
  // UE Database
  OSET_POOL(ue_pool, sched_nr_ue);
  oset_hash_t			      *ue_db;//static_circular_map<uint16_t, std::unique_ptr<sched_nr_ue>, SRSENB_MAX_UES>
    // Feedback management
  event_manager               pending_events;
  // metrics extraction
  ue_metrics_manager          metrics_handler;
}sched_nr;



void sched_nr_init(sched_nr *scheluder);
void sched_nr_destory(sched_nr *scheluder);
int sched_nr_config(sched_nr *scheluder, sched_args_t *sched_cfg, cvector_vector_t(sched_nr_cell_cfg_t) sche_cells);

#ifdef __cplusplus
}
#endif

#endif
