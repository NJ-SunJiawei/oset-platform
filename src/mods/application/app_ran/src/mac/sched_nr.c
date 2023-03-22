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
	scheluder->ue_db = oset_hash_make();

	scheluder->metrics_handler.ues = scheluder->ue_db;
}

void sched_nr_destory(sched_nr *scheluder)
{
    int i = 0;
	oset_assert(scheluder);
	//cell_config_manager_destory
	byn_array_empty(&scheluder->cfg.cells);


	oset_pool_final(&scheluder->ue_pool);
	oset_hash_destroy(scheluder->ue_db);
}

int sched_nr_config(sched_nr *scheluder, sched_args_t *sched_cfg, void *sche_cells)
{
	A_DYN_ARRAY_OF(sched_nr_cell_cfg_t) *cell_list = (A_DYN_ARRAY_OF(sched_nr_cell_cfg_t) *)sche_cells;

	scheluder->cfg.sched_cfg = sched_cfg;

	// Initiate Common Sched Configuration
	byn_array_set_bounded(&scheluder->cfg.cells, byn_array_get_count(cell_list));
	for (uint32_t cc = 0; cc < byn_array_get_count(cell_list); ++cc) {
		sched_nr_cell_cfg_t *cell = byn_array_get_data(cell_list, cc);
		cell_config_manager *cell_cof_manager = oset_core_alloc(mac_manager_self()->app_pool, sizeof(cell_config_manager));
		byn_array_add(&scheluder->cfg.cells, cell_cof_manager);
		cell_config_manager_init(cell_cof_manager, cc, cell, sched_cfg);
	}

	scheluder->pending_events.reset(new event_manager{cfg});//调度事件管理模块
	oset_apr_mutex_init(&scheluder->pending_events.event_mutex, OSET_MUTEX_NESTED, mac_manager_self()->app_pool);
	byn_array_set_bounded(&scheluder->pending_events.carriers, byn_array_get_count(&scheluder->cfg.cells));

	

	// Initiate cell-specific schedulers
	cc_workers.resize(cfg.cells.size());
	for (uint32_t cc = 0; cc < cfg.cells.size(); ++cc) {
	cc_workers[cc].reset(new slot_cc_worker{cfg.cells[cc]});//小区slot调度task
	}

  return SRSRAN_SUCCESS;
}


int dl_rach_info(rar_info_t *rar_info)
{
	ue *u = NULL; 

	// create user object outside of sched main thread
	oset_pool_alloc(&mac_manager_self()->sched.ue_pool, &u);
	unique_ue_ptr u =
	  srsran::make_pool_obj_with_fallback<ue>(*ue_pool, rar_info.temp_crnti, rar_info.temp_crnti, rar_info.cc, cfg);

	// enqueue UE creation event + RACH handling
	auto add_ue = [this, rar_info, u = std::move(u)](event_manager::logger& ev_logger) mutable {
	uint16_t rnti = rar_info.temp_crnti;
	if (add_ue_impl(rnti, std::move(u)) == SRSRAN_SUCCESS) {
	  ev_logger.push("dl_rach_info(temp c-rnti=0x{:x})", rar_info.temp_crnti);
	  // RACH is handled only once the UE object is created and inserted in the ue_db
	  uint32_t cc = rar_info.cc;
	  cc_workers[cc]->dl_rach_info(rar_info);//void cc_worker::dl_rach_info(const sched_nr_interface::rar_info_t& rar_info)
	} else {
	  logger->warning("Failed to create UE object with rnti=0x%x", rar_info.temp_crnti);
	}
	};
	pending_events->enqueue_event("dl_rach_info", std::move(add_ue));
	return SRSRAN_SUCCESS;
}


