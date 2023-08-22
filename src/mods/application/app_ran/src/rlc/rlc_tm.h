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

#include "rlc/rlc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	static_tm_size 128

typedef struct{
	rlc_common           common;
	oset_apr_mutex_t    *bsr_callback_mutex;
	bsr_callback_t       bsr_callback;

	bool                 tx_enabled;//true

	oset_apr_mutex_t    *metrics_mutex;
	rlc_bearer_metrics_t metrics;

	// Thread-safe queues for MAC messages
	//OSET_POOL(tm_pool, byte_buffer_t);// 256
	oset_apr_mutex_t    *unread_bytes_mutex;
	uint32_t             unread_bytes;
	oset_queue_t         *dl_pdu_queue; // 128 // byte_buffer_queue
}rlc_tm;

rlc_tm *rlc_tm_init(uint32_t lcid_, 	uint16_t rnti_, oset_apr_memory_pool_t	*usepool);
void rlc_tm_get_buffer_state(rlc_common* tm_common, uint32_t* newtx_queue, uint32_t* prio_tx_queue);
bool rlc_tm_configure(rlc_common* tm_common, rlc_config_t* cnfg);
void rlc_tm_set_bsr_callback(rlc_common* tm_common, bsr_callback_t callback);
void rlc_tm_reset_metrics(rlc_common* tm_common);
void rlc_tm_reestablish(rlc_common* tm_common);
void rlc_tm_write_ul_pdu(rlc_common* tm_common, uint8_t* payload, uint32_t nof_bytes);
uint32_t rlc_tm_read_dl_pdu(rlc_common* tm_common, uint8_t* payload, uint32_t nof_bytes);
void rlc_tm_write_dl_sdu(rlc_common* tm_common, byte_buffer_t *sdu);
rlc_mode_t rlc_tm_get_mode(void);
void rlc_tm_stop(rlc_common* tm_common);


#ifdef __cplusplus
}
#endif

#endif
