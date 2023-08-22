/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/

#ifndef SCHED_TIME_RR_H_
#define SCHED_TIME_RR_H_

#include "mac/sched_grant_allocator.h"


#ifdef __cplusplus
extern "C" {
#endif


void sched_nr_time_rr_sched_dl_users(bwp_slot_allocator *bwp_alloc, cvector_vector_t(slot_ue *) slot_ue_list);
void sched_nr_time_rr_sched_ul_users(bwp_slot_allocator *bwp_alloc, cvector_vector_t(slot_ue *) slot_ue_list);

#ifdef __cplusplus
}
#endif

#endif
