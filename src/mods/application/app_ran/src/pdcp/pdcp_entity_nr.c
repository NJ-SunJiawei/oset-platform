/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "pdcp/pdcp_entity_nr.h"
#include "rlc/rlc.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-pdcpNR"

/*
 * Timers
 */
// Reordering Timer Callback (t-reordering)
void reordering_callback(void *data)
{
	pdcp_entity_nr *pdcp_nr = (pdcp_entity_nr *)data;

	oset_info("Reordering timer expired. RX_REORD=%u, re-order queue size=%ld", pdcp_nr->rx_reord, oset_list_count(&pdcp_nr->reorder_list));

	// Deliver all PDCP SDU(s) with associated COUNT value(s) < RX_REORD
    for (pdcp_nr_pdu_t *pdu = oset_list_first(&pdcp_nr->reorder_list);\
		(pdu) && (pdu->count < pdcp_nr->rx_reord);\
        pdu = oset_list_next(pdu)){
		    // Deliver to upper layers
		    parent->pass_to_upper_layers(std::move(it->second));
			oset_list_remove(&pdcp_nr->reorder_list, pdu);
			oset_hash_set(pdcp_nr->reorder_hash, pdu->count, sizeof(uint32_t), NULL);
		}	


	// Update RX_DELIV to the first PDCP SDU not delivered to the upper layers
	pdcp_nr->rx_deliv = pdcp_nr->rx_reord;

	// Deliver all PDCP SDU(s) consecutively associated COUNT value(s) starting from RX_REORD
	pdcp_nr->deliver_all_consecutive_counts();

	if (pdcp_nr->rx_deliv < pdcp_nr->rx_next) {
		oset_debug("Updating RX_REORD to %ld. Old RX_REORD=%ld, RX_DELIV=%ld",
		                     pdcp_nr->rx_next,
		                     pdcp_nr->rx_reord,
		                     pdcp_nr->rx_deliv);
		pdcp_nr->rx_reord = pdcp_nr->rx_next;
		gnb_timer_start(pdcp_nr->reordering_timer, pdcp_nr->base.cfg.t_reordering);
	}
}

////////////////////////////////////////////////////////////////////////////
bool pdcp_entity_nr_configure(pdcp_entity_nr* pdcp_nr, pdcp_config_t *cnfg_)
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


pdcp_entity_nr* pdcp_entity_nr_init(uint32_t lcid_, uint16_t rnti_, oset_apr_memory_pool_t	*usepool)
{
	oset_assert(usepool);
	pdcp_entity_nr *pdcp_nr = oset_calloc(1, sizeof(pdcp_entity_nr));
	ASSERT_IF_NOT(pdcp_nr, "lcid %u Could not allocate pdcp nr context from pool", lcid_);
	memset(pdcp_nr, 0, sizeof(pdcp_entity_nr));

	pdcp_entity_base_init(&pdcp_nr->base, lcid_, rnti_, usepool);
	oset_list_init(&pdcp_nr->reorder_list);
	pdcp_nr->reorder_hash = oset_hash_make();
	pdcp_nr->discard_timers_map = oset_hash_make();
	pdcp_nr->rx_overflow = false;
	pdcp_nr->tx_overflow = false;
}


void pdcp_entity_nr_stop(pdcp_entity_nr *pdcp_nr)
{
	oset_hash_destroy(pdcp_nr->reorder_hash);
	oset_hash_destroy(pdcp_nr->discard_timers_map);
	pdcp_entity_base_stop(&pdcp_nr->base);
	oset_free(pdcp_nr);
}

// RLC ===>RRC
// nas 先加密再完保
// pdcp 先完保再加密
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
  
  // Sanity check
  if (pdu->N_bytes <= pdcp_nr->base.cfg.hdr_len_bytes) {
    return;
  }

  oset_debug("Rx PDCP state - RX_NEXT=%u, RX_DELIV=%u, RX_REORD=%u", rx_next, rx_deliv, rx_reord);

  // Extract RCVD_SN from header
  uint32_t rcvd_sn = read_data_header(pdu);

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
  uint32_t rcvd_hfn, rcvd_count;
  if ((int64_t)rcvd_sn < (int64_t)SN(rx_deliv) - (int64_t)window_size) {
    rcvd_hfn = HFN(rx_deliv) + 1;
  } else if (rcvd_sn >= SN(rx_deliv) + window_size) {
    rcvd_hfn = HFN(rx_deliv) - 1;
  } else {
    rcvd_hfn = HFN(rx_deliv);
  }
  rcvd_count = COUNT(rcvd_hfn, rcvd_sn);

  logger.debug("Estimated RCVD_HFN=%u, RCVD_SN=%u, RCVD_COUNT=%u", rcvd_hfn, rcvd_sn, rcvd_count);

  /*
   * TS 38.323, section 5.8: Deciphering
   *
   * The data unit that is ciphered is the MAC-I and the
   * data part of the PDCP Data PDU except the
   * SDAP header and the SDAP Control PDU if included in the PDCP SDU.
   */
  if (encryption_direction == DIRECTION_RX || encryption_direction == DIRECTION_TXRX) {
    cipher_decrypt(
        &pdu->msg[cfg.hdr_len_bytes], pdu->N_bytes - cfg.hdr_len_bytes, rcvd_count, &pdu->msg[cfg.hdr_len_bytes]);
  }

  /*
   * Extract MAC-I:
   * Always extract from SRBs, only extract from DRBs if integrity is enabled
   */
  uint8_t mac[4] = {};
  if (is_srb() || (is_drb() && (integrity_direction == DIRECTION_TX || integrity_direction == DIRECTION_TXRX))) {
    extract_mac(pdu, mac);
  }

  /*
   * TS 38.323, section 5.9: Integrity verification
   *
   * The data unit that is integrity protected is the PDU header
   * and the data part of the PDU before ciphering.
   */
  if (integrity_direction == DIRECTION_TX || integrity_direction == DIRECTION_TXRX) {
    bool is_valid = integrity_verify(pdu->msg, pdu->N_bytes, rcvd_count, mac);
    if (!is_valid) {
      logger.error(pdu->msg, pdu->N_bytes, "%s Dropping PDU", rb_name.c_str());
      rrc->notify_pdcp_integrity_error(lcid);
      return; // Invalid packet, drop.
    } else {
      logger.debug(pdu->msg, pdu->N_bytes, "%s: Integrity verification successful", rb_name.c_str());
    }
  }

  // After checking the integrity, we can discard the header.
  discard_data_header(pdu);

  /*
   * Check valid rcvd_count:
   *
   * - if RCVD_COUNT < RX_DELIV; or
   * - if the PDCP Data PDU with COUNT = RCVD_COUNT has been received before:
   *   - discard the PDCP Data PDU;
   */
  if (rcvd_count < rx_deliv) {
    logger.debug("Out-of-order after time-out, duplicate or COUNT wrap-around");
    logger.debug("RCVD_COUNT %u, RCVD_COUNT %u", rcvd_count, rx_deliv);
    return; // Invalid count, drop.
  }

  // Check if PDU has been received
  if (reorder_list.find(rcvd_count) != reorder_list.end()) {
    logger.debug("Duplicate PDU, dropping");
    return; // PDU already present, drop.
  }

  // Store PDU in reception buffer
  reorder_list[rcvd_count] = std::move(pdu);

  // Update RX_NEXT
  if (rcvd_count >= rx_next) {
    rx_next = rcvd_count + 1;
  }

  // TODO if out-of-order configured, submit to upper layer

  if (rcvd_count == rx_deliv) {
    // Deliver to upper layers in ascending order of associated COUNT
    deliver_all_consecutive_counts();
  }

  // Handle reordering timers
  if (reordering_timer.is_running() and rx_deliv >= rx_reord) {
    reordering_timer.stop();
    logger.debug("Stopped t-Reordering - RX_DELIV=%d, RX_REORD=%ld", rx_deliv, rx_reord);
  }

  if (cfg.t_reordering != pdcp_t_reordering_t::infinity) {
    if (not reordering_timer.is_running() and rx_deliv < rx_next) {
      rx_reord = rx_next;
      reordering_timer.run();
      logger.debug("Started t-Reordering - RX_REORD=%ld, RX_DELIV=%ld, RX_NEXT=%ld", rx_reord, rx_deliv, rx_next);
    }
  }

  logger.debug("Rx PDCP state - RX_NEXT=%u, RX_DELIV=%u, RX_REORD=%u", rx_next, rx_deliv, rx_reord);
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

