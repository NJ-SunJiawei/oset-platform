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
void reordering_callback(pdcp_entity_base* pdcp_base, uint32_t timer_id)
{
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
	pdcp_entity_nr *pdcp_nr = (pdcp_entity_nr *)pdcp_base;

	if (pdcp_base->active) {
	// Already configured
	if (cnfg_ != cfg) {
		oset_error("Bearer reconfiguration not supported. LCID=%s.", pdcp_base->rb_name);
		return false;
	}
		return true;
	}

	cfg         = cnfg_;
	rb_name     = cfg.get_rb_name();
	window_size = 1 << (cfg.sn_len - 1);

	rlc_mode = rlc->rb_is_um(lcid) ? rlc_mode_t::UM : rlc_mode_t::AM;

	// t-Reordering timer
	if (cfg.t_reordering != pdcp_t_reordering_t::infinity) {
	reordering_timer = task_sched.get_unique_timer();
	if (static_cast<uint32_t>(cfg.t_reordering) > 0) {
	  reordering_timer.set(static_cast<uint32_t>(cfg.t_reordering), *reordering_fnc);
	}
	} else if (rlc_mode == rlc_mode_t::UM) {
	logger.warning("%s possible PDCP-NR misconfiguration: using infinite re-ordering timer with RLC UM bearer.",
	               rb_name);
	}

	active = true;
	logger.info("%s PDCP-NR entity configured. SN_LEN=%d, Discard timer %d, Re-ordering timer %d, RLC=%s, RAT=%s",
	          rb_name,
	          cfg.sn_len,
	          cfg.discard_timer,
	          cfg.t_reordering,
	          rlc_mode == rlc_mode_t::UM ? "UM" : "AM",
	          to_string(cfg.rat));

	// disable discard timer if using UM
	if (rlc_mode == rlc_mode_t::UM) {
		cfg.discard_timer = pdcp_discard_timer_t::infinity;
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

	pdcp_nr->reordering_fnc = reordering_callback;
	pdcp_nr->reorder_queue = oset_hash_make();
	pdcp_nr->discard_timers_map = oset_hash_make();
}


