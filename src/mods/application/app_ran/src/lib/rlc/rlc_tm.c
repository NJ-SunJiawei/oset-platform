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


rlc_tm * rlc_tm_init(uint32_t lcid_, oset_apr_memory_pool_t	*usepool)
{
	oset_assert(usepool);
	rlc_tm *tm = oset_core_alloc(usepool, sizeof(*tm));
	ASSERT_IF_NOT(tm, "lcid %u Could not allocate rlc tm context from pool", lcid_);
	memset(tm, 0, sizeof(rlc_tm));

	tm->common.rb_name = "SRB0";
	tm->common.suspended = false;
	tm->common.usepool = usepool;
	tm->common.mode = (rlc_mode_t)tm;
	tm->common.rx_pdu_resume_queue = oset_ring_queue_create(static_blocking_queue_size);
	tm->common.tx_sdu_resume_queue = oset_ring_queue_create(static_blocking_queue_size);

	oset_pool_init(&tm->pool, 4096);
	tm->lcid = lcid_;
	oset_apr_mutex_init(&tm->bsr_callback_mutex, OSET_MUTEX_NESTED, usepool);
	oset_apr_mutex_init(&tm->metrics_mutex, OSET_MUTEX_NESTED, usepool);
	tm->tx_enabled = true;
}


void rlc_tm_reset_metrics(rlc_tm *tm)
{
  oset_apr_mutex_lock(tm->metrics_mutex);
  tm->metrics = {0};
  oset_apr_mutex_unlock(tm->metrics_mutex);
}

