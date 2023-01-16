/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-mac"


static mac_manager_t mac_manager = {0};

rrc_manager_t *mac_manager_self(void)
{
	return &mac_manager;
}

static void mac_manager_init(void)
{
	mac_manager.app_pool = gnb_manager_self()->app_pool;

}

static void mac_manager_destory(void)
{
	mac_manager.app_pool = NULL; /*app_pool release by openset process*/
}


void mac_rach_detected(rach_info_t *rach_info)
{
  uint32_t enb_cc_idx = 0;
  /*to change*/
  stack_task_queue.push([this, rach_info, enb_cc_idx]() mutable {

    uint16_t rnti = alloc_ue(enb_cc_idx);//申请rnti

    // Log this event.
    {
      srsran::rwlock_write_guard lock(rwmutex);
      ++detected_rachs[enb_cc_idx];
    }

    // Trigger scheduler RACH
    rar_info_t rar_info = {};
    rar_info.cc                                     = enb_cc_idx;
    rar_info.preamble_idx                           = rach_info.preamble;
    rar_info.temp_crnti                             = rnti;
    rar_info.ta_cmd                                 = rach_info.time_adv;
    rar_info.prach_slot                             = slot_point{NUMEROLOGY_IDX, rach_info.slot_index};
    sched->dl_rach_info(rar_info); //int sched_nr::dl_rach_info(const rar_info_t& rar_info)
    rrc->add_user(rnti, enb_cc_idx);

    logger.info("RACH:  slot=%d, cc=%d, preamble=%d, offset=%d, temp_crnti=0x%x",
                rach_info.slot_index,
                enb_cc_idx,
                rach_info.preamble,
                rach_info.time_adv,
                rnti);
    srsran::console("RACH:  slot=%d, cc=%d, preamble=%d, offset=%d, temp_crnti=0x%x\n",
                    rach_info.slot_index,
                    enb_cc_idx,
                    rach_info.preamble,
                    rach_info.time_adv,
                    rnti);
  });
}


void *gnb_mac_task(oset_threadplus_t *thread, void *data)
{
    msg_def_t *received_msg = NULL;
	uint32_t length = 0;
    task_map_t *task = task_map_self(TASK_MAC);
    int rv = 0;
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Starting MAC layer thread");

     for ( ;; ){
		 rv = oset_ring_queue_try_get(task->msg_queue, &received_msg, &length);
		 if(rv != OSET_OK)
		 {
			if (rv == OSET_DONE)
				break;
		 
			if (rv == OSET_RETRY){
				continue;
			}
		 }
		 //func
		 received_msg = NULL;
		 length = 0;
	}
}


