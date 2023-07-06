/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef RRC_TIMER_H_
#define RRC_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef enum {
    RRC_TIMER_BASE = 100,

    RRC_TIMER_ACTIVITY,

    MAX_NUM_OF_RRC_TIMER,

} rrc_timer_e;


void activity_timer_event(void *data);

#ifdef __cplusplus
}
#endif

#endif
