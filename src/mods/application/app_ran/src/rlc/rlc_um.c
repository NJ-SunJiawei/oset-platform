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

//  UDM PDU

// complete RLC PDU
// | SI |R|R|R|R|R|R|
// |      Data      |
// |      ...       |

// first RLC PDU segment(sn 6 bit)      first RLC PDU segment(sn 12 bit)
// | SI |    SN     |                   | SI |R|R|   SN   |
// |      Data      |                   |      SN         |
// |      ...       |                   |      Data       |


// other RLC PDU segment(sn 6 bit)      other RLC PDU segment(sn 12 bit)
// | SI |    SN     |                   | SI |R|R|   SN   |
// |      SO        |                   |      SO         |
// |      SO        |                   |      SO         |
// |      Data      |                   |      SN         |
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

//////////////////////////////////////////////////////////////////////////////
/////////////////////////////////rlc_um_nr_rx/////////////////////////////////
// TS 38.322 v15.003 Section 5.2.2.2.4
void rlc_um_nr_rx_timer_expired(void *data)
{
  rlc_um_nr_rx *rx = (rlc_um_nr_rx *)data;

  oset_thread_mutex_lock(&rx->mutex);
  if (rx->reassembly_timer.id() == timeout_id) {
    RlcDebug("reassembly timeout expiry for SN=%d - updating RX_Next_Reassembly and reassembling", RX_Next_Reassembly);

    metrics.num_lost_pdus++;

    if (rx_sdu != nullptr) {
      rx_sdu->clear();
    }

    // update RX_Next_Reassembly to the next SN that has not been reassembled yet
    RX_Next_Reassembly = RX_Timer_Trigger;
    while (RX_MOD_NR_BASE(RX_Next_Reassembly) < RX_MOD_NR_BASE(RX_Next_Highest)) {
      RX_Next_Reassembly = (RX_Next_Reassembly + 1) % mod;
      debug_state();
    }

    // discard all segments with SN < updated RX_Next_Reassembly
    for (auto it = rx_window.begin(); it != rx_window.end();) {
      if (RX_MOD_NR_BASE(it->first) < RX_MOD_NR_BASE(RX_Next_Reassembly)) {
        it = rx_window.erase(it);
      } else {
        ++it;
      }
    }

    // check start of t_reassembly
    if (RX_MOD_NR_BASE(RX_Next_Highest) > RX_MOD_NR_BASE(RX_Next_Reassembly + 1) ||
        ((RX_MOD_NR_BASE(RX_Next_Highest) == RX_MOD_NR_BASE(RX_Next_Reassembly + 1) &&
          has_missing_byte_segment(RX_Next_Reassembly)))) {
      RlcDebug("starting reassembly timer for SN=%d", rb_name.c_str(), RX_Next_Reassembly);
      reassembly_timer.run();
      RX_Timer_Trigger = RX_Next_Highest;
    }

    debug_state();
  }
  oset_thread_mutex_unlock(&rx->mutex);
}


static void rlc_um_nr_rx_init(rlc_um_nr_rx *rx)
{
	oset_thread_mutex_init(&rx->mutex); 
}

static void rlc_um_nr_rx_stop(rlc_um_nr_rx *rx)
{
	oset_thread_mutex_destroy(&rx->mutex);
	if(rx->reassembly_timer) gnb_timer_delete(rx->reassembly_timer);

}

static bool rlc_um_nr_rx_configure(rlc_um_nr_rx *rx, rlc_config_t *cnfg_, char *rb_name_)
{
	rx->base_rx.rb_name = rb_name_;
	rx->base_rx.cfg = *cnfg_;

	rx->mod  = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 64 : 4096;
	rx->UM_Window_Size = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 32 : 2048;

	// configure timer
	if (rx->base_rx.cfg.um_nr.t_reassembly_ms > 0) {
		rx->reassembly_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_um_nr_rx_timer_expired, rx);
	}

	return true;
}

/////////////////////////////////rlc_um_nr_tx/////////////////////////////////

static void rlc_um_nr_tx_init(rlc_um_nr_tx *tx)
{

}

static void rlc_um_nr_tx_stop(rlc_um_nr_tx *tx)
{

}

static bool rlc_um_nr_tx_configure(rlc_um_nr_tx *tx, rlc_config_t *cnfg_, char *rb_name_)
{
  tx->base_tx.rb_name = rb_name_;
  tx->base_tx.cfg = *cnfg_;

  tx->mod            = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 64 : 4096;
  tx->UM_Window_Size = (cnfg_->um_nr.sn_field_length == (rlc_um_nr_sn_size_t)size6bits) ? 32 : 2048;

  // calculate header sizes for configured SN length
  rlc_um_nr_pdu_header_t header = {0};
  header.si        = (rlc_nr_si_field_t)first_segment;
  header.so        = 0;//无偏移
  tx->head_len_first   = rlc_um_nr_packed_length(&header);

  header.so        = 1;//有偏移
  tx->head_len_segment = rlc_um_nr_packed_length(&header);

  tx->base_tx.tx_sdu_queue.resize(cnfg_->tx_queue_length);
  return true;
}

//////////////////////////////////////////////////////////////////////////////
////////////////////////////////rlc_um_nr/////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void rlc_um_nr_stop(rlc_common *tm_common)
{
	rlc_um_nr *um = (rlc_um_nr *)tm_common;

	rlc_um_base_stop(&um->base);
	rlc_um_nr_rx_stop(&um->rx);
	rlc_um_nr_tx_stop(&um->tx);
}

bool rlc_um_nr_configure(rlc_common *tm_common, rlc_config_t *cnfg_)
{
	rlc_um_nr *um = (rlc_um_nr *)tm_common;

	// store config
	um->base.cfg = *cnfg_;

	// determine bearer name and configure Rx/Tx objects
	um->base.common.rb_name = oset_msprintf("DRB%s", um->base.cfg.um_nr.bearer_id);

	rlc_um_nr_rx_init(&um->rx);
	if (! rlc_um_nr_rx_configure(&um->rx, &um->base.cfg, um->base.common.rb_name)) {
		return false;
	}

	rlc_um_nr_tx_init(&um->tx);
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


