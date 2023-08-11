/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "lib/pdcp/pdcp_entity_nr.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-libpdcpNR"

/*
 * Timers
 */
// Reordering Timer Callback (t-reordering)
void reordering_callback(void *data)
{
	pdcp_entity_base *pdcp_base = (pdcp_entity_base*)data;
	pdcp_entity_nr *pdcp_nr = (pdcp_entity_nr *)pdcp_base;

	oset_info("Reordering timer expired. RX_REORD=%u, re-order queue size=%ld", pdcp_nr->rx_reord, pdcp_nr->reorder_queue.size());

	// Deliver all PDCP SDU(s) with associated COUNT value(s) < RX_REORD
	for (std::map<uint32_t, unique_byte_buffer_t>::iterator it = pdcp_nr->reorder_queue.begin();
	   it != pdcp_nr->reorder_queue.end() && it->first < pdcp_nr->rx_reord;
	   pdcp_nr->reorder_queue.erase(it++)) {
		// Deliver to upper layers
		pdcp_nr->pass_to_upper_layers(std::move(it->second));
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
		pdcp_nr->reordering_timer.run();
	}
}

////////////////////////////////////////////////////////////////////////////
bool pdcp_entity_nr_configure(pdcp_entity_base* pdcp_base, pdcp_config_t *cnfg_)
{
	char name[64] = {0};
	pdcp_entity_nr *pdcp_nr = (pdcp_entity_nr *)pdcp_base;

	if (pdcp_base->active) {
		// Already configured
		if (0 != memcmp(&pdcp_base->cfg, cnfg_, sizeof(pdcp_config_t))){
			oset_error("Bearer reconfiguration not supported. LCID=%s.", pdcp_base->rb_name);
			return false;
		}
		return true;
	}

	pdcp_base->cfg      = cnfg_;
	pdcp_base->rb_name  = sprintf(name, "%s%d", cnfg_->rb_type == PDCP_RB_IS_DRB ? "DRB" : "SRB", cnfg_->bearer_id);
	pdcp_nr->window_size = 1 << (pdcp_base->cfg.sn_len - 1);

	pdcp_nr->rlc_mode = rlc->rb_is_um(lcid) ? pdcp_entity_nr::UM : pdcp_entity_nr::AM;

	// t-Reordering timer
	if (pdcp_base->cfg.t_reordering != (pdcp_t_reordering_t)infinity) {
		if ((uint32_t)pdcp_base->cfg.t_reordering > 0) {
			pdcp_nr->reordering_timer = gnb_timer_add(gnb_manager_self()->app_timer, reordering_callback, pdcp_base);
		}
	} else if (pdcp_nr->rlc_mode == pdcp_entity_nr::UM) {
		oset_warn("%s possible PDCP-NR misconfiguration: using infinite re-ordering timer with RLC UM bearer.",
	               pdcp_base->rb_name);
	}

	pdcp_base->active = true;
	oset_info("%s PDCP-NR entity configured. SN_LEN=%d, Discard timer %d, Re-ordering timer %d, RLC=%s, RAT=%s",
	          pdcp_base->rb_name,
	          pdcp_base->cfg.sn_len,
	          pdcp_base->cfg.discard_timer,
	          pdcp_base->cfg.t_reordering,
	          pdcp_nr->rlc_mode == pdcp_entity_nr::UM ? "UM" : "AM",
	          "NR");

	// disable discard timer if using UM
	if (pdcp_nr->rlc_mode == pdcp_entity_nr::UM) {
		pdcp_base->cfg.discard_timer = (pdcp_discard_timer_t)infinity;
	}
	return true;
}


pdcp_entity_nr* pdcp_entity_nr_init(uint32_t lcid_, uint16_t rnti_, oset_apr_memory_pool_t	*usepool)
{
	oset_assert(usepool);
	pdcp_entity_nr *pdcp_nr = oset_core_alloc(usepool, sizeof(pdcp_entity_nr));
	ASSERT_IF_NOT(pdcp_nr, "lcid %u Could not allocate pdcp nr context from pool", lcid_);
	memset(pdcp_nr, 0, sizeof(pdcp_entity_nr));

	pdcp_entity_base_init(&pdcp_nr->base, lcid_, rnti_, usepool);
	pdcp_nr->base.func = {
							._configure = pdcp_entity_nr_configure,
						 };

	pdcp_nr->reorder_queue = oset_hash_make();
	pdcp_nr->discard_timers_map = oset_hash_make();
}

