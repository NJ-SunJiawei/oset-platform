/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_rb.h"
		
		
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-rb"

/// TS 38.214, Table 6.1.2.2.1-1 - Nominal RBG size P
static uint32_t get_P(uint32_t bwp_nof_prb, bool config_1_or_2)
{
  ASSERT_IF_NOT(bwp_nof_prb > 0 && bwp_nof_prb <= 275, "Invalid BWP size");
  if (bwp_nof_prb <= 36) {
    return config_1_or_2 ? 2 : 4;
  }
  if (bwp_nof_prb <= 72) {
    return config_1_or_2 ? 4 : 8;
  }
  if (bwp_nof_prb <= 144) {
    return config_1_or_2 ? 8 : 16;
  }
  return 16;
}


/// TS 38.214 - total number of RBGs for a uplink bandwidth part of size "bwp_nof_prb" PRBs
static uint32_t get_nof_rbgs(uint32_t bwp_nof_prb, uint32_t bwp_start, bool config1_or_2)
{
  uint32_t P = get_P(bwp_nof_prb, config1_or_2);
  return ceil_div(bwp_nof_prb + (bwp_start % P), P);
}

uint32_t get_rbg_size(uint32_t bwp_nof_prb, uint32_t bwp_start, bool config1_or_2, uint32_t rbg_idx)
{
  uint32_t P        = get_P(bwp_nof_prb, config1_or_2);
  uint32_t nof_rbgs = get_nof_rbgs(bwp_nof_prb, bwp_start, config1_or_2);
  if (rbg_idx == 0) {
    return P - (bwp_start % P);
  }
  if (rbg_idx == nof_rbgs - 1) {
    uint32_t ret = (bwp_start + bwp_nof_prb) % P;
    return ret > 0 ? ret : P;
  }
  return P;
}


void bwp_rb_bitmap_init(bwp_rb_bitmap *prbmap, uint32_t bwp_nof_prbs, uint32_t bwp_prb_start_, bool config1_or_2)
{
	prbmap->prbs_ = bwp_nof_prbs;
	prbmap->rbgs_ = get_nof_rbgs(bwp_nof_prbs, bwp_prb_start_, config1_or_2);
	prbmap->P_ = get_P(bwp_nof_prbs, config1_or_2);
	prbmap->Pnofbits = log2(prbmap->P_);
	prbmap->first_rbg_size = get_rbg_size(bwp_nof_prbs, bwp_prb_start_, config1_or_2, 0);
}

