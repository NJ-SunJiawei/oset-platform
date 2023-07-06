/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#include "gnb_common.h"
#include "rrc/rrc_timer.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc-timer"

static void rrc_timer_event_send(rrc_timer_e timer_id, void *data)
{
    oset_assert(data);

	msg_def_t *msg_ptr = task_alloc_msg(TASK_RRC, TIMER_HAS_EXPIRED);
	oset_assert(msg_ptr);

	TIMER_HAS_EXPIRED(msg_ptr).timer_id = timer_id;
	TIMER_HAS_EXPIRED(msg_ptr).arg = data;
	task_send_msg(TASK_TIMER, msg_ptr);
}

void activity_timer_event(void *data)
{
    rrc_timer_event_send(RRC_TIMER_ACTIVITY, data);
}

