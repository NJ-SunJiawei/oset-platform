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

// Transmitter sub-class base
typedef struct {
	char                  *rb_name;
	bsr_callback_t        bsr_callback;
	rlc_config_t          cfg;
	// TX SDU buffers
	byte_buffer_queue     tx_sdu_queue;//list
	byte_buffer_t         *tx_sdu;
	// Mutexes
	oset_thread_mutex_t   mutex;
	// Metrics
	rolling_average_t(double) mean_pdu_latency_us;
}rlc_um_base_tx;

typedef struct {
	gnb_timer_t			*timers;
	char                  *rb_name;
	rlc_config_t          cfg;
	byte_buffer_t         *rx_sdu;
	uint32_t              lcid;
}rlc_um_base_rx;


typedef struct rlc_um_base_s{
	rlc_common            common;
	gnb_timer_t           *timers;
	rlc_config_t          cfg;
	bool                  tx_enabled;
	bool                  rx_enabled;
	oset_thread_mutex_t   metrics_mutex;
	rlc_bearer_metrics_t  metrics;
}rlc_um_base;

void rlc_um_base_init(rlc_um_base *base, uint32_t lcid_, uint16_t rnti_);
void rlc_um_base_stop(rlc_um_base *base);

#ifdef __cplusplus
}
#endif

#endif
