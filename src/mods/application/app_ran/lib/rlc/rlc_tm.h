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

#define	static_tm_size 128

typedef struct{
	rlc_common           common;
	uint32_t             lcid;

	oset_apr_mutex_t    *bsr_callback_mutex;
	bsr_callback_t       bsr_callback;

	bool                 tx_enabled;//true

	oset_apr_mutex_t    *metrics_mutex;
	rlc_bearer_metrics_t metrics;

	// Thread-safe queues for MAC messages
	OSET_POOL(tm_pool, byte_buffer_t);// 256
	oset_queue_t    *ul_queue; // 128 // byte_buffer_queue
}rlc_tm;

rlc_tm *rlc_tm_init(uint32_t lcid_, 	uint16_t rnti_, oset_apr_memory_pool_t	*usepool);
bool rlc_tm_configure(rlc_common *tm_common, rlc_config_t *cnfg);
void rlc_tm_set_bsr_callback(rlc_common *tm_common, bsr_callback_t callback);
void rlc_tm_reset_metrics(rlc_common *tm_common);
void rlc_tm_reestablish(rlc_common *tm_common);
void rlc_tm_stop(rlc_common *tm_common);


#ifdef __cplusplus
}
#endif

#endif
