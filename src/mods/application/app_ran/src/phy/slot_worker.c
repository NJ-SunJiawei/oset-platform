/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "phy/phy.h"
#include "phy/slot_worker.h"
#include "mac/mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-slot-worker"

static OSET_POOL(slot_worker_pool, slot_worker_t);

//Process one at a time, in sequence
static slot_manager_t slot_manager = {0};

slot_manager_t *slot_manager_self(void)
{
	return &slot_manager;
}


void slot_worker_init(void)
{
    oset_pool_init(&slot_worker_pool, FDD_HARQ_DELAY_UL_MS);
}

void slot_worker_final(void)
{
    oset_pool_final(&slot_worker_pool);
}

slot_worker_t* slot_worker_alloc(slot_manager_t *slot_manager)
{
    slot_worker_t *slot_wk = NULL;
    oset_pool_alloc(&slot_worker_pool, &slot_wk);
	memcpy(slot_wk, &slot_manager->slot_worker, sizeof(*slot_wk));
	return slot_wk;
}

void slot_worker_free(slot_worker_t *slot_wk)
{
    oset_pool_free(&slot_worker_pool, slot_wk);
}

void slot_worker_process(oset_threadplus_t *thread, void *data)
{
	slot_worker_t *slot_wk = (slot_worker_t *)data;
	if(NULL == slot_wk) return;

	// Inform Scheduler about new slot
	mac_slot_indication(&slot_wk->dl_slot_cfg);//do nothing

	// Get Transmission buffer
	uint32_t  nof_ant  = (uint32_t)slot_wk->tx_buffer.size();

	rf_buffer_t tx_rf_buffer = {0};
	set_nof_samples(&tx_rf_buffer, slot_wk->sf_len);//1slot(15khz)

	//bind tx_buffer and tx_rf_buffer
	for (uint32_t a = 0; a < nof_ant; a++) {
	tx_rf_buffer.set(rf_port, a, nof_ant, tx_buffer[a]);
	}

	//(优先处理上行消息)，需要通过slot task同步确保下行消息顺序

	// Process uplink
	if (! work_ul()) {//phy up
	// Wait and release synchronization
	sync.wait(this);
	sync.release();
	common.worker_end(context, false, tx_rf_buffer);
	return;
	}

	// Process downlink
	if (! work_dl()) {//phy down
	common.worker_end(context, false, tx_rf_buffer);
	return;
	}

	common.worker_end(context, true, tx_rf_buffer);

#ifdef DEBUG_WRITE_FILE
	if (num_slots++ < slots_to_dump) {
	printf("Writing slot %d\n", dl_slot_cfg.idx);
	fwrite(tx_rf_buffer.get(0), tx_rf_buffer.get_nof_samples() * sizeof(cf_t), 1, f);
	} else if (num_slots == slots_to_dump) {
	printf("Baseband signaled dump finished. Please close app.\n");
	fclose(f);
	}
#endif

	slot_worker_free(slot_wk);

	oset_apr_mutex_lock(phy_manager_self()->mutex);
	oset_apr_thread_cond_signal(phy_manager_self()->cond);
	oset_apr_mutex_unlock(phy_manager_self()->mutex);
}

