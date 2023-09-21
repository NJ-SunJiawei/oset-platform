/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "rlc/rlc_common.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlcBase"

void rlc_common_init(rlc_common *common, char *rb_name, uint16_t rnti, uint32_t lcid, rlc_mode_t mode, oset_apr_memory_pool_t *usepool)
{
	common->usepool = usepool;
	if(rb_name) common->rb_name = oset_strdup(rb_name);
	common->suspended = false;
	common->rnti = rnti;
	common->lcid = lcid;
	common->mode = mode;
	common->rx_pdu_resume_queue = oset_queue_create(static_blocking_queue_size);
	common->tx_sdu_resume_queue = oset_queue_create(static_blocking_queue_size);
}

void rlc_common_destory(rlc_common *common)
{
	oset_free(common->rb_name);
	oset_queue_term(common->rx_pdu_resume_queue);
	oset_queue_destroy(common->rx_pdu_resume_queue);
	oset_queue_term(common->tx_sdu_resume_queue);
	oset_queue_destroy(common->tx_sdu_resume_queue);
}

// Enqueues the Rx PDU in the resume queue
void rlc_common_queue_rx_pdu(rlc_common *common, uint8_t* payload, uint32_t nof_bytes)
{
	byte_buffer_t *rx_pdu = byte_buffer_dup_data(payload, nof_bytes);

	if (rx_pdu == NULL) {
		RlcWarning("Couldn't allocate PDU");
		return;
	}

	// Do not block ever
	if (OSET_OK != oset_queue_trypush(common->rx_pdu_resume_queue, rx_pdu)) {
		RlcWarning("Dropping SDUs while bearer suspended.");
		return;
	}
}

// Enqueues the Tx SDU in the resume queue
void rlc_common_queue_tx_sdu(rlc_common *common, byte_buffer_t *sdu)
{
	byte_buffer_t *tx_sdu = byte_buffer_dup(sdu);

	if (tx_sdu == NULL) {
		RlcWarning("Couldn't allocate SDU");
		return;
	}

	// Do not block ever
	if (OSET_OK != oset_queue_trypush(common->tx_sdu_resume_queue, tx_sdu))) {
	RlcWarning("Dropping SDUs while bearer suspended.");
	return;
	}
}


bool is_suspended(rlc_common *common)
{
	return common->suspended;
}


