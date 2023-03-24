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


void si_sched_init(si_sched *si,bwp_params_t *bwp_cfg_)
{
	si->bwp_cfg = bwp_cfg_;

  for (uint32_t i = 0; i < byn_array_get_count(bwp_cfg_->cell_cfg.sibs); ++i) {
    si->pending_sis.emplace_back();

    si_msg_ctxt_t& si = pending_sis.back();
    si.n              = i;
    si.len_bytes      = bwp_cfg->cell_cfg.sibs[i].len;
    si.period_frames  = bwp_cfg->cell_cfg.sibs[i].period_rf;
    si.win_len_slots  = bwp_cfg->cell_cfg.sibs[i].si_window_slots;
    si.si_softbuffer  = harq_softbuffer_pool::get_instance().get_tx(bwp_cfg->nof_prb);
  }
}

