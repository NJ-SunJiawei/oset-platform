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
	cvector_reserve(si->pending_sis, 10);

	ASSERT_IF_NOT(cvector_size(bwp_cfg_->cell_cfg.sibs) <= 10, "si_sched_init error")

	for (uint32_t i = 0; i < cvector_size(bwp_cfg_->cell_cfg.sibs); ++i) {
		si_msg_ctxt_t *si_ct = si->pending_sis[i];
		si_ct->n              = i;
		si_ct->len_bytes      = bwp_cfg_->cell_cfg.sibs[i].len;
		si_ct->period_frames  = bwp_cfg_->cell_cfg.sibs[i].period_rf;
		si_ct->win_len_slots  = bwp_cfg_->cell_cfg.sibs[i].si_window_slots;
		si_ct->si_softbuffer  = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->nof_prb);
	}
}

