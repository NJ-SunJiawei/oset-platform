/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "rlc/rlc_um.h"
#include "pdcp/pdcp.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlcUM"

#define UM_RXTX

/////////////////////////////////////////////////////////////////////////////////////////////////
//  UMD PDU
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

static uint32_t rlc_um_nr_read_data_pdu_header(uint8_t  *            payload,
                                                           uint32_t  nof_bytes,
                                                           rlc_um_nr_sn_size_t sn_size,
                                                           rlc_um_nr_pdu_header_t *header)
{
  uint8_t* ptr = payload;

  header->sn_size = sn_size;

  // 0x03 = 0b00000011
  // Fixed part
  if (sn_size == (rlc_um_nr_sn_size_t)size6bits) {
    header->si = (rlc_nr_si_field_t)((*ptr >> 6) & 0x03); // 2 bits SI
    header->sn = *ptr & 0x3F;                             // 6 bits SN
    // sanity check
    if (header->si == (rlc_nr_si_field_t)full_sdu && header->sn != 0) {
      oset_error("Malformed PDU, reserved bits are set");
      return 0;
    }
    ptr++;
  } else if (sn_size == (rlc_um_nr_sn_size_t)size12bits) {
    header->si = (rlc_nr_si_field_t)((*ptr >> 6) & 0x03); // 2 bits SI
    header->sn = (*ptr & 0x0F) << 8;                      // 4 bits SN
    if (header->si == (rlc_nr_si_field_t)full_sdu && header->sn != 0) {
      oset_error("Malformed PDU, reserved bits are set");
      return 0;
    }

    // sanity check
    if (header->si == (rlc_nr_si_field_t)first_segment) {
      // make sure two reserved bits are not set
      if (((*ptr >> 4) & 0x03) != 0) {
        oset_error("Malformed PDU, reserved bits are set.");
        return 0;
      }
    }

    if (header->si != (rlc_nr_si_field_t)full_sdu) {
      // continue unpacking remaining SN
      ptr++;
      header->sn |= (*ptr & 0xFF); // 8 bits SN
    }

    ptr++;
  } else {
    oset_error("Unsupported SN length");
    return 0;
  }

  // Read optional part
  if (header->si == (rlc_nr_si_field_t)last_segment ||
      header->si == (rlc_nr_si_field_t)neither_first_nor_last_segment) {
    // read SO
    header->so = (*ptr & 0xFF) << 8;
    ptr++;
    header->so |= (*ptr & 0xFF);
    ptr++;
  }

  // return consumed bytes
  return (ptr - payload);
}

/////////////////////////////////rx/////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/*static int modulus_rx(nr_rlc_entity_um_t *entity, int a)
{
	// as per 38.322 7.1, modulus base is rx_next_highest - window_size
	int r = a - (entity->rx_next_highest - entity->window_size);
	if (r < 0) r += entity->sn_modulus;
	return r % entity->sn_modulus;
}*/

// ??? x & (rx->mod - 1)
static uint32_t RX_MOD_NR_BASE(rlc_um_nr_rx *rx, uint32_t x)
{
	// 由于SN循环取值，因此会出现SN wrap around的问题。
	// 为避免SN wrap around时实际接收窗口缩小导致丢包，协议规定相关状态变量在做运算和比较时必须遵守以下两个原则从而保证实际接收窗口固定不变：
	// final value = [value from arithmetic operation] modulo 2^{[sn-FieldLength]}
	/* as per 38.322 7.1, modulus base is rx_next_highest - window_size */
	return (((x)-rx->RX_Next_Highest - rx->UM_Window_Size) % (rx->mod))
}

static int rlc_umd_pdu_free(void *rec, const void *key, int klen, const void *value)
{
	oset_hash_t *segments = (oset_hash_t *)rec;
	oset_hash_set(segments, key, sizeof(klen), NULL);

	rlc_umd_pdu_nr_t  *umd_pdu = (rlc_umd_pdu_nr_t *)value;
	oset_free(umd_pdu->buf);
	oset_free(umd_pdu);
	return 1;
}

static int rlc_umd_pdu_segments_free(void *rec, const void *key, int klen, const void *value)
{
	oset_hash_t *rx_window = (oset_hash_t *)rec;
	oset_hash_set(rx_window, key, sizeof(klen), NULL);

	rlc_umd_pdu_segments_nr_t  *pdu_segments = (rlc_umd_pdu_segments_nr_t *)value;
	oset_hash_do(rlc_umd_pdu_free, pdu_segments->segments, pdu_segments->segments);
	oset_hash_destroy(pdu_segments->segments);
	if(pdu_segments->sdu) oset_free(pdu_segments->sdu);
	oset_free(pdu_segments);
	return 1;
}

// Sec 5.2.2.2.1
static bool sn_in_reassembly_window(rlc_um_nr_rx *rx, const uint32_t sn)
{
	return (RX_MOD_NR_BASE(rx->RX_Next_Highest - rx->UM_Window_Size) <= RX_MOD_NR_BASE(sn) &&
	      RX_MOD_NR_BASE(sn) < RX_MOD_NR_BASE(rx->RX_Next_Highest));
}

// Sec 5.2.2.2.2
static bool sn_invalid_for_rx_buffer(rlc_um_nr_rx *rx, uint32_t sn)
{
	// 若SN值落在 [ RX_Next_Highest - UM_Window_Size , RX_Next_Reassembly) 范围内，则出窗，将PDU丢弃
	// 为什么UM敢把落在特定范围内叫做“出窗”？因为UM没有重传，sn永远是向前走的，不可能落在窗口的左侧。
	return (RX_MOD_NR_BASE(rx->RX_Next_Highest - rx->UM_Window_Size) <= RX_MOD_NR_BASE(sn) &&
		 RX_MOD_NR_BASE(sn) < RX_MOD_NR_BASE(rx->RX_Next_Reassembly));
}

static void update_total_sdu_length(rlc_umd_pdu_segments_nr_t *pdu_segments,
                                             rlc_umd_pdu_nr_t *rx_pdu)
{
	if (rx_pdu->header.si == (rlc_nr_si_field_t)last_segment) {
		pdu_segments->total_sdu_length = rx_pdu->header.so + rx_pdu->buf->N_bytes;
		RlcDebug("updating total SDU length for SN=%d to %d B", rx_pdu.header.sn, pdu_segments.total_sdu_length);
	}
}

static byte_buffer_t* strip_pdu_header(rlc_um_nr_pdu_header_t *header,
                                                uint8_t   *payload,
                                                uint32_t  nof_bytes)
{
	byte_buffer_t *sdu = byte_buffer_init();
	if (sdu == NULL) {
		RlcError("Couldn't allocate PDU");
		return NULL;
	}
	memcpy(sdu->msg, payload, nof_bytes);
	sdu->N_bytes = nof_bytes;

	// strip RLC header
	int header_len = rlc_um_nr_packed_length(header);
	sdu->msg += header_len;
	sdu->N_bytes -= header_len;
	return sdu;
}

// TS 38.322 v15.003 Section 5.2.2.2.4
static bool has_missing_byte_segment(rlc_um_nr_rx *rx, uint32_t sn)
{
	// is at least one missing byte segment of the RLC SDU associated with SN = RX_Next_Reassembly before the last byte of
	// all received segments of this RLC SDU
	return (NULL !=  oset_hash_get(rx->rx_window, &sn, sizeof(sn));
}

// Sect 5.2.2.2.3
static void handle_rx_buffer_update(rlc_um_base_rx *base_rx, uint16_t sn, rlc_umd_pdu_segments_nr_t *pdu)
{
  rlc_um_nr_rx *rx = base_rx->rx;
  rlc_um_base  *base = base_rx->base;

  if (NULL != pdu) {
    bool sdu_complete = false;
    oset_hash_index_t *hi = NULL;
    // iterate over received segments and try to assemble full SDU
    for (hi = oset_hash_first(pdu->segments); hi;){
	  uint16_t so = *(uint16_t *)oset_hash_this_key(hi);
	  rlc_umd_pdu_nr_t  *it = oset_hash_this_val(hi);
      RlcDebug("Have %s segment with SO=%d for SN=%d",
               rlc_nr_si_field_to_string(it->header.si),
               so,
               it->header.sn);
      if (it->header.so == pdu->next_expected_so) {
        if (pdu->next_expected_so == 0) {
          if (pdu->sdu == NULL) {
            // reuse buffer of first segment for final SDU
            pdu->sdu              = byte_buffer_dup(it->buf);
            pdu->next_expected_so = pdu->sdu->N_bytes;
            RlcDebug("Reusing first segment of SN=%d for final SDU", it->header.sn);
			oset_free(it->buf);
			oset_hash_set(pdu->segments, &it->header.so, sizeof(it->header.so), NULL);
          } else {
            RlcDebug("SDU buffer already allocated. Possible retransmission of first segment.");
            if (it->header.so != pdu->next_expected_so) {
              RlcError("Invalid PDU. SO doesn't match. Discarding all segments of SN=%d.", sn);
			  rlc_umd_pdu_segments_free(rx->rx_window, &sn, sizeof(sn), pdu);
			  oset_thread_mutex_lock(&base->metrics_mutex);
			  base->metrics.num_lost_pdus++;
			  oset_thread_mutex_unlock(&base->metrics_mutex);
              return;
            }
          }
        } else {
          if (it->buf->N_bytes > byte_buffer_get_tailroom(pdu->sdu)) {
            RlcError("Cannot fit RLC PDU in SDU buffer (tailroom=%d, len=%d), dropping both. Erasing SN=%d.",
                     byte_buffer_get_tailroom(pdu->sdu),
                     it->buf->N_bytes,
                     it->header.sn);
			rlc_umd_pdu_segments_free(rx->rx_window, &sn, sizeof(sn), pdu);
			oset_thread_mutex_lock(&base->metrics_mutex);
            base->metrics.num_lost_pdus++;
			oset_thread_mutex_unlock(&base->metrics_mutex);
            return;
          }

          // add this segment to the end of the SDU buffer
          memcpy(pdu->sdu->msg + pdu.sdu->N_bytes, it->buf->msg, it->buf->N_bytes);
          pdu->sdu->N_bytes += it->buf->N_bytes;
          pdu->next_expected_so += it->buf->N_bytes;
          RlcDebug("Appended SO=%d of SN=%d", it->header.so, it->header.sn);
		  rlc_umd_pdu_free(pdu->segments, &it->header.so, sizeof(it->header.so), it);

          if (pdu->next_expected_so == pdu->total_sdu_length) {
            // entire SDU has been received, it will be passed up the stack outside the loop
            sdu_complete = true;
            break;
          }
        }
      } else {
        // handle next segment
		hi = oset_hash_next(hi);
      }
    }

    if (sdu_complete) {
      // deliver full SDU to upper layers
      RlcInfo("Rx SDU (%d B)", pdu->sdu->N_bytes);
	  API_pdcp_rlc_write_ul_pdu(base->common.rnti, base->common.lcid, pdu->sdu);
      // delete PDU from rx_window
	  rlc_umd_pdu_segments_free(rx->rx_window, &sn, sizeof(sn), pdu);

      // find next SN in rx buffer
	  //若sn = RX_Next_Reassembly，则更新RX_Next_Reassembly为目前下一个最早的SN值。
      if (sn == rx->RX_Next_Reassembly) {
        if (0 == oset_hash_count(rx->rx_window)) {
          // no further segments received
          rx->RX_Next_Reassembly = rx->RX_Next_Highest;
        } else {
		  for (oset_hash_index_t *h = oset_hash_first(rx->rx_window); h;h = oset_hash_next(h)){
			uint16_t k_sn = *(uint16_t *)oset_hash_this_key(h);
			rlc_umd_pdu_segments_nr_t  *it = oset_hash_this_val(h);
		    //只要进入该分支，必定有分片，full_sdu报文已经被发送出去。
            RlcDebug("SN=%d has %zd segments", k_sn, oset_hash_count(it->segments));
            if (RX_MOD_NR_BASE(k_sn) > RX_MOD_NR_BASE(rx->RX_Next_Reassembly)) {
              rx->RX_Next_Reassembly = k_sn;
              break;
            }
          }
        }
        RlcDebug("Updating RX_Next_Reassembly=%d", rx->RX_Next_Reassembly);
      }
    } else if (!sn_in_reassembly_window(sn)) {
      // 此处只可能sn > RX_Next_Highest,出窗的情况已经在sn_invalid_for_rx_buffer判断过
      // SN outside of rx window
	  // 若sn大于RX_Next_Highest，则RX_Next_Highest设为sn+1，相应地 reassembly window也向前移。
	  // 因为前移而落到reassembly window之外的PDU，将被丢弃。
      rx->RX_Next_Highest = (sn + 1) % rx->mod; // update RX_Next_highest
      RlcDebug("Updating RX_Next_Highest=%d", rx->RX_Next_Highest);

      // drop all SNs outside of new rx window
      for (oset_hash_index_t *h = oset_hash_first(rx->rx_window); h;){
	  	uint16_t k_sn = *(uint16_t *)oset_hash_this_key(h);
	  	rlc_umd_pdu_segments_nr_t  *it = oset_hash_this_val(h);
        if (!sn_in_reassembly_window(it)) {
          RlcInfo("SN=%d outside rx window [%d:%d] - discarding",
                  k_sn,
                  rx->RX_Next_Highest - rx->UM_Window_Size,
                  rx->RX_Next_Highest);
		  rlc_umd_pdu_segments_free(rx->rx_window, &k_sn, sizeof(k_sn), it);
		  oset_thread_mutex_lock(&base->metrics_mutex);
		  base->metrics.num_lost_pdus++;
		  oset_thread_mutex_unlock(&base->metrics_mutex);
        } else {
          h = oset_hash_next(h);
        }
      }

	  // 若前移后RX_Next_Reassembly也落到重组窗口外，则将其值更新为
	  // 大于等于RX_Next_Highest - UM_Window_Size，但还未被重组并传往上层的最早一个SN值。
      if (!sn_in_reassembly_window(rx->RX_Next_Reassembly)) {
        // update RX_Next_Reassembly to first SN that has not been reassembled and delivered
        for (oset_hash_index_t *h = oset_hash_first(rx->rx_window); h; h = oset_hash_next(h)){
		  uint16_t k_sn = *(uint16_t *)oset_hash_this_key(h);
          if (k_sn >= RX_MOD_NR_BASE(rx->RX_Next_Highest - rx->UM_Window_Size)) {
            rx->RX_Next_Reassembly = k_sn;
            RlcDebug("Updating RX_Next_Reassembly=%d", rx->RX_Next_Reassembly);
            break;
          }
        }
      }
    }

	// 若t-Reassembly正在运行:
	// 1、如果RX_Timer_Trigger <= RX_Next_Reassembly(认为接收窗口内小于RX_Next_Reassembly的包都收到了，不需要针对这些包开启定时器)，或者;
	// 2、RX_Timer_Trigger落在reassembly window之外并且不等于RX_Next_Highest(接收窗口下边界);
	// 3、RX_Next_Highest = RX_Next_Reassembly + 1，且SN = RX_Next_Reassembly的SDU已收齐;
	// 则停止并重置t-Reassembly
    if (true == rx->reassembly_timer.running) {
      if (rx->RX_Timer_Trigger <= rx->RX_Next_Reassembly ||
          (!sn_in_reassembly_window(rx->RX_Timer_Trigger) && rx->RX_Timer_Trigger != rx->RX_Next_Highest) ||
          ((rx->RX_Next_Highest == rx->RX_Next_Reassembly + 1) && !has_missing_byte_segment(rx->RX_Next_Reassembly))) {
        RlcDebug("stopping reassembly timer");
        gnb_timer_stop(rx->reassembly_timer);
      }
    }

	// 若t-Reassembly没有运行(包括因为前面步骤导致的停止运行)，且
	// RX_Next_Highest > RX_Next_Reassembly+1（即至少还有一个 SN < RX_Next_Highest没收到，应启动定时器等待未接收的PDU）
	// RX_Next_Highest = RX_Next_Reassembly + 1，且SN = RX_Next_Reassembly的SDU没收齐;
	// 则启动t-Reassembly，并将RX_Timer_Trigger设置为RX_Next_Highest。
    if (false == rx->reassembly_timer.running) {
      if ((RX_MOD_NR_BASE(rx->RX_Next_Highest) > RX_MOD_NR_BASE(rx->RX_Next_Reassembly + 1)) ||
          ((RX_MOD_NR_BASE(rx->RX_Next_Highest) == RX_MOD_NR_BASE(rx->RX_Next_Reassembly + 1)) &&
           has_missing_byte_segment(rx->RX_Next_Reassembly))) {
        RlcDebug("Starting reassembly timer for SN=%d", sn);
        gnb_timer_start(rx->reassembly_timer, base_rx->cfg.um_nr.t_reassembly_ms);
        rx->RX_Timer_Trigger = rx->RX_Next_Highest;
      }
    }
  } else {
    RlcError("SN=%d does not exist in Rx buffer", sn);
  }
}

/////////////////////////////////tx////////////////////////////////////////

static byte_buffer_t* tx_sdu_queue_read(rlc_um_base_tx *base_tx)
{
	rlc_um_base_tx_sdu_t *node = oset_list_first(&base_tx->tx_sdu_queue);
	if(node){
		byte_buffer_t *sdu = node->buffer;
		oset_list_remove(&base_tx->tx_sdu_queue, node);
		oset_free(node);
		oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
		base_tx->unread_bytes -= sdu->N_bytes;
		oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
		return sdu;
	}
	return NULL;
}

static uint32_t build_dl_data_pdu(rlc_um_base_tx *base_tx, uint8_t *payload, uint32_t nof_bytes)
{
  uint32_t ret = 0;

  // Sanity check (we need at least 2Byte for a SDU)
  if (nof_bytes < 2) {
    RlcWarning("Cannot build a PDU with %d byte.", nof_bytes);
    return 0;
  }

  byte_buffer_t *pdu = byte_buffer_init();

  rlc_um_nr_tx *tx = base_tx->tx;

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
  ret = pdu->N_bytes;
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

#if UM_RXTX
static void empty_queue(rlc_um_base_tx *base_tx)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	// Drop all messages in TX queue
	rlc_um_base_tx_sdu_t *next_node, *node = NULL;
	oset_list_for_each_safe(&base_tx->tx_sdu_queue, next_node, node){
			byte_buffer_t *sdu = node->buffer;
			oset_list_remove(&base_tx->tx_sdu_queue, node);
			oset_free(node);
			oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
			base_tx->unread_bytes -= sdu->N_bytes;
			oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
			oset_free(sdu)
	}

	// deallocate SDU that is currently processed
	if(base_tx->tx_sdu){
		oset_free(base_tx->tx_sdu);
		base_tx->tx_sdu = NULL;
	}

	oset_assert(0 == base_tx->unread_bytes);
	oset_assert(0 == oset_list_count(base_tx->tx_sdu_queue));
	oset_thread_mutex_unlock(&base_tx->mutex);
}

static uint32_t rlc_um_base_tx_discard_sdu(rlc_um_base_tx *base_tx, uint32_t discard_sn)
{
	bool discarded = false;

	oset_thread_mutex_lock(&base_tx->mutex);
	rlc_um_base_tx_sdu_t *next_node, *node = NULL;
	oset_list_for_each_safe(&base_tx->tx_sdu_queue, next_node, node){
		if (node != NULL && node->buffer->md.pdcp_sn == discard_sn) {
			byte_buffer_t *sdu = node->buffer;
			oset_list_remove(&base_tx->tx_sdu_queue, node);
			oset_free(node);
			oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
			base_tx->unread_bytes -= sdu->N_bytes;
			oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
			oset_free(sdu);
			node = NULL;
			discarded = true;
		}
	}	
	oset_thread_mutex_unlock(&base_tx->mutex);
	// Discard fails when the PDCP PDU is already in Tx window.
	RlcInfo("%s PDU with PDCP_SN=%d", discarded ? "Discarding" : "Couldn't discard", discard_sn);
}

static uint32_t rlc_um_base_tx_get_buffer_state(rlc_um_base_tx *base_tx, uint32_t prio_tx_queue)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	// Bytes needed for tx SDUs
	uint32_t n_sdus  = oset_list_count(&base_tx->tx_sdu_queue);
	uint32_t n_bytes = base_tx->unread_bytes;
	if (base_tx->tx_sdu) {
		//若当前存在分片
		n_sdus++;
		n_bytes += base_tx->tx_sdu->N_bytes;
	}

	// Room needed for header extensions? (integer rounding)（整数舍入）为了头部扩展
	if (n_sdus > 1) {
		n_bytes += ((n_sdus - 1) * 1.5) + 0.5;
	}

	// Room needed for fixed header?
	if (n_bytes > 0) {
		n_bytes += (base_tx->cfg.um.is_mrb) ? 2 : 3;
	}

	if (base_tx->bsr_callback) {
		base_tx->bsr_callback(base_tx->base->common.rnti, base_tx->base->common.lcid, n_bytes, prio_tx_queue);
	}
	oset_thread_mutex_unlock(&base_tx->mutex);
	return n_bytes;
}


static uint32_t rlc_um_base_tx_build_dl_data_pdu(rlc_um_base_tx *base_tx, uint8_t *payload, uint32_t nof_bytes)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	RlcDebug("MAC opportunity - %d bytes", nof_bytes);

	if (base_tx->tx_sdu == NULL && oset_list_empty(&base_tx->tx_sdu_queue)) {
		RlcInfo("No data available to be sent");
		return 0;
	}
	
	uint32_t ret = build_dl_data_pdu(base_tx, payload, nof_bytes);
	oset_thread_mutex_unlock(&base_tx->mutex);

	return ret;

	{
	  // Sanity check (we need at least 2B for a SDU)
	}
}

static int rlc_um_base_tx_write_dl_sdu(rlc_um_base_tx *base_tx, byte_buffer_t *sdu)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	if (sdu) {
		uint8_t*   msg_ptr   = sdu->msg;
		uint32_t   nof_bytes = sdu->N_bytes;
		int count = oset_list_count(&base_tx->tx_sdu_queue);

		if((uint32_t)count <= base_tx->cfg.tx_queue_length){
			rlc_um_base_tx_sdu_t *sdu_node = oset_malloc(rlc_um_base_tx_sdu_t);
			oset_assert(sdu_node);
			sdu_node->buffer = byte_buffer_dup(sdu);
			oset_list_add(&base_tx->tx_sdu_queue, sdu_node);
			oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
			base_tx->unread_bytes += sdu->N_bytes;
			oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
			RlcHexInfo(msg_ptr, nof_bytes, "Tx SDU (%d B, tx_sdu_queue_len=%d)", nof_bytes, oset_list_count(&base_tx->tx_sdu_queue));
			oset_thread_mutex_unlock(&base_tx->mutex);
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

	oset_thread_mutex_unlock(&base_tx->mutex);
	return OSET_ERROR;
}

static void rlc_um_base_tx_stop(rlc_um_base_tx *base_tx)
{
	oset_thread_mutex_destroy(&base_tx->unread_bytes_mutex);
	oset_thread_mutex_destroy(&base_tx->mutex);
}

static void rlc_um_base_tx_init(rlc_um_base_tx *base_tx)
{
	oset_thread_mutex_init(&base_tx->unread_bytes_mutex);
	oset_thread_mutex_init(&base_tx->mutex);
}

//////////////////////////////////////////////////////////////////////////////////
// 当t-Reassembly超时，则：
// 将RX_Next_Reassembly更新为不小于RX_Timer_Trigger，且未重组的第一个SN值。
// 丢弃所有SN < 新RX_Next_Reassembly的PDU。
// 若此时RX_Next_Highest >= RX_Next_Reassembly+1，且SN = RX_Next_Reassembly的SDU没收齐;
// 则启动t-Reassembly，并将RX_Timer_Trigger设置为 RX_Next_Highest
static void rlc_um_base_rx_timer_expired(void *data)
{
	rlc_um_base_rx *base_rx = (rlc_um_base_rx *)data;
	rlc_um_nr_rx *rx = base_rx->rx;
	rlc_um_base *base = base_rx->base;

	oset_thread_mutex_lock(&base_rx->mutex);

    RlcDebug("reassembly timeout expiry for SN=%d - updating RX_Next_Reassembly and reassembling", rx->RX_Next_Reassembly);

	oset_thread_mutex_lock(&base->metrics_mutex);
    base->metrics.num_lost_pdus++;
	oset_thread_mutex_unlock(&base->metrics_mutex);

    // update RX_Next_Reassembly to the next SN that has not been reassembled yet
    // 将RX_Next_Reassembly更新为不小于RX_Timer_Trigger，且未重组的第一个SN值。
    rx->RX_Next_Reassembly = rx->RX_Timer_Trigger;
    while (RX_MOD_NR_BASE(rx, rx->RX_Next_Reassembly) < RX_MOD_NR_BASE(rx, rx->RX_Next_Highest)) {
      rx->RX_Next_Reassembly = (rx->RX_Next_Reassembly + 1) % rx->mod;
	  RlcDebug("RX_Next_Reassembly=%d, RX_Timer_Trigger=%d, RX_Next_Highest=%d, t_Reassembly=%s",
			 rx->RX_Next_Reassembly,
			 rx->RX_Timer_Trigger,
			 rx->RX_Next_Highest,
			 rx->reassembly_timer.running ? "running" : "stopped");
    }

    // discard all segments with SN < updated RX_Next_Reassembly
	// 丢弃所有SN < 新RX_Next_Reassembly的PDU
    for (oset_hash_index_t *hi = oset_hash_first(rx->rx_window); hi;){
	  uint16_t k_sn = *(uint16_t *)oset_hash_this_key(hi);
	  rlc_umd_pdu_segments_nr_t  *it = oset_hash_this_val(hi);
      if (RX_MOD_NR_BASE(rx, it) < RX_MOD_NR_BASE(rx, rx->RX_Next_Reassembly)) {
		rlc_umd_pdu_segments_free(rx->rx_window, &k_sn, sizeof(k_sn), it);
      } else {
		hi = oset_hash_next(hi);
      }
    }

	// RX_Next_Highest > RX_Next_Reassembly + 1;(这个条件说明还有包没有重组递交上层)
	// RX_Next_Highest = RX_Next_Reassembly + 1 且SN = RX_Next_Reassembly的SDU没收齐;
	// 则启动t-Reassembly，并将RX_Timer_Trigger设置为 RX_Next_Highest
    if (RX_MOD_NR_BASE(rx, rx->RX_Next_Highest) > RX_MOD_NR_BASE(rx, rx->RX_Next_Reassembly + 1) ||
        ((RX_MOD_NR_BASE(rx, rx->RX_Next_Highest) == RX_MOD_NR_BASE(rx, rx->RX_Next_Reassembly + 1) &&
          has_missing_byte_segment(rx, rx->RX_Next_Reassembly)))) {
      RlcDebug("starting reassembly timer for SN=%d", base_rx->rb_name, rx->RX_Next_Reassembly);
	  gnb_timer_start(rx->reassembly_timer, base_rx->cfg.um_nr.t_reassembly_ms);
      rx->RX_Timer_Trigger = rx->RX_Next_Highest;
    }

	RlcDebug("RX_Next_Reassembly=%d, RX_Timer_Trigger=%d, RX_Next_Highest=%d, t_Reassembly=%s",
		   rx->RX_Next_Reassembly,
		   rx->RX_Timer_Trigger,
		   rx->RX_Next_Highest,
		   rx->reassembly_timer.running ? "running" : "stopped");

	oset_thread_mutex_unlock(&base_rx->mutex);
}

static void rlc_um_base_rx_handle_data_pdu(rlc_um_base_rx *base_rx, uint8_t* payload, uint32_t nof_bytes)
{
  rlc_um_nr_rx *rx = base_rx->rx;
  rlc_um_base *base = base_rx->base;

  oset_thread_mutex_lock(&base_rx->mutex);

  rlc_um_nr_pdu_header_t header = {0};
  rlc_um_nr_read_data_pdu_header(payload, nof_bytes, base_rx->cfg.um_nr.sn_field_length, &header);
  RlcHexDebug(payload, nof_bytes, "Rx data PDU (%d B)", nof_bytes);

  // 若该UMD PDU不包含SN（说明这是个完整包），则去掉RLC header，将RLC SDU发给上层
  // 若SN值落在 [RX_Next_Highest - UM_Window_Size , RX_Next_Reassembly) 范围内，则出窗，将PDU丢弃
  // 否则，将PDU放入reception buffer

  // check if PDU contains a SN
  if (header.si == (rlc_nr_si_field_t)full_sdu) {
    // copy full PDU into buffer
    byte_buffer_t *sdu = strip_pdu_header(header, payload, nof_bytes);

    // deliver to PDCP
    RlcInfo("Rx SDU (%d B)", sdu->N_bytes);
    API_pdcp_rlc_write_ul_pdu(base->common.rnti, base->common.lcid, sdu);
	oset_free(sdu);
  } else if (sn_invalid_for_rx_buffer(rx, header.sn)) {
  	// (RX_Next_Highest – UM_Window_Size) <= SN < RX_Next_Reassembly, droping
    RlcInfo("Discarding SN=%d", header.sn);
	oset_thread_mutex_lock(&base->metrics_mutex);
	base->metrics.num_lost_pdus++;
	oset_thread_mutex_unlock(&base->metrics_mutex);
    // Nothing else to do here ..
  } else {
  	// rlc片段处理
    // place PDU in receive buffer
    rlc_umd_pdu_nr_t *rx_pdu = oset_malloc(rlc_umd_pdu_nr_t);
	oset_assert(rx_pdu);
    rx_pdu->header           = header;
    rx_pdu->buf              = strip_pdu_header(header, payload, nof_bytes);

    // check if this SN is already present in rx buffer
    rlc_umd_pdu_segments_nr_t *um_pdu_segments = oset_hash_get(rx->rx_window, &header.sn, sizeof(header.sn));
    if (NULL == um_pdu_segments) {
	  // rlc sn首个片段
      // first received segment of this SN, add to rx buffer
      RlcHexDebug(rx_pdu->buf->msg,
                  rx_pdu->buf->N_bytes,
                  "placing %s segment of SN=%d (%d B) in Rx buffer",
                  rlc_nr_si_field_to_string(header.si),
                  header.sn,
                  rx_pdu->buf->N_bytes);

      rlc_umd_pdu_segments_nr_t *pdu_segments = oset_malloc(rlc_umd_pdu_segments_nr_t);
	  oset_assert(pdu_segments);
	  pdu_segments->segments = oset_hash_make();

      update_total_sdu_length(pdu_segments, rx_pdu);

	  oset_hash_set(pdu_segments->segments, &rx_pdu->header.so, sizeof(rx_pdu->header.so), NULL);
	  oset_hash_set(pdu_segments->segments, &rx_pdu->header.so, sizeof(rx_pdu->header.so), rx_pdu);

	  oset_hash_set(rx->rx_window, &rx_pdu->header.sn, sizeof(rx_pdu->header.sn), NULL);
	  oset_hash_set(rx->rx_window, &rx_pdu->header.sn, sizeof(rx_pdu->header.sn), pdu_segments);
    } else {
      // other segment for this SN already present, update received data
      RlcHexDebug(rx_pdu->buf->msg,
                  rx_pdu->buf->N_bytes,
                  "updating SN=%d at SO=%d with %d B",
                  rx_pdu->header.sn,
                  rx_pdu->header.so,
                  rx_pdu->buf->N_bytes);

      rlc_umd_pdu_segments_nr_t *pdu_segments = um_pdu_segments;

      // calculate total SDU length
      update_total_sdu_length(pdu_segments, rx_pdu);

      // append to list of segments
	  oset_hash_set(pdu_segments->segments, &rx_pdu->header.so, sizeof(rx_pdu->header.so), NULL);
	  oset_hash_set(pdu_segments->segments, &rx_pdu->header.so, sizeof(rx_pdu->header.so), rx_pdu);
    }

    // handle received segments
    handle_rx_buffer_update(base_rx, header.sn, um_pdu_segments);
  }

  RlcDebug("RX_Next_Reassembly=%d, RX_Timer_Trigger=%d, RX_Next_Highest=%d, t_Reassembly=%s",
		 rx->RX_Next_Reassembly,
		 rx->RX_Timer_Trigger,
		 rx->RX_Next_Highest,
		 rx->reassembly_timer.running ? "running" : "stopped");

  oset_thread_mutex_unlock(&base_rx->mutex);
}

static void rlc_um_base_rx_stop(rlc_um_base_rx *base_rx)
{
	//todo
	oset_thread_mutex_destroy(&base_rx->mutex);
}

static void rlc_um_base_rx_init(rlc_um_base_rx *base_rx)
{
	oset_thread_mutex_init(&base_rx->mutex);
	//base_rx->metrics = &base_rx->base->metrics;
	//base_rx->lcid = base_rx->base->common.lcid;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
static void rlc_um_base_init(rlc_um_base *base, uint32_t lcid_, uint16_t rnti_)
{
	rlc_common_init(&base->common, NULL, rnti_, lcid_, (rlc_mode_t)um, NULL);
	base->rx_enabled = false;
	base->tx_enabled = false;
	base->metrics = {0};
	oset_thread_mutex_init(&base->metrics_mutex);
}

static void rlc_um_base_stop(rlc_um_base *base)
{
	rlc_common_destory(&base->common);
	base->rx_enabled = false;
	base->tx_enabled = false;
	oset_thread_mutex_destroy(&base->metrics_mutex);
	base->metrics = {0};
}

/*
一、当一个UMD PDU从下层被接收，则UM RLC实体应：

	1、若该UMD PDU不包含SN（说明这是个完整包），则去掉RLC header，将RLC SDU发给上层
	2、若SN值落在 [ RX_Next_Highest - UM_Window_Size , RX_Next_Reassembly) 范围内，则出窗，将PDU丢弃
	3、否则，将PDU放入reception buffer
	为什么UM敢把落在特定范围内叫做“出窗”？因为UM没有重传，sn永远是向前走的，不可能落在窗口的左侧。
	落在窗口左侧的pdu会被视为正常包接收，并移动窗口。

二、当一个SN = x 的 UMD PDU 放入 Reception buffer时，接收端会按照下列步骤操作：

	1、若至此SN = x 的SDU的所有byte都已收到，则去掉RLC header，并将SDU重组传给上层。
	2、若x = RX_Next_Reassembly，则更新RX_Next_Reassembly为目前下一个最早的SN值。
	3、若x大于RX_Next_Highest，则RX_Next_Highest设为x+1，相应地 reassembly window也向前移。 因为前移而落到reassembly window之外的PDU，将被丢弃。
	   若前移后RX_Next_Reassembly也落到重组窗口外，则将其值更新为 大于等于RX_Next_Highest - UM_Window_Size，但还未被重组并传往上层的最早一个SN值。
	4、若t-Reassembly正在运行
		如果RX_Timer_Trigger <= RX_Next_Reassembly，或者;
		RX_Timer_Trigger落在reassembly window之外并且不等于RX_Next_Highest;
		RX_Next_Highest = RX_Next_Reassembly + 1，且SN = RX_Next_Reassembly的SDU已收齐;
		则停止并重置t-Reassembly
	5、若t-Reassembly没有运行(包括因为前面步骤导致的停止运行)，且
		RX_Next_Highest > RX_Next_Reassembly+1（即至少还有一个 SN < RX_Next_Highest没收到，应启动定时器等待未接收的PDU）
		RX_Next_Highest = RX_Next_Reassembly + 1，且SN = RX_Next_Reassembly的SDU没收齐;
		则启动t-Reassembly，并将RX_Timer_Trigger设置为RX_Next_Highest。

三、当t-Reassembly超时，则：

	1、将RX_Next_Reassembly更新为不小于RX_Timer_Trigger，且未重组的第一个SN值。
	2、丢弃所有SN < 新RX_Next_Reassembly的PDU。
	3、若此时RX_Next_Highest >= RX_Next_Reassembly+1，且SN = RX_Next_Reassembly的SDU没收齐;
	   则启动t-Reassembly，并将RX_Timer_Trigger设置为 RX_Next_Highest
*/
static void rlc_um_base_write_ul_pdu(rlc_um_base *base, rlc_um_base_rx *base_rx, uint8_t *payload, uint32_t nof_bytes)
{
	if (base_rx->rx && base->rx_enabled) {
		{
			oset_thread_mutex_lock(&base->metrics_mutex);
			base->metrics.num_rx_pdus++;
			base->metrics.num_rx_pdu_bytes += nof_bytes;
			oset_thread_mutex_unlock(&base->metrics_mutex);
		}
		rlc_um_base_rx_handle_data_pdu(base_rx, payload, nof_bytes);
	}
}

static uint32_t rlc_um_base_read_dl_pdu(rlc_um_base *base, rlc_um_base_tx *base_tx, uint8_t *payload, uint32_t nof_bytes)
{
	if (base_tx->tx && base->tx_enabled) {
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

static void rlc_um_base_write_dl_sdu(rlc_um_base *base, rlc_um_base_tx *base_tx, byte_buffer_t *sdu)
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

#if UM_RXTX
/////////////////////////////////rlc_um_nr_rx/////////////////////////////////
static void rlc_um_nr_rx_reset(rlc_um_nr_rx *rx)
{
	rx->RX_Next_Reassembly = 0;
	rx->RX_Timer_Trigger   = 0;
	rx->RX_Next_Highest    = 0;

	// Drop all messages in RX window
	oset_hash_do(rlc_umd_pdu_segments_free, rx->rx_window, rx->rx_window);
	oset_hash_clear(rx->rx_window);

	// stop timer
	if (rx->reassembly_timer.running) {
		gnb_timer_stop(rx->reassembly_timer);
	}
}

static void rlc_um_nr_rx_init(rlc_um_nr_rx *rx, rlc_um_base *base)
{
	rx->base_rx.base = base;
	rx->base_rx.rx	= rx;
	rx->rx_window = oset_hash_make();
	rlc_um_base_rx_init(&rx->base_rx, base)
}

static void rlc_um_nr_rx_stop(rlc_um_nr_rx *rx)
{
	oset_thread_mutex_lock(&rx->base_rx.mutex);
	rlc_um_nr_rx_reset(rx);
	oset_hash_destroy(rx->rx_window);

	if(rx->reassembly_timer) gnb_timer_delete(rx->reassembly_timer);
	oset_thread_mutex_unlock(&rx->base_rx.mutex);
	rlc_um_base_rx_stop(&rx->base_rx);
}

static void rlc_um_nr_rx_reestablish(rlc_um_nr_rx *rx)
{
	// drop all SDUs, SDU segments, PDUs and reset timers
	rlc_um_nr_rx_reset(rx);
}

static bool rlc_um_nr_rx_configure(rlc_um_nr_rx *rx, rlc_config_t *cnfg_, char *rb_name_)
{
	rx->base_rx.rb_name = rb_name_;
	rx->base_rx.cfg = *cnfg_;

	//2^{[sn-FieldLength]}//模
	rx->mod  = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 64 : 4096;
	//[0, 2^{[sn-FieldLength]} – 1]
	rx->UM_Window_Size = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 32 : 2048;

	// configure timer
	if (rx->base_rx.cfg.um_nr.t_reassembly_ms > 0) {
		rx->reassembly_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_um_base_rx_timer_expired, &rx->base_rx);
	}

	return true;
}

/////////////////////////////////rlc_um_nr_tx/////////////////////////////////
static void rlc_um_nr_tx_reset(rlc_um_nr_tx *tx)
{
	tx->TX_Next = 0;
	tx->next_so = 0;
}

static void suspend(rlc_um_nr_tx *tx)
{
	empty_queue(&tx->base_tx);
	rlc_um_nr_tx_reset(tx);
}

static void rlc_um_nr_tx_init(rlc_um_nr_tx *tx, rlc_um_base *base)
{
	tx->base_tx.base = base;
	tx->base_tx.tx  = tx;
	rlc_um_base_tx_init(&tx->base_tx);
}

static void rlc_um_nr_tx_stop(rlc_um_nr_tx *tx)
{
	suspend(tx);
	rlc_um_base_tx_stop(&tx->base_tx);
}

static void rlc_um_nr_tx_reestablish(rlc_um_nr_tx *tx)
{
	suspend(tx);
}

static bool rlc_um_nr_tx_configure(rlc_um_nr_tx *tx, rlc_config_t *cnfg_, char *rb_name_)
{
	tx->base_tx.rb_name = rb_name_;
	tx->base_tx.cfg = *cnfg_;

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

	oset_list_init(&tx->base_tx.tx_sdu_queue);
	//oset_assert(oset_list_count(tx->base_tx.tx_sdu_queue) <= cnfg_->tx_queue_length);

	return true;
}

#endif
//////////////////////////////////////////////////////////////////////////////
////////////////////////////////rlc_um_nr/////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void rlc_um_nr_stop(rlc_common *um_common)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;

	rlc_um_nr_rx_stop(&um->rx);
	rlc_um_nr_tx_stop(&um->tx);
	rlc_um_base_stop(&um->base);
}

void rlc_um_nr_get_buffer_state(rlc_common *um_common, uint32_t *newtx_queue, uint32_t *prio_tx_queue)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;

	*prio_tx_queue = 0;
	*newtx_queue   = rlc_um_base_tx_get_buffer_state(&um->tx.base_tx, *prio_tx_queue);
}

bool rlc_um_nr_configure(rlc_common *um_common, rlc_config_t *cnfg_)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;

	// store config
	um->base.cfg = *cnfg_;

	// determine bearer name and configure Rx/Tx objects
	um->base.common.rb_name = oset_msprintf("DRB%s", um->base.cfg.um_nr.bearer_id);

	rlc_um_nr_rx_init(&um->rx, &um->base);
	if (! rlc_um_nr_rx_configure(&um->rx, &um->base.cfg, um->base.common.rb_name)) {
		return false;
	}

	rlc_um_nr_tx_init(&um->tx, &um->base);
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

void rlc_um_nr_set_bsr_callback(rlc_common *um_common, bsr_callback_t callback)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;

	um->tx.base_tx.bsr_callback = callback;
}

void rlc_um_nr_reset_metrics(rlc_common *um_common)
{
	rlc_um_nr *um = (rlc_tm *)um_common;

	oset_thread_mutex_lock(&um->base.metrics_mutex);
	um->base.metrics = {0};
	oset_thread_mutex_unlock(&um->base.metrics_mutex);
}

void rlc_um_nr_reestablish(rlc_common *um_common)
{
	rlc_um_nr *um = (rlc_tm *)um_common;

	um->base.tx_enabled = false;

	rlc_um_nr_tx_reestablish(&um->tx); // calls stop and enables tx again

	rlc_um_nr_rx_reestablish(&um->rx); // nothing else needed

	um->base.tx_enabled = true;
}

rlc_bearer_metrics_t rlc_um_nr_get_metrics(rlc_common *um_common)
{
	rlc_um_nr *um = (rlc_tm *)um_common;

	return um->base.metrics;
}

// 关于PULL window，接收RLC实体维护一个接收窗口如下：
// (RX_Next_Highest – UM_Window_Size) <= SN <RX_Next_Highest
// 接收窗口只能依赖于接收窗口上边界状态变量(RX_Next_Highest)更新才能移动
void rlc_um_nr_write_ul_pdu(rlc_common *um_common, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;

	rlc_um_base_write_ul_pdu(&um->base, &um->rx.base_rx, payload, nof_bytes);
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

void rlc_um_nr_discard_sdu(rlc_common *um_common, uint32_t discard_sn)
{
	rlc_um_nr *um = (rlc_um_nr *)um_common;

	if (!um->base.tx_enabled) {
		return;
	}

	rlc_um_base_tx_discard_sdu(&um->tx.base_tx, discard_sn);
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

