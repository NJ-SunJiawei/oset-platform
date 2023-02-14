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

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-mac"

//static OSET_POOL(bcch_bch_payload_pool, byte_buffer_t);
//static OSET_POOL(rar_pdu_buffer_pool, byte_buffer_t);

static mac_manager_t mac_manager = {0};

mac_manager_t *mac_manager_self(void)
{
	return &mac_manager;
}

static void mac_manager_init(void)
{
	mac_manager.app_pool = gnb_manager_self()->app_pool;
	oset_apr_mutex_init(&mac_manager.mutex, OSET_MUTEX_NESTED, mac_manager.app_pool);
	oset_apr_thread_cond_create(&mac_manager.cond, mac_manager.app_pool);
	oset_apr_thread_rwlock_create(&mac_manager.rwmutex, mac_manager.app_pool);

}

static void mac_manager_destory(void)
{
	oset_apr_mutex_destroy(mac_manager.mutex);
	oset_apr_thread_cond_destroy(mac_manager.cond);
	oset_apr_thread_rwlock_destroy(mac_manager.rwmutex);

	mac_manager.app_pool = NULL; /*app_pool release by openset process*/
}

int mac_init(void)
{
	mac_manager_init();
	//oset_pool_init(&bcch_bch_payload_pool, SRSENB_MAX_UES);
	//oset_pool_init(&rar_pdu_buffer_pool, SRSENB_MAX_UES*4);
    oset_pool_init(&mac_manager.ue_nr_pool, SRSENB_MAX_UES);
    oset_list_init(&mac_manager.ue_db);
    mac_manager.ue_db_ht = oset_hash_make();

    //todo
    return OSET_OK;
}

int mac_destory(void)
{
    //todo
    oset_assert(mac_manager.ue_db_ht);
    oset_hash_destroy(mac_manager.ue_db_ht);
	oset_pool_final(&mac_manager.ue_nr_pool);
	//oset_pool_final(&bcch_bch_payload_pool);
	//oset_pool_final(&rar_pdu_buffer_pool);
	mac_manager_destory();
    return OSET_OK;
}

static bool is_rnti_valid_nolock(uint16_t rnti)
{
  if (!mac_manager.started) {
    oset_error("RACH ignored as eNB is being shutdown");
    return false;
  }
  if (mac_manager.ue_db.full()) {
    oset_error("Maximum number of connected UEs %zd connected to the eNB. Ignoring PRACH", SRSENB_MAX_UES);
    return false;
  }
  if (not mac_manager.ue_db.has_space(rnti)) {
    oset_error("Failed to allocate rnti=0x%x. Attempting a different rnti.", rnti);
    return false;
  }
  return true;
}



uint16_t mac_alloc_ue(uint32_t enb_cc_idx)
{
  ue_nr*   inserted_ue = NULL;
  uint16_t rnti        = SRSRAN_INVALID_RNTI;

  do {
    // Assign new RNTI
    rnti = (uint16_t)FIRST_RNTI + (mac_manager.ue_counter++) % 60000);

    // Pre-check if rnti is valid
    {
      //srsran::rwlock_read_guard read_lock(rwmutex);
      //oset_apr_thread_rwlock_rdlock(mac_manager.rwmutex);
      if (! is_rnti_valid_nolock(rnti)) {
		  //oset_apr_thread_rwlock_unlock(mac_manager.rwmutex);
        continue;
      }
      //oset_apr_thread_rwlock_unlock(mac_manager.rwmutex);
    }

    // Allocate and initialize UE object
    inserted_ue = ue_nr_add(rnti);
    if(NULL == inserted_ue){
        oset_error("Failed to allocate rnti=0x%x. Attempting a different rnti.", rnti);
    }

  } while (inserted_ue == NULL);

  return rnti;
}


void mac_rach_detected(rach_info_t *rach_info)
{
  uint32_t enb_cc_idx = 0;
  /*to change*/
  stack_task_queue.push([this, rach_info, enb_cc_idx]() mutable {

    uint16_t rnti = mac_alloc_ue(enb_cc_idx);//alloc rnti

    // Log this event.
    {
      srsran::rwlock_write_guard lock(rwmutex);
      ++mac_manager.detected_rachs[enb_cc_idx];
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


