/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr.h"
#include "mac/mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched"

//////////////////////////////metrics/////////////////////////////////////
static void save_metrics_nolock(ue_metrics_manager *manager)
{
    if (NULL == manager->pending_metrics) {
		return;
    }

	mac_ue_metrics_t *ue_metric = NULL;
	cvector_for_each_in(ue_metric, manager->pending_metrics->ues){
		sched_nr_ue * ue = sched_ue_nr_find_by_rnti(ue_metric->rnti);
		if (NULL != ue ) {
			sched_nr_ue_cc_cfg_t *cell= NULL;
			cvector_for_each_in(cell, ue->ue_cfg.carriers){
				ue_carrier *ue_cc	  = ue->carriers[cell->cc];//now cell->cc = 0
				if(NULL != ue_cc){
					ue_metric->tx_brate  = ue_cc->metrics.tx_brate;
					ue_metric->tx_errors = ue_cc->metrics.tx_errors;
					ue_metric->tx_pkts   = ue_cc->metrics.tx_pkts;
					ue_cc->metrics 	     = {0};
				}
			}
		}
	}

    manager->pending_metrics = NULL;
}

static void stop_metrics(ue_metrics_manager *manager)
{
  if (!manager->stopped) {
	manager->stopped = true;
	// requests during sched::stop may not be fulfilled by sched main thread
	save_metrics_nolock(manager);
	oset_apr_mutex_lock(manager->mutex);
	oset_apr_thread_cond_broadcast(manager->cvar, manager->mutex);
	oset_apr_mutex_unlock(manager->mutex);
  }
}

/// called from within the scheduler main thread to save metrics
static void save_metrics(ue_metrics_manager *manager)
{
	save_metrics_nolock(manager);
	if (!manager->stopped){
		oset_apr_mutex_lock(manager->mutex);
		oset_apr_thread_cond_signal(manager->cvar);
		oset_apr_mutex_unlock(manager->mutex);
	}
}

/// Blocking call that waits for the metrics to be filled
static void get_metrics(ue_metrics_manager *manager, mac_metrics_t *requested_metrics)
{
	manager->pending_metrics = requested_metrics;
	if (!manager->stopped) {
		if(NULL != manager->pending_metrics){
			oset_apr_mutex_lock(manager->mutex);
			oset_apr_thread_cond_wait(manager->cvar, manager->mutex);
			oset_apr_mutex_unlock(manager->mutex);
		}
	} else {
		save_metrics_nolock(manager);
	}
}

//////////////////////////sched_nr////////////////////////////////////////
static void process_events(sched_nr *scheluder)
{
	event_manager *pending_events = &scheluder->pending_events;

    // Extract pending feedback events
    {	
    	oset_apr_thread_rwlock_rdlock(pending_events->event_mutex);
		cvector_clear(pending_events->current_slot_ue_events);
		cvector_clear(pending_events->current_slot_events);

		cvector_copy(pending_events->next_slot_ue_events, pending_events->current_slot_ue_events);
		cvector_copy(pending_events->next_slot_events, pending_events->current_slot_events);

		cvector_clear(pending_events->next_slot_ue_events);
		cvector_clear(pending_events->next_slot_events);
		oset_apr_thread_rwlock_unlock(pending_events->event_mutex);
    }

	// non-UE specific events
	event_t *ev = NULL;
	cvector_for_each_in(ev, pending_events->current_slot_events){
		if(DL_RACH_INFO == ev->event_name) ev->callback(&ev->u.dl_rach_info_argv);
		if(UE_CFG == ev->event_name) ev->callback(&ev->u.ue_cfg_argv);
		if(UE_REM == ev->event_name) ev->callback(&ev->u.ue_rem_argv);
	}

	ue_event_t *u_ev = NULL;
	cvector_for_each_in(u_ev, pending_events->current_slot_ue_events){
		sched_nr_ue *ue_it = sched_ue_nr_find_by_rnti(u_ev->rnti);
		if (NULL == ue_it) {
		  oset_warn("SCHED: \"%s\" called for unknown rnti=0x%x.", u_ev->event_name, u_ev->rnti);
		  u_ev->rnti = SRSRAN_INVALID_RNTI;
		} else if (sched_nr_ue_has_ca(ue_it)) {
		  // events specific to existing UEs with CA
		  u_ev->callback(&ev->u);//todo
		  u_ev->rnti = SRSRAN_INVALID_RNTI;
		}
	}
}


void sched_nr_init(sched_nr *scheluder)
{
	oset_assert(scheluder);

	// Initiate sched_nr_ue memory pool//最大调度64个用户
	oset_pool_init(scheluder->ue_pool, SRSENB_MAX_UES);
	oset_list_init(&scheluder->sched_ue_list);
	scheluder->ue_db = oset_hash_make();

	//scheluder->metrics_handler.ues = scheluder->ue_db;
	oset_apr_thread_cond_create(&scheluder->metrics_handler.cvar, mac_manager_self()->app_pool);
	oset_apr_mutex_init(&scheluder->metrics_handler.mutex, OSET_MUTEX_NESTED, mac_manager_self()->app_pool);
}

void sched_nr_destory(sched_nr *scheluder)
{
	oset_assert(scheluder);
	stop_metrics();

	//cvector_free(scheluder->metrics_handler.pending_metrics.cc_info);
	//cvector_free(scheluder->metrics_handler.pending_metrics.ues);
	oset_apr_mutex_destroy(scheluder->metrics_handler.mutex);
	oset_apr_thread_cond_destroy(scheluder->metrics_handler.cvar);

	//cell_config_manager
	cell_config_manager *cell_cof_manager = NULL;
	cvector_for_each_in(cell_cof_manager, scheluder->cfg.cells){
		cell_config_manager_destory(cell_cof_manager);
	}
	cvector_free(scheluder->cfg.cells);

	//callback event
	oset_apr_thread_rwlock_destroy(scheluder->pending_events.event_mutex);
	cc_events *cc_e = NULL;
	cvector_for_each_in(cc_e, scheluder->pending_events.carriers){
		oset_apr_thread_rwlock_destroy(cc_e->event_cc_mutex);
		cvector_free(cc_e->next_slot_ue_events);
		cvector_free(cc_e->current_slot_ue_events);
	}
	cvector_free(scheluder->pending_events.carriers);
	cvector_free(scheluder->pending_events.next_slot_events);
	cvector_free(scheluder->pending_events.current_slot_events);
	cvector_free(scheluder->pending_events.next_slot_ue_events);
	cvector_free(scheluder->pending_events.current_slot_ue_events);

	//cc_worker
	cc_worker *cc_w = NULL;
	cvector_for_each_in(cc_w, scheluder->cc_workers){
		cc_worker_destoy(cc_w);
	}
	cvector_free(scheluder->cc_workers);

	sched_nr_ue_remove_all();
	oset_hash_destroy(scheluder->ue_db);
	oset_list_empty(&scheluder->sched_ue_list);
	oset_pool_final(&scheluder->ue_pool);
}

int sched_nr_config(sched_nr *scheluder, sched_args_t *sched_cfg, cvector_vector_t(sched_nr_cell_cfg_t) sched_cells)
{
	uint32_t cc = 0;
	uint32 cell_size = cvector_size(sched_cells);

	scheluder->cfg.sched_cfg = sched_cfg;

	// Initiate Common Sched Configuration form rrc sched-config
	//cell_config_manager
	cvector_reserve(scheluder->cfg.cells, cell_size);
	for (cc = 0; cc < cell_size; ++cc) {
		cell_config_manager cell_cof_manager = {0};
		cell_config_manager_init(&cell_cof_manager, cc, &sched_cells[cc], sched_cfg);
		cvector_push_back(scheluder->cfg.cells, cell_cof_manager);
	}

	//callback event
	oset_apr_thread_rwlock_create(&scheluder->pending_events.event_mutex, mac_manager_self()->app_pool);
	cvector_reserve(scheluder->pending_events.carriers, cell_size);
	for(cc = 0; cc < cell_size; ++cc){
		cc_events cc_e = {0};
		oset_apr_thread_rwlock_create(&cc_e.event_cc_mutex, mac_manager_self()->app_pool);
		cvector_push_back(scheluder->pending_events.carriers, cc_e);
	}

	//cc_worker
	cvector_reserve(scheluder->cc_workers, cell_size);
	for (cc = 0; cc < cell_size; ++cc) {
		cc_worker cc_w = {0};
		cc_worker_init(&cc_w, &scheluder->cfg.cells[cc]);
		cvector_push_back(scheluder->cc_workers, cc_w);
	}

  return OSET_OK;
}

void sched_nr_get_metrics(ue_metrics_manager *manager, mac_metrics_t *metrics)
{
	get_metrics(manager, metrics);
}


// NOTE: there is no parallelism in these operations
// 这些操作没有并行性
void sched_nr_slot_indication(sched_nr *scheluder, slot_point slot_tx)
{
	// process non-cc specific feedback if pending (e.g. SRs, buffer state updates, UE config) for CA-enabled UEs
	// Note: non-CA UEs are updated later in get_dl_sched, to leverage parallelism
	process_events(scheluder);

	// prepare CA-enabled UEs internal state for new slot
	// Note: non-CA UEs are updated later in get_dl_sched, to leverage parallelism
	// Find first available channel that supports this frequency and allocated it
	sched_nr_ue *ue = NULL, *next_ue = NULL;
	oset_list_for_each_safe(&scheluder->sched_ue_list, next_ue, ue){
		if (sched_nr_ue_has_ca(ue)) {
			sched_nr_ue_new_slot(ue, slot_tx);
		}
	}

	// If UE metrics were externally requested, store the current UE state
	save_metrics(&scheluder->metrics_handler);
}

/// Generate {pdcch_slot,cc} scheduling decision
dl_res_t* sched_nr_get_dl_sched(sched_nr *scheluder, slot_point pdsch_tti, uint32_t cc)
{

	// process non-cc specific feedback if pending (e.g. SRs, buffer state updates, UE config) for non-CA UEs
	pending_events->process_cc_events(ue_db, cc);//处理接收到的上行消息

	// prepare non-CA UEs internal state for new slot为新时隙准备非CA UE的内部状态
	for (auto& u : ue_db) {
	if (not u.second->has_ca() and u.second->carriers[cc] != nullptr) {//？？？不确定  在随机接入完成前不会进入 u.second->carriers[cc]
	  u.second->new_slot(current_slot_tx);//void ue::new_slot(slot_point pdcch_slot)
	  //计算每个用户slot时间上下行需要bytes
	}
	}

	// Process pending CC-specific feedback, generate {slot_idx,cc} scheduling decision
	sched_nr::dl_res_t* ret = cc_workers[cc]->run_slot(pdsch_tti, ue_db);//dl_sched_res_t* cc_worker::run_slot(slot_point tx_sl, ue_map_t& ue_db)

  return ret;
}

////////////////////////////////////sched callback/////////////////////////////////////////
static int sched_nr_ue_cfg_impl(sched_nr *scheluder, uint16_t rnti, sched_nr_ue_cfg_t *uecfg)
{
	sched_nr_ue *u = sched_ue_nr_find_by_rnti(rnti);

	if (NULL == u) {
		// create user object
		sched_nr_ue *u = sched_nr_ue_add_inner(rnti, uecfg, scheluder->cfg);
		oset_assert(u);
		return sched_nr_add_ue_impl(rnti, u, uecfg->carriers[0].cc);
	}

	sched_nr_ue_set_cfg(u, uecfg);
	return OSET_OK;
}

void sched_nr_ue_cfg_impl_callback(void *argv)
{
	ue_cfg_argv_t *ue_cfg_argv = (ue_cfg_argv_t *)argv;
	oset_assert(ue_cfg_argv);

	sched_nr_ue_cfg_t *uecfg = ue_cfg_argv->uecfg;
	oset_assert(uecfg);

	sched_nr_ue_cfg_impl(&mac_manager_self()->sched, ue_cfg_argv->rnti, uecfg);
}

static int is_ue_cfg_valid(uint16_t rnti, sched_nr_ue_cfg_t *uecfg)
{
	int num = 0;
	for(int i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_CORESET, ++i){
		if(uecfg->phy_cfg.pdcch.coreset_present[i]){
			num = num + 1;
		}
	}
	ERROR_IF_NOT(num > 0, "Provided rnti=0x%x configuration does not contain any coreset", rnti);
	return OSET_OK;
}

void sched_nr_ue_cfg(sched_nr *scheluder, uint16_t rnti, sched_nr_ue_cfg_t *uecfg)
{
	ASSERT_IF_NOT(is_ue_cfg_valid(rnti, uecfg) == OSET_OK, "Invalid UE configuration");

	event_t ue_cfg = {0};
	ue_cfg.event_name = UE_CFG;
	ue_cfg.callback = sched_nr_ue_cfg_impl_callback;
	ue_cfg.u.ue_cfg_argv.rnti = rnti;
	ue_cfg.u.ue_cfg_argv.uecfg = uecfg;
	oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
	cvector_push_back(scheluder->pending_events.next_slot_events, ue_cfg);
	oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
}

void sched_nr_ue_remove_callback(void *argv)
{
	ue_rem_argv_t *ue_rem_argv = (ue_rem_argv_t *)argv;
	oset_assert(ue_rem_argv);

	sched_nr_ue *u = ue_rem_argv->u;
	oset_assert(u);

	sched_nr_ue_remove(u);
}

void sched_nr_ue_rem(sched_nr *scheluder, uint16_t rnti)
{
  sched_nr_ue* u = sched_ue_nr_find_by_rnti(rnti);
  if(NULL == u) return;

  event_t ue_rem = {0};
  ue_rem.event_name = UE_REM;
  ue_rem.callback = sched_nr_ue_remove_callback;
  ue_rem.u.ue_rem_argv.u = u;

  oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
  cvector_push_back(scheluder->pending_events.next_slot_events, ue_rem);
  oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
}

void dl_rach_info_callback(void *argv)
{
	dl_rach_info_argv_t *dl_rach_info_argv = (dl_rach_info_argv_t *)argv;
	oset_assert(dl_rach_info_argv);

	rar_info_t *rar_info = dl_rach_info_argv->rar_info;
	oset_assert(rar_info);
	uint16_t rnti = rar_info->temp_crnti;
	uint32_t cc = rar_info->cc;

	sched_nr_ue *u = sched_nr_ue_add(rnti, cc, &mac_manager_self()->sched.cfg);
	oset_assert(u);

	sched_nr_add_ue_impl(rnti, u, cc);

	if (sched_ue_nr_find_by_rnti(rnti)) {
		oset_info("dl_rach_info(temp c-rnti=0x{:x})", rnti);
		// RACH is handled only once the UE object is created and inserted in the ue_db
		cc_worker_dl_rach_info(&mac_manager_self()->sched.cc_workers[cc], rar_info);//pre deal msg2
	} else {
		oset_error("Failed to create UE object with rnti=0x%x", rnti);
	}
};

int sched_nr_dl_rach_info(sched_nr *scheluder, rar_info_t *rar_info)
{
	// enqueue UE creation event + RACH handling
	event_t dl_rach_info = {0};
	dl_rach_info.event_name = DL_RACH_INFO;
	dl_rach_info.callback = dl_rach_info_callback;
	dl_rach_info.u.dl_rach_info_argv.rar_info = *rar_info;//need free
	oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
	cvector_push_back(scheluder->pending_events.next_slot_events, dl_rach_info);
	oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
	return OSET_OK;
}


