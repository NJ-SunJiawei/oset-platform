/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef SCHED_NR_RB_H_
#define SCHED_NR_RB_H_

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
    rbg_bitmap   rbgs;
    prb_interval interv;
  } alloc;
}prb_grant;


uint32_t get_P(uint32_t bwp_nof_prb, bool config_1_or_2);
uint32_t get_nof_rbgs(uint32_t bwp_nof_prb, uint32_t bwp_start, bool config1_or_2);
uint32_t get_rbg_size(uint32_t bwp_nof_prb, uint32_t bwp_start, bool config1_or_2, uint32_t rbg_idx);
void bwp_rb_bitmap_init(bwp_rb_bitmap *prb_map, uint32_t bwp_nof_prbs, uint32_t bwp_prb_start_, bool config1_or_2);
void prb_interval_init(prb_interval *prb_interval, uint32_t start_point, uint32_t stop_point);


#ifdef __cplusplus
}
#endif

#endif
