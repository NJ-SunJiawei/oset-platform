/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_sch.h"
			
			
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-sch"

void pdsch_allocator_reset(pdsch_allocator *pdsch)
{
	cvector_clear(pdsch->pdschs);
	bit_reset_all(&pdsch->dl_prbs.prbs_);
	bit_reset_all(&pdsch->dl_prbs.rbgs_);
}


void pdsch_allocator_init(pdsch_allocator *pdsch, bwp_params_t *cfg_, uint32_t slot_index, cvector_vector_t(pdsch_t) pdsch_lst)
{
	pdsch->bwp_cfg = cfg_;
	pdsch->slot_idx = slot_index;
	pdsch->pdschs = pdsch_lst;
	//dl_prbs
	bwp_rb_bitmap_init(&pdsch->dl_prbs, cfg_->cfg.rb_width, cfg_->cfg.start_rb, cfg_->cfg.pdsch.rbg_size_cfg_1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void pusch_allocator_reset(pusch_allocator *pusch)
{
	cvector_clear(pusch->puschs);
	bit_reset_all(&pusch->ul_prbs.prbs_);
	bit_reset_all(&pusch->ul_prbs.rbgs_);
}

void pusch_allocator_init(pusch_allocator *pusch, bwp_params_t *cfg_, uint32_t slot_index,  cvector_vector_t(pusch_t) pusch_lst)
{
	pusch->bwp_cfg = cfg_;
	pusch->slot_idx = slot_idx;
	pusch->puschs = pusch_lst;
	//ul_prbs
	bwp_rb_bitmap_init(&pusch->ul_prbs, cfg_->cfg.rb_width, cfg_->cfg.start_rb, cfg_->cfg.pdsch.rbg_size_cfg_1);
}

