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
	oset_assert(scheluder);
	//cell_config_manager_destory
	cell_config_manager *cell_cof_manager = NULL;
	cvector_for_each_in(cell_cof_manager, scheluder->cfg.cells){
		cvector_free(cell_cof_manager->sibs);
		// just idx0 for BWP-common
		bwp_params_t *bwp = NULL;
		cvector_for_each_in(bwp, cell_cof_manager->bwps){
			cvector_free(bwp->pusch_ra_list);
		}
		cvector_free(cell_cof_manager->bwps);
	}
	cvector_free(scheluder->cfg.cells);

	//event
	oset_apr_mutex_destroy(scheluder->pending_events.event_mutex);
	cc_events *cc_e = NULL;
	cvector_for_each_in(cc_e, scheluder->pending_events.carriers){
		oset_apr_mutex_destroy(cc_e->event_cc_mutex);
		oset_list_empty(&cc_e->next_slot_ue_events);
		oset_list_empty(&cc_e->current_slot_ue_events);
	}
	cvector_free(scheluder->pending_events.carriers);
	oset_list_empty(&scheluder->pending_events.next_slot_events);
	oset_list_empty(&scheluder->pending_events.current_slot_events);
	oset_list_empty(&scheluder->pending_events.next_slot_ue_events);
	oset_list_empty(&scheluder->pending_events.current_slot_ue_events);

	//cc_worker
	cc_worker *cc_w = NULL;
	cvector_for_each_in(cc_w, scheluder->cc_workers){
		// just idx0 for BWP-common
		bwp_manager *bwp = NULL;
		cvector_for_each_in(bwp, cc_w->bwps){
			//si
			cvector_free(bwp->si.pending_sis);
			//bwp_res_grid
			bwp_slot_grid *slot = NULL;
			cvector_for_each_in(slot, bwp->grid.slots){
				//dl
				cvector_free(slot->dl.phy.ssb);
				cvector_free(slot->dl.phy.pdcch_dl);
				cvector_free(slot->dl.phy.pdcch_ul);
				cvector_free(slot->dl.phy.pdsch);
				cvector_free(slot->dl.phy.nzp_csi_rs);
				cvector_free(slot->dl.data);
				cvector_free(slot->dl.rar);
				cvector_free(slot->dl.sib_idxs);
				//ul
				cvector_free(slot->ul.pucch);
				cvector_free(slot->ul.pusch);
				//ack
				cvector_free(bwp->grid.slots[sl].pending_acks);
				//pddchs
				for (uint32_t cs_idx = 0; cs_idx < SRSRAN_UE_DL_NR_MAX_NOF_CORESET; ++cs_idx) {
					cvector_free(slot->pdcchs.coresets[cs_idx].dci_list);
					cvector_free(slot->pdcchs.coresets[cs_idx].dfs_tree);
					cvector_free(slot->pdcchs.coresets[cs_idx].saved_dfs_tree);
				}
			}
			cvector_free(bwp->grid.slots);
		}		
		//release harqbuffer
		harq_softbuffer_pool_destory(harq_buffer_pool_self(cc), cc_w->cfg->carrier.nof_prb, 4 * MAX_HARQ, 0);
	}
	cvector_free(scheluder->cc_workers);

	oset_pool_final(&scheluder->ue_pool);
	oset_hash_destroy(scheluder->ue_db);
}

int sched_nr_config(sched_nr *scheluder, sched_args_t *sched_cfg, cvector_vector_t(sched_nr_cell_cfg_t) sche_cells)
{
	uint32_t cc = 0;
	uint32 cell_size = cvector_size(sche_cells);

	scheluder->cfg.sched_cfg = sched_cfg;

	// Initiate Common Sched Configuration form rrc sched-config
	cvector_reserve(scheluder->cfg.cells, cell_size);
	for (cc = 0; cc < cvector_size(scheluder->cfg.cells); ++cc) {
		cell_config_manager *cell_cof_manager = &scheluder->cfg.cells[cc];
		cell_config_manager_init(cell_cof_manager, cc, &sche_cells[cc], sched_cfg);
	}

	//调度事件管理模块
	oset_apr_mutex_init(&scheluder->pending_events.event_mutex, OSET_MUTEX_NESTED, mac_manager_self()->app_pool);
	cvector_reserve(scheluder->pending_events.carriers, cell_size);
	for(cc = 0; cc < cvector_size(scheluder->cfg.cells); ++cc){
		cc_events *cc_e = scheluder->pending_events.carriers[cc];
		oset_apr_mutex_init(&cc_e->event_cc_mutex, OSET_MUTEX_NESTED, mac_manager_self()->app_pool);
		oset_list_init(&cc_e->next_slot_ue_events);
		oset_list_init(&cc_e->current_slot_ue_events);
	}
	oset_list_init(&scheluder->pending_events.next_slot_events);
	oset_list_init(&scheluder->pending_events.current_slot_events);
	oset_list_init(&scheluder->pending_events.next_slot_ue_events);
	oset_list_init(&scheluder->pending_events.current_slot_ue_events);

	// Initiate cell-specific schedulers
	cvector_reserve(scheluder->cc_workers, cell_size);
	for (cc = 0; cc < cvector_size(scheluder->cc_workers); ++cc) {
		cc_worker *cc_w = &scheluder->cc_workers[cc];
		cc_worker_init(cc_w, &scheluder->cfg.cells[cc]);
	}

  return OSET_OK;
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


