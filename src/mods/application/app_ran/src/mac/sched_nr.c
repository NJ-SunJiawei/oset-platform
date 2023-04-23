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

void sched_nr_init(sched_nr *scheluder)
{
	oset_assert(scheluder);

	// Initiate sched_nr_ue memory pool//最大调度64个用户
	oset_pool_init(scheluder->ue_pool, SRSENB_MAX_UES);
	oset_list_init(&scheluder->sched_ue_list);
	scheluder->ue_db = oset_hash_make();

	scheluder->metrics_handler.ues = scheluder->ue_db;
}

void sched_nr_destory(sched_nr *scheluder)
{
	oset_assert(scheluder);

	//cell_config_manager
	cell_config_manager *cell_cof_manager = NULL;
	cvector_for_each_in(cell_cof_manager, scheluder->cfg.cells){
		cell_config_manager_destory(cell_cof_manager);
	}
	cvector_free(scheluder->cfg.cells);

	//callback event
	oset_apr_mutex_destroy(scheluder->pending_events.event_mutex);
	cc_events *cc_e = NULL;
	cvector_for_each_in(cc_e, scheluder->pending_events.carriers){
		oset_apr_mutex_destroy(cc_e->event_cc_mutex);
		oset_stl_queue_term(&cc_e->next_slot_ue_events);
		oset_stl_queue_term(&cc_e->current_slot_ue_events);
	}
	cvector_free(scheluder->pending_events.carriers);
	oset_stl_queue_term(&scheluder->pending_events.next_slot_events);
	oset_stl_queue_term(&scheluder->pending_events.current_slot_events);
	oset_stl_queue_term(&scheluder->pending_events.next_slot_ue_events);
	oset_stl_queue_term(&scheluder->pending_events.current_slot_ue_events);

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
	oset_apr_mutex_init(&scheluder->pending_events.event_mutex, OSET_MUTEX_NESTED, mac_manager_self()->app_pool);
	cvector_reserve(scheluder->pending_events.carriers, cell_size);
	for(cc = 0; cc < cell_size; ++cc){
		cc_events cc_e = {0};
		oset_apr_mutex_init(&cc_e.event_cc_mutex, OSET_MUTEX_NESTED, mac_manager_self()->app_pool);
		oset_stl_queue_init(&cc_e.next_slot_ue_events);
		oset_stl_queue_init(&cc_e.current_slot_ue_events);
		cvector_push_back(scheluder->pending_events.carriers, cc_e);
	}
	oset_stl_queue_init(&scheluder->pending_events.next_slot_events);
	oset_stl_queue_init(&scheluder->pending_events.current_slot_events);
	oset_stl_queue_init(&scheluder->pending_events.next_slot_ue_events);
	oset_stl_queue_init(&scheluder->pending_events.current_slot_ue_events);

	//cc_worker
	cvector_reserve(scheluder->cc_workers, cell_size);
	for (cc = 0; cc < cell_size; ++cc) {
		cc_worker cc_w = {0};
		cc_worker_init(&cc_w, &scheluder->cfg.cells[cc]);
		cvector_push_back(scheluder->cc_workers, cc_w);
	}

  return OSET_OK;
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
	oset_stl_queue_add_last(&scheluder->pending_events.next_slot_events, ue_cfg);
}

void sched_nr_ue_remove_callback(int argc, void **argv)
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
  oset_stl_queue_add_last(&scheluder->pending_events.next_slot_events, ue_rem);
}

void dl_rach_info_callback(void *argv)
{
	dl_rach_info_argv_t *dl_rach_info_argv = (dl_rach_info_argv_t *)argv;
	oset_assert(dl_rach_info_argv);

	rar_info_t *rar_info = dl_rach_info_argv->rar_info;
	sched_nr_ue *u = dl_rach_info_argv->u;
	oset_assert(rar_info);
	oset_assert(u);

	uint16_t rnti = rar_info->temp_crnti;
	uint32_t cc = rar_info->cc;
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
	sched_nr_ue *u = sched_nr_ue_add(rar_info->temp_crnti, rar_info->cc, scheluder->cfg);
	oset_assert(u);

	// enqueue UE creation event + RACH handling
	event_t dl_rach_info = {0};
	dl_rach_info.event_name = DL_RACH_INFO;
	dl_rach_info.callback = dl_rach_info_callback;
	dl_rach_info.u.dl_rach_info_argv.rar_info = *rar_info;//need free
	dl_rach_info.u.dl_rach_info_argv.u = u;
	oset_stl_queue_add_last(&scheluder->pending_events.next_slot_events, dl_rach_info);
	return OSET_OK;
}


