/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-timer"

/*
static void gnb_timer_task_handle(msg_def_t *msg_p, uint32_t msg_l)
{
	oset_assert(msg_p);
	oset_assert(msg_l > 0);
	switch (RQUE_MSG_ID(msg_p))
	{	
		case TIMER_HAS_EXPIRED:
			//rrc
			if (TIMER_HAS_EXPIRED (msg_p).timer_id == RRC_TIMER_ACTIVITY) {
				activity_timer_expired(TIMER_HAS_EXPIRED (msg_p).arg);
			}

			break;
		
		default:
			oset_error("Received unknown message: %d:%s",  RQUE_MSG_ID(msg_p), RQUE_MSG_NAME(msg_p));
			break;
	}
}*/


void *gnb_timer_task(oset_threadplus_t *thread, void *data)
{
	msg_def_t *received_msg = NULL;
	uint32_t length = 0;
	task_map_t *task = taskmap[TASK_TIMER];
	int rv = 0;
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Starting Timer thread");

	while(gnb_manager_self()->running){
		 gnb_timer_mgr_expire(gnb_manager_self()->app_timer);		   
		 /*for ( ;; ){
			 rv = oset_ring_queue_try_get(task->msg_queue, &received_msg, &length);
			 if(rv != OSET_OK)
			 {
				if (rv == OSET_DONE)
					return;
			 
				if (rv == OSET_RETRY){
					break;
				}
			 }
			 gnb_timer_task_handle(received_msg, length);
			 task_free_msg(RQUE_MSG_ORIGIN_ID(received_msg), received_msg);
			 received_msg = NULL;
			 length = 0;
		}*/
	}
}

