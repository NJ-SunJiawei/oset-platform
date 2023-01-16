/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_timer.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-timer"

void *gnb_timer_task(oset_threadplus_t *thread, void *data)
{
	msg_def_t *received_msg = NULL;
	uint32_t length = 0;
	task_map_t *task = taskmap[TASK_TIMER];
	int rv = 0;
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Starting Timer thread");

	while(gnb_manager_self()->running){
		 oset_timer_mgr_expire(gnb_manager_self()->app_timer);		   
		 for ( ;; ){
			 rv = oset_ring_queue_try_get(task->msg_queue, &received_msg, &length);
			 if(rv != OSET_OK)
			 {
				if (rv == OSET_DONE)
					return;
			 
				if (rv == OSET_RETRY){
					break;
				}
			 }
			 //func
			 received_msg = NULL;
			 length = 0;
		}
	}
}

