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

