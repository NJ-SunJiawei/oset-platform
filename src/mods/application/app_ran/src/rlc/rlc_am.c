/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.09
************************************************************************/
#include "lib/common/common_nr.h"
#include "rlc/rlc_um.h"
#include "pdcp/pdcp.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlcAM"

#define AM_RXTX
/////////////////////////////////////////////////////////////////////////////
#if AM_RXTX
static void empty_queue_no_lock(rlc_am_base_tx *base_tx)
{
	// Drop all messages in TX queue
	rlc_um_base_tx_sdu_t *next_node, *node = NULL;
	oset_list_for_each_safe(&base_tx->tx_sdu_queue, next_node, node){
			byte_buffer_t *sdu = node->buffer;
			oset_list_remove(&base_tx->tx_sdu_queue, node);
			oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
			base_tx->unread_bytes -= sdu->N_bytes;
			oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
			oset_free(sdu)
	}

	oset_assert(0 == base_tx->unread_bytes);
	oset_assert(0 == oset_list_count(base_tx->tx_sdu_queue));
}

void empty_queue(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	empty_queue_no_lock(base_tx);
	oset_thread_mutex_unlock(&base_tx->mutex);
}

static void rlc_am_base_tx_stop(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_destroy(&base_tx->unread_bytes_mutex);
	oset_thread_mutex_destroy(&base_tx->mutex);
}

static void rlc_am_base_tx_init(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_init(&base_tx->mutex);
	oset_thread_mutex_init(&base_tx->unread_bytes_mutex);
}

static void rlc_am_base_rx_stop(rlc_am_base_rx *base_rx)
{
	//todo
	base_rx->do_status = false;
	oset_thread_mutex_destroy(&base_rx->mutex);
}

static void rlc_am_base_rx_init(rlc_um_base_rx *base_rx)
{
	oset_thread_mutex_init(&base_rx->mutex);
}

static void rlc_am_base_init(rlc_am_base *base, uint32_t lcid_, uint16_t rnti_)
{
	rlc_common_init(&base->common, NULL, rnti_, lcid_, (rlc_mode_t)um, NULL);
	base->rx_enabled = false;
	base->tx_enabled = false;
	base->metrics = {0};
	oset_thread_mutex_init(&base->metrics_mutex);
}

static void rlc_am_base_stop(rlc_am_base *base)
{
	rlc_common_destory(&base->common);
	base->rx_enabled = false;
	base->tx_enabled = false;
	oset_thread_mutex_destroy(&base->metrics_mutex);
	base->metrics = {0};
}

#endif
/////////////////////////////////////////////////////////////////////////////
#if AM_RXTX
static void suspend_tx(rlc_am_nr_tx *tx)
{
	oset_thread_mutex_lock(&tx->base_tx.mutex);
	empty_queue_no_lock(&tx->base_tx);

	if(tx->poll_retransmit_timer) gnb_timer_delete(tx->poll_retransmit_timer);

	tx->st = {0};
	tx->sdu_under_segmentation_sn = INVALID_RLC_SN;

	// Drop all messages in TX window
	tx->tx_window->clear();
	oset_hash_destroy(tx->tx_window);

	// Drop all messages in RETX queue
	tx->retx_queue.clear();

	tx->base_tx.base.tx_enabled = false;
	oset_thread_mutex_unlock(&tx->base_tx.mutex);
}

static void rlc_am_nr_tx_stop(rlc_am_nr_tx *tx)
{
	suspend_tx(tx);
	rlc_am_base_tx_stop(&tx->base_tx);
}

static void rlc_am_nr_tx_reestablish(rlc_am_nr_tx *tx)
{
	suspend_tx(tx);
}

static bool rlc_am_nr_tx_configure(rlc_am_nr_tx *tx, rlc_config_t *cfg_, char *rb_name_)
{
	tx->base_tx.rb_name = rb_name_;
	tx->base_tx.cfg = cfg_->am_nr;

	if (cfg_->tx_queue_length > max_tx_queue_size) {
	  RlcError("configuring tx queue length of %d PDUs too big. Maximum value is %d.",
			   cfg_->tx_queue_length,
			   max_tx_queue_size);
	  return false;
	}
	
	tx->mod_nr = cardinality(tx->base_tx.cfg.tx_sn_field_length);
	tx->AM_Window_Size = am_window_size(tx->base_tx.cfg.tx_sn_field_length);

	switch (tx->base_tx.cfg.tx_sn_field_length) {
	  case (rlc_am_nr_sn_size_t)size12bits:
		tx->min_hdr_size = 2;
		break;
	  case (rlc_am_nr_sn_size_t)size18bits:
		tx->min_hdr_size = 3;
		break;
	  default:
		RlcError("attempt to configure unsupported tx_sn_field_length %s", rlc_am_nr_sn_size_to_string(tx->base_tx.cfg.tx_sn_field_length));
		return false;
	}
	
	tx->max_hdr_size = tx->min_hdr_size + tx->so_size;

	// make sure Tx queue is empty before attempting to resize
	oset_list_init(&tx->base_tx.tx_sdu_queue);//cfg_->tx_queue_length
	
	// Configure t_poll_retransmission timer
	if (tx->base_tx.cfg.t_poll_retx > 0) {
	  tx->poll_retransmit_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_tx_timer_expired, &tx->base_tx);
	}
	
	tx->base_tx.base.tx_enabled = true;
	
	RlcDebug("RLC AM NR configured tx entity");

	return true;
}

static void rlc_am_nr_tx_init(rlc_am_nr_tx *tx, rlc_am_nr_rx *rx, rlc_um_base *base)
{
	tx->base_tx.base = base;
	tx->base_tx.rx = rx;
	tx->base_tx.tx = tx;

	tx->min_hdr_size = 2;
	tx->so_size      = 2;
	tx->max_hdr_size = 4;
	tx->mod_nr       = 0;

	tx->tx_window = oset_hash_make();
	oset_list_init(&tx->tx_window);

	rlc_am_base_tx_init(&tx->base_tx);
}

static void suspend_rx(rlc_am_nr_rx *rx)
{
	oset_thread_mutex_lock(&rx->base_rx.mutex);
	if(rx->status_prohibit_timer) gnb_timer_delete(rx->status_prohibit_timer);
	if(rx->reassembly_timer) gnb_timer_delete(rx->reassembly_timer);

	rx->st = {0};
	// Drop all messages in RX window
	oset_hash_destroy(rx->rx_window);
	oset_thread_mutex_unlock(&rx->base_rx.mutex);
}

static void rlc_am_nr_rx_reestablish(rlc_am_nr_rx *rx)
{
	suspend_rx(rx);
}

static void rlc_am_nr_rx_stop(rlc_am_nr_rx *rx)
{
	suspend_rx(rx);
	rlc_am_base_rx_stop(&rx->base_rx);
}

static bool rlc_am_nr_rx_configure(rlc_am_nr_rx *rx, rlc_config_t *cfg_, char *rb_name_)
{
	rx->base_rx.rb_name = rb_name_;
	rx->base_rx.cfg = cfg_->am_nr;

	// configure timer
	if (rx->base_rx.cfg.t_status_prohibit > 0) {
		rx->status_prohibit_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_rx_timer_expired, &rx->base_rx);
	}

	// Configure t_reassembly timer
	if (rx->base_rx.cfg.t_reassembly > 0) {
		rx->reassembly_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_rx_timer_expired, &rx->base_rx);
	}

	rx->mod_nr = cardinality(rx->base_rx.cfg.rx_sn_field_length);
	rx->AM_Window_Size = am_window_size(rx->base_rx.cfg.rx_sn_field_length);

	rx->base_rx.base.rx_enabled = true;

	RlcDebug("RLC AM NR configured rx entity.");

	return true;
}

static void rlc_am_nr_rx_init(rlc_am_nr_rx *rx, rlc_am_nr_tx *tx, rlc_um_base *base)
{
	rx->base_rx.base = base;
	rx->base_rx.tx = tx;
	rx->base_rx.rx = rx;

	rx->rx_window = oset_hash_make();
	rlc_am_base_rx_init(&rx->base_rx, base)
}

#endif

/////////////////////////////////////////////////////////////////////////////
void rlc_am_nr_stop(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_um_nr *)am_common;

	rlc_am_nr_rx_stop(&am->rx);
	rlc_am_nr_tx_stop(&am->tx);
	rlc_am_base_stop(&am->base);
}


bool rlc_am_nr_configure(rlc_common *am_common, rlc_config_t *cfg_)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	// store config
	am->base.cfg = *cfg_;

	// determine bearer name and configure Rx/Tx objects
	am->base.common.rb_name = oset_strdup(get_rb_name(am->base.common.lcid));

	if (! rlc_am_nr_rx_configure(&am->rx, &am->base.cfg, am->base.common.rb_name)) {
		RlcError("Error configuring bearer (RX)");
		return false;
	}

	if (! rlc_am_nr_tx_configure(&am->tx, &am->base.cfg, am->base.common.rb_name)) {
		RlcError("Error configuring bearer (TX)");
		return false;
	}

	RlcInfo("AM NR configured - tx_sn_field_length=%d, rx_sn_field_length=%d, "
	        "t_poll_retx=%d, poll_pdu=%d, poll_byte=%d, "
	        "max_retx_thresh=%d, t_reassembly=%d, t_status_prohibit=%d, tx_queue_length=%d",
	        rlc_am_nr_sn_size_to_number(am->base.cfg.am_nr.tx_sn_field_length),
	        rlc_am_nr_sn_size_to_number(am->base.cfg.am_nr.rx_sn_field_length),
	        am->base.cfg.am_nr.t_poll_retx,
	        am->base.cfg.am_nr.poll_pdu,
	        am->base.cfg.am_nr.poll_byte,
	        am->base.cfg.am_nr.max_retx_thresh,
	        am->base.cfg.am_nr.t_reassembly,
	        am->base.cfg.am_nr.t_status_prohibit,
	        am->base.cfg.tx_queue_length);

  return true;
}

void rlc_am_nr_reestablish(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	RlcDebug("Reestablished bearer");
	rlc_am_nr_tx_reestablish(&am->tx); // calls stop and enables tx again
	rlc_am_nr_rx_reestablish(&am->rx); // calls only stop
	am->base.tx_enabled = true;
}

rlc_am_nr *rlc_am_nr_init(uint32_t lcid_,	uint16_t rnti_)
{
	rlc_am_nr *am = oset_malloc(sizeof(rlc_am_nr));
	ASSERT_IF_NOT(am, "lcid %u Could not allocate pdcp nr context from pool", lcid_);
	memset(am, 0, sizeof(rlc_am_nr));

	rlc_am_base_init(&am->base, lcid_, rnti_);
	rlc_am_nr_tx_init(&am->tx, &am->rx, &am->base);
	rlc_am_nr_rx_init(&am->rx, &am->tx, &am->base);

	am->base.common.func = {
						._get_buffer_state  = rlc_am_nr_get_buffer_state,
						._configure         = rlc_am_nr_configure,
						._set_bsr_callback  = rlc_am_nr_set_bsr_callback,
						._get_metrics 	    = rlc_am_nr_get_metrics,
						._reset_metrics     = rlc_am_nr_reset_metrics,
						._reestablish       = rlc_am_nr_reestablish,
						._write_ul_pdu      = rlc_am_nr_write_ul_pdu,
						._read_dl_pdu       = rlc_am_nr_read_dl_pdu;
						._write_dl_sdu      = rlc_am_nr_write_dl_sdu,
						._get_mode		    = rlc_am_nr_get_mode,
						._sdu_queue_is_full	= rlc_am_nr_sdu_queue_is_full,
						._discard_sdu	    = rlc_am_nr_discard_sdu,
						._stop              = rlc_am_nr_stop,
					  };
}

