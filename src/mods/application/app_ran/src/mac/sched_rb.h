/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef SCHED_RB_H_
#define SCHED_RB_H_

#include "lib/common/bitset_helper.h"

//#define SCHED_NR_MAX_NOF_RBGS  18

#ifdef __cplusplus
extern "C" {
#endif

/// Struct to express a {min,...,max} range of PRBs
typedef struct {
	uint32_t start_;
	uint32_t stop_;
}prb_interval;

typedef bounded_bitset prb_bitmap;
typedef bounded_bitset rbg_bitmap;

typedef struct {
	prb_bitmap prbs_;//bounded_bitset<SRSRAN_MAX_PRB_NR, true>
	rbg_bitmap rbgs_;//bounded_bitset<SCHED_NR_MAX_NOF_RBGS, true>
	uint32_t   P_;
	uint32_t   Pnofbits;
	uint32_t   first_rbg_size;
}bwp_rb_bitmap;

typedef struct {
  bool alloc_type_0;
  union alloc_t {
    rbg_bitmap   rbgs; //true
    prb_interval interv;
  } alloc;
}prb_grant;

prb_bitmap* get_prbs(bwp_rb_bitmap *prb_map);
rbg_bitmap* get_rbgs(bwp_rb_bitmap *prb_map);
uint32_t get_P(uint32_t bwp_nof_prb, bool config_1_or_2);
uint32_t get_nof_rbgs(uint32_t bwp_nof_prb, uint32_t bwp_start, bool config1_or_2);
uint32_t get_rbg_size(uint32_t bwp_nof_prb, uint32_t bwp_start, bool config1_or_2, uint32_t rbg_idx);

void bwp_rb_bitmap_reset(bwp_rb_bitmap *prb_map);
void bwp_rb_bitmap_init(bwp_rb_bitmap *prb_map, uint32_t bwp_nof_prbs, uint32_t bwp_prb_start_, bool config1_or_2);
void bwp_rb_bitmap_add_by_prb_interval(bwp_rb_bitmap *prb_map, prb_interval *grant);
void bwp_rb_bitmap_add_by_prb_bitmap(bwp_rb_bitmap *prb_map, prb_bitmap *grant);
void bwp_rb_bitmap_add_by_rbg_bitmap(bwp_rb_bitmap *prb_map, rbg_bitmap *grant);
void bwp_rb_bitmap_add_by_prb_grant(bwp_rb_bitmap *prb_map, prb_grant *grant);

void prb_interval_init(prb_interval *prb_interval, uint32_t start_point, uint32_t stop_point);
bool prb_interval_empty(prb_interval *prb_interval);
int32_t prb_interval_length(prb_interval *prb_interval);

prb_grant prb_grant_interval_init(prb_interval *interval);
prb_grant prb_grant_rbgs_init(rbg_bitmap *rbgs);
bool prb_grant_collides(prb_grant *grant);
bool is_alloc_type0(prb_grant *prb_grant);
bool is_alloc_type1(prb_grant *prb_grant);

prb_interval find_next_empty_interval(prb_bitmap *mask, size_t start_prb_idx, size_t last_prb_idx);
prb_interval find_empty_interval_of_length(prb_bitmap *mask, size_t nof_prbs, uint32_t start_prb_idx);

#ifdef __cplusplus
}
#endif

#endif
