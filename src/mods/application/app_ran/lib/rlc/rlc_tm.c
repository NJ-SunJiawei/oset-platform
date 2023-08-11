/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "lib/rlc/rlc_tm.h"
#include "rrc/rrc.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librlcTM"

static void empty_queue(rlc_tm *tm)
{
	// Drop all messages in TX queue
	byte_buffer_t *buf = NULL;
	while(OSET_OK == oset_queue_trypop(tm->dl_sdu_queue, &buf)) {
		oset_free(buf);
		tm->unread_bytes -= buf->N_bytes;
	}
	oset_assert(0 == tm->unread_bytes);
	oset_assert(0 == oset_queue_size(tm->dl_sdu_queue));
}

static void suspend(rlc_tm *tm)
{
	tm->tx_enabled = false;
	empty_queue(tm);
}

//////////////////////////////////////////////////////////////////////////////////
void rlc_tm_get_buffer_state(rlc_common *tm_common, uint32_t *newtx_queue, uint32_t *prio_tx_queue)
{
	rlc_tm *tm = (rlc_tm *)tm_common;
	oset_apr_mutex_lock(tm->bsr_callback_mutex);
	*newtx_queue   = tm->unread_bytes;
	*prio_tx_queue = 0;
	if (tm->bsr_callback) {
		tm->bsr_callback(tm_common->rnti, tm->lcid, *newtx_queue, *prio_tx_queue);
	}
	oset_apr_mutex_unlock(tm->bsr_callback_mutex);
}


bool rlc_tm_configure(rlc_common *tm_common, rlc_config_t *cnfg)
{
	RlcError("Attempted to configure TM RLC entity");
	return true;
}

void rlc_tm_set_bsr_callback(rlc_common *tm_common, bsr_callback_t callback)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	tm->bsr_callback = callback;
}

void rlc_tm_reset_metrics(rlc_common *tm_common)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	oset_apr_mutex_lock(tm->metrics_mutex);
	tm->metrics = {0};
	oset_apr_mutex_unlock(tm->metrics_mutex);
}

void rlc_tm_reestablish(rlc_common *tm_common)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	suspend(tm);
	tm->tx_enabled = true;
}

//上行方向 MAC===》RRC
void rlc_tm_write_ul_pdu(rlc_common *tm_common, uint8_t* payload, uint32_t nof_bytes)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	byte_buffer_t *sdu = byte_buffer_init();
	//RLC_BUFF_ALLOC(&tm->tm_pool, sdu);
	if (sdu != NULL) {
		memcpy(sdu->msg, payload, nof_bytes);
		sdu->N_bytes = nof_bytes;
		byte_buffer_set_timestamp(sdu);
		{
			oset_apr_mutex_lock(tm->metrics_mutex);
			tm->metrics.num_rx_pdu_bytes += nof_bytes;
			tm->metrics.num_rx_pdus++;
			oset_apr_mutex_unlock(tm->metrics_mutex);
		}
		if (srb_to_lcid(srb0) == tm->lcid) {
			API_rrc_rlc_write_ul_pdu(tm_common->rnti, tm->lcid, sdu);
		} else {
			/* RLC calls PDCP to push a PDCP PDU. */
			API_pdcp_rlc_write_ul_pdu(tm_common->rnti, tm->lcid, sdu);
		}
		//RLC_BUFF_FREE(&tm->tm_pool, sdu);
		oset_free(sdu);
	} else {
		RlcError("Fatal Error: Couldn't allocate buffer in rlc_tm::write_pdu()");
	}
}


// rrc interface
void rlc_tm_write_dl_sdu(rlc_common *tm_common, byte_buffer_t *sdu)
{
	//rrc===>rlc
	rlc_tm *tm = (rlc_tm *)tm_common;

	if (!tm->tx_enabled) {
		return;
	}

	if (sdu != NULL) {
		uint8_t*   msg_ptr   = sdu->msg;
		uint32_t   nof_bytes = sdu->N_bytes;
		int        ret = oset_queue_trypush(tm->dl_sdu_queue, byte_buffer_dup(sdu));
		if (OSET_OK == ret) {
			oset_apr_mutex_lock(tm->unread_bytes_mutex);
			tm->unread_bytes += sdu->N_bytes;
			oset_apr_mutex_unlock(tm->unread_bytes_mutex);
			RlcHexInfo(msg_ptr, nof_bytes, "Tx SDU, queue size=%d, sdu size = %d, sdu bytes=%d", tm->dl_sdu_queue.bounds, oset_queue_size(tm->dl_sdu_queue), tm->unread_bytes);
		} else {
			RlcWarning("[Dropped SDU] Tx SDU, queue size=%d, sdu size = %d, sdu bytes=%d",
			            tm->dl_sdu_queue.bounds,
			            oset_queue_size(tm->dl_sdu_queue),
			            tm->unread_bytes);
		}
	} else {
		RlcWarning("NULL SDU pointer in write_sdu()");
	}
}



void rlc_tm_stop(rlc_common *tm_common)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	suspend(tm);
	oset_queue_term(tm->dl_sdu_queue);
	oset_queue_destroy(tm->dl_sdu_queue);
	//oset_pool_final(&tm->tm_pool);
	oset_apr_mutex_destroy(tm->bsr_callback_mutex);
	oset_apr_mutex_destroy(tm->metrics_mutex);
	oset_apr_mutex_destroy(tm->unread_bytes_mutex);

	rlc_common_destory(&tm->common);
}

rlc_tm *rlc_tm_init(uint32_t lcid_,	uint16_t rnti_, oset_apr_memory_pool_t	*usepool)
{
	oset_assert(usepool);
	rlc_tm *tm = oset_core_alloc(usepool, sizeof(rlc_tm));
	ASSERT_IF_NOT(tm, "lcid %u Could not allocate rlc tm context from pool", lcid_);
	memset(tm, 0, sizeof(rlc_tm));

	rlc_common_init(&tm->common, "SRB0", rnti_, lcid_, (rlc_mode_t)tm, usepool);

	tm->common.func = {
						._get_buffer_state  = rlc_tm_get_buffer_state,
						._configure         = rlc_tm_configure,
						._set_bsr_callback  = rlc_tm_set_bsr_callback,
						._reset_metrics     = rlc_tm_reset_metrics,
						._reestablish       = rlc_tm_reestablish,
						._write_ul_pdu      = rlc_tm_write_ul_pdu,
						._write_dl_sdu      = rlc_tm_write_dl_sdu,
						._stop              = rlc_tm_stop,
					  };

	oset_apr_mutex_init(&tm->bsr_callback_mutex, OSET_MUTEX_NESTED, usepool);
	oset_apr_mutex_init(&tm->metrics_mutex, OSET_MUTEX_NESTED, usepool);
	oset_apr_mutex_init(&tm->unread_bytes_mutex, OSET_MUTEX_NESTED, usepool);

	tm->dl_sdu_queue = oset_queue_create(static_tm_size);

	//oset_pool_init(&tm->tm_pool, static_tm_size);
	//for(int i = 0; i < static_tm_size; ++i){
	//	byte_buffer_clear(&tm->tm_pool.array[i]);
	//}

	tm->tx_enabled = true;
}

