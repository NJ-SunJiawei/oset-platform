/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
//#include "lib/rrc/rrc_util.h"
#include "rrc/rrc.h"//tochange
#include "lib/mac/sched_nr_util.h"
#include "mac/mac.h"


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-mac"


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

static int mac_init(void)
{
	mac_manager_init();
	mac_manager.bcch_bch_payload = oset_malloc(sizeof(byte_buffer_t));
	mac_manager.rar_pdu_buffer = oset_malloc(sizeof(byte_buffer_t));
	sched_nr_init(&mac_manager.sched);

	mac_manager.started = true;
	mac_manager.args = &gnb_manager_self()->args.nr_stack.mac_nr;

	if (mac_manager.args->pcap.enable) {
		oset_apr_mutex_init(&mac_manager.pcap.base.mutex, OSET_MUTEX_NESTED, mac_manager.app_pool);
		mac_manager.pcap.base.queue = oset_ring_queue_create(MAX_MAC_PCAP_QUE_SIZE);
	  	mac_manager.pcap.base.buf = oset_ring_buf_create(MAX_MAC_PCAP_QUE_SIZE, sizeof(pcap_pdu_t));
		mac_pcap_open(&mac_manager.pcap, mac_manager.args->pcap.filename, 0);
	}

	oset_pool_init(&mac_manager.ue_nr_mac_pool, SRSENB_MAX_UES);
	mac_manager.ue_db = oset_hash_make();

	//todo
	return OSET_OK;
}

static int mac_destory(void)
{
	cvector_free(mac_manager.bcch_dlsch_payload);
	cvector_free(mac_manager.detected_rachs);
	mac_manager.cell_config = NULL;

	//todo
	oset_hash_destroy(mac_manager.ue_db);
	oset_pool_final(&mac_manager.ue_nr_mac_pool);
	if (mac_manager.args->pcap.enable) {
	  	mac_pcap_close(&mac_manager.pcap);
		oset_ring_buf_destroy(mac_manager.pcap.base.buf);
		oset_ring_queue_destroy(mac_manager.pcap.base.queue);
		oset_apr_mutex_destroy(mac_manager.pcap.base.mutex);
	}
	mac_manager.started = false;

    sched_nr_destory(&mac_manager.sched);
	oset_free(mac_manager.bcch_bch_payload);
	oset_free(mac_manager.rar_pdu_buffer);
	mac_manager_destory();
	return OSET_OK;
}

int mac_cell_cfg(cvector_vector_t(sched_nr_cell_cfg_t) sched_cells)
{
	mac_manager.cell_config = sched_cells;
	sched_nr_config(&mac_manager.sched, &mac_manager.args->sched_cfg, sched_cells);
	cvector_reserve(mac_manager.detected_rachs, cvector_size(mac_manager.cell_config));

    //cell 0
    sched_nr_cell_cfg_t *sched_nr_cell_cfg = &mac_manager.cell_config[0];
	oset_assert(sched_nr_cell_cfg);
	// read SIBs from RRC (SIB1 for now only)
	for (uint32_t i = 0; i < cvector_size(sched_nr_cell_cfg->sibs); i++) {
		sib_info_t sib  = {0};
		sib.index       = i;
		sib.periodicity = 160; // TODO: read period_rf from config
		if (rrc_read_pdu_bcch_dlsch(sib.index, sib.payload) != OSET_OK) {
		  oset_error("Couldn't read SIB %d from RRC", sib.index);
		}

		oset_info("Including SIB %d into SI scheduling", sib.index + 1);
		cvector_push_back(mac_manager.bcch_dlsch_payload, sib);
	}
	
	//mac_manager.rx = new mac_nr_rx; ???callback func

	mac_manager.default_ue_phy_cfg = get_common_ue_phy_cfg(sched_nr_cell_cfg);

	return OSET_OK;
}

static bool is_rnti_valid(uint16_t rnti)
{
	if (!mac_manager.started) {
		oset_error("RACH ignored as eNB is being shutdown");
		return false;
	}
	if (oset_hash_count(&mac_manager.ue_db) > SRSENB_MAX_UES) {
		oset_error("Maximum number of connected UEs %zd connected to the eNB. Ignoring PRACH", SRSENB_MAX_UES);
		return false;
	}
	if (ue_nr_find_by_rnti(rnti)) {
		oset_error("Failed to allocate rnti=0x%x. Attempting a different rnti.", rnti);
		return false;
	}
	return true;
}

static uint16_t mac_alloc_ue(uint32_t enb_cc_idx)
{
	ue_nr*   inserted_ue = NULL;
	uint16_t rnti        = SRSRAN_INVALID_RNTI;

	do {
		// Assign new RNTI
		//oset_apr_mutex_lock(mac_manager.mutex);
		rnti = (uint16_t)FIRST_RNTI + (mac_manager.ue_counter++) % 60000);
		//oset_apr_mutex_unlock(mac_manager.mutex);

		// Pre-check if rnti is valid
		{
		  //srsran::rwlock_read_guard read_lock(rwmutex);
		  if (! is_rnti_valid(rnti)) {
		    continue;
		  }
		}

		// Allocate and initialize UE object
		inserted_ue = ue_nr_add(rnti);
		if(NULL == inserted_ue){
		    oset_error("Failed to allocate rnti=0x%x. Attempting a different rnti.", rnti);
		}

	} while (inserted_ue == NULL);

	return rnti;
}

static void mac_handle_rach_info(rach_info_t *rach_info)
{
	uint16_t rnti = FIRST_RNTI;

	rnti = mac_alloc_ue(rach_info->enb_cc_idx);//alloc rnti
	{
		//srsran::rwlock_write_guard lock(rwmutex);
		//oset_apr_mutex_lock(mac_manager.mutex);
		++mac_manager.detected_rachs[rach_info->enb_cc_idx];
		//oset_apr_mutex_unlock(mac_manager.mutex);
	}
	// Trigger scheduler RACH
	rar_info_t rar_info = {0};
	rar_info.cc			 = rach_info->enb_cc_idx;
	rar_info.preamble_idx  = rach_info->preamble;
	rar_info.temp_crnti	 = rnti;
	rar_info.ta_cmd		 = rach_info->time_adv;
	slot_point_init(&rar_info.prach_slot, NUMEROLOGY_IDX, rach_info->slot_index);

	sched_nr_dl_rach_info(&mac_manager.sched, &rar_info); //int sched_nr::dl_rach_info(const rar_info_t& rar_info)//todo
	rrc->add_user(rnti, rach_info->enb_cc_idx);//todo

	oset_info("RACH:slot=%d, cc=%d, preamble=%d, offset=%d, temp_crnti=0x%x",
			  rach_info->slot_index,
			  rach_info->enb_cc_idx,
			  rach_info->preamble,
			  rach_info->time_adv,
			  rnti);
}

static void gnb_mac_task_handle(msg_def_t *msg_p, uint32_t msg_l)
{
	oset_assert(msg_p);
	oset_assert(msg_l > 0);
	switch (RQUE_MSG_ID(msg_p))
	{	
		case RACH_MAC_DETECTED_INFO:
			/*rach info handle*/
			mac_handle_rach_info(&RACH_MAC_DETECTED_INFO(msg_p));
			break;
		
		default:
			oset_error("Received unknown message: %d:%s",  RQUE_MSG_ID(msg_p), RQUE_MSG_NAME(msg_p));
			break;
	}
}

void *gnb_mac_task(oset_threadplus_t *thread, void *data)
{
	msg_def_t *received_msg = NULL;
	uint32_t length = 0;
	task_map_t *task = task_map_self(TASK_MAC);
	int rv = 0;

	mac_init();
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Starting MAC layer thread");

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
		 gnb_mac_task_handle(received_msg, length);
		 task_free_msg(RQUE_MSG_ORIGIN_ID(received_msg), received_msg);
		 received_msg = NULL;
		 length = 0;
	}
	mac_destory();
}


