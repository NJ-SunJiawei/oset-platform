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

void pdcp_entity_base_init(pdcp_entity_base *base, uint32_t         lcid_, uint16_t rnti_, oset_apr_memory_pool_t *usepool)
{
	base->usepool              = usepool;
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
	base->usepool              = NULL;
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

