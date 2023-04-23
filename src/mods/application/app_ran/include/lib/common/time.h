/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#ifndef GNB_TIME_H_
#define GNB_TIME_H_

#include "oset-core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gnd_timer_mgr_s gnb_timer_mgr_t;

typedef struct gnb_timer_s {
    oset_rbnode_t rbnode;
    oset_lnode_t lnode;

    void (*cb)(void*);
    void *data;

    gnb_timer_mgr_t *manager;
    bool running;
    oset_time_t timeout;
} gnb_timer_t;


gnb_timer_mgr_t *gnb_timer_mgr_create(unsigned int capacity);
void gnb_timer_mgr_destroy(gnb_timer_mgr_t *manager);

gnb_timer_t *gnb_timer_add(
        gnb_timer_mgr_t *manager, void (*cb)(void *data), void *data);
void gnb_timer_delete(gnb_timer_t *timer);

void gnb_timer_start(gnb_timer_t *timer, oset_time_t duration);
void gnb_timer_stop(gnb_timer_t *timer);

void gnb_timer_mgr_expire(gnb_timer_mgr_t *manager);
void gnb_time_tick(int frame, int subframe);

#ifdef __cplusplus
}
#endif

#endif
