/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_signalling.h"
#include "mac/sched_nr_worker.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-si"

void si_sched_init(si_sched *si,bwp_params_t *bwp_cfg_)
{
	si->bwp_cfg = bwp_cfg_;

	for (uint32_t i = 0; i < byn_array_get_count(bwp_cfg_->cell_cfg.sibs); ++i) {
		sched_nr_cell_cfg_sib_t *sibs = byn_array_get_data(bwp_cfg_->cell_cfg.sibs, i);

		si_msg_ctxt_t *si_ct = oset_core_alloc(mac_manager_self()->app_pool, sizeof(si_msg_ctxt_t));
		byn_array_add(&si->pending_sis, si_ct);
		si_ct->n              = i;
		si_ct->len_bytes      = sibs->len;
		si_ct->period_frames  = sibs->period_rf;
		si_ct->win_len_slots  = sibs->si_window_slots;
		si_ct->si_softbuffer  = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->nof_prb);
	}
}

