/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/

#ifndef RLC_TM_H_
#define RLC_TM_H_

#include "lib/rlc/rlc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	rlc_common          common;
	OSET_POOL(pool, byte_buffer_t);// 256
	uint32_t             lcid;

	oset_apr_mutex_t    *bsr_callback_mutex;
	bsr_callback_t       bsr_callback;

	bool                 tx_enabled;//true

	oset_apr_mutex_t    *metrics_mutex;
	rlc_bearer_metrics_t metrics;

	// Thread-safe queues for MAC messages
	oset_ring_queue_t    *ul_queue; // 128 // byte_buffer_queue
	oset_ring_buf_t      *ul_buffer; // 128 // byte_buffer_queue
}rlc_tm;

void rlc_tm_init(rlc_tm *tm, uint32_t lcid_, oset_apr_memory_pool_t	*usepool);
void rlc_tm_reset_metrics(rlc_tm *tm);

#ifdef __cplusplus
}
#endif

#endif
