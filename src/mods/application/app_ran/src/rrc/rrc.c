/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "rrc.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc"


static rrc_manager_t rrc_manager = {0};

rrc_manager_t *rrc_manager_self(void)
{
	return &rrc_manager;
}

static void rrc_manager_init(void)
{
	rrc_manager.app_pool = gnb_manager_self()->app_pool;

}

static void rrc_manager_destory(void)
{
	rrc_manager.app_pool = NULL; /*app_pool release by openset process*/
}


void *gnb_rrc_task(oset_threadplus_t *thread, void *data)
{
    msg_def_t *received_msg = NULL;
	uint32_t length = 0;
    task_map_t *task = task_map_self(TASK_RRC);
    int rv = 0;
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Starting RRC layer thread");

     for ( ;; ){
		 rv = oset_ring_queue_try_get(task->msg_queue, &received_msg, &length);
		 if(rv != OSET_OK)
		 {
			if (rv == OSET_DONE)
				break;
		 
			if (rv == OSET_RETRY){
				continue;
			}
		 }
		 //func
		 received_msg = NULL;
		 length = 0;
	}
}


