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
	oset_apr_mutex_init(&rrc_manager.mutex, OSET_MUTEX_NESTED, rrc_manager.app_pool);
	oset_apr_thread_cond_create(&rrc_manager.cond, rrc_manager.app_pool);

}

static void rrc_manager_destory(void)
{
	oset_apr_mutex_destroy(rrc_manager.mutex);
	oset_apr_thread_cond_destroy(rrc_manager.cond);
	rrc_manager.app_pool = NULL; /*app_pool release by openset process*/
}

static int rrc_init(void)
{
	rrc_manager_init();
	//todo
	rrc_manager.cfg = ;

	return OSET_OK;
}

static int rrc_destory(void)
{
	//todo
	rrc_manager_destory();
	return OSET_OK;
}

static void gnb_rrc_task_handle(msg_def_t *msg_p, uint32_t msg_l)
{
	oset_assert(msg_p);
	oset_assert(msg_l > 0);
	switch (RQUE_MSG_ID(msg_p))
	{	
		case 1111:
			/*info handle*/
			break;

		default:
			oset_error("Received unhandled message: %d:%s",  RQUE_MSG_ID(msg_p), RQUE_MSG_NAME(msg_p));
			break;
	}
}

void *gnb_rrc_task(oset_threadplus_t *thread, void *data)
{
	msg_def_t *received_msg = NULL;
	uint32_t length = 0;
	task_map_t *task = task_map_self(TASK_RRC);
	int rv = 0;

	rrc_init();
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Starting RRC layer thread");
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
		gnb_rrc_task_handle(received_msg, length);
		task_free_msg(RQUE_MSG_ORIGIN_ID(received_msg), received_msg);
		received_msg = NULL;
		length = 0;
	}
	rrc_destory();
}


