/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "rlc/rlc_um.h"
#include "rrc/rrc.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlcUM"

#define RX_MOD_NR_BASE(rx, x) (((x)-(rx->RX_Next_Highest) - (rx->UM_Window_Size)) % (rx->mod))

/////////////////////////////////////////////////////////////////////////////////////////////////
static uint32_t rlc_um_nr_write_data_pdu_header(rlc_um_nr_pdu_header_t *header, byte_buffer_t *pdu)
{
  // Make room for the header
  uint32_t len = rlc_um_nr_packed_length(header);
  pdu->msg -= len;
  uint8_t* ptr = pdu->msg;

  // write SI field
  *ptr = (header->si & 0x03) << 6; // 2 bits SI

  if (header->si == (rlc_nr_si_field_t)full_sdu) {
    // that's all ..
    ptr++;
  } else {
    if (header->sn_size == (rlc_um_nr_sn_size_t)size6bits) {
      // write SN
      *ptr |= (header->sn & 0x3f); // 6 bit SN
      ptr++;
    } else {
      // 12bit SN
      *ptr |= (header->sn >> 8) & 0xf; // high part of SN (4 bit)
      ptr++;
      *ptr = (header->sn & 0xFF); // remaining 8 bit SN
      ptr++;
    }
    if (header->so) {
      // write SO
      *ptr = (header->so) >> 8; // first part of SO
      ptr++;
      *ptr = (header->so & 0xFF); // second part of SO
      ptr++;
    }
  }

  pdu->N_bytes += (ptr - pdu->msg);// + len

  return len;
}

//  UDM PDU
// complete RLC PDU (head_len_full = 1)
// | SI |R|R|R|R|R|R|
// |      Data      |
// |      ...       |

// first RLC PDU segment(sn 6 bit)      first RLC PDU segment(sn 12 bit)
// | SI |    SN     |                   | SI |R|R|   SN   |
// |      Data      |                   |      SN         |
// |      ...       |                   |      Data       |


// other RLC PDU segment(sn 6 bit)      other RLC PDU segment(sn 12 bit)
// | SI |    SN     |                   | SI |R|R|   SN   |
// |      SO        |                   |      SN         |
// |      SO        |                   |      SO         |
// |      Data      |                   |      SO         |
// |      ...       |                   |      Data       |

uint32_t rlc_um_nr_packed_length(rlc_um_nr_pdu_header_t *header)
{
  uint32_t len = 0;
  if (header->si == (rlc_nr_si_field_t)full_sdu) {
    // that's all ..
    len++;
  } else {
    if (header->sn_size == (rlc_um_nr_sn_size_t)size6bits) {
      // Only 1Byte for SN
      len++;
    } else {
      // 2 Byte for 12bit SN
      len += 2;
    }
    if (header->so) {
      // Two bytes always for segment information
      len += 2;
    }
  }
  return len;
}

/////////////////////////////////Base/////////////////////////////////////////

static byte_buffer_t* tx_sdu_queue_read(rlc_um_base_tx *base_tx)
{
	rlc_um_base_tx_sdu_t *node = oset_list_first(&base_tx->tx_sdu_queue);
	if(node){
		byte_buffer_t *sdu = node->buffer;
		oset_list_remove(&base_tx->tx_sdu_queue, node);
		return sdu;
	}
	return NULL;
}

static uint32_t build_dl_data_pdu(rlc_um_base_tx *base_tx, uint8_t *payload, uint32_t nof_bytes)
{
  // Sanity check (we need at least 2Byte for a SDU)
  if (nof_bytes < 2) {
    RlcWarning("Cannot build a PDU with %d byte.", nof_bytes);
    return 0;
  }

  byte_buffer_t *pdu = byte_buffer_init();

  rlc_um_nr_tx *tx = base_tx->tx;

  //std::lock_guard<std::mutex> lock(mutex);
  rlc_um_nr_pdu_header_t      header = {0};
  header.si          = (rlc_nr_si_field_t)full_sdu;
  header.sn          = tx->TX_Next;
  header.sn_size     = base_tx->cfg.um_nr.sn_field_length;

  // pdu剩余空间
  uint32_t pdu_space = SRSRAN_MIN(nof_bytes, byte_buffer_get_tailroom(pdu));

  // Select segmentation information and header size
  if (base_tx->tx_sdu == NULL) {
    // Read a new SDU
    do {
      base_tx->tx_sdu = tx_sdu_queue_read(&base_tx->tx_sdu_queue);
    } while (base_tx->tx_sdu == NULL && !oset_list_empty(&base_tx->tx_sdu_queue));

	if (base_tx->tx_sdu == NULL) {
      RlcDebug("Cannot build any PDU, tx_sdu_queue has no non-null SDU.");
      return 0;
    }

	// complete RLC PDU (head_len_full = 1)
	// | SI |R|R|R|R|R|R|
	// |	  Data		|
	// |	  ...		|
	
	// first RLC PDU segment(sn 6 bit)		first RLC PDU segment(sn 12 bit)
	// | SI |	 SN 	|					| SI |R|R|	 SN   |
	// |	  Data		|					|	   SN		  |
	// |	  ...		|					|	   Data 	  |
	
	
	// other RLC PDU segment(sn 6 bit)		other RLC PDU segment(sn 12 bit)
	// | SI |	 SN 	|					| SI |R|R|	 SN   |
	// |	  SO		|					|	   SN		  |
	// |	  SO		|					|	   SO		  |
	// |	  Data		|					|	   SO		  |
	// |	  ...		|					|	   Data 	  |


    tx->next_so = 0;

    // Check for full SDU case
    if (base_tx->tx_sdu->N_bytes <= pdu_space - head_len_full) {
      header.si = (rlc_nr_si_field_t)full_sdu;//若待发送的数据 < tb可以发送的大小，则全部发送
    } else {
      header.si = (rlc_nr_si_field_t)first_segment;//分片，并且为首条报文
    }
  } else {
    // 上次未处理完毕的sdu数据剩余部分
    // The SDU is not new; check for last segment
    if (base_tx->tx_sdu->N_bytes <= pdu_space - tx->head_len_segment) {
      header.si = (rlc_nr_si_field_t)last_segment;//分片的最后一条数据
    } else {
      header.si = (rlc_nr_si_field_t)neither_first_nor_last_segment;//分片的中间数据
    }
  }

  header.so = tx->next_so;//位偏移

  // Calculate actual header length
  uint32_t head_len = rlc_um_nr_packed_length(header);//计算实际头长度
  if (pdu_space <= head_len + 1) {
    RlcInfo("Cannot build a PDU - %d bytes available, %d bytes required for header", nof_bytes, head_len);
    return 0;
  }

  // Calculate the amount of data to move
  uint32_t space   = pdu_space - head_len;
  uint32_t to_move = space >= base_tx->tx_sdu->N_bytes ? base_tx->tx_sdu->N_bytes : space;

  // Log
  RlcDebug("adding %s - (%d/%d)", rlc_nr_si_field_to_string(header.si), to_move, base_tx->tx_sdu->N_bytes);

  // Move data from SDU to PDU
  uint8_t *pdu_ptr = pdu->msg;
  memcpy(pdu_ptr, base_tx->tx_sdu->msg, to_move);
  //pdu_ptr += to_move;
  pdu->N_bytes += to_move;
  base_tx->tx_sdu->N_bytes -= to_move;
  base_tx->tx_sdu->msg += to_move;

  // Release SDU if emptied
  if (base_tx->tx_sdu->N_bytes == 0) {
  	oset_free(base_tx->tx_sdu);
    base_tx->tx_sdu = NULL;
  }

  // advance SO offset
  tx->next_so += to_move;

  // Update SN if needed
  if (header.si == (rlc_nr_si_field_t)last_segment) {
    tx->TX_Next = (tx->TX_Next + 1) % tx->mod;
    tx->next_so = 0;
  }

  // Add header and TX
  rlc_um_nr_write_data_pdu_header(&header, pdu);
  memcpy(payload, pdu->msg, pdu->N_bytes);
  uint32_t ret = pdu->N_bytes;
  oset_free(pdu);
  // Assert number of bytes
  ASSERT_IF_NOT(ret <= nof_bytes, "Error while packing MAC PDU (more bytes written (%d) than expected (%d)!", ret, nof_bytes);

  if (header.si == (rlc_nr_si_field_t)full_sdu) {
    // log without SN
    RlcHexInfo(payload, ret, "Tx PDU (%d B)", ret);
  } else {
    RlcHexInfo(payload, ret, "Tx PDU SN=%d (%d B)", header.sn, ret);
  }

  RlcDebug("TX_Next=%d, next_so=%d", tx->TX_Next, tx->next_so);

  return ret;
}

#if 1
static uint32_t rlc_um_base_tx_build_dl_data_pdu(rlc_um_base_tx *base_tx, uint8_t* payload, uint32_t nof_bytes)
{
	RlcDebug("MAC opportunity - %d bytes", nof_bytes);

	if (base_tx->tx_sdu == NULL && oset_list_empty(&base_tx->tx_sdu_queue)) {
		RlcInfo("No data available to be sent");
		return 0;
	}

	return build_dl_data_pdu(base_tx, payload, nof_bytes);

	{
	  // Sanity check (we need at least 2B for a SDU)
	}
}

static int rlc_um_base_tx_write_dl_sdu(rlc_um_base_tx *base_tx, byte_buffer_t *sdu)
{
  if (sdu) {
    uint8_t*   msg_ptr   = sdu->msg;
    uint32_t   nof_bytes = sdu->N_bytes;
  	int count = oset_list_count(&base_tx->tx_sdu_queue);

 	if((uint32_t)count > base_tx->cfg.tx_queue_length){
      rlc_um_base_tx_sdu_t *sdu_node = oset_malloc(rlc_um_base_tx_sdu_t);
	  oset_assert(sdu_node);
      sdu_node->buffer = byte_buffer_dup(sdu);
  	  oset_list_add(&base_tx->tx_sdu_queue, sdu_node);
	  oset_thread_mutex_lock(base_tx->unread_bytes_mutex);
	  base_tx->unread_bytes += sdu->N_bytes;
	  oset_thread_mutex_unlock(base_tx->unread_bytes_mutex);
      RlcHexInfo(msg_ptr, nof_bytes, "Tx SDU (%d B, tx_sdu_queue_len=%d)", nof_bytes, count);
      return OSET_OK;
    } else {
      RlcHexWarning(msg_ptr,
                    nof_bytes,
                    "[Dropped SDU] %s Tx SDU (%d B, tx_sdu_queue_len=%d)",
                    base_tx->rb_name,
                    nof_bytes,
                    count);
    }
  } else {
    RlcWarning("NULL SDU pointer in write_sdu()");
  }
  return OSET_ERROR;
}

static void rlc_um_base_tx_stop(rlc_um_base_tx *base_tx)
{
	oset_list_init(&base_tx->tx_sdu_queue);
	oset_thread_mutex_destroy(&base_tx->unread_bytes_mutex);
}

static void rlc_um_base_tx_init(rlc_um_base_tx *base_tx, rlc_um_base *base_)
{
	oset_thread_mutex_init(&base_tx->unread_bytes_mutex);
}

//////////////////////////////////////////////////////////////////////////////////
static void rlc_um_base_rx_stop(rlc_um_base_rx *base_rx)
{
	*base_rx = {0};
}

static void rlc_um_base_rx_init(rlc_um_base_rx *base_rx, rlc_um_base *base_)
{
	base_rx->metrics = &base_->metrics;
	base_rx->lcid = base_->common.lcid;
}

//////////////////////////////////////////////////////////////////////////////////
void rlc_um_base_init(rlc_um_base *base, uint32_t lcid_, uint16_t rnti_)
{
	rlc_common_init(&base->common, NULL, rnti_, lcid_, (rlc_mode_t)um, NULL);
	base->rx_enabled = false;
	base->tx_enabled = false;
	oset_thread_mutex_init(&base->metrics_mutex);
	base->metrics = {0};
}

void rlc_um_base_stop(rlc_um_base *base)
{
	rlc_common_destory(&base->common);
	base->rx_enabled = false;
	base->tx_enabled = false;
	oset_thread_mutex_destroy(&base->metrics_mutex);
	base->metrics = {0};
}

uint32_t rlc_um_base_read_dl_pdu(rlc_um_base *base, rlc_um_base_tx *base_tx, uint8_t *payload, uint32_t nof_bytes)
{
	if (base_tx && base->tx_enabled) {
	  uint32_t len = rlc_um_base_tx_build_dl_data_pdu(base_tx, payload, nof_bytes);
	  if (len > 0) {
		oset_thread_mutex_lock(&base->metrics_mutex);
		base->metrics.num_tx_pdu_bytes += len;
		base->metrics.num_tx_pdus++;
		oset_thread_mutex_unlock(&base->metrics_mutex);
	  }
	  return len;
	}
	return 0;

}

void rlc_um_base_write_dl_sdu(rlc_um_base *base, rlc_um_base_tx *base_tx, byte_buffer_t *sdu)
{
  if (! base->tx_enabled || ! base_tx) {
    RlcDebug("RB is currently deactivated. Dropping SDU (%d B)", sdu->N_bytes);
    oset_thread_mutex_lock(&base->metrics_mutex);
    base->metrics.num_lost_sdus++;
    oset_thread_mutex_unlock(&base->metrics_mutex);
    return;
  }

  int sdu_bytes = sdu->N_bytes; //< Store SDU length for book-keeping
  if (rlc_um_base_tx_write_dl_sdu(base_tx, sdu) == OSET_OK) {
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
//////////////////////////////////////////////////////////////////////////////
/////////////////////////////////rlc_um_nr_rx/////////////////////////////////
bool rlc_um_nr_rx_has_missing_byte_segment(rlc_um_nr_rx *rx, uint32_t sn)
{
  // is at least one missing byte segment of the RLC SDU associated with SN = RX_Next_Reassembly before the last byte of
  // all received segments of this RLC SDU
  return (rx_window.find(sn) != rx_window.end());
}


// TS 38.322 v15.003 Section 5.2.2.2.4
void rlc_um_nr_rx_timer_expired(void *data)
{
	rlc_um_nr_rx *rx = (rlc_um_nr_rx *)data;

	oset_thread_mutex_lock(&rx->mutex);

    RlcDebug("reassembly timeout expiry for SN=%d - updating RX_Next_Reassembly and reassembling", rx->RX_Next_Reassembly);

    rx->base_rx.metrics->num_lost_pdus++;

    if (rx->base_rx.rx_sdu != NULL) {
		byte_buffer_clear(rx->base_rx.rx_sdu);
    }

    // update RX_Next_Reassembly to the next SN that has not been reassembled yet
    rx->RX_Next_Reassembly = rx->RX_Timer_Trigger;
    while (RX_MOD_NR_BASE(rx, rx->RX_Next_Reassembly) < RX_MOD_NR_BASE(rx, rx->RX_Next_Highest)) {
      rx->RX_Next_Reassembly = (rx->RX_Next_Reassembly + 1) % rx->mod;
      debug_state();
    }

    // discard all segments with SN < updated RX_Next_Reassembly
    for (auto it = rx->rx_window.begin(); it != rx->rx_window.end();) {
      if (RX_MOD_NR_BASE(rx, it->first) < RX_MOD_NR_BASE(rx, rx->RX_Next_Reassembly)) {
        it = rx_window.erase(it);
      } else {
        ++it;
      }
    }

    // check start of t_reassembly
	// RX_Next_Highest > RX_Next_Reassembly + 1;(这个条件说明还有包没有重组递交上层)
	// RX_Next_Highest = RX_Next_Reassembly + 1 and there is at least one missing byte segment of the RLC SDU associated with SN = RX_Next_Reassembly before the last byte of all received segments of this RLC SDU；
	// 设置RX_Next_Trigger=RX_Next_Highest;
    if (RX_MOD_NR_BASE(rx, rx->RX_Next_Highest) > RX_MOD_NR_BASE(rx, rx->RX_Next_Reassembly + 1) ||
        ((RX_MOD_NR_BASE(rx, rx->RX_Next_Highest) == RX_MOD_NR_BASE(rx, rx->RX_Next_Reassembly + 1) &&
          rlc_um_nr_rx_has_missing_byte_segment(rx, rx->RX_Next_Reassembly)))) {
      RlcDebug("starting reassembly timer for SN=%d", rx->base_rx.rb_name, rx->RX_Next_Reassembly);
	  gnb_timer_start(rx->reassembly_timer, rx->base_rx.cfg.um_nr.t_reassembly_ms);
      rx->RX_Timer_Trigger = rx->RX_Next_Highest;
    }

    debug_state();
	oset_thread_mutex_unlock(&rx->mutex);
}


static void rlc_um_nr_rx_init(rlc_um_base *base, rlc_um_nr_rx *rx)
{
	oset_thread_mutex_init(&rx->mutex);
	rlc_um_base_rx_init(&rx->base_rx, base)
}

static void rlc_um_nr_rx_stop(rlc_um_nr_rx *rx)
{
	oset_thread_mutex_destroy(&rx->mutex);
	if(rx->reassembly_timer) gnb_timer_delete(rx->reassembly_timer);
	rlc_um_base_rx_stop(&rx->base_rx);
}

static bool rlc_um_nr_rx_configure(rlc_um_nr_rx *rx, rlc_config_t *cnfg_, char *rb_name_)
{
	rx->base_rx.rb_name = rb_name_;
	rx->base_rx.cfg = *cnfg_;
	rx->base_rx.rx	= rx;

	rx->mod  = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 64 : 4096;
	rx->UM_Window_Size = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 32 : 2048;

	// configure timer
	if (rx->base_rx.cfg.um_nr.t_reassembly_ms > 0) {
		rx->reassembly_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_um_nr_rx_timer_expired, rx);
	}

	return true;
}

/////////////////////////////////rlc_um_nr_tx/////////////////////////////////

static void rlc_um_nr_tx_init(rlc_um_base *base, rlc_um_nr_tx *tx)
{
	rlc_um_base_tx_init(&tx->base_tx, base)
}

static void rlc_um_nr_tx_stop(rlc_um_nr_tx *tx)
{
	rlc_um_base_tx_stop(&tx->base_tx);
}

static bool rlc_um_nr_tx_configure(rlc_um_nr_tx *tx, rlc_config_t *cnfg_, char *rb_name_)
{
  tx->base_tx.rb_name = rb_name_;
  tx->base_tx.cfg = *cnfg_;
  tx->base_tx.tx  = tx;

  tx->mod            = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 64 : 4096;
  tx->UM_Window_Size = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 32 : 2048;

  // calculate header sizes for configured SN length
  rlc_um_nr_pdu_header_t header = {0};
  header.sn_size   = cnfg_->um_nr.sn_field_length;
  header.si        = (rlc_nr_si_field_t)first_segment;

  header.so        = 0;//无偏移
  tx->head_len_first   = rlc_um_nr_packed_length(&header);

  header.so        = 1;//有偏移
  tx->head_len_segment = rlc_um_nr_packed_length(&header);

  //oset_assert(oset_list_count(tx->base_tx.tx_sdu_queue) <= cnfg_->tx_queue_length);

  return true;
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////////////rlc_um_nr/////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void rlc_um_nr_stop(rlc_common *um_common)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;

	rlc_um_base_stop(&um->base);
	rlc_um_nr_rx_stop(&um->rx);
	rlc_um_nr_tx_stop(&um->tx);
}

bool rlc_um_nr_configure(rlc_common *um_common, rlc_config_t *cnfg_)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;

	// store config
	um->base.cfg = *cnfg_;

	// determine bearer name and configure Rx/Tx objects
	um->base.common.rb_name = oset_msprintf("DRB%s", um->base.cfg.um_nr.bearer_id);

	rlc_um_nr_rx_init(&um->base, &um->rx);
	if (! rlc_um_nr_rx_configure(&um->rx, &um->base.cfg, um->base.common.rb_name)) {
		return false;
	}

	rlc_um_nr_tx_init(&um->base, &um->tx);
	if (! rlc_um_nr_tx_configure(&um->tx, &um->base.cfg, um->base.common.rb_name)) {
		return false;
	}

	RlcInfo("configured in %s: sn_field_length=%u bits, t_reassembly=%d ms",
	      rlc_mode_to_string(um->base.cfg.rlc_mode, false),
	      rlc_um_nr_sn_size_to_number(um->base.cfg.um_nr.sn_field_length),
	      um->base.cfg.um_nr.t_reassembly_ms);

	um->base.rx_enabled = true;
	um->base.tx_enabled = true;

	return true;
}


void rlc_um_nr_read_dl_pdu(rlc_common *um_common, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;
	rlc_um_base_read_dl_pdu(&um->base, &um->tx.base_tx, payload, nof_bytes);
}


void rlc_um_nr_write_dl_sdu(rlc_common *um_common, byte_buffer_t *sdu)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;
	rlc_um_base_write_dl_sdu(&um->base, &um->tx.base_tx, sdu);
}

rlc_mode_t rlc_um_nr_get_mode(void)
{
  return rlc_mode_t(um);
}

bool rlc_um_nr_sdu_queue_is_full(rlc_common *um_common)
{
  rlc_um_nr *um = (rlc_um_nr *)um_common;
  return oset_list_count(um->tx.base_tx.tx_sdu_queue) == um->base.cfg.tx_queue_length;
}

rlc_um_nr *rlc_um_nr_init(uint32_t lcid_,	uint16_t rnti_)
{
	rlc_um_nr *um = oset_malloc(sizeof(rlc_um_nr));
	ASSERT_IF_NOT(um, "lcid %u Could not allocate pdcp nr context from pool", lcid_);
	memset(um, 0, sizeof(rlc_um_nr));

	rlc_um_base_init(&um->base, lcid_, rnti_);

	um->base.common.func = {
						._get_buffer_state  = rlc_um_nr_get_buffer_state,
						._configure         = rlc_um_nr_configure,
						._set_bsr_callback  = rlc_um_nr_set_bsr_callback,
						._get_metrics 	    = rlc_um_nr_get_metrics,
						._reset_metrics     = rlc_um_nr_reset_metrics,
						._reestablish       = rlc_um_nr_reestablish,
						._write_ul_pdu      = rlc_um_nr_write_ul_pdu,
						._read_dl_pdu       = rlc_um_nr_read_dl_pdu;
						._write_dl_sdu      = rlc_um_nr_write_dl_sdu,
						._get_mode		    = rlc_um_nr_get_mode,
						._sdu_queue_is_full	= rlc_um_nr_sdu_queue_is_full,
						._discard_sdu	    = rlc_um_nr_discard_sdu,
						._stop              = rlc_um_nr_stop,
					  };
}


