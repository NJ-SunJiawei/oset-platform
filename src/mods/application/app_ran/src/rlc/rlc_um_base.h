/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/

#ifndef RLC_UM_BASE_H_
#define RLC_UM_BASE_H_

#include "rlc/rlc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	rlc_common            common;
	gnb_timer_t           *timers;
	uint32_t              lcid;
	rlc_config_t          cfg;
	byte_buffer_pool      *pool;
	// Rx and Tx objects
	rlc_um_base_tx        *tx;
	rlc_um_base_rx        *rx;

	bool                  tx_enabled;
	bool                  rx_enabled;
	oset_thread_mutex_t   metrics_mutex;
	rlc_bearer_metrics_t  metrics;
}rlc_um_base;


#ifdef __cplusplus
}
#endif

#endif
