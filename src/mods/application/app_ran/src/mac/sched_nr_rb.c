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

static uint32_t prb_to_rbg_idx(bwp_rb_bitmap *prb_map, uint32_t prb_idx) const
{
  return ((prb_idx + prb_map->P_ - prb_map->first_rbg_size) >> prb_map->Pnofbits);
}

static void add_prbs_to_rbgs_by_bitmap(bwp_rb_bitmap *prb_map, prb_bitmap *grant)
{
  int idx = 0;
  do {
    idx = bit_find_lowest(grant, idx, bit_size(grant), true);
    if (idx < 0) {
      return;
    }
    uint32_t rbg_idx = prb_to_rbg_idx(idx);
    bit_set(prb_map->rbgs_, rbg_idx, true);
    idx++;
  } while (idx != (int)bit_size(prb_map->prbs_));
}


static void add_prbs_to_rbgs_by_interval(bwp_rb_bitmap *prb_map, prb_interval *grant)
{
  uint32_t rbg_start = prb_to_rbg_idx(prb_map, grant->start_);
  uint32_t rbg_stop  = MIN(prb_to_rbg_idx(prb_map, grant->stop_ - 1) + 1u, (uint32_t)bit_size(prb_map->rbgs_));
  bit_fill(&prb_map->rbgs_, rbg_start, rbg_stop, true);
}

static void add_rbgs_to_prbs(bwp_rb_bitmap *prb_map, rbg_bitmap *grant)
{
  int idx = 0;
  do {
    idx = bit_find_lowest(grant, idx, bit_size(grant), true);
    if (idx < 0) {
      return;
    }
    uint32_t prb_idx = (idx - 1) * prb_map->P_ + prb_map->first_rbg_size;
    uint32_t prb_end = MIN(prb_idx + ((idx == 0) ? prb_map->first_rbg_size : prb_map->P_), (uint32_t)bit_size(prb_map->prbs_));
    bit_fill(prb_map->prbs_, prb_idx, prb_end, true);
    idx++;
  } while (idx != (int)bit_size(prb_map->prbs_));
}

prb_bitmap get_prbs(bwp_rb_bitmap *prb_map)
{ 
	return prb_map->prbs_;
}

rbg_bitmap get_rbgs(bwp_rb_bitmap *prb_map)
{
	return prb_map->rbgs_;
}

/// TS 38.214, Table 6.1.2.2.1-1 - Nominal RBG size P
uint32_t get_P(uint32_t bwp_nof_prb, bool config_1_or_2)
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
uint32_t get_nof_rbgs(uint32_t bwp_nof_prb, uint32_t bwp_start, bool config1_or_2)
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

void bwp_rb_bitmap_add_by_interval(bwp_rb_bitmap *prb_map, prb_interval *grant)
{
	bit_fill(&prb_map->prbs_, grant->start_, grant->stop_, true);
	add_prbs_to_rbgs_by_interval(prb_map, grant);
}

void bwp_rb_bitmap_add_by_bitmap(bwp_rb_bitmap *prb_map, prb_bitmap *grant)
{
	bit_or(prb_map->prbs_, grant);
	add_prbs_to_rbgs_by_bitmap(prb_map, grant);
}


void bwp_rb_bitmap_init(bwp_rb_bitmap *prb_map, uint32_t bwp_nof_prbs, uint32_t bwp_prb_start_, bool config1_or_2)
{
	bit_init(&prb_map->prbs_, SRSRAN_MAX_PRB_NR, bwp_nof_prbs, true);//SRSRAN_MAX_PRB_NR
	bit_init(&prb_map->rbgs_, SCHED_NR_MAX_NOF_RBGS, get_nof_rbgs(bwp_nof_prbs, bwp_prb_start_, config1_or_2), true);//SCHED_NR_MAX_NOF_RBGS

	prb_map->P_ = get_P(bwp_nof_prbs, config1_or_2);
	prb_map->Pnofbits = log2(prb_map->P_);
	prb_map->first_rbg_size = get_rbg_size(bwp_nof_prbs, bwp_prb_start_, config1_or_2, 0);
}


void prb_interval_init(prb_interval *prb_interval, uint32_t start_point, uint32_t stop_point)
{
	prb_interval->start_ = start_point;
	prb_interval->stop_ = stop_point;
}

