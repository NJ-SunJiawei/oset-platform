/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "pdcp/pdcp_entity_base.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-pdcpBase"

bool is_srb(pdcp_entity_base *base) { return base->cfg.rb_type == PDCP_RB_IS_SRB; }
bool is_drb(pdcp_entity_base *base) { return base->cfg.rb_type == PDCP_RB_IS_DRB; }

uint32_t pdcp_HFN(pdcp_entity_base *base, uint32_t count)
{
  return (count >> base->cfg.sn_len);
}

uint32_t pdcp_SN(pdcp_entity_base *base, uint32_t count)
{
  return count & (0xFFFFFFFF >> (32 - base->cfg.sn_len));
}

uint32_t pdcp_COUNT(pdcp_entity_base *base, uint32_t hfn, uint32_t sn)
{
  return (hfn << base->cfg.sn_len) | sn;
} 

void pdcp_entity_base_init(pdcp_entity_base *base, uint32_t         lcid_, uint16_t rnti_)
{
	base->active               = false;
	base->rnti                 = rnti_;
	base->lcid                 = lcid_;
	base->integrity_direction  = DIRECTION_NONE;
	base->encryption_direction = DIRECTION_NONE;
	base->enable_security_rx_sn = -1;
	base->enable_security_tx_sn = -1;
	base->cfg = {
					.bearer_id = 1,
					.rb_type = SECURITY_DIRECTION_DOWNLINK,
					.tx_direction = SECURITY_DIRECTION_DOWNLINK,
					.rx_direction = SECURITY_DIRECTION_UPLINK,
					.sn_len = PDCP_SN_LEN_12,
					.t_reordering = (pdcp_t_reordering_t)ms500,
					.discard_timer = (pdcp_discard_timer_t)infinity,
					.status_report_required = false,
					.rat = (srsran_rat_t)nr,
					.hdr_len_bytes = ceilf((float)PDCP_SN_LEN_12 / 8),
				};
}

void pdcp_entity_base_stop(pdcp_entity_base *base)
{
	base->active               = false;
}

void pdcp_entity_base_config_security(pdcp_entity_base *base, struct as_security_config_t *sec_cfg_)
{
	base->sec_cfg = *sec_cfg_;

	oset_info("Configuring security with %s and %s",
	          integrity_algorithm_id_text[sec_cfg_->integ_algo],
	          ciphering_algorithm_id_text[sec_cfg_->cipher_algo]);

	oset_debug("K_rrc_enc") && oset_log2_hexdump(OSET_LOG2_DEBUG, sec_cfg_->k_rrc_enc, 32);
	oset_debug("K_up_enc") && oset_log2_hexdump(OSET_LOG2_DEBUG, sec_cfg_->k_up_enc, 32);
	oset_debug("K_rrc_int") && oset_log2_hexdump(OSET_LOG2_DEBUG, sec_cfg_->k_rrc_int, 32);
	oset_debug("K_up_int") && oset_log2_hexdump(OSET_LOG2_DEBUG, sec_cfg_->k_up_int, 32);
}

void pdcp_entity_base_enable_integrity(pdcp_entity_base *base, srsran_direction_t direction)
{
	// if either DL or UL is already enabled, both are enabled
	if (base->integrity_direction == DIRECTION_TX && direction == DIRECTION_RX) {
		base->integrity_direction = DIRECTION_TXRX;
	} else if (base->integrity_direction == DIRECTION_RX && direction == DIRECTION_TX) {
		base->integrity_direction = DIRECTION_TXRX;
	} else {
		base->integrity_direction = direction;
	}
	oset_debug("Enabled integrity. LCID=%d, integrity=%s", base->lcid, srsran_direction_text[base->integrity_direction]);
}

void pdcp_entity_base_enable_encryption(pdcp_entity_base *base, srsran_direction_t direction)
{
	// if either DL or UL is already enabled, both are enabled
	if (base->encryption_direction == DIRECTION_TX && direction == DIRECTION_RX) {
		base->encryption_direction = DIRECTION_TXRX;
	} else if (base->encryption_direction == DIRECTION_RX && direction == DIRECTION_TX) {
		base->encryption_direction = DIRECTION_TXRX;
	} else {
		base->encryption_direction = direction;
	}
	oset_debug("Enabled encryption. LCID=%d, encryption=%s", base->lcid, srsran_direction_text[base->encryption_direction]);
}

uint32_t pdcp_entity_base_read_data_header(pdcp_entity_base *base, byte_buffer_t *pdu)
{
  // Check PDU is long enough to extract header
  if (pdu->N_bytes <= base->cfg.hdr_len_bytes) {
    oset_error("PDU too small to extract header");
    return 0;
  }

  // Extract RCVD_SN
  uint16_t rcvd_sn_16 = 0;
  uint32_t rcvd_sn_32 = 0;
  switch (base->cfg.sn_len) {
    case PDCP_SN_LEN_5:
      rcvd_sn_32 = pdcp_SN(base, pdu->msg[0]);
      break;
    case PDCP_SN_LEN_7:
      rcvd_sn_32 = pdcp_SN(base, pdu->msg[0]);
      break;
    case PDCP_SN_LEN_12:
      uint8_to_uint16(pdu->msg, &rcvd_sn_16);
      rcvd_sn_32 = pdcp_SN(base, rcvd_sn_16);
      break;
    case PDCP_SN_LEN_18:
      uint8_to_uint24(pdu->msg, &rcvd_sn_32);
      rcvd_sn_32 = pdcp_SN(base, rcvd_sn_32);
      break;
    default:
      oset_error("Cannot extract RCVD_SN, invalid SN length configured: %d", base->cfg.sn_len);
  }

  return rcvd_sn_32;
}

void pdcp_entity_base_cipher_encrypt(pdcp_entity_base *base, uint8_t *msg, uint32_t msg_len, uint32_t count, uint8_t *ct)
{
  uint8_t  *k_enc = NULL;
  uint8_t  ct_tmp[PDCP_MAX_SDU_SIZE];

  // If control plane use RRC encrytion key. If data use user plane key
  if (is_srb(base)) {
    k_enc = base->sec_cfg.k_rrc_enc;
  } else {
    k_enc = base->sec_cfg.k_up_enc;
  }

  oset_debug("Cipher encrypt input: COUNT: %" PRIu32 ", Bearer ID: %d, Direction %s",
               count,
               base->cfg.bearer_id,
               base->cfg.tx_direction == SECURITY_DIRECTION_DOWNLINK ? "Downlink" : "Uplink");
  oset_debug("Cipher encrypt key:") && oset_log2_hexdump(OSET_LOG2_DEBUG, k_enc, 32);
  oset_debug("Cipher encrypt input msg:") && oset_log2_hexdump(OSET_LOG2_DEBUG, msg, msg_len);

  switch (base->sec_cfg.cipher_algo) {
    case CIPHERING_ALGORITHM_ID_EEA0:
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA1:
      security_128_eea1(&(k_enc[16]), count, base->cfg.bearer_id - 1, base->cfg.tx_direction, msg, msg_len, ct_tmp);
      memcpy(ct, ct_tmp, msg_len);
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA2:
      security_128_eea2(&(k_enc[16]), count, base->cfg.bearer_id - 1, base->cfg.tx_direction, msg, msg_len, ct_tmp);
      memcpy(ct, ct_tmp, msg_len);
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA3:
      security_128_eea3(&(k_enc[16]), count, base->cfg.bearer_id - 1, base->cfg.tx_direction, msg, msg_len, ct_tmp);
      memcpy(ct, ct_tmp, msg_len);
      break;
    default:
      break;
  }
  oset_debug("Cipher encrypt output msg:") && oset_log2_hexdump(OSET_LOG2_DEBUG, ct, msg_len);
}


void pdcp_entity_base_cipher_decrypt(pdcp_entity_base *base, uint8_t *ct, uint32_t ct_len, uint32_t count, uint8_t *msg)
{
  uint8_t  *k_enc = NULL;
  uint8_t  msg_tmp[PDCP_MAX_SDU_SIZE] = {0};

  // If control plane use RRC encrytion key. If data use user plane key
  if (is_srb(base)) {
    k_enc = base->sec_cfg.k_rrc_enc;
  } else {
    k_enc = base->sec_cfg.k_up_enc;
  }

  oset_debug("Cipher decrypt input: COUNT: %" PRIu32 ", Bearer ID: %d, Direction %s",
               count,
               base->cfg.bearer_id,
               (base->cfg.rx_direction == SECURITY_DIRECTION_DOWNLINK) ? "Downlink" : "Uplink");
  
  oset_debug("Cipher decrypt key:") && oset_log2_hexdump(OSET_LOG2_DEBUG, k_enc, 32);
  oset_debug("Cipher decrypt input msg:") && oset_log2_hexdump(OSET_LOG2_DEBUG, ct, ct_len);
 
  switch (base->sec_cfg.cipher_algo) {
    case CIPHERING_ALGORITHM_ID_EEA0:
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA1:
      security_128_eea1(&k_enc[16], count, base->cfg.bearer_id - 1, base->cfg.rx_direction, ct, ct_len, msg_tmp);
      memcpy(msg, msg_tmp, ct_len);
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA2:
      security_128_eea2(&k_enc[16], count, base->cfg.bearer_id - 1, base->cfg.rx_direction, ct, ct_len, msg_tmp);
      memcpy(msg, msg_tmp, ct_len);
      break;
    case CIPHERING_ALGORITHM_ID_128_EEA3:
      security_128_eea3(&k_enc[16], count, base->cfg.bearer_id - 1, base->cfg.rx_direction, ct, ct_len, msg_tmp);
      memcpy(msg, msg_tmp, ct_len);
      break;
    default:
      break;
  }
  oset_debug("Cipher decrypt output msg:") && oset_log2_hexdump(OSET_LOG2_DEBUG, msg, ct_len);
}

void pdcp_entity_base_append_mac(byte_buffer_t *sdu, uint8_t *mac)
{
  // Check enough space for MAC
  if (sdu->N_bytes + 4 > byte_buffer_get_tailroom(sdu)) {
    oset_error("Not enough space to add MAC-I");
    return;
  }

  // Append MAC
  memcpy(&sdu->msg[sdu->N_bytes], mac, 4);
  sdu->N_bytes += 4;
}

void pdcp_entity_base_extract_mac(byte_buffer_t *pdu, uint8_t *mac)
{
  // Check enough space for MAC
  if (pdu->N_bytes < 4) {
    oset_error("PDU too small to extract MAC-I");
    return;
  }

  // Extract MAC
  memcpy(mac, &pdu->msg[pdu->N_bytes - 4], 4);
  pdu->N_bytes -= 4;
}

void pdcp_entity_base_integrity_generate(pdcp_entity_base *base, uint8_t *msg, uint32_t msg_len, uint32_t count, uint8_t *mac)
{
  uint8_t* k_int;

  // If control plane use RRC integrity key. If data use user plane key
  if (is_srb(base)) {
    k_int = base->sec_cfg.k_rrc_int;
  } else {
    k_int = base->sec_cfg.k_up_int;
  }

  switch (base->sec_cfg.integ_algo) {
    case INTEGRITY_ALGORITHM_ID_EIA0:
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA1:
      security_128_eia1(&k_int[16], count, base->cfg.bearer_id - 1, base->cfg.tx_direction, msg, msg_len, mac);
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA2:
      security_128_eia2(&k_int[16], count, base->cfg.bearer_id - 1, base->cfg.tx_direction, msg, msg_len, mac);
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA3:
      security_128_eia3(&k_int[16], count, base->cfg.bearer_id - 1, base->cfg.tx_direction, msg, msg_len, mac);
      break;
    default:
      break;
  }

  oset_debug("Integrity gen input: COUNT %" PRIu32 ", Bearer ID %d, Direction %s",
               count,
               base->cfg.bearer_id,
               (base->cfg.tx_direction == SECURITY_DIRECTION_DOWNLINK ? "Downlink" : "Uplink"));
  oset_debug("Integrity gen key:") && oset_log2_hexdump(OSET_LOG2_DEBUG, k_int, 32);
  oset_debug("Integrity gen input msg:") && oset_log2_hexdump(OSET_LOG2_DEBUG, msg, msg_len);
  oset_debug("MAC (generated)") && oset_log2_hexdump(OSET_LOG2_DEBUG, mac, 4);
}

bool pdcp_entity_base_integrity_verify(pdcp_entity_base *base, uint8_t *msg, uint32_t msg_len, uint32_t count, uint8_t *mac)
{
  uint8_t  mac_exp[4] = {0};
  bool     is_valid   = true;
  uint8_t  *k_int = NULL;

  // If control plane use RRC integrity key. If data use user plane key
  if (is_srb(base)) {
    k_int = base->sec_cfg.k_rrc_int;
  } else {
    k_int = base->sec_cfg.k_up_int;
  }

  switch (base->sec_cfg.integ_algo) {
    case INTEGRITY_ALGORITHM_ID_EIA0:
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA1:
      security_128_eia1(&k_int[16], count, base->cfg.bearer_id - 1, base->cfg.rx_direction, msg, msg_len, mac_exp);
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA2:
      security_128_eia2(&k_int[16], count, base->cfg.bearer_id - 1, base->cfg.rx_direction, msg, msg_len, mac_exp);
      break;
    case INTEGRITY_ALGORITHM_ID_128_EIA3:
      security_128_eia3(&k_int[16], count, base->cfg.bearer_id - 1, base->cfg.rx_direction, msg, msg_len, mac_exp);
      break;
    default:
      break;
  }

  if (base->sec_cfg.integ_algo != INTEGRITY_ALGORITHM_ID_EIA0) {
    for (uint8_t i = 0; i < 4; i++) {
      if (mac[i] != mac_exp[i]) {
        is_valid = false;
        break;
      }
    }
	if(is_valid){
		oset_debug("Integrity check input - COUNT %" PRIu32 ", Bearer ID %d, Direction %s",
				count,
				base->cfg.bearer_id,
				base->cfg.rx_direction == SECURITY_DIRECTION_DOWNLINK ? "Downlink" : "Uplink");
		oset_debug("Integrity check key:") && oset_log2_hexdump(OSET_LOG2_DEBUG, k_int, 32);
		oset_debug("MAC %s (expected):", is_valid ? "match" : "mismatch") && oset_log2_hexdump(OSET_LOG2_DEBUG, mac_exp, 4);
		oset_debug("MAC %s (found):", is_valid ? "match" : "mismatch") && oset_log2_hexdump(OSET_LOG2_DEBUG, mac, 4);
		oset_debug("Integrity check input msg (Bytes=%" PRIu32 "):") && oset_log2_hexdump(OSET_LOG2_DEBUG, msg, msg_len);
	}else{
		oset_error("Integrity check input - COUNT %" PRIu32 ", Bearer ID %d, Direction %s",
				count,
				base->cfg.bearer_id,
				base->cfg.rx_direction == SECURITY_DIRECTION_DOWNLINK ? "Downlink" : "Uplink");
		oset_error("Integrity check key:") && oset_log2_hexdump(OSET_LOG2_DEBUG, k_int, 32);
		oset_error("MAC %s (expected):", is_valid ? "match" : "mismatch") && oset_log2_hexdump(OSET_LOG2_DEBUG, mac_exp, 4);
		oset_error("MAC %s (found):", is_valid ? "match" : "mismatch") && oset_log2_hexdump(OSET_LOG2_DEBUG, mac, 4);
		oset_error("Integrity check input msg (Bytes=%" PRIu32 "):") && oset_log2_hexdump(OSET_LOG2_DEBUG, msg, msg_len);
	}
  }

  return is_valid;
}

void pdcp_entity_base_discard_data_header(pdcp_entity_base *base, byte_buffer_t *pdu)
{
  pdu->msg += base->cfg.hdr_len_bytes;
  pdu->N_bytes -= base->cfg.hdr_len_bytes;
}

void pdcp_entity_base_write_data_header(pdcp_entity_base *base, byte_buffer_t *sdu, uint32_t count)
{
  // Add room for header
  if (base->cfg.hdr_len_bytes > byte_buffer_get_headroom(sdu)) {
    oset_error("Not enough space to add header");
    return;
  }
  sdu->msg -= base->cfg.hdr_len_bytes;
  sdu->N_bytes += base->cfg.hdr_len_bytes;

  // PDCP_SN_LEN_12 SRB             PDCP_SN_LEN_12 DRB            PDCP_SN_LEN_18 DRB
  // |R|R|R|R| PDCP SN|Oct 1        |D/C|R|R|R| PDCP SN|Oct 1     |D/C|R|R|R| PDCP SN|Oct 1
  // |   PDCP SN      |Oct 2        |   PDCP SN        |Oct 2     |   PDCP SN        |Oct 2
  // |   Data         |Oct 3        |   Data           |Oct 3     |   PDCP SN        |Oct 3
  // |   ...          |Oct n-1      |   ...            |Oct n-1   |   Data ...       |Oct 4
  // |   MAC-I        |Oct n        |   MAC-I(可选)      |Oct n     |   MAC-I(可选)      |Oct n
  // D/C：0表示control pdu(srb)，1表示data pdu(drb)

  // Add SN
  switch (base->cfg.sn_len) {
    case PDCP_SN_LEN_5:
      sdu->msg[0] = pdcp_SN(base, count); // Data PDU and SN LEN 5 implies SRB, D flag must not be present
      break;
    case PDCP_SN_LEN_7:
      sdu->msg[0] = pdcp_SN(base, count);
      if (is_drb(base)) {
        sdu->msg[0] |= 0x80; // On Data PDUs for DRBs we must set the D flag.
      }
      break;
    case PDCP_SN_LEN_12:
      uint16_to_uint8(pdcp_SN(base, count), sdu->msg);
      if (is_drb(base)) {
        sdu->msg[0] |= 0x80; // On Data PDUs for DRBs we must set the D flag.
      }
      break;
    case PDCP_SN_LEN_18:
      uint24_to_uint8(pdcp_SN(base, count), sdu->msg);
      sdu->msg[0] |= 0x80; // Data PDU and SN LEN 18 implies DRB, D flag must be present
      break;
    default:
      oset_error("Invalid SN length configuration: %d bits", base->cfg.sn_len);
  }
}

