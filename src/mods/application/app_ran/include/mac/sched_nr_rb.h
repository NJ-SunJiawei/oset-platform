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

#include "lib/srsran/phy/common/phy_common_nr.h"
#include "lib/common/bitset_helper.h"

#define SCHED_NR_MAX_NOF_RBGS  18

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


#ifdef __cplusplus
}
#endif

#endif
