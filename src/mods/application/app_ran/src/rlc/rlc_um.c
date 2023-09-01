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

/////////////////////////////////rlc_um_nr_rx/////////////////////////////////

void rlc_um_nr_rx_init(rlc_um_nr_rx *rx)
{
	rx->reassembly_timer = gnb_timer_add(gnb_manager_self()->app_timer, , );

}


/////////////////////////////////rlc_um_nr_tx/////////////////////////////////

void rlc_um_nr_tx_init(rlc_um_nr_tx *tx)
{

}

////////////////////////////////rlc_um_nr/////////////////////////////////////
void rlc_um_nr_stop(rlc_common *tm_common)
{
	rlc_um_nr *um = (rlc_um_nr *)tm_common;

	rlc_um_base_stop(&um->base);
}

bool rlc_um_nr_configure(rlc_common *tm_common, rlc_config_t *cnfg_)
{
	rlc_um_nr *um = (rlc_um_nr *)tm_common;

	// store config
	um->base.cfg = *cnfg_;

	// determine bearer name and configure Rx/Tx objects
	um->base.common.rb_name = oset_msprintf("DRB%s", um->base.cfg.um_nr.bearer_id);

	rlc_um_nr_tx_init(&um->tx);
	if (not rx->configure(cfg, rb_name)) {
		return false;
	}

	rlc_um_nr_rx_init(&um->rx);
	if (not tx->configure(cfg, rb_name)) {
		return false;
	}

	RlcInfo("configured in %s: sn_field_length=%u bits, t_reassembly=%d ms",
	      srsran::to_string(cnfg_.rlc_mode),
	      srsran::to_number(cfg.um_nr.sn_field_length),
	      cfg.um_nr.t_reassembly_ms);

	rx_enabled = true;
	tx_enabled = true;

	return true;
}

rlc_mode_t rlc_um_nr_get_mode(void)
{
  return rlc_mode_t(um);
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


