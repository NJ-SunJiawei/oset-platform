/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.09
************************************************************************/
#include "lib/common/common_nr.h"
#include "rlc/rlc_um.h"
#include "pdcp/pdcp.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlcAM"

/////////////////////////////status/////////////////////////////////////////////
void rlc_am_nr_status_pdu_reset(rlc_am_nr_status_pdu_t *status)
{
	status->cpt   = (rlc_am_nr_control_pdu_type_t)status_pdu;
	status->ack_sn = INVALID_RLC_SN;
	cvector_clear(status->nacks)
	status->packed_size = rlc_am_nr_status_pdu_sizeof_header_ack_sn;
}

#define AM_RXTX
uint32_t rlc_am_base_rx_get_status_pdu(rlc_am_nr_status_pdu_t *status, uint32_t max_len);

/////////////////////////////////////////////////////////////////////////////
//  AMD PDU都有SN，这是因为AM RLC需要基于SN确保每个RLC PDU都成功接收
// SN: 编号
// D/C: AM报文包含两大类，一个是数据报文(1)，一个是控制报文(0)
// SI:  full_sdu 0b00, first_segment 0b01, last_segment 0b10, neither_first_nor_last_segment 0b11
// P: P字段是为重传而准备的, 回复状态状告(1), 不需要回复状态报告(0)
// SO: 用于指示RLC SDU segment在原始RLC SDU中的位置，以byte为单位

// no so(sn 12 bit)                     no so(sn 18 bit)
// |D/C|P|SI | SN   |                   |D/C|P|SI|R|R| SN |   
// |      SN        |                   |      SN         |
// |      Data      |                   |      SN         |
// |      ...       |                   |      Data       |


// with so(sn 12 bit)                   with so(sn 18 bit)
// |D/C|P|SI | SN   |                   |D/C|P|SI|R|R| SN |
// |      SN        |                   |      SN         |
// |      SO        |                   |      SN         |
// |      SO        |                   |      SO         |
// |      Data      |                   |      SO         |
// |      ...       |                   |      Data       |

///////////////////////////////////////////////////////////////////////////
// D/C: AM报文包含两大类，一个是数据报文(1)，一个是控制报文(0)
// CPT: 字段是为了标识不同种类的控制报文，当前为000
// ACK_SN: 此序列号及之后的报文没有被接收到，也没有明确告知没有接收到
// E1: 表示后面是否跟随一个NACK_SN/E1/E2/E3字段。 跟随(1)
// NACK_SN: 标示丢失没有收到的报文
// E2: 标识是否跟随有SoStart和SoEnd。主要用于报文的不完整接收场景的处理
// E3: 字段表示后面是否跟着关于一连串RLC SDU未被接收的消息。是否有range字段
// NACK range: 字段表示从NACK_SN开始(包括NACK_SN)，有几个连续的RLC SDU丢失
// SOstart, Soend. 表示被RLC接收端发现丢失的SN=NACK_SN的SDU的某个部分。SOstart的值表示该丢失的SDU部分在原始SDU中的哪一个byte处开始.Soend表示哪个byte结束

// acK (sn 12 bit)                      acK (sn 18 bit)
// |D/C|CPT | ACK_SN  |                 |D/C|CPT |    ACK_SN   |
// |     ACK_SN       |                 |         ACK_SN       |
// | E1 |R|R|R|R|R|R|R|                 |  ACK_SN         |E1|R|
// |     NACK_SN      |                 |         NACK_SN      |
// |NACK_SN|E1|E2|E3|R|                 |         NACK_SN      |
// |     NACK_SN      |                 |NACK_SN|E1|E2|E3|R|R|R|
// |NACK_SN|E1|E2|E3|R|                 |         NACK_SN      |
// |     SOstart      |                 |         NACK_SN      |
// |     SOstart      |                 |NACK_SN|E1|E2|E3|R|R|R|
// |     SOend        |                 |         SOstart      |
// |     SOend        |                 |         SOstart      |
// |     NACK_range   |                 |         SOend        |
// |     NACK_SN      |                 |         SOend        |
// |NACK_SN|E1|E2|E3|R|                 |         NACK_range   |
// |      ...         |                 |         NACK_SN      |
//                                      |         NACK_SN      |
//                                      |NACK_SN|E1|E2|E3|R|R|R|
//                                      |          ...         |
#if AM_RXTX
static void empty_queue_no_lock(rlc_am_base_tx *base_tx)
{
	// Drop all messages in TX queue
	rlc_um_base_tx_sdu_t *next_node, *node = NULL;
	oset_list_for_each_safe(&base_tx->tx_sdu_queue, next_node, node){
			byte_buffer_t *sdu = node->buffer;
			oset_list_remove(&base_tx->tx_sdu_queue, node);
			oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
			base_tx->unread_bytes -= sdu->N_bytes;
			oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
			oset_free(sdu)
	}

	oset_assert(0 == base_tx->unread_bytes);
	oset_assert(0 == oset_list_count(base_tx->tx_sdu_queue));
}

static void empty_queue(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	empty_queue_no_lock(base_tx);
	oset_thread_mutex_unlock(&base_tx->mutex);
}

// t-StatusProhibit ARQ-状态报告定时器(防止频繁上报):
//    状态报告触发后，如果t-StatusProhibit 定时器没有运行，则在新传时（MAC 指示的），将构建的STATUS PDU递交底层，同时开启定时器；
//    状态报告触发后，如果t-StatusProhibit 定时器在运行，状态报告是不能发送的。需要等到定时器超时后，在第一个传输机会到来时，将构建的STATUS PDU递交给底层，同时开启定时器；
//  构建STATUS PDU：
//    需要注意的是，t-Reassembly定时器超时会触发状态变量（RX_Highest_Status）更新以及状态报告。因此，应该以更新状态变量后的包接收状态来构建STATUS PDU(可理解为状态报告的触发点在状态变量更新之后)

bool do_status(rlc_am_base_rx *base_rx)
{
  return base_rx->do_status && !base_rx->rx->status_prohibit_timer.running;
}

static void rlc_am_base_tx_discard_sdu(rlc_am_base_tx *base_tx, uint32_t discard_sn)
{
	bool discarded = false;

	oset_thread_mutex_lock(&base_tx->mutex);
	rlc_am_base_tx_sdu_t *next_node, *node = NULL;
	oset_list_for_each_safe(&base_tx->tx_sdu_queue, next_node, node){
	  if (node != NULL && node->buffer->md.pdcp_sn == discard_sn) {
		  byte_buffer_t *sdu = node->buffer;
		  oset_list_remove(&base_tx->tx_sdu_queue, node);
		  oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
		  base_tx->unread_bytes -= sdu->N_bytes;
		  oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
		  oset_free(sdu);
		  node = NULL;
		  discarded = true;
	  }
	}   

	// Discard fails when the PDCP PDU is already in Tx window.
	RlcInfo("%s PDU with PDCP_SN=%d", discarded ? "Discarding" : "Couldn't discard", discard_sn);
}


static uint32_t rlc_am_base_tx_build_status_pdu(rlc_am_base_tx *base_tx, byte_buffer_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr_tx *tx = base_tx->tx;
	rlc_am_nr_rx *rx = base_tx->rx;
	rlc_am_base *base = base_tx->base;

	RlcInfo("generating status PDU. Bytes available:%d", nof_bytes);

	rlc_am_nr_status_pdu_t status = {.sn_size = base_tx->cfg.rx_sn_field_length,};// carries status of RX entity, hence use SN length of RX
	int pdu_len = rlc_am_base_rx_get_status_pdu(&rx->base_rx, &status, nof_bytes);
	if (pdu_len == SRSRAN_ERROR) {
		RlcDebug("deferred status PDU. Cause: Failed to acquire rx lock");
		pdu_len = 0;
	} else if (pdu_len > 0 && nof_bytes >= static_cast<uint32_t>(pdu_len)) {
		RlcDebug("generated status PDU. Bytes:%d", pdu_len);
		log_rlc_am_nr_status_pdu_to_string(logger.info, "TX status PDU - %s", &status, rb_name);
		pdu_len = rlc_am_nr_write_status_pdu(status, cfg.tx_sn_field_length, payload);
	} else {
		RlcInfo("cannot tx status PDU - %d bytes available, %d bytes required", nof_bytes, pdu_len);
		pdu_len = 0;
	}

	return payload->N_bytes;
}

/**
 * Builds the RLC PDU.
 *
 * Called by the MAC, trough one of the PHY worker threads.
 *
 * \param [payload] is a pointer to the buffer that will hold the PDU.
 * \param [nof_bytes] is the number of bytes the RLC is allowed to fill.
 *
 * \returns the number of bytes written to the payload buffer.
 * \remark: This will be called multiple times from the MAC,
 * while there is something to TX and enough space in the TB.
 */
static uint32_t rlc_am_base_tx_read_dl_pdu(rlc_am_base_tx *base_tx, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr_tx *tx = base_tx->tx;
	rlc_am_nr_rx *rx = base_tx->rx;
	rlc_am_base *base = base_tx->base;

	oset_thread_mutex_lock(&base_tx->mutex);

	if (!base->tx_enabled) {
		RlcDebug("RLC entity not active. Not generating PDU.");
		return 0;
	}
	RlcDebug("MAC opportunity - bytes=%d, tx_window size=%zu PDUs", nof_bytes, oset_hash_count(tx->tx_window));

	// Tx STATUS if requested
	// 状态报告
	if (do_status(&rx->base_rx)) {
		byte_buffer_t *tx_pdu = byte_buffer_init();
		if (tx_pdu == NULL) {
			RlcError("Couldn't allocate PDU");
			return 0;
		}
		rlc_am_base_tx_build_status_pdu(base_tx, tx_pdu, nof_bytes);
		memcpy(payload, tx_pdu->msg, tx_pdu->N_bytes);
		RlcDebug("Status PDU built - %d bytes", tx_pdu->N_bytes);
		oset_free(tx_pdu);
		return tx_pdu->N_bytes;
	}

	// Retransmit if required
	if (! retx_queue.empty()) {//重传
		RlcInfo("Re-transmission required. Retransmission queue size: %d", retx_queue.size());
		return build_retx_pdu(payload, nof_bytes);
	}

	// Send remaining segment, if it exists//发送剩余段（如果存在）
	if (sdu_under_segmentation_sn != INVALID_RLC_SN) {
		if (not tx_window->has_sn(sdu_under_segmentation_sn)) {
		  sdu_under_segmentation_sn = INVALID_RLC_SN;
		  RlcError("SDU currently being segmented does not exist in tx_window. Aborting segmentation SN=%d",
				   sdu_under_segmentation_sn);
		  return 0;
		}
		return build_continuation_sdu_segment((*tx_window)[sdu_under_segmentation_sn], payload, nof_bytes);
	}

	// Check whether there is something to TX
	if (tx_sdu_queue.is_empty()) {
		RlcInfo("No data available to be sent");
		return 0;
	}

	//发送新数据
	return build_new_pdu(payload, nof_bytes);
}


static int rlc_am_base_tx_write_dl_sdu(rlc_am_base_tx *base_tx, byte_buffer_t *sdu)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	if (sdu) {
		// Get SDU info
		uint32_t sdu_pdcp_sn = sdu->md.pdcp_sn;

		// Store SDU
		uint8_t*   msg_ptr	 = sdu->msg;
		uint32_t   nof_bytes = sdu->N_bytes;
		int count = oset_list_count(&base_tx->tx_sdu_queue);

		if((uint32_t)count <= base_tx->base->cfg.tx_queue_length) {
			rlc_am_base_tx_sdu_t *sdu_node = oset_malloc(rlc_um_base_tx_sdu_t);
			oset_assert(sdu_node);
			sdu_node->buffer = byte_buffer_dup(sdu);
			oset_list_add(&base_tx->tx_sdu_queue, sdu_node);
			oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
			base_tx->unread_bytes += sdu->N_bytes;
			oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);

			RlcHexInfo(msg_ptr,
			           nof_bytes,
			           "Tx SDU (%d B, PDCP_SN=%ld tx_sdu_queue_len=%d)",
			           nof_bytes,
			           sdu_pdcp_sn,
			           oset_list_count(&base_tx->tx_sdu_queue));
			oset_thread_mutex_unlock(&base_tx->mutex);
			return OSET_OK;
		} else {
			// in case of fail, the try_write returns back the sdu
			RlcHexWarning(msg_ptr,
			              nof_bytes,
			              "[Dropped SDU] Tx SDU (%d B, PDCP_SN=%ld, tx_sdu_queue_len=%d)",
			              nof_bytes,
			              sdu_pdcp_sn,
			              count);
		}else{
			RlcWarning("NULL SDU pointer in write_sdu()");
		}
	}
	oset_thread_mutex_unlock(&base_tx->mutex);
	return OSET_ERROR;
}


static void rlc_am_base_tx_get_buffer_state(rlc_am_base_tx *base_tx, uint32_t *n_bytes_new, uint32_t *n_bytes_prio)
{
  rlc_am_nr_tx *tx = base_tx->tx;
  rlc_am_nr_rx *rx = base_tx->rx;
  rlc_am_base *base = base_tx->base;

  oset_thread_mutex_lock(&base_tx->mutex);
  RlcDebug("buffer state - do_status=%s", do_status() ? "yes" : "no");

  if (!base->tx_enabled) {
    RlcError("get_buffer_state() failed: TX is not enabled.");
    goto end;
  }

  // Bytes needed for status report
  if (do_status(&rx->base_rx)) {
    n_bytes_prio += rx->get_status_pdu_length();
    RlcDebug("buffer state - total status report: %d bytes", n_bytes_prio);
  }

  // Bytes needed for retx
  for (const rlc_amd_retx_nr_t& retx : retx_queue.get_inner_queue()) {
    RlcDebug("buffer state - retx - SN=%d, Segment: %s, %d:%d",
             retx.sn,
             retx.is_segment ? "true" : "false",
             retx.so_start,
             retx.so_start + retx.segment_length - 1);
    if (tx_window->has_sn(retx.sn)) {
      int req_bytes     = retx.segment_length;
      int hdr_req_bytes = (retx.is_segment && retx.current_so != 0) ? max_hdr_size : min_hdr_size;
      if (req_bytes <= 0) {
        RlcError("buffer state - retx - invalid length=%d for SN=%d", req_bytes, retx.sn);
      } else {
        n_bytes_prio += (req_bytes + hdr_req_bytes);
        RlcDebug("buffer state - retx: %d bytes", n_bytes_prio);
      }
    } else {
      RlcWarning("buffer state - retx for SN=%d is outside the tx_window", retx.sn);
    }
  }

  // Bytes needed for tx of the rest of the SDU that is currently under segmentation (if any)
  if (sdu_under_segmentation_sn != INVALID_RLC_SN) {
    if (tx_window->has_sn(sdu_under_segmentation_sn)) {
      rlc_amd_tx_pdu_nr& seg_pdu = (*tx_window)[sdu_under_segmentation_sn];
      if (not seg_pdu.segment_list.empty()) {
        // obtain amount of already transmitted Bytes
        const rlc_amd_tx_pdu_nr::pdu_segment& seg       = seg_pdu.segment_list.back();
        uint32_t                              last_byte = seg.so + seg.payload_len;
        if (last_byte <= seg_pdu.sdu_buf->N_bytes) {
          // compute remaining bytes pending for transmission
          uint32_t remaining_bytes = seg_pdu.sdu_buf->N_bytes - last_byte;
          n_bytes_new += remaining_bytes + max_hdr_size;
        } else {
          RlcError(
              "buffer state - last segment of SDU under segmentation exceeds SDU len. SDU len=%d B, last_byte=%d B",
              seg_pdu.sdu_buf->N_bytes,
              last_byte);
        }
      } else {
        RlcError("buffer state - SDU under segmentation has empty segment list. Ignoring SN=%d",
                 sdu_under_segmentation_sn);
      }
    } else {
      sdu_under_segmentation_sn = INVALID_RLC_SN;
      RlcError("buffer state - SDU under segmentation does not exist in tx_window. Aborting segmentation SN=%d",
               sdu_under_segmentation_sn);
    }
  }

  // Bytes needed for tx SDUs in queue
  uint32_t n_sdus = tx_sdu_queue.get_n_sdus();
  n_bytes_new += tx_sdu_queue.size_bytes();

  // Room needed for fixed header of data PDUs
  n_bytes_new += min_hdr_size * n_sdus;
  RlcDebug("total buffer state - %d SDUs (%d B)", n_sdus, n_bytes_new + n_bytes_prio);

  if (base_tx->bsr_callback) {
    RlcDebug("calling BSR callback - %d new_tx, %d priority bytes", n_bytes_new, n_bytes_prio);
    base_tx->bsr_callback(base->common.rnti, base->common.lcid, n_bytes_new, n_bytes_prio);
  }

end:
  oset_thread_mutex_lock(&base_tx->mutex);
  return;
}

static void rlc_am_base_tx_timer_expired(void *data)
{
  rlc_am_base_tx *base_tx = (rlc_am_base_tx *)data;
  rlc_am_nr_tx *tx = base_tx->tx;
  rlc_am_base *base = base_tx->base;

  oset_thread_mutex_lock(&base_tx->mutex);

  // t-PollRetransmit
  if (tx->poll_retransmit_timer) {
    RlcDebug("Poll retransmission timer expired after %dms", poll_retransmit_timer.duration());
    debug_state();
    /*
     * - if both the transmission buffer and the retransmission buffer are empty
     *   (excluding transmitted RLC SDU or RLC SDU segment awaiting acknowledgements); or
     * - if no new RLC SDU or RLC SDU segment can be transmitted (e.g. due to window stalling):
     *   - consider the RLC SDU with the highest SN among the RLC SDUs submitted to lower layer for
     *   retransmission; or
     *   - consider any RLC SDU which has not been positively acknowledged for retransmission.
     * - include a poll in an AMD PDU as described in section 5.3.3.2.
     */
    if ((tx_sdu_queue.is_empty() && retx_queue.empty()) || tx_window->full()) {
      if (tx_window->empty()) {
        RlcError("t-PollRetransmit expired, but the tx_window is empty. POLL_SN=%d, Tx_Next_Ack=%d, tx_window_size=%d",
                 st.poll_sn,
                 st.tx_next_ack,
                 tx_window->size());
        goto end;
      }
      if (not tx_window->has_sn(st.tx_next_ack)) {
        RlcError("t-PollRetransmit expired, but Tx_Next_Ack is not in the tx_widow. POLL_SN=%d, Tx_Next_Ack=%d, "
                 "tx_window_size=%d",
                 st.poll_sn,
                 st.tx_next_ack,
                 tx_window->size());
        goto end;
      }
      // RETX first RLC SDU that has not been ACKed
      // or first SDU segment of the first RLC SDU
      // that has not been acked
      rlc_amd_retx_nr_t& retx = retx_queue.push();
      retx.sn                 = st.tx_next_ack;
      if ((*tx_window)[st.tx_next_ack].segment_list.empty()) {
        // Full SDU
        retx.is_segment     = false;
        retx.so_start       = 0;
        retx.segment_length = (*tx_window)[st.tx_next_ack].sdu_buf->N_bytes;
        retx.current_so     = 0;
      } else {
        // To make sure we do not mess up the segment list
        // We RETX an SDU segment instead of the full SDU
        // if the SDU has been segmented before.
        // As we cannot know which segments have been ACKed before
        // we simply RETX the first one.
        retx.is_segment     = true;
        retx.so_start       = 0;
        retx.current_so     = 0;
        retx.segment_length = (*tx_window)[st.tx_next_ack].segment_list.begin()->payload_len;
      }
      RlcDebug("Retransmission because of t-PollRetransmit. RETX SN=%d, is_segment=%s, so_start=%d, segment_length=%d",
               retx.sn,
               retx.is_segment ? "true" : "false",
               retx.so_start,
               retx.segment_length);
    }
    goto end;
  }
end:
	oset_thread_mutex_unlock(&base_tx->mutex);
	return;
}


static void rlc_am_base_tx_stop(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_destroy(&base_tx->unread_bytes_mutex);
	oset_thread_mutex_destroy(&base_tx->mutex);
}

static void rlc_am_base_tx_init(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_init(&base_tx->mutex);
	oset_thread_mutex_init(&base_tx->unread_bytes_mutex);
}

static void rlc_am_base_rx_timer_expired(void *data)
{
  rlc_am_base_rx *base_rx = (rlc_am_base_rx *)data;
  rlc_am_nr_rx *rx = base_rx->rx;
  rlc_am_base *base = base_rx->base;

  oset_thread_mutex_lock(&base_rx->mutex);

  // Status Prohibit
  if (status_prohibit_timer.is_valid() && status_prohibit_timer.id() == timeout_id) {
    RlcDebug("Status prohibit timer expired after %dms", status_prohibit_timer.duration());
    goto end;
  }

  // Reassembly
  if (reassembly_timer.is_valid() && reassembly_timer.id() == timeout_id) {
    RlcDebug("Reassembly timer expired after %dms", reassembly_timer.duration());
    /*
     * 5.2.3.2.4 Actions when t-Reassembly expires:
     * - update RX_Highest_Status to the SN of the first RLC SDU with SN >= RX_Next_Status_Trigger for which not
     *   all bytes have been received;
     * - if RX_Next_Highest> RX_Highest_Status +1: or
     * - if RX_Next_Highest = RX_Highest_Status + 1 and there is at least one missing byte segment of the SDU
     *   associated with SN = RX_Highest_Status before the last byte of all received segments of this SDU:
     *   - start t-Reassembly;
     *   - set RX_Next_Status_Trigger to RX_Next_Highest.
     */
    uint32_t sn_upd = {};
    for (sn_upd = st.rx_next_status_trigger; rx_mod_base_nr(sn_upd) < rx_mod_base_nr(st.rx_next_highest);
         sn_upd = (sn_upd + 1) % mod_nr) {
      if (not rx_window->has_sn(sn_upd) || (rx_window->has_sn(sn_upd) && not(*rx_window)[sn_upd].fully_received)) {
        break;
      }
    }
    st.rx_highest_status = sn_upd;
    if (not valid_ack_sn(st.rx_highest_status)) {
      RlcError("Rx_Highest_Status not inside RX window");
      debug_state();
    }
    srsran_assert(valid_ack_sn(st.rx_highest_status), "Error: rx_highest_status assigned outside rx window");

    bool restart_reassembly_timer = false;
    if (rx_mod_base_nr(st.rx_next_highest) > rx_mod_base_nr(st.rx_highest_status + 1)) {
      restart_reassembly_timer = true;
    }
    if (rx_mod_base_nr(st.rx_next_highest) == rx_mod_base_nr(st.rx_highest_status + 1)) {
      if (rx_window->has_sn(st.rx_highest_status) && (*rx_window)[st.rx_highest_status].has_gap) {
        restart_reassembly_timer = true;
      }
    }
    if (restart_reassembly_timer) {
      reassembly_timer.run();
      st.rx_next_status_trigger = st.rx_next_highest;
    }

    /* 5.3.4 Status reporting:
     * - The receiving side of an AM RLC entity shall trigger a STATUS report when t-Reassembly expires.
     *   NOTE 2: The expiry of t-Reassembly triggers both RX_Highest_Status to be updated and a STATUS report to be
     *   triggered, but the STATUS report shall be triggered after RX_Highest_Status is updated.
     */
    do_status = true;
    debug_state();
    debug_window();
    goto end;
  }
end:
  oset_thread_mutex_unlock(&base_rx->mutex);
  return;
}

/*
 * Status PDU
 */
uint32_t rlc_am_base_rx_get_status_pdu(rlc_am_base_rx *base_rx, rlc_am_nr_status_pdu_t *status, uint32_t max_len)
{

  if (0 != oset_thread_mutex_trylock(&base_rx->mutex)) {
	return OSET_ERROR;
  }

  rlc_am_nr_status_pdu_reset(status);

  /*
   * - for the RLC SDUs with SN such that RX_Next <= SN < RX_Highest_Status that has not been completely
   *   received yet, in increasing SN order of RLC SDUs and increasing byte segment order within RLC SDUs,
   *   starting with SN = RX_Next up to the point where the resulting STATUS PDU still fits to the total size of RLC
   *   PDU(s) indicated by lower layer:
   */
  RlcDebug("Generating status PDU");
  for (uint32_t i = st.rx_next; rx_mod_base_nr(i) < rx_mod_base_nr(st.rx_highest_status); i = (i + 1) % mod_nr) {
    if ((rx_window->has_sn(i) && (*rx_window)[i].fully_received)) {
      RlcDebug("SDU SN=%d is fully received", i);
    } else {
      if (not rx_window->has_sn(i)) {
        // No segment received, NACK the whole SDU
        RlcDebug("Adding NACK for full SDU. NACK SN=%d", i);
        rlc_status_nack_t nack;
        nack.nack_sn = i;
        nack.has_so  = false;
        status->push_nack(nack);
      } else if (not(*rx_window)[i].fully_received) {
        // Some segments were received, but not all.
        // NACK non consecutive missing bytes
        RlcDebug("Adding NACKs for segmented SDU. NACK SN=%d", i);
        uint32_t last_so         = 0;
        bool     last_segment_rx = false;
        for (auto segm = (*rx_window)[i].segments.begin(); segm != (*rx_window)[i].segments.end(); segm++) {
          if (segm->header.so != last_so) {
            // Some bytes were not received
            rlc_status_nack_t nack;
            nack.nack_sn  = i;
            nack.has_so   = true;
            nack.so_start = last_so;
            nack.so_end   = segm->header.so - 1; // set to last missing byte
            status->push_nack(nack);
            if (nack.so_start > nack.so_end) {
              // Print segment list
              for (auto segm_it = (*rx_window)[i].segments.begin(); segm_it != (*rx_window)[i].segments.end();
                   segm_it++) {
                RlcError("Segment: segm.header.so=%d, segm.buf.N_bytes=%d", segm_it->header.so, segm_it->buf->N_bytes);
              }
              RlcError("Error: SO_start=%d > SO_end=%d. NACK_SN=%d. SO_start=%d, SO_end=%d, seg.so=%d",
                       nack.so_start,
                       nack.so_end,
                       nack.nack_sn,
                       nack.so_start,
                       nack.so_end,
                       segm->header.so);
              srsran_assert(nack.so_start <= nack.so_end,
                            "Error: SO_start=%d > SO_end=%d. NACK_SN=%d",
                            nack.so_start,
                            nack.so_end,
                            nack.nack_sn);
            } else {
              RlcDebug("First/middle segment missing. NACK_SN=%d. SO_start=%d, SO_end=%d",
                       nack.nack_sn,
                       nack.so_start,
                       nack.so_end);
            }
          }
          if (segm->header.si == rlc_nr_si_field_t::last_segment) {
            last_segment_rx = true;
          }
          last_so = segm->header.so + segm->buf->N_bytes;
        } // Segment loop
        if (not last_segment_rx) {
          rlc_status_nack_t nack;
          nack.nack_sn  = i;
          nack.has_so   = true;
          nack.so_start = last_so;
          nack.so_end   = rlc_status_nack_t::so_end_of_sdu;
          status->push_nack(nack);
          RlcDebug(
              "Final segment missing. NACK_SN=%d. SO_start=%d, SO_end=%d", nack.nack_sn, nack.so_start, nack.so_end);
          srsran_assert(nack.so_start <= nack.so_end, "Error: SO_start > SO_end. NACK_SN=%d", nack.nack_sn);
        }
      }
    }
  } // NACK loop

  /*
   * - set the ACK_SN to the SN of the next not received RLC SDU which is not
   * indicated as missing in the resulting STATUS PDU.
   */
  status->ack_sn = st.rx_highest_status;

  // trim PDU if necessary
  if (status->packed_size > max_len) {
    RlcInfo("Trimming status PDU with %d NACKs and packed_size=%d into max_len=%d",
            status->nacks.size(),
            status->packed_size,
            max_len);
    log_rlc_am_nr_status_pdu_to_string(logger.debug, "Untrimmed status PDU - %s", status, rb_name);
    if (not status->trim(max_len)) {
      RlcError("Failed to trim status PDU into provided space: max_len=%d", max_len);
    }
  }

  if (max_len != UINT32_MAX) {
    // UINT32_MAX is used just to query the status PDU length
    if (status_prohibit_timer.is_valid()) {
      status_prohibit_timer.run();
    }
    do_status = false;
  }

  oset_thread_mutex_unlock(&base_rx->mutex);

  return status->packed_size;
}


static uint32_t rlc_am_base_rx_get_sdu_rx_latency_ms(rlc_am_base_rx *base_rx)
{
  return 0;
}

static uint32_t rlc_am_base_rx_get_rx_buffered_bytes(rlc_am_base_rx *base_rx)
{
  return 0;
}

static void rlc_am_base_rx_stop(rlc_am_base_rx *base_rx)
{
	//todo
	base_rx->do_status = false;
	oset_thread_mutex_destroy(&base_rx->mutex);
}

static void rlc_am_base_rx_init(rlc_um_base_rx *base_rx)
{
	oset_thread_mutex_init(&base_rx->mutex);
}

static void rlc_am_base_init(rlc_am_base *base, uint32_t lcid_, uint16_t rnti_)
{
	rlc_common_init(&base->common, NULL, rnti_, lcid_, (rlc_mode_t)um, NULL);
	base->rx_enabled = false;
	base->tx_enabled = false;
	base->metrics = {0};
	oset_thread_mutex_init(&base->metrics_mutex);
}

static void rlc_am_base_stop(rlc_am_base *base)
{
	rlc_common_destory(&base->common);
	base->rx_enabled = false;
	base->tx_enabled = false;
	oset_thread_mutex_destroy(&base->metrics_mutex);
	base->metrics = {0};
}

static uint32_t rlc_am_base_read_dl_pdu(rlc_am_base *base, rlc_am_base_tx *base_tx, uint8_t* payload, uint32_t nof_bytes)
{
	uint32_t read_bytes = rlc_am_base_tx_read_dl_pdu(base_tx, payload, nof_bytes);

	oset_thread_mutex_lock(&base->metrics_mutex);
	base->metrics.num_tx_pdus += read_bytes > 0 ? 1 : 0;
	base->metrics.num_tx_pdu_bytes += read_bytes;
	oset_thread_mutex_unlock(&base->metrics_mutex);

	return read_bytes;
}


static void rlc_am_base_write_dl_sdu(rlc_am_base *base, rlc_am_base_tx *base_tx, byte_buffer_t *sdu)
{
	if (! base->tx_enabled || ! base_tx) {
		RlcDebug("RB is currently deactivated. Dropping SDU (%d B)", sdu->N_bytes);
		oset_thread_mutex_lock(&base->metrics_mutex);
		base->metrics.num_lost_sdus++;
		oset_thread_mutex_unlock(&base->metrics_mutex);
		return;
	}

	int sdu_bytes = sdu->N_bytes; //< Store SDU length for book-keeping
	if (rlc_am_base_tx_write_dl_sdu(base_tx, sdu) == OSET_OK) {
		oset_thread_mutex_lock(&base->metrics_mutex);
		base->metrics.num_tx_sdus++;
		base->metrics.num_tx_sdu_bytes += sdu_bytes;
		oset_thread_mutex_unlock(&base->metrics_mutex);
	} else {
		oset_thread_mutex_lock(&base->metrics_mutex);
		base->metrics.num_lost_sdus++;
		oset_thread_mutex_unlock(&base->metrics_mutex);
	}
}


#endif
/////////////////////////////////////////////////////////////////////////////
#if AM_RXTX
static void suspend_tx(rlc_am_nr_tx *tx)
{
	oset_thread_mutex_lock(&tx->base_tx.mutex);
	empty_queue_no_lock(&tx->base_tx);

	if(tx->poll_retransmit_timer) gnb_timer_delete(tx->poll_retransmit_timer);

	tx->st = {0};
	tx->sdu_under_segmentation_sn = INVALID_RLC_SN;

	// Drop all messages in TX window
	tx->tx_window->clear();
	oset_hash_destroy(tx->tx_window);

	// Drop all messages in RETX queue
	tx->retx_queue.clear();

	tx->base_tx.base.tx_enabled = false;
	oset_thread_mutex_unlock(&tx->base_tx.mutex);
}

static void rlc_am_nr_tx_stop(rlc_am_nr_tx *tx)
{
	suspend_tx(tx);
	rlc_am_base_tx_stop(&tx->base_tx);
}

static void rlc_am_nr_tx_reestablish(rlc_am_nr_tx *tx)
{
	suspend_tx(tx);
}

static bool rlc_am_nr_tx_configure(rlc_am_nr_tx *tx, rlc_config_t *cfg_, char *rb_name_)
{
	tx->base_tx.rb_name = rb_name_;
	tx->base_tx.cfg = cfg_->am_nr;

	if (cfg_->tx_queue_length > max_tx_queue_size) {
	  RlcError("configuring tx queue length of %d PDUs too big. Maximum value is %d.",
			   cfg_->tx_queue_length,
			   max_tx_queue_size);
	  return false;
	}
	
	tx->mod_nr = cardinality(tx->base_tx.cfg.tx_sn_field_length);
	tx->AM_Window_Size = am_window_size(tx->base_tx.cfg.tx_sn_field_length);

	switch (tx->base_tx.cfg.tx_sn_field_length) {
	  case (rlc_am_nr_sn_size_t)size12bits:
		tx->min_hdr_size = 2;
		break;
	  case (rlc_am_nr_sn_size_t)size18bits:
		tx->min_hdr_size = 3;
		break;
	  default:
		RlcError("attempt to configure unsupported tx_sn_field_length %s", rlc_am_nr_sn_size_to_string(tx->base_tx.cfg.tx_sn_field_length));
		return false;
	}
	
	tx->max_hdr_size = tx->min_hdr_size + tx->so_size;

	// make sure Tx queue is empty before attempting to resize
	oset_list_init(&tx->base_tx.tx_sdu_queue);//cfg_->tx_queue_length
	
	// Configure t_poll_retransmission timer
	if (tx->base_tx.cfg.t_poll_retx > 0) {
	  tx->poll_retransmit_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_tx_timer_expired, &tx->base_tx);
	}
	
	tx->base_tx.base.tx_enabled = true;
	
	RlcDebug("RLC AM NR configured tx entity");

	return true;
}

static void rlc_am_nr_tx_init(rlc_am_nr_tx *tx, rlc_am_nr_rx *rx, rlc_um_base *base)
{
	tx->base_tx.base = base;
	tx->base_tx.rx = rx;
	tx->base_tx.tx = tx;

	tx->min_hdr_size = 2;
	tx->so_size      = 2;
	tx->max_hdr_size = 4;
	tx->mod_nr       = 0;

	tx->tx_window = oset_hash_make();
	oset_list_init(&tx->tx_window);

	rlc_am_base_tx_init(&tx->base_tx);
}

static void suspend_rx(rlc_am_nr_rx *rx)
{
	oset_thread_mutex_lock(&rx->base_rx.mutex);
	if(rx->status_prohibit_timer) gnb_timer_delete(rx->status_prohibit_timer);
	if(rx->reassembly_timer) gnb_timer_delete(rx->reassembly_timer);

	rx->st = {0};
	// Drop all messages in RX window
	oset_hash_destroy(rx->rx_window);
	oset_thread_mutex_unlock(&rx->base_rx.mutex);
}

static void rlc_am_nr_rx_reestablish(rlc_am_nr_rx *rx)
{
	suspend_rx(rx);
}

static void rlc_am_nr_rx_stop(rlc_am_nr_rx *rx)
{
	suspend_rx(rx);
	rlc_am_base_rx_stop(&rx->base_rx);
}

static bool rlc_am_nr_rx_configure(rlc_am_nr_rx *rx, rlc_config_t *cfg_, char *rb_name_)
{
	rx->base_rx.rb_name = rb_name_;
	rx->base_rx.cfg = cfg_->am_nr;

	// configure timer
	if (rx->base_rx.cfg.t_status_prohibit > 0) {
		rx->status_prohibit_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_rx_timer_expired, &rx->base_rx);
	}

	// Configure t_reassembly timer
	if (rx->base_rx.cfg.t_reassembly > 0) {
		rx->reassembly_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_rx_timer_expired, &rx->base_rx);
	}

	rx->mod_nr = cardinality(rx->base_rx.cfg.rx_sn_field_length);
	rx->AM_Window_Size = am_window_size(rx->base_rx.cfg.rx_sn_field_length);

	rx->base_rx.base.rx_enabled = true;

	RlcDebug("RLC AM NR configured rx entity.");

	return true;
}

static void rlc_am_nr_rx_init(rlc_am_nr_rx *rx, rlc_am_nr_tx *tx, rlc_um_base *base)
{
	rx->base_rx.base = base;
	rx->base_rx.tx = tx;
	rx->base_rx.rx = rx;

	rx->rx_window = oset_hash_make();
	rlc_am_base_rx_init(&rx->base_rx, base)
}

#endif

/////////////////////////////////////////////////////////////////////////////
void rlc_am_nr_stop(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_um_nr *)am_common;

	rlc_am_nr_rx_stop(&am->rx);
	rlc_am_nr_tx_stop(&am->tx);
	rlc_am_base_stop(&am->base);
}

void rlc_am_nr_get_buffer_state(rlc_common *am_common, uint32_t *n_bytes_newtx, uint32_t *n_bytes_prio)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	rlc_am_base_tx_get_buffer_state(&am->tx.base_tx, n_bytes_newtx, n_bytes_prio);
}


bool rlc_am_nr_configure(rlc_common *am_common, rlc_config_t *cfg_)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	// store config
	am->base.cfg = *cfg_;

	// determine bearer name and configure Rx/Tx objects
	am->base.common.rb_name = oset_strdup(get_rb_name(am->base.common.lcid));

	if (! rlc_am_nr_rx_configure(&am->rx, &am->base.cfg, am->base.common.rb_name)) {
		RlcError("Error configuring bearer (RX)");
		return false;
	}

	if (! rlc_am_nr_tx_configure(&am->tx, &am->base.cfg, am->base.common.rb_name)) {
		RlcError("Error configuring bearer (TX)");
		return false;
	}

	RlcInfo("AM NR configured - tx_sn_field_length=%d, rx_sn_field_length=%d, "
	        "t_poll_retx=%d, poll_pdu=%d, poll_byte=%d, "
	        "max_retx_thresh=%d, t_reassembly=%d, t_status_prohibit=%d, tx_queue_length=%d",
	        rlc_am_nr_sn_size_to_number(am->base.cfg.am_nr.tx_sn_field_length),
	        rlc_am_nr_sn_size_to_number(am->base.cfg.am_nr.rx_sn_field_length),
	        am->base.cfg.am_nr.t_poll_retx,
	        am->base.cfg.am_nr.poll_pdu,
	        am->base.cfg.am_nr.poll_byte,
	        am->base.cfg.am_nr.max_retx_thresh,
	        am->base.cfg.am_nr.t_reassembly,
	        am->base.cfg.am_nr.t_status_prohibit,
	        am->base.cfg.tx_queue_length);

  return true;
}

void rlc_am_nr_set_bsr_callback(rlc_common *am_common, bsr_callback_t callback)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	am->tx.base_tx.bsr_callback = callback;
}

rlc_bearer_metrics_t rlc_am_nr_get_metrics(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	// update values that aren't calculated on the fly
	uint32_t latency        = rlc_am_base_rx_get_sdu_rx_latency_ms(&am->rx.base_rx);
	uint32_t buffered_bytes = rlc_am_base_rx_get_rx_buffered_bytes(&am->rx.base_rx);

	oset_thread_mutex_lock(&am->base.metrics_mutex);
	am->base.metrics.rx_latency_ms     = latency;
	am->base.metrics.rx_buffered_bytes = buffered_bytes;
	oset_thread_mutex_unlock(&am->base.metrics_mutex);
	return am->base.metrics;
}

void rlc_am_nr_reset_metrics(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	oset_thread_mutex_lock(&am->base.metrics_mutex);
	am->base.metrics = {0};
	oset_thread_mutex_unlock(&am->base.metrics_mutex);
}

void rlc_am_nr_reestablish(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	RlcDebug("Reestablished bearer");
	rlc_am_nr_tx_reestablish(&am->tx); // calls stop and enables tx again
	rlc_am_nr_rx_reestablish(&am->rx); // calls only stop
	am->base.tx_enabled = true;
}

void rlc_am_nr_read_dl_pdu(rlc_common *am_common, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr *am = (rlc_um_nr *)am_common;

	rlc_am_base_read_dl_pdu(&am->base, &am->tx.base_tx, payload, nof_bytes);
}


void rlc_am_nr_write_dl_sdu(rlc_common *am_common, byte_buffer_t *sdu)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	rlc_am_base_write_dl_sdu(&am->base, &am->tx.base_tx, sdu);
}

rlc_mode_t rlc_am_nr_get_mode(void)
{
	return rlc_mode_t(am);
}

bool rlc_am_nr_sdu_queue_is_full(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	return oset_list_count(am->tx.base_tx.tx_sdu_queue) == am->base.cfg.tx_queue_length;
}

void rlc_am_nr_discard_sdu(rlc_common *am_common, uint32_t discard_sn)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	if (!am->base.tx_enabled) {
		return;
	}

	rlc_am_base_tx_discard_sdu(&am->tx.base_tx, discard_sn);
}

rlc_am_nr *rlc_am_nr_init(uint32_t lcid_,	uint16_t rnti_)
{
	rlc_am_nr *am = oset_malloc(sizeof(rlc_am_nr));
	ASSERT_IF_NOT(am, "lcid %u Could not allocate pdcp nr context from pool", lcid_);
	memset(am, 0, sizeof(rlc_am_nr));

	rlc_am_base_init(&am->base, lcid_, rnti_);
	rlc_am_nr_tx_init(&am->tx, &am->rx, &am->base);
	rlc_am_nr_rx_init(&am->rx, &am->tx, &am->base);

	am->base.common.func = {
						._get_buffer_state  = rlc_am_nr_get_buffer_state,
						._configure         = rlc_am_nr_configure,
						._set_bsr_callback  = rlc_am_nr_set_bsr_callback,
						._get_metrics 	    = rlc_am_nr_get_metrics,
						._reset_metrics     = rlc_am_nr_reset_metrics,
						._reestablish       = rlc_am_nr_reestablish,
						._write_ul_pdu      = rlc_am_nr_write_ul_pdu,
						._read_dl_pdu       = rlc_am_nr_read_dl_pdu;
						._write_dl_sdu      = rlc_am_nr_write_dl_sdu,
						._get_mode		    = rlc_am_nr_get_mode,
						._sdu_queue_is_full	= rlc_am_nr_sdu_queue_is_full,
						._discard_sdu	    = rlc_am_nr_discard_sdu,
						._stop              = rlc_am_nr_stop,
					  };
}

