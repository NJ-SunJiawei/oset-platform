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


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched"

void sched_nr_init(sched_nr *scheluder)
{
	oset_assert(scheluder);
	oset_pool_init(&scheluder->ue_pool, SRSENB_MAX_UES);
   
}

void sched_nr_destory(sched_nr *scheluder)
{
	oset_assert(scheluder);
	oset_pool_final(&scheluder->ue_pool);

   
}

int dl_rach_info(rar_info_t *rar_info)
{
	ue *u = NULL; 

	// create user object outside of sched main thread
	oset_pool_alloc(&pool_omc, &u);
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


