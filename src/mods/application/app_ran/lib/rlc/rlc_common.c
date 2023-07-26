/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "lib/rlc/rlc_common.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librlccom"

void rlc_common_init(rlc_common *common, char *rb_name, uint16_t rnti, rlc_mode_t mode, oset_apr_memory_pool_t *usepool)
{
	common->usepool = usepool;
	common->rb_name = oset_core_strdup(usepool, rb_name);
	common->suspended = false;
	common->rnti = rnti;
	common->mode = mode;
	common->rx_pdu_resume_queue = oset_queue_create(static_blocking_queue_size);
	common->tx_sdu_resume_queue = oset_queue_create(static_blocking_queue_size);
	//common->func = oset_core_alloc(usepool, sizeof(rlc_func_entity));
	oset_pool_init(&common->resume_pool, static_blocking_queue_size);
	for(int i = 0; i < static_blocking_queue_size; ++i){
		byte_buffer_clear(&common->resume_pool.array[i]);
	}

}

void rlc_common_destory(rlc_common *common)
{
	oset_pool_final(&common->resume_pool);

	oset_queue_term(common->rx_pdu_resume_queue);
	oset_queue_destroy(common->rx_pdu_resume_queue);
	oset_queue_term(common->tx_sdu_resume_queue);
	oset_queue_destroy(common->tx_sdu_resume_queue);
}

// Enqueues the Rx PDU in the resume queue
void rlc_common_queue_rx_pdu(rlc_common *common, uint8_t* payload, uint32_t nof_bytes)
{
	//byte_buffer_t *rx_pdu = byte_buffer_init();

	byte_buffer_t *rx_pdu = NULL;
	RLC_BUFF_ALLOC(&common->resume_pool, rx_pdu);

	if (rx_pdu == NULL) {
		RlcWarning("Couldn't allocate PDU");
		return;
	}

	if (byte_buffer_get_headroom(rx_pdu) < nof_bytes) {
		RlcWarning("Not enough space to store PDU.");
		return;
	}

	memcpy(rx_pdu->msg, payload, nof_bytes);
	rx_pdu->N_bytes = nof_bytes;

	// Do not block ever
	if (OSET_OK != oset_queue_trypush(common->rx_pdu_resume_queue, rx_pdu)) {
		RlcWarning("Dropping SDUs while bearer suspended.");
		return;
	}
}



