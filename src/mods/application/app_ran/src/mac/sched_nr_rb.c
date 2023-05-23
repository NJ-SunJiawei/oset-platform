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

static void add_prbs_to_rbgs_by_prb_bitmap(bwp_rb_bitmap *prb_map, prb_bitmap *grant)
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


static void add_prbs_to_rbgs_by_prb_interval(bwp_rb_bitmap *prb_map, prb_interval *grant)
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

void bwp_rb_bitmap_add_by_prb_interval(bwp_rb_bitmap *prb_map, prb_interval *grant)
{
	bit_fill(&prb_map->prbs_, grant->start_, grant->stop_, true);
	add_prbs_to_rbgs_by_prb_interval(prb_map, grant);
}

void bwp_rb_bitmap_add_by_rbg_bitmap(bwp_rb_bitmap *prb_map, rbg_bitmap *grant)
{
	bit_or_eq(prb_map->rbgs_, grant);
	add_rbgs_to_prbs(prb_map, grant);
}

void bwp_rb_bitmap_add_by_prb_bitmap(bwp_rb_bitmap *prb_map, prb_bitmap *grant)
{
	bit_or_eq(prb_map->prbs_, grant);
	add_prbs_to_rbgs_by_prb_bitmap(prb_map, grant);
}

void bwp_rb_bitmap_add_by_prb_grant(bwp_rb_bitmap *prb_map, prb_grant *grant)
{
  if (is_alloc_type0(grant)) {
  	oset_assert(is_alloc_type0(grant), "Invalid access to rbgs() field of grant with alloc type 1");
	bwp_rb_bitmap_add_by_rbg_bitmap(prb_map, &grant->alloc.rbgs);
  } else {
  	oset_assert(is_alloc_type1(grant), "Invalid access to prbs() field of grant with alloc type 0");
	bwp_rb_bitmap_add_by_prb_interval(prb_map, &grant->alloc.interv);
  }
}

void bwp_rb_bitmap_reset(bwp_rb_bitmap *prb_map)
{
	bit_reset_all(&prb_map->prbs_);
	bit_reset_all(&prb_map->rbgs_);
}

void bwp_rb_bitmap_init(bwp_rb_bitmap *prb_map, uint32_t bwp_nof_prbs, uint32_t bwp_prb_start_, bool config1_or_2)
{
	bit_init(&prb_map->prbs_, SRSRAN_MAX_PRB_NR, bwp_nof_prbs, true);//SRSRAN_MAX_PRB_NR
	bit_resize(&prb_map->prbs_, bwp_nof_prbs);
	bit_init(&prb_map->rbgs_, SCHED_NR_MAX_NOF_RBGS, get_nof_rbgs(bwp_nof_prbs, bwp_prb_start_, config1_or_2), true);//SCHED_NR_MAX_NOF_RBGS
	bit_resize(&prb_map->rbgs_, get_nof_rbgs(bwp_nof_prbs, bwp_prb_start_, config1_or_2));

	prb_map->P_ = get_P(bwp_nof_prbs, config1_or_2);
	prb_map->Pnofbits = log2(prb_map->P_);
	prb_map->first_rbg_size = get_rbg_size(bwp_nof_prbs, bwp_prb_start_, config1_or_2, 0);
}

void prb_interval_init(prb_interval *prb_interval, uint32_t start_point, uint32_t stop_point)
{
	prb_interval->start_ = start_point;
	prb_interval->stop_ = stop_point;
}

bool prb_interval_empty(prb_interval *prb_interval)
{
	return (prb_interval->stop_ == prb_interval->start_);
}

uint32_t prb_interval_length(prb_interval *prb_interval)
{
	return (prb_interval->stop_ - prb_interval->start_);
}

prb_grant* prb_grant_interval_init(prb_grant *grant, prb_interval *interval)
{
	grant->alloc_type_0 = false;
	grant->alloc.interv = *interval;

	return grant;
}

prb_grant* prb_grant_rbgs_init(prb_grant *grant, rbg_bitmap *rbgs)
{
	grant->alloc_type_0 = true;
	grant->alloc.rbgs = *rbgs;

	return grant;
}

//查看是否交集
bool prb_grant_collides(bwp_rb_bitmap *prb_map, prb_grant *grant)
{
  if (is_alloc_type0(grant)) {
  	bounded_bitset res = bit_and(&prb_map->rbgs_, &grant->alloc.rbgs);
	return bit_any(&res);
  }
  return bit_any_range(&prb_map->prbs_, grant->alloc.interv.start_, grant->alloc.interv.stop_);
}

bool is_alloc_type0(prb_grant *prb_grant)  { return prb_grant->alloc_type_0; }
bool is_alloc_type1(prb_grant *prb_grant)  { return !is_alloc_type0(prb_grant); }

prb_interval find_next_empty_interval(prb_bitmap *mask, size_t start_prb_idx, size_t last_prb_idx)
{
  int rb_start = bit_find_lowest(mask, start_prb_idx, MIN(bit_size(mask), last_prb_idx), false);
  if (rb_start != -1) {
    int rb_end = bit_find_lowest(mask, rb_start + 1, MIN(bit_size(mask), last_prb_idx), true);
    return {(uint32_t)rb_start, (uint32_t)(rb_end < 0 ? bit_size(mask) : rb_end)};
  }
  return {0, 0};
}

prb_interval find_empty_interval_of_length(prb_bitmap *mask, size_t nof_prbs, uint32_t start_prb_idx)
{
  prb_interval max_interv = {0};
  do {
    prb_interval interv = find_next_empty_interval(mask, start_prb_idx, bit_size(mask));
    if (prb_interval_empty(&interv)) {
		break;
    }
    if (prb_interval_length(&interv) >= nof_prbs) {
		max_interv.start_ = interv.start_;
		max_interv.stop_  = interv.start_ + nof_prbs;
		break;
    }
    if (prb_interval_length(&interv) > prb_interval_length(&max_interv)) {
		max_interv = interv;
    }
    start_prb_idx = interv.stop_ + 1;
  } while (start_prb_idx < bit_size(mask));
  return max_interv;
}

