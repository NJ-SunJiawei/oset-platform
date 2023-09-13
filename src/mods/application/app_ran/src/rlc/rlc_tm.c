/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "rlc/rlc_tm.h"
#include "rrc/rrc.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlcTM"

static uint32_t tm_sdu_peek_first_bytes(oset_queue_t *tx_sdu_queue)
{
	uint32_t size_next = 0;
	byte_buffer_t *front_val = oset_queue_pop_peek(tx_sdu_queue);
		if (front_val != NULL) {
		size_next = front_val->N_bytes;
	}
	return size_next;
}

static void empty_queue(rlc_tm *tm)
{
	// Drop all messages in TX queue
	byte_buffer_t *buf = NULL;
	while(OSET_OK == oset_queue_trypop(tm->tx_sdu_queue, &buf)) {
		oset_apr_mutex_lock(tm->unread_bytes_mutex);
		tm->unread_bytes -= buf->N_bytes;
		oset_apr_mutex_unlock(tm->unread_bytes_mutex);
		oset_free(buf);
	}

	oset_assert(0 == tm->unread_bytes);
	oset_assert(0 == oset_queue_size(tm->tx_sdu_queue));
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
		tm->bsr_callback(tm_common->rnti, tm_common->lcid, *newtx_queue, *prio_tx_queue);
	}
	oset_apr_mutex_unlock(tm->bsr_callback_mutex);
}


bool rlc_tm_configure(rlc_common* tm_common, rlc_config_t* cnfg)
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

rlc_bearer_metrics_t rlc_tm_get_metrics(rlc_common *tm_common)
{
	rlc_tm *tm = (rlc_tm *)tm_common;
	return tm->metrics;
}

void rlc_tm_reestablish(rlc_common *tm_common)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	suspend(tm);
	tm->tx_enabled = true;
}

//上行方向 MAC===》RRC
void rlc_tm_write_ul_pdu(rlc_common *tm_common, uint8_t *payload, uint32_t nof_bytes)
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

		// gnb_rrc_task_handle??? push queue
		if (srb_to_lcid(srb0) == tm_common->lcid) {
			API_rrc_rlc_write_ul_pdu(tm_common->rnti, tm_common->lcid, sdu);
		} else {
			/* RLC calls PDCP to push a PDCP PDU. */
			//API_pdcp_rlc_write_ul_pdu(tm_common->rnti, tm_common->lcid, sdu);
		}
		//RLC_BUFF_FREE(&tm->tm_pool, sdu);
		oset_free(sdu);
	} else {
		RlcError("Fatal Error: Couldn't allocate buffer in rlc_tm_write_ul_pdu()");
	}
}

//下行方向 MAC get from RLC tx_sdu_queue
uint32_t rlc_tm_read_dl_pdu(rlc_common *tm_common, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	uint32_t pdu_size = tm_sdu_peek_first_bytes(tm->tx_sdu_queue);
	if (pdu_size > nof_bytes) {
		RlcInfo("Tx PDU size larger than MAC opportunity (%d > %d)", pdu_size, nof_bytes);
		return 0;
	}
	byte_buffer_t *buf = NULL;
	if (OSET_OK == oset_queue_trypop(tm->tx_sdu_queue, &buf)) {
		oset_apr_mutex_lock(tm->unread_bytes_mutex);
		tm->unread_bytes -= buf->N_bytes;
		oset_apr_mutex_unlock(tm->unread_bytes_mutex);
		pdu_size = buf->N_bytes;
		memcpy(payload, buf->msg, buf->N_bytes);
		oset_free(buf);
		RlcDebug("Complete SDU scheduled for tx. Stack latency: %" PRIu64 " us", (uint64_t)buf->md.tp);
		RlcHexInfo(payload,
		           pdu_size,
		           "Tx %s PDU, queue size=%d, bytes=%d",
		           rlc_mode_to_string((rlc_mode_t)tm, false),
		           oset_queue_size(tm->tx_sdu_queue),
		           tm->unread_bytes);

		oset_apr_mutex_lock(tm->metrics_mutex);
		tm->metrics.num_tx_pdu_bytes += pdu_size;
		tm->metrics.num_tx_pdus++;
		oset_apr_mutex_unlock(tm->metrics_mutex);
		return pdu_size;
	}

	if (tm->unread_bytes > 0) {
		RlcWarning("Corrupted queue: empty but size_bytes > 0. Resetting queue");
		tm->unread_bytes = 0;
	}
	return 0;
}

// rrc interface
//下行方向 RRC===》RLC
void rlc_tm_write_dl_sdu(rlc_common *tm_common, byte_buffer_t *sdu)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	if (!tm->tx_enabled) {
		return;
	}

	if (sdu != NULL) {
		uint8_t    *msg_ptr  = sdu->msg;
		uint32_t   nof_bytes = sdu->N_bytes;
		byte_buffer_t *dup = byte_buffer_dup(sdu);
		int ret = oset_queue_trypush(tm->tx_sdu_queue, dup);
		if (OSET_OK == ret) {
			oset_apr_mutex_lock(tm->unread_bytes_mutex);
			tm->unread_bytes += sdu->N_bytes;
			oset_apr_mutex_unlock(tm->unread_bytes_mutex);

			oset_apr_mutex_lock(tm->metrics_mutex);
			tm->metrics.num_tx_sdu_bytes += nof_bytes;
			tm->metrics.num_tx_sdus++;
			oset_apr_mutex_unlock(tm->metrics_mutex);
			RlcHexInfo(msg_ptr, nof_bytes, "Tx SDU, queue size=%d, sdu size = %d, sdu bytes=%d", tm->tx_sdu_queue.bounds, oset_queue_size(tm->dl_sdu_queue), tm->unread_bytes);
		} else {
			oset_free(dup);
			oset_apr_mutex_lock(tm->metrics_mutex);
			tm->metrics.num_lost_sdus++;
			oset_apr_mutex_unlock(tm->metrics_mutex);
			RlcWarning("[Dropped SDU] Tx SDU, queue size=%d, sdu size = %d, sdu bytes=%d",
			            tm->tx_sdu_queue.bounds,
			            oset_queue_size(tm->tx_sdu_queue),
			            tm->unread_bytes);
		}
	} else {
		RlcWarning("NULL SDU pointer in write_sdu()");
	}
}

rlc_mode_t rlc_tm_get_mode(void)
{
	return (rlc_mode_t)tm;
}

bool rlc_tm_sdu_queue_is_full(rlc_common *tm_common)
{
  rlc_tm *tm = (rlc_tm *)tm_common;

  return oset_queue_full(tm->tx_sdu_queue);
}

void rlc_tm_discard_sdu(rlc_common *tm_common, uint32_t discard_sn)
{
  rlc_tm *tm = (rlc_tm *)tm_common;

  if (!tm->tx_enabled) {
    return;
  }
  RlcWarning("SDU discard not implemented on RLC TM");
}

void rlc_tm_stop(rlc_common *tm_common)
{
	rlc_tm *tm = (rlc_tm *)tm_common;

	suspend(tm);
	oset_queue_term(tm->tx_sdu_queue);
	oset_queue_destroy(tm->tx_sdu_queue);
	//oset_pool_final(&tm->tm_pool);
	oset_apr_mutex_destroy(tm->bsr_callback_mutex);
	oset_apr_mutex_destroy(tm->metrics_mutex);
	oset_apr_mutex_destroy(tm->unread_bytes_mutex);

	rlc_common_destory(&tm->common);
}

rlc_tm *rlc_tm_init(uint32_t lcid_,	uint16_t rnti_, oset_apr_memory_pool_t *usepool)
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
						._get_metrics 	    = rlc_tm_get_metrics,
						._reset_metrics     = rlc_tm_reset_metrics,
						._reestablish       = rlc_tm_reestablish,
						._write_ul_pdu      = rlc_tm_write_ul_pdu,
						._read_dl_pdu       = rlc_tm_read_dl_pdu;
						._write_dl_sdu      = rlc_tm_write_dl_sdu,
						._get_mode		    = rlc_tm_get_mode,
						._sdu_queue_is_full	= rlc_tm_sdu_queue_is_full,
						._discard_sdu	    = rlc_tm_discard_sdu,
						._stop              = rlc_tm_stop,
					  };

	oset_apr_mutex_init(&tm->bsr_callback_mutex, OSET_MUTEX_NESTED, usepool);
	oset_apr_mutex_init(&tm->metrics_mutex, OSET_MUTEX_NESTED, usepool);
	oset_apr_mutex_init(&tm->unread_bytes_mutex, OSET_MUTEX_NESTED, usepool);

	tm->tx_sdu_queue = oset_queue_create(static_tm_size);

	//oset_pool_init(&tm->tm_pool, static_tm_size);
	//for(int i = 0; i < static_tm_size; ++i){
	//	byte_buffer_clear(&tm->tm_pool.array[i]);
	//}

	tm->tx_enabled = true;
}

