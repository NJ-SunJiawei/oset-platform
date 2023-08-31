/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "pdcp/pdcp_entity_nr.h"
#include "rrc/rrc.h"
#include "rlc/rlc.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-pdcpNR"

static void pass_to_upper_layers(pdcp_entity_nr *pdcp_nr, pdcp_nr_pdu_t *pdu)
{
  pdcp_nr->base.metrics.num_rx_pdu_bytes += pdu->buffer->N_bytes;
  pdcp_nr->base.metrics.num_rx_pdus++;

  // gnb_rrc_task_handle??? push queue
  if (is_srb(&pdcp_nr->base)) {
    API_rrc_pdcp_write_ul_pdu(pdcp_nr->base.rnti, pdcp_nr->base.lcid, pdu->buffer);
  } else {
    gw->write_pdu(lcid, pdu);
  }

  oset_free(pdu->buffer);
  oset_free(pdu);
}


// Deliver all consecutively associated COUNTs.
// Update RX_NEXT after submitting to higher layers
static void deliver_all_consecutive_counts(pdcp_entity_nr *pdcp_nr)
{
  for (pdcp_nr_pdu_t *pdu = oset_list_first(&pdcp_nr->reorder_list);\
		(pdu) && (pdu->count == pdcp_nr->rx_deliv);\
        pdu = oset_list_next(pdu)){
    oset_debug("Delivering SDU with RCVD_COUNT %u", pdu->count);

    // Check RX_DELIV overflow
    if (pdcp_nr->rx_overflow) {
      oset_warn("RX_DELIV has overflowed. Droping packet");
      return;
    }

	// pdcp_nr->rx_deliv = -1 (0xFFFFFFFF)
    if (pdcp_nr->rx_deliv + 1 == 0) {
      pdcp_nr->rx_overflow = true;
    }

    // Pass PDCP SDU to the next layers
    pass_to_upper_layers(pdcp_nr, pdu);

    // Update RX_DELIV
    pdcp_nr->rx_deliv = pdcp_nr->rx_deliv + 1;
  }
}


/*
 * Timers
 */
// Reordering Timer Callback (t-reordering)
static void reordering_callback(void *data)
{
	pdcp_entity_nr *pdcp_nr = (pdcp_entity_nr *)data;

	oset_info("Reordering timer expired. RX_REORD=%u, re-order queue size=%ld", pdcp_nr->rx_reord, oset_list_count(&pdcp_nr->reorder_list));

	// Deliver all PDCP SDU(s) with associated COUNT value(s) < RX_REORD
    for (pdcp_nr_pdu_t *pdu = oset_list_first(&pdcp_nr->reorder_list);\
		(pdu) && (pdu->count < pdcp_nr->rx_reord);\
        pdu = oset_list_next(pdu)){
		    // Deliver to upper layers
		    pass_to_upper_layers(pdcp_nr, pdu);
			oset_list_remove(&pdcp_nr->reorder_list, pdu);
		}	


	// Update RX_DELIV to the first PDCP SDU not delivered to the upper layers
	pdcp_nr->rx_deliv = pdcp_nr->rx_reord;

	// Deliver all PDCP SDU(s) consecutively associated COUNT value(s) starting from RX_REORD
	deliver_all_consecutive_counts(pdcp_nr);

	if (pdcp_nr->rx_deliv < pdcp_nr->rx_next) {
		oset_debug("Updating RX_REORD to %ld. Old RX_REORD=%ld, RX_DELIV=%ld",
		                     pdcp_nr->rx_next,
		                     pdcp_nr->rx_reord,
		                     pdcp_nr->rx_deliv);
		pdcp_nr->rx_reord = pdcp_nr->rx_next;
		gnb_timer_start(pdcp_nr->reordering_timer, pdcp_nr->base.cfg.t_reordering);
	}
}

// Discard Timer Callback (discardTimer)
static void discard_callback(void *data)
{
  discard_timer_t *discard_node = (discard_timer_t *)data;

  oset_debug("Discard timer expired for PDU with SN=%d", discard_node->discard_sn);

  // Notify the RLC of the discard. It's the RLC to actually discard, if no segment was transmitted yet.
  API_rlc_pdcp_discard_sdu(discard_node->pdcp_nr->base.rnti,
  							discard_node->pdcp_nr->base.lcid,
  							discard_node->discard_sn);

  // Remove timer from map
  // NOTE: this will delete the callback. It *must* be the last instruction.
  gnb_timer_delete(discard_node->discard_timer);
  oset_list_remove(&discard_node->pdcp_nr->discard_timers_list, discard_node);
  oset_free(discard_node);
}


static int count_compare(pdcp_nr_pdu_t *pdu1, pdcp_nr_pdu_t *pdu2)
{
    if (pdu1->count == pdu2->count)
        return 0;
    else if (pdu1->count < pdu2->count)
        return -1;
    else
        return 1;
}

////////////////////////////////////////////////////////////////////////////
bool pdcp_entity_nr_configure(pdcp_entity_nr *pdcp_nr, pdcp_config_t *cnfg_)
{
	char name[64] = {0};
	if (pdcp_nr->base.active) {
		// Already configured
		if (0 != memcmp(&pdcp_nr->base.cfg, cnfg_, sizeof(pdcp_config_t))){
			oset_error("Bearer reconfiguration not supported. LCID=%s", pdcp_nr->base.rb_name);
			return false;
		}
		return true;
	}

	pdcp_nr->base.cfg      = cnfg_;
	pdcp_nr->base.rb_name  = sprintf(name, "%s%d", cnfg_->rb_type == PDCP_RB_IS_DRB ? "DRB" : "SRB", cnfg_->bearer_id);
	pdcp_nr->window_size = 1 << (pdcp_nr->base.cfg.sn_len - 1);

	pdcp_nr->rlc_mode = API_rlc_pdcp_rb_is_um(pdcp_nr->base.rnti, pdcp_nr->base.lcid) ? pdcp_entity_nr::UM : pdcp_entity_nr::AM;

	// t-Reordering timer
	if (pdcp_nr->base.cfg.t_reordering != (pdcp_t_reordering_t)infinity) {
		if ((uint32_t)pdcp_nr->base.cfg.t_reordering > 0) {
			pdcp_nr->reordering_timer = gnb_timer_add(gnb_manager_self()->app_timer, reordering_callback, pdcp_nr);
		}
	} else if (pdcp_nr->rlc_mode == pdcp_entity_nr::UM) {
		oset_warn("%s possible PDCP-NR misconfiguration: using infinite re-ordering timer with RLC UM bearer.",
	               pdcp_nr->base.rb_name);
	}

	pdcp_nr->base.active = true;
	oset_info("%s PDCP-NR entity configured. SN_LEN=%d, Discard timer %d, Re-ordering timer %d, RLC=%s, RAT=%s",
	          pdcp_nr->base.rb_name,
	          pdcp_nr->base.cfg.sn_len,
	          pdcp_nr->base.cfg.discard_timer,
	          pdcp_nr->base.cfg.t_reordering,
	          pdcp_nr->rlc_mode == pdcp_entity_nr::UM ? "UM" : "AM",
	          "NR");

	// disable discard timer if using UM
	if (pdcp_nr->rlc_mode == pdcp_entity_nr::UM) {
		pdcp_nr->base.cfg.discard_timer = (pdcp_discard_timer_t)infinity;
	}
	return true;
}


pdcp_entity_nr* pdcp_entity_nr_init(uint32_t lcid_, uint16_t rnti_)
{
	pdcp_entity_nr *pdcp_nr = oset_malloc(sizeof(pdcp_entity_nr));
	ASSERT_IF_NOT(pdcp_nr, "lcid %u Could not allocate pdcp nr context from pool", lcid_);
	memset(pdcp_nr, 0, sizeof(pdcp_entity_nr));

	pdcp_entity_base_init(&pdcp_nr->base, lcid_, rnti_);
	oset_list_init(&pdcp_nr->reorder_list);
	oset_list_init(&pdcp_nr->discard_timers_list);
	pdcp_nr->rx_overflow = false;
	pdcp_nr->tx_overflow = false;
}


void pdcp_entity_nr_stop(pdcp_entity_nr *pdcp_nr)
{
	byte_buffer_t *buffer_node = NULL;
	oset_list_for_each(pdcp_nr->reorder_list, buffer_node){
		oset_free(buffer_node);
	}
	gnb_timer_delete(pdcp_nr->reordering_timer);

	discard_timer_t *timer_node = NULL;
	oset_list_for_each(&pdcp_nr->discard_timers_list, timer_node){
		gnb_timer_delete(timer_node->discard_timer);
		oset_free(timer_node);
	}

	pdcp_entity_base_stop(&pdcp_nr->base);
	oset_free(pdcp_nr);
}

// RLC ===>RRC 接收到ue侧的pdcp包
// nas 先加密再完保   pdcp 先完保再加密(完整性保护是PDU头和PDU的数据部分，pdu head不加密pdu dat,mac-I加密)
// NR PDCP采用PUSH window（下边沿驱动）+t-reordering timer的形式接收下层递交的PDCP PDU
// 关于PUSH window：接收窗口只能依赖于接收窗口下边界状态变量（RX_DELIV）更新才能移动
void pdcp_entity_nr_write_ul_pdu(pdcp_entity_nr *pdcp_nr, byte_buffer_t *pdu)
{
  // Log PDU
  oset_debug("pdcp_ul_pdu") && oset_log2_hexdump(OSET_LOG2_DEBUG, pdu->msg, pdu->N_bytes);
  oset_debug("RX %s PDU (%d B), integrity=%s, encryption=%s",
              pdcp_nr->base.rb_name,
              pdu->N_bytes,
              srsran_direction_text[pdcp_nr->base.integrity_direction],
              srsran_direction_text[pdcp_nr->base.encryption_direction]);

  // 计数越界 0xFFFFFFFF
  if (pdcp_nr->rx_overflow) {
    oset_warn("Rx PDCP COUNTs have overflowed. Discarding SDU.");
    return;
  }

  // PDCP_SN_LEN_12 SRB             PDCP_SN_LEN_12 DRB            PDCP_SN_LEN_18 DRB
  // |R|R|R|R| PDCP SN|Oct 1        |D/C|R|R|R| PDCP SN|Oct 1     |D/C|R|R|R| PDCP SN|Oct 1
  // |   PDCP SN      |Oct 2        |   PDCP SN        |Oct 2     |   PDCP SN        |Oct 2
  // |   Data         |Oct 3        |   Data           |Oct 3     |   PDCP SN        |Oct 3
  // |   ...          |Oct n-1      |   ...            |Oct n-1   |   Data ...       |Oct 4
  // |   MAC-I        |Oct n        |   MAC-I(可选)      |Oct n     |   MAC-I(可选)      |Oct n
  // D/C：0表示control pdu，1表示data pdu

  // Sanity check
  // pdu为ue侧pdcp包，需要解析
  if (pdu->N_bytes <= pdcp_nr->base.cfg.hdr_len_bytes) {
    return;
  }

  oset_debug("Rx PDCP state - RX_NEXT=%u, RX_DELIV=%u, RX_REORD=%u", pdcp_nr->rx_next, pdcp_nr->rx_deliv, pdcp_nr->rx_reord);

  // decode ue==>gnb ul pdcp pdu
  // Extract RCVD_SN from header
  uint32_t rcvd_sn = pdcp_entity_base_read_data_header(&pdcp_nr->base, pdu);

  /*
   * Calculate RCVD_COUNT:
   *
   * - if RCVD_SN < SN(RX_DELIV) – Window_Size:
   *   - RCVD_HFN = HFN(RX_DELIV) + 1.
   * - else if RCVD_SN >= SN(RX_DELIV) + Window_Size:
   *   - RCVD_HFN = HFN(RX_DELIV) – 1.
   * - else:
   *   - RCVD_HFN = HFN(RX_DELIV);
   * - RCVD_COUNT = [RCVD_HFN, RCVD_SN].
   */
   // COUNT值共32bit，其中低SN bit代表SN号；高（32减SN)bit代表HFN;
  uint32_t rcvd_hfn, rcvd_count;
  if ((int64_t)rcvd_sn < ((int64_t)pdcp_SN(&pdcp_nr->base, pdcp_nr->rx_deliv) - (int64_t)pdcp_nr->window_size)) {
    rcvd_hfn = pdcp_HFN(&pdcp_nr->base, pdcp_nr->rx_deliv) + 1;
  } else if (rcvd_sn >= (pdcp_SN(&pdcp_nr->base, pdcp_nr->rx_deliv) + pdcp_nr->window_size)) {
    rcvd_hfn = pdcp_HFN(&pdcp_nr->base, pdcp_nr->rx_deliv) - 1;
  } else {
    rcvd_hfn = pdcp_HFN(&pdcp_nr->base, pdcp_nr->rx_deliv);
  }
  rcvd_count = pdcp_COUNT(&pdcp_nr->base, rcvd_hfn, rcvd_sn);

  oset_debug("Estimated RCVD_HFN=%u, RCVD_SN=%u, RCVD_COUNT=%u", rcvd_hfn, rcvd_sn, rcvd_count);

  // PDCP层加密功能只对Data部分（不包含SDAP协议头）进行。
  // PDCP提供两种RB承载，SRB和DRB，其中SRB的Data PDU必须进行完整性保护，
  // DRB的Data PDU可根据配置需要进行完整性保护。

  /*
   * TS 38.323, section 5.8: Deciphering 解密
   *
   * The data unit that is ciphered is the MAC-I and the
   * data part of the PDCP Data PDU except the
   * SDAP header and the SDAP Control PDU if included in the PDCP SDU.
   */
  if (pdcp_nr->base.encryption_direction == DIRECTION_RX || pdcp_nr->base.encryption_direction == DIRECTION_TXRX) {
    pdcp_entity_base_cipher_decrypt(&pdcp_nr->base,
									&pdu->msg[pdcp_nr->base.cfg.hdr_len_bytes],
									pdu->N_bytes - pdcp_nr->base.cfg.hdr_len_bytes,
									rcvd_count,
									&pdu->msg[pdcp_nr->base.cfg.hdr_len_bytes]);
  }

  /*
   * Extract MAC-I:
   * Always extract from SRBs, only extract from DRBs if integrity is enabled
   */
  // 获取mac值
  uint8_t mac[4] = {0};
  if (is_srb(&pdcp_nr->base) || (is_drb(&pdcp_nr->base) && (pdcp_nr->base.integrity_direction == DIRECTION_TX || pdcp_nr->base.integrity_direction == DIRECTION_TXRX))) {
    pdcp_entity_base_extract_mac(pdu, mac);
  }

  /*
   * TS 38.323, section 5.9: Integrity verification完保校验
   *
   * The data unit that is integrity protected is the PDU header
   * and the data part of the PDU before ciphering.
   */
  if (pdcp_nr->base.integrity_direction == DIRECTION_TX || pdcp_nr->base.integrity_direction == DIRECTION_TXRX) {
    bool is_valid = pdcp_entity_base_integrity_verify(&pdcp_nr->base,
														pdu->msg,
														pdu->N_bytes,
														rcvd_count,
														mac);
    if (!is_valid) {
      oset_error(pdu->msg, pdu->N_bytes, "%s Dropping PDU", pdcp_nr->base.rb_name);
      API_rrc_pdcp_notify_integrity_error(pdcp_nr->base.rnti, pdcp_nr->base.lcid);
      return; // Invalid packet, drop.
    } else {
      oset_debug(pdu->msg, pdu->N_bytes, "%s: Integrity verification successful", pdcp_nr->base.rb_name;
    }
  }

  // After checking the integrity, we can discard the header.
  pdcp_entity_base_discard_data_header(pdu);

  /*
   * Check valid rcvd_count:
   *
   * - if RCVD_COUNT < RX_DELIV; or
   * - if the PDCP Data PDU with COUNT = RCVD_COUNT has been received before:
   *   - discard the PDCP Data PDU;
   */
  if (rcvd_count < pdcp_nr->rx_deliv) {
    oset_debug("Out-of-order after time-out, duplicate or COUNT wrap-around");
    oset_debug("RCVD_COUNT %u, RCVD_COUNT %u", rcvd_count, pdcp_nr->rx_deliv);
    return; // Invalid count, drop.
  }

  // Check if PDU has been received
  pdcp_nr_pdu_t *node = NULL;  
  oset_list_for_each(&pdcp_nr->reorder_list, node) {
	  if (rcvd_count == node->count) {
	  	// PDU already present, drop.
		oset_debug("Duplicate PDU, dropping");
		return;
	  }
  }

  // Store PDU in reception buffer
  pdcp_nr_pdu_t *pdu_node = oset_malloc(pdcp_nr_pdu_t);
  pdu_node->count = rcvd_count;
  pdu_node->buffer = byte_buffer_dup(pdu);
  oset_list_insert_sorted(&pdcp_nr->reorder_list, pdu_node, count_compare);

  // Update RX_NEXT
  if (rcvd_count >= pdcp_nr->rx_next) {
    pdcp_nr->rx_next = rcvd_count + 1;
  }

  // TODO if out-of-order configured, submit to upper layer

  if (rcvd_count == pdcp_nr->rx_deliv) {
    // Deliver to upper layers in ascending order of associated COUNT
    deliver_all_consecutive_counts(pdcp_nr);
  }

  // 判断是否要停止或启动t-reordering 定时器:
  //	1、如果定时器正在运行且更新后的RX_DELIV>=RX_REORD,停止定时器;
  //    2、如果定时没有运行（包括因前述行为停止的）且更新后的RX_DELIV < RX_NEXT，启动定时器，并设置RX_REORD=RX_NEXT;

  // Handle reordering timers
  if ((true == pdcp_nr->reordering_timer.running) && (pdcp_nr->rx_deliv >= pdcp_nr->rx_reord)) {
    gnb_timer_stop(pdcp_nr->reordering_timer);
    oset_debug("Stopped t-Reordering - RX_DELIV=%d, RX_REORD=%ld", pdcp_nr->rx_deliv, pdcp_nr->rx_reord);
  }

  if (pdcp_nr->base.cfg.t_reordering != (pdcp_t_reordering_t)infinity) {
    if ((false == pdcp_nr->reordering_timer.running) && (pdcp_nr->rx_deliv < pdcp_nr->rx_next)) {
      pdcp_nr->rx_reord = pdcp_nr->rx_next;
      gnb_timer_start(pdcp_nr->reordering_timer, pdcp_nr->base.cfg.t_reordering);
      oset_debug("Started t-Reordering - RX_REORD=%ld, RX_DELIV=%ld, RX_NEXT=%ld", pdcp_nr->rx_reord, pdcp_nr->rx_deliv, pdcp_nr->rx_next);
    }
  }

  oset_debug("Rx PDCP state - RX_NEXT=%u, RX_DELIV=%u, RX_REORD=%u", pdcp_nr->rx_next, pdcp_nr->rx_deliv, pdcp_nr->rx_reord);
}

// SDAP/RRC interface
void pdcp_entity_nr_write_dl_sdu(pdcp_entity_nr *pdcp_nr, byte_buffer_t *sdu, int sn)
{
  // Log SDU
  oset_info(sdu->msg,
              sdu->N_bytes,
              "TX %s SDU (%dB), integrity=%s, encryption=%s",
              pdcp_nr->base.rb_name,
              sdu->N_bytes,
              srsran_direction_text[pdcp_nr->base.integrity_direction],
              srsran_direction_text[pdcp_nr->base.encryption_direction]);

  if (API_rlc_pdcp_sdu_queue_is_full(pdcp_nr->base.rnti, pdcp_nr->base.lcid)) {
  	oset_info("Dropping %s SDU due to full queue", pdcp_nr->base.rb_name);
	oset_log2_hexdump(OSET_LOG2_INFO, sdu->msg, sdu->N_bytes);
    return;
  }

  // Check for COUNT overflow 0xFFFFFFFF
  if (pdcp_nr->tx_overflow) {
    oset_warn("TX_NEXT has overflowed. Dropping packet");
    return;
  }
  // pdcp_nr->tx_next = -1 (0xFFFFFFFF)
  if (pdcp_nr->tx_next + 1 == 0) {
    pdcp_nr->tx_overflow = true;
  }

  // Start discard timer
  // DRB丢弃定时器，只有DRB才有，防止阻塞
  if (pdcp_nr->base.cfg.discard_timer != (pdcp_discard_timer_t)infinity) {
	discard_timer_t *discard_node = oset_malloc(sizeof(discard_timer_t));
	oset_assert(discard_node);
	discard_node->discard_sn = pdcp_nr->tx_next;
	discard_node->pdcp_nr = pdcp_nr;
    discard_node->discard_timer = gnb_timer_add(gnb_manager_self()->app_timer, discard_callback, discard_node);
	gnb_timer_start(discard_node->discard_timer, pdcp_nr->base.cfg.discard_timer);
	oset_list_add(&pdcp_nr->discard_timers_list, discard_node);
    oset_debug("Discard Timer set for SN %u. Timeout: %ums", pdcp_nr->tx_next, (uint32_t)(pdcp_nr->base.cfg.discard_timer));
  }

  // Perform header compression TODO

  // Write PDCP header info
  pdcp_entity_base_write_data_header(&pdcp_nr->base, sdu, pdcp_nr->tx_next);

  // TS 38.323, section 5.9: Integrity protection
  // The data unit that is integrity protected is the PDU header
  // and the data part of the PDU before ciphering.
  // 完保
  uint8_t mac[4] = {0};
  if (is_srb(&pdcp_nr->base) || (is_drb(&pdcp_nr->base) && (pdcp_nr->base.integrity_direction == DIRECTION_TX || pdcp_nr->base.integrity_direction == DIRECTION_TXRX))) {
    pdcp_entity_base_integrity_generate(&pdcp_nr->base, sdu->msg, sdu->N_bytes, pdcp_nr->tx_next, mac);
  }
  // Append MAC-I
  if (is_srb(&pdcp_nr->base) || (is_drb(&pdcp_nr->base) && (pdcp_nr->base.integrity_direction == DIRECTION_TX || pdcp_nr->base.integrity_direction == DIRECTION_TXRX))) {
    pdcp_entity_base_append_mac(sdu, mac);
  }

  // TS 38.323, section 5.8: Ciphering
  // The data unit that is ciphered is the MAC-I and the
  // data part of the PDCP Data PDU except the
  // SDAP header and the SDAP Control PDU if included in the PDCP SDU.
  // 加密
  if (pdcp_nr->base.encryption_direction == DIRECTION_TX || pdcp_nr->base.encryption_direction == DIRECTION_TXRX) {
    pdcp_entity_base_cipher_encrypt(&pdcp_nr->base,
									&sdu->msg[pdcp_nr->base.cfg.hdr_len_bytes],
									sdu->N_bytes - pdcp_nr->base.cfg.hdr_len_bytes,
									pdcp_nr->tx_next,
									&sdu->msg[pdcp_nr->base.cfg.hdr_len_bytes]);
  }

  // Set meta-data for RLC AM
  sdu->md.pdcp_sn = pdcp_nr->tx_next;

  oset_info(sdu->msg,
              sdu->N_bytes,
              "TX %s PDU (%dB), HFN=%d, SN=%d, integrity=%s, encryption=%s",
              pdcp_nr->base.rb_name,
              sdu->N_bytes,
              pdcp_HFN(&pdcp_nr->base, pdcp_nr->tx_next),
              pdcp_SN(&pdcp_nr->base, pdcp_nr->tx_next),
              srsran_direction_text[pdcp_nr->base.integrity_direction],
              srsran_direction_text[pdcp_nr->base.encryption_direction]);

  pdcp_nr->base.metrics.num_tx_pdu_bytes += sdu->N_bytes;
  pdcp_nr->base.metrics.num_tx_pdus++;

  // Check if PDCP is associated with more than on RLC entity TODO
  // Write to lower layers
  API_rlc_pdcp_write_dl_sdu(pdcp_nr->base.rnti, pdcp_nr->base.lcid, sdu);

  // Increment TX_NEXT
  pdcp_nr->tx_next++;
}


pdcp_bearer_metrics_t pdcp_entity_nr_get_metrics(pdcp_entity_nr *pdcp_nr)
{
  // TODO
  return pdcp_nr->base.metrics;
}

void pdcp_entity_nr_reset_metrics(pdcp_entity_nr *pdcp_nr)
{
	pdcp_nr->base.metrics = {0};
}

