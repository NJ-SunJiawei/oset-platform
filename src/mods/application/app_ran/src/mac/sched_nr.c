/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac/mac.h"
#include "mac/sched_nr.h"
#include "mac/sched_nr_worker.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched"

////////////////////////////////////sched callback/////////////////////////////////////////
/// Enqueue feedback directed at a given UE in a given cell (e.g. ACKs, CQI)
/// enqueue_ue_cc_feedback
void dl_cqi_info_callback(ue_carrier *carriers, void *argv)
{
	dl_cqi_info_argv_t *dl_cqi_info_argv = (dl_cqi_info_argv_t *)argv;
	oset_assert(dl_cqi_info_argv);

	carriers->dl_cqi = dl_cqi_info_argv->cqi_value;
    oset_info("rnti=0x%x dl_cqi_info(cqi=%u)", carriers->rnti, dl_cqi_info_argv->cqi_value);
}


void sched_nr_dl_cqi_info(sched_nr *scheluder, uint16_t rnti, uint32_t cc, uint32_t cqi_value)
{
	ue_cc_event_t dl_cqi_info = {0};
	dl_cqi_info.cc   = cc;
	dl_cqi_info.rnti = rnti;
	dl_cqi_info.event_name = DL_CQI_INFO;
	dl_cqi_info.callback = dl_cqi_info_callback;
	dl_cqi_info.u.dl_cqi_info_argv.cqi_value = cqi_value;

    ASSERT_IF_NOT(rnti != SRSRAN_INVALID_RNTI, "Invalid rnti=0x%x passed to event manager", rnti);
    ASSERT_IF_NOT(cc <  cvector_size(scheluder->pending_events.carriers), "Invalid cc=%d passed to event manager", cc);
	//oset_apr_thread_rwlock_wrlock(scheluder->pending_events.carriers[cc].event_cc_mutex);
	cvector_push_back(scheluder->pending_events.carriers[cc].next_slot_ue_events, dl_cqi_info);
	//oset_apr_thread_rwlock_unlock(scheluder->pending_events.carriers[cc].event_cc_mutex);
}

void ul_crc_info_callback(ue_carrier *carriers, void *argv)
{
	ul_crc_info_argv_t *ul_crc_info_argv = (ul_crc_info_argv_t *)argv;
	oset_assert(ul_crc_info_argv);

	ue_carrier_ul_crc_info(carriers, ul_crc_info_argv->pid, ul_crc_info_argv->crc);
	oset_info("rnti=0x%x ul_crc_info(pid=%u, crc=%s)", carriers->rnti, ul_crc_info_argv->pid, ul_crc_info_argv->crc ? "OK" : "KO");
}

void sched_nr_ul_crc_info(sched_nr *scheluder, uint16_t rnti, uint32_t cc, uint32_t pid, bool crc)
{
	ue_cc_event_t ul_crc_info = {0};
	ul_crc_info.cc   = cc;
	ul_crc_info.rnti = rnti;
	ul_crc_info.event_name = UL_CRC_INFO;
	ul_crc_info.callback = ul_crc_info_callback;
	ul_crc_info.u.ul_crc_info_argv.pid = pid;
	ul_crc_info.u.ul_crc_info_argv.crc = crc;

    ASSERT_IF_NOT(rnti != SRSRAN_INVALID_RNTI, "Invalid rnti=0x%x passed to event manager", rnti);
    ASSERT_IF_NOT(cc <  cvector_size(scheluder->pending_events.carriers), "Invalid cc=%d passed to event manager", cc);
	//oset_apr_thread_rwlock_wrlock(scheluder->pending_events.carriers[cc].event_cc_mutex);
	cvector_push_back(scheluder->pending_events.carriers[cc].next_slot_ue_events, ul_crc_info);
	//oset_apr_thread_rwlock_unlock(scheluder->pending_events.carriers[cc].event_cc_mutex);
}


void dl_ack_info_callback(ue_carrier *carriers, void *argv)
{
	dl_ack_info_argv_t *dl_ack_info_argv = (dl_ack_info_argv_t *)argv;
	oset_assert(dl_ack_info_argv);

	ue_carrier_dl_ack_info(carriers, dl_ack_info_argv->pid, dl_ack_info_argv->tb_idx, dl_ack_info_argv->ack);
	oset_info("rnti=0x%x dl_ack_info(pid=%u, ack=%s)", carriers->rnti, dl_ack_info_argv->pid, dl_ack_info_argv->ack ? "OK" : "KO");
}

void sched_nr_dl_ack_info(sched_nr *scheluder, uint16_t rnti, uint32_t cc, uint32_t pid, uint32_t tb_idx, bool ack)
{ 
	ue_cc_event_t dl_ack_info = {0};
	dl_ack_info.cc   = cc;
	dl_ack_info.rnti = rnti;
	dl_ack_info.event_name = DL_ACK_INFO;
	dl_ack_info.callback = dl_ack_info_callback;
	dl_ack_info.u.dl_ack_info_argv.pid    = pid;
	dl_ack_info.u.dl_ack_info_argv.tb_idx = tb_idx;
	dl_ack_info.u.dl_ack_info_argv.ack    = ack;

    ASSERT_IF_NOT(rnti != SRSRAN_INVALID_RNTI, "Invalid rnti=0x%x passed to event manager", rnti);
    ASSERT_IF_NOT(cc <  cvector_size(scheluder->pending_events.carriers), "Invalid cc=%d passed to event manager", cc);
	//oset_apr_thread_rwlock_wrlock(scheluder->pending_events.carriers[cc].event_cc_mutex);
	cvector_push_back(scheluder->pending_events.carriers[cc].next_slot_ue_events, dl_ack_info);
	//oset_apr_thread_rwlock_unlock(scheluder->pending_events.carriers[cc].event_cc_mutex);
}


/// enqueue_ue_event
/// Enqueue an event that directly maps into a ue method (e.g. ul_sr_info, ul_bsr, etc.)
/// Note: these events can be processed sequentially or in parallel, depending on whether the UE supports CA
void ul_sr_info_callback(sched_nr_ue *ue, void *argv)
{
	sched_nr_ue_ul_sr_info(ue);
	oset_info("rnti=0x%x ul_sr_info()", ue->rnti);
}


void sched_nr_ul_sr_info(sched_nr *scheluder, uint16_t rnti)
{
  ue_event_t ul_sr_info = {0};
  ul_sr_info.rnti = rnti;
  ul_sr_info.event_name = UL_SR_INFO;
  ul_sr_info.callback = ul_sr_info_callback;

  ASSERT_IF_NOT(rnti != SRSRAN_INVALID_RNTI, "Invalid rnti=0x%x passed to event manager", rnti);
  //oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
  cvector_push_back(scheluder->pending_events.next_slot_ue_events, ul_sr_info);
  //oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
}


void ul_bsr_callback(sched_nr_ue *ue, void *argv)
{	
	ul_bsr_argv_t *ul_bsr_argv = (ul_bsr_argv_t *)argv;
	oset_assert(ul_bsr_argv);

	sched_nr_ue_ul_bsr(ue, ul_bsr_argv->lcg_id, ul_bsr_argv->bsr);

	oset_info("rnti=0x%x ul_bsr(lcg=%u, bsr=%u)", ue->rnti, ul_bsr_argv->lcg_id, ul_bsr_argv->bsr);
}


void sched_nr_ul_bsr(sched_nr *scheluder, uint16_t rnti, uint32_t lcg_id, uint32_t bsr)
{
  ue_event_t ul_bsr = {0};
  ul_bsr.rnti = rnti;
  ul_bsr.event_name = UL_BSR;
  ul_bsr.callback = ul_bsr_callback;
  ul_bsr.u.ul_bsr_argv.lcg_id = lcg_id;
  ul_bsr.u.ul_bsr_argv.bsr = bsr;

  ASSERT_IF_NOT(rnti != SRSRAN_INVALID_RNTI, "Invalid rnti=0x%x passed to event manager", rnti);
  //oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
  cvector_push_back(scheluder->pending_events.next_slot_ue_events, ul_bsr);
  //oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
}

void dl_mac_ce_callback(sched_nr_ue *ue, void *argv)
{	
	dl_mac_ce_argv_t *dl_mac_ce_argv = (dl_mac_ce_argv_t *)argv;
	oset_assert(dl_mac_ce_argv);

	// CE is added to list of pending CE
	sched_nr_ue_add_dl_mac_ce(ue, dl_mac_ce_argv->ce_lcid, 1);

	oset_info("rnti=0x%x dl_mac_ce(lcid=%u)", ue->rnti, dl_mac_ce_argv->ce_lcid);
}


void sched_nr_dl_mac_ce(sched_nr *scheluder, uint16_t rnti, uint32_t ce_lcid)
{
  ue_event_t dl_mac_ce = {0};
  dl_mac_ce.rnti = rnti;
  dl_mac_ce.event_name = DL_MAC_CE;
  dl_mac_ce.callback = dl_mac_ce_callback;
  dl_mac_ce.u.dl_mac_ce_argv.ce_lcid = ce_lcid;

  ASSERT_IF_NOT(rnti != SRSRAN_INVALID_RNTI, "Invalid rnti=0x%x passed to event manager", rnti);
  //oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
  cvector_push_back(scheluder->pending_events.next_slot_ue_events, dl_mac_ce);
  //oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
}

void dl_buffer_state_callback(sched_nr_ue *ue, void *argv)
{	
	dl_buffer_state_argv_t *dl_buffer_state_argv = (dl_buffer_state_argv_t *)argv;
	oset_assert(dl_buffer_state_argv);

	sched_nr_ue_rlc_buffer_state(ue, dl_buffer_state_argv->lcid, dl_buffer_state_argv->newtx, dl_buffer_state_argv->retx);

	oset_info("rnti=0x%x  dl_buffer_state(lcid=%u, bsr=%u,%u)", ue->rnti, dl_buffer_state_argv->lcid, dl_buffer_state_argv->newtx, dl_buffer_state_argv->retx);
}


void sched_nr_dl_buffer_state(sched_nr *scheluder, uint16_t rnti, uint32_t lcid, uint32_t newtx, uint32_t retx)
{
	ue_event_t dl_buffer_state = {0};
	dl_buffer_state.rnti = rnti;
	dl_buffer_state.event_name = DL_BUFFER_STATE;
	dl_buffer_state.callback = dl_buffer_state_callback;
	dl_buffer_state.u.dl_buffer_state_argv.lcid = lcid;
	dl_buffer_state.u.dl_buffer_state_argv.newtx = newtx;
	dl_buffer_state.u.dl_buffer_state_argv.retx = retx;

	ASSERT_IF_NOT(rnti != SRSRAN_INVALID_RNTI, "Invalid rnti=0x%x passed to event manager", rnti);
	//oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
	cvector_push_back(scheluder->pending_events.next_slot_ue_events, dl_buffer_state);
	//oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
}


/// enqueue_event
/// Enqueue an event that does not map into a ue method (e.g. rem_user, add_user)
static int sched_nr_ue_cfg_impl(sched_nr *scheluder, uint16_t rnti, sched_nr_ue_cfg_t *uecfg)
{
	sched_nr_ue *u = sched_nr_ue_find_by_rnti(rnti);

	if (NULL == u) {
		// create user object
		sched_nr_ue *u = sched_nr_ue_add_inner(rnti, uecfg, scheluder->cfg);
		oset_assert(u);
		sched_nr_add_ue_impl(rnti, u, uecfg->carriers[0].cc);
		return OSET_OK;
	}

	sched_nr_ue_set_cfg(u, uecfg);
	return OSET_OK;
}

void ue_cfg_impl_callback(uint16_t rnti, void *argv)
{
	ue_cfg_argv_t *ue_cfg_argv = (ue_cfg_argv_t *)argv;
	oset_assert(ue_cfg_argv);

	sched_nr_ue_cfg_t *uecfg = ue_cfg_argv->uecfg;
	oset_assert(uecfg);

	if (OSET_OK == sched_nr_ue_cfg_impl(&mac_manager_self()->sched, rnti, uecfg)) {
      oset_info("rnti=0x%x ue_cfg()", rnti);
    } else {
      oset_error("Failed to create UE object for rnti=0x%x", rnti);
    }
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
	ue_cfg.rnti = rnti;
	ue_cfg.event_name = UE_CFG;
	ue_cfg.callback = ue_cfg_impl_callback;
	ue_cfg.u.ue_cfg_argv.uecfg = uecfg;
	//oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
	cvector_push_back(scheluder->pending_events.next_slot_events, ue_cfg);
	//oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
}

void ue_remove_callback(uint16_t rnti, void *argv)
{
	sched_nr_ue* u = sched_nr_ue_find_by_rnti(rnti);
	if(NULL == u) return;

	sched_nr_ue_remove(u);
	oset_info("SCHED: Removed user rnti=0x%x", rnti);
}

void sched_nr_ue_rem(sched_nr *scheluder, uint16_t rnti)
{
  event_t ue_rem = {0};
  ue_rem.rnti = rnti;
  ue_rem.event_name = UE_REM;
  ue_rem.callback = ue_remove_callback;

  //oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
  cvector_push_back(scheluder->pending_events.next_slot_events, ue_rem);
  //oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
}

void dl_rach_info_callback(uint16_t rnti, void *argv)
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

	if (sched_nr_ue_find_by_rnti(rnti)) {
		oset_info("[%5u] dl_rach_info(temp c-rnti=0x%x)", count_idx(&rar_info->prach_slot), rnti);
		// RACH is handled only once the UE object is created and inserted in the ue_db
		cc_worker_dl_rach_info(&mac_manager_self()->sched.cc_workers[cc], rar_info);//pre deal msg2
	} else {
		oset_error("[%5u] Failed to create UE object with rnti=0x%x", count_idx(&rar_info->prach_slot), rnti);
	}
};

int sched_nr_dl_rach_info(sched_nr *scheluder, rar_info_t *rar_info)
{
	// enqueue UE creation event + RACH handling
	event_t dl_rach_info = {0};
	dl_rach_info.rnti = rar_info->temp_crnti;
	dl_rach_info.event_name = DL_RACH_INFO;
	dl_rach_info.callback = dl_rach_info_callback;
	dl_rach_info.u.dl_rach_info_argv.rar_info = *rar_info;//need free
	//oset_apr_thread_rwlock_wrlock(scheluder->pending_events.event_mutex);
	cvector_push_back(scheluder->pending_events.next_slot_events, dl_rach_info);
	//oset_apr_thread_rwlock_unlock(scheluder->pending_events.event_mutex);
	return OSET_OK;
}


//////////////////////////////metrics/////////////////////////////////////
static void save_metrics_nolock(ue_metrics_manager *manager)
{
    if (NULL == manager->pending_metrics) {
		return;
    }

	mac_ue_metrics_t *ue_metric = NULL;
	cvector_for_each_in(ue_metric, manager->pending_metrics->ues){
		sched_nr_ue * ue = sched_nr_ue_find_by_rnti(ue_metric->rnti);
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

void sched_nr_get_metrics(ue_metrics_manager *manager, mac_metrics_t *metrics)
{
	get_metrics(manager, metrics);
}
//////////////////////////sched_nr////////////////////////////////////////
/// Process all events that are not specific to a carrier or that are directed at CA-enabled UEs
/// Note: non-CA UEs are updated later in get_dl_sched, to leverage parallelism
/// 处理非特定于运营商或针对支持CA的UE的所有事件
/// 注意：非CA UE稍后在get_dl_sched中更新，以利用并行性
static void process_common(sched_nr *scheluder, uint32_t slot_rx_id)
{
	event_manager *pending_events = &scheluder->pending_events;

    // Extract pending feedback events
    {	
    	//todo need lock?
    	//oset_apr_thread_rwlock_rdlock(pending_events->event_mutex);
		cvector_clear(pending_events->current_slot_ue_events);
		cvector_clear(pending_events->current_slot_events);

		cvector_copy(pending_events->next_slot_ue_events, pending_events->current_slot_ue_events);
		cvector_copy(pending_events->next_slot_events, pending_events->current_slot_events);

		cvector_clear(pending_events->next_slot_ue_events);
		cvector_clear(pending_events->next_slot_events);
		//oset_apr_thread_rwlock_unlock(pending_events->event_mutex);
    }

	// non-UE specific events
	event_t *ev = NULL;
	cvector_for_each_in(ev, pending_events->current_slot_events){
		if(DL_RACH_INFO == ev->event_name) ev->callback(ev->rnti, &ev->u.dl_rach_info_argv);
		if(UE_CFG == ev->event_name)       ev->callback(ev->rnti, &ev->u.ue_cfg_argv);
		if(UE_REM == ev->event_name)       ev->callback(ev->rnti, NULL);
	}

	//handle ca()
	ue_event_t *u_ev = NULL;
	cvector_for_each_in(u_ev, pending_events->current_slot_ue_events){
		sched_nr_ue *ue_it = sched_nr_ue_find_by_rnti(u_ev->rnti);
		if (NULL == ue_it) {
		  oset_warn("[5%lu] SCHED: %s called for unknown rnti=0x%x", slot_rx_id, u_ev->event_name, u_ev->rnti);
		  u_ev->rnti = SRSRAN_INVALID_RNTI;
		} else if (sched_nr_ue_has_ca(ue_it)) {
			// events specific to existing UEs with CA
			if(UL_SR_INFO == u_ev->event_name)      u_ev->callback(ue_it, NULL);
			if(UL_BSR == u_ev->event_name)          u_ev->callback(ue_it, &u_ev->u.ul_bsr_argv);
			if(DL_MAC_CE == u_ev->event_name)       u_ev->callback(ue_it, &u_ev->u.dl_mac_ce_argv);
			if(DL_BUFFER_STATE == u_ev->event_name) u_ev->callback(ue_it, &u_ev->u.dl_buffer_state_argv);
			u_ev->rnti = SRSRAN_INVALID_RNTI;
		}
	}
}

/// Process events synchronized during slot_indication() that are directed at non CA-enabled UEs
/// 处理在slot_indication()期间同步的事件，这些事件指向未启用CA的UE
 static void process_cc_events(sched_nr *scheluder, uint32_t cc, uint32_t slot_rx_id)
 {
	event_manager *pending_events = &scheluder->pending_events;

	{
		cvector_clear(pending_events->carriers[cc].current_slot_ue_events);
		cvector_copy(pending_events->carriers[cc].next_slot_ue_events, pending_events->carriers[cc].current_slot_ue_events);
		cvector_clear(pending_events->carriers[cc].next_slot_ue_events);
	}

	ue_event_t *u_ev = NULL;
	cvector_for_each_in(u_ev, pending_events->current_slot_ue_events){
		if (SRSRAN_INVALID_RNTI == u_ev->rnti) {
			// events already processed
			continue;
		}
		sched_nr_ue *ue_it = sched_nr_ue_find_by_rnti(u_ev->rnti);
		if (NULL == ue_it) {
			oset_warn("[5%lu] SCHED: %s called for unknown rnti=0x%x", slot_rx_id, u_ev->event_name, u_ev->rnti);
			u_ev->rnti = SRSRAN_INVALID_RNTI;
		} else if (!sched_nr_ue_has_ca(ue_it)) {
			// && NULL != ue_it->carriers[cc]
			if(UL_SR_INFO == u_ev->event_name)      u_ev->callback(ue_it, NULL);
			if(UL_BSR == u_ev->event_name)          u_ev->callback(ue_it, &u_ev->u.ul_bsr_argv);
			if(DL_MAC_CE == u_ev->event_name)       u_ev->callback(ue_it, &u_ev->u.dl_mac_ce_argv);
			if(DL_BUFFER_STATE == u_ev->event_name) u_ev->callback(ue_it, &u_ev->u.dl_buffer_state_argv);
			u_ev->rnti = SRSRAN_INVALID_RNTI;
		}
	}

	ue_cc_event_t *u_cc_ev = NULL;
	cvector_for_each_in(u_cc_ev, pending_events->carriers[cc].current_slot_ue_events){
		sched_nr_ue *ue_it = sched_nr_ue_find_by_rnti(u_cc_ev->rnti);
		if (NULL != ue_it && NULL != ue_it->carriers[u_cc_ev->cc]) {
			if(DL_ACK_INFO == u_cc_ev->event_name) u_cc_ev->callback(ue_it->carriers[u_cc_ev->cc], &u_cc_ev->u.dl_ack_info_argv);
			if(UL_CRC_INFO == u_cc_ev->event_name) u_cc_ev->callback(ue_it->carriers[u_cc_ev->cc], &u_cc_ev->u.ul_crc_info_argv);
			if(DL_CQI_INFO == u_cc_ev->event_name) u_cc_ev->callback(ue_it->carriers[u_cc_ev->cc], &u_cc_ev->u.dl_cqi_info_argv);
		} else {
			oset_warn("[5%lu] SCHED: %s called for unknown rnti=0x%x, cc=%d", slot_rx_id, u_cc_ev->event_name, u_cc_ev->rnti, u_cc_ev->cc);
		}
	}
 }


void sched_nr_init(sched_nr *scheluder)
{
	oset_assert(scheluder);

	// Initiate sched_nr_ue memory pool//最大调度64个用户
	oset_pool_init(&scheluder->ue_pool, SRSENB_MAX_UES);
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

// NOTE: there is no parallelism in these operations
// 这些操作没有并行性
void sched_nr_slot_indication(sched_nr *scheluder, slot_point slot_tx)
{
	// process non-cc specific feedback if pending (e.g. SRs, buffer state updates, UE config) for CA-enabled UEs
	// Note: non-CA UEs are updated later in get_dl_sched, to leverage parallelism
	process_common(scheluder, GET_RSLOT_ID(slot_tx));

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
	process_cc_events(scheluder, cc, GET_RSLOT_ID(pdsch_tti));

	// prepare non-CA UEs internal state for new slot
	sched_nr_ue *ue = NULL, *next_ue = NULL;
	oset_list_for_each_safe(&scheluder->sched_ue_list, next_ue, ue){
		if (!sched_nr_ue_has_ca(ue) && NULL != ue->carriers[cc]) {
			sched_nr_ue_new_slot(ue, pdsch_tti);
		}
	}

	// Process pending CC-specific feedback, generate {slot_idx,cc} scheduling decision
	dl_res_t* ret = cc_worker_run_slot(&scheluder->cc_workers[cc], pdsch_tti, &scheluder->sched_ue_list);

  return ret;
}

