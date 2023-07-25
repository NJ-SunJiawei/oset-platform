/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "lib/rlc/rlc_tm.h"
		
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librlcTM"

static void rlc_tm_empty_queue(rlc_tm *tm)
{
	// Drop all messages in TX queue
	byte_buffer_t *buf = NULL;
	while(OSET_OK == oset_queue_trypop(tm->ul_queue, &buf)) {
		oset_pool_free(&tm->pool, &buf);
	}
	//oset_queue_size(tm->ul_queue) = 0;
}

static void rlc_tm_suspend(rlc_tm *tm)
{
	tm->tx_enabled = false;
	rlc_tm_empty_queue(tm);
}

void rlc_tm_reset_metrics(rlc_tm *tm)
{
  oset_apr_mutex_lock(tm->metrics_mutex);
  tm->metrics = {0};
  oset_apr_mutex_unlock(tm->metrics_mutex);
}

void rlc_tm_reestablish(rlc_tm *tm)
{
	rlc_tm_suspend(tm);
	tm->tx_enabled = true;
}

void rlc_tm_set_bsr_callback(rlc_tm *tm, bsr_callback_t callback)
{
	tm->bsr_callback = callback;
}

void rlc_tm_stop(rlc_tm *tm)
{
	rlc_tm_suspend(tm);
	oset_queue_term(tm->ul_queue);
	oset_queue_destroy(tm->ul_queue);
	oset_pool_final(&tm->pool);
	oset_apr_mutex_destroy(tm->bsr_callback_mutex);
	oset_apr_mutex_destroy(tm->metrics_mutex);

	rlc_common_destory(&tm->common);
}

bool rlc_tm_configure(rlc_tm *tm, rlc_config_t *cnfg)
{
	RlcError("Attempted to configure TM RLC entity");
	return true;
}

rlc_tm * rlc_tm_init(uint32_t lcid_, oset_apr_memory_pool_t	*usepool)
{
	oset_assert(usepool);
	rlc_tm *tm = oset_core_alloc(usepool, sizeof(*tm));
	ASSERT_IF_NOT(tm, "lcid %u Could not allocate rlc tm context from pool", lcid_);
	memset(tm, 0, sizeof(rlc_tm));

	rlc_common_init(&tm->common, "SRB0", (rlc_mode_t)tm, usepool);
	
	tm->common.func->_configure = rlc_tm_configure;
	tm->common.func->_set_bsr_callback = rlc_tm_set_bsr_callback;

	tm->lcid = lcid_;
	oset_apr_mutex_init(&tm->bsr_callback_mutex, OSET_MUTEX_NESTED, usepool);
	oset_apr_mutex_init(&tm->metrics_mutex, OSET_MUTEX_NESTED, usepool);

	tm->ul_queue = oset_queue_create(static_tm_size);

	oset_pool_init(&tm->pool, static_tm_size);
	for(int i = 0; i < static_tm_size; ++i){
		byte_buffer_clear(&tm->pool.array[i]);
	}

	tm->tx_enabled = true;
}

