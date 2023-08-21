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
}


void pdcp_entity_nr_stop(pdcp_entity_nr *pdcp_nr)
{
	oset_hash_destroy(pdcp_nr->reorder_hash);
	oset_hash_destroy(pdcp_nr->discard_timers_map);
	pdcp_entity_base_stop(&pdcp_nr->base);
	oset_free(pdcp_nr);
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

