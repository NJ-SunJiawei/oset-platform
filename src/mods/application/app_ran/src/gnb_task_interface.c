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
#define OSET_LOG2_DOMAIN   "app-gnb-task"

static task_map_t *taskmap[TASK_MAX] = {NULL};
static int nb_queues=0;
#define NUM_OF_TASK_TIMER      1


static int task_queue_create(const task_info_t *taskInfo) 
{
    int newQueue=nb_queues++;
	oset_assert(nb_queues <= TASK_MAX);
	taskmap[newQueue] = oset_malloc(sizeof(task_map_t));
	oset_assert(taskmap[newQueue]);
	taskmap[newQueue]->task_id = newQueue;
	memcpy(&taskmap[newQueue]->info, taskInfo, sizeof(task_info_t));
	taskmap[newQueue]->msg_queue = oset_ring_queue_create(taskInfo->queue_size);
	oset_assert(taskmap[newQueue]->msg_queue);

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "create task queue:%s to task %d", taskInfo->name, newQueue);
	return newQueue;
}

int task_queue_init(const task_info_t *tasks)
{
    for(int i=0; i<TASK_MAX; ++i)
    {
	  task_queue_create(&tasks[i]);
    }
    return OSET_OK;
}

void task_queue_termination()
{
    for(int i=0; i<nb_queues; ++i)
    {
        oset_ring_queue_term(taskmap[i]->msg_queue);
    }
}

static void task_queue_destory(const task_info_t *taskInfo) 
{
    int queue=nb_queues--;
	oset_assert(nb_queues >= 0);
	oset_ring_queue_destroy(taskmap[queue]->msg_queue);
	oset_free(taskmap[queue]);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "destory task queue:%s", taskInfo->name);
}


int task_queue_end(const task_info_t *tasks)
{
    for(int i=0; i<TASK_MAX; ++i)
    {
	  task_queue_destory(&tasks[i]);
    }
    return OSET_OK;
}

msg_def_t *task_alloc_msg(task_id_t origin_task_id, msg_ids_t message_id)
{
    int size=sizeof(msg_header_t) + messages_info[message_id].size;
    msg_def_t *temp = (msg_def_t *)oset_malloc(size);
    oset_expect_or_return_val(temp, NULL);
    temp->msg_header.message_id = message_id;
    temp->msg_header.ori_task_id = origin_task_id;
    temp->msg_header.det_task_id=TASK_UNKNOWN;
    temp->msg_header.phy_time = {0};
    temp->msg_header.size = size;
    return temp;
}

int task_send_msg(task_id_t destination_task_id, msg_def_t *message)
{
    int ret = OSET_ERROR;
    task_map_t *t=taskmap[destination_task_id];

	message->msg_header.det_task_id = destination_task_id;
    ret = oset_ring_queue_put(t->msg_queue, (uint8_t *)message, message->msg_header.size);
    if (OSET_ERROR == ret) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "task send msg[%s] to %s failed",get_message_name(message->msg_header.message_id), get_task_name(destination_task_id));
	    oset_free(message);
    }
    return ret;
}

task_map_t *task_map_self(task_id_t task_id)
{
    return taskmap[task_id];
}

const char *get_message_name(msg_ids_t message_id)
{
    return messages_info[message_id].name;
}

const char *get_task_name(task_id_t task_id)
{
    return taskmap[task_id]->info.name;
}

int task_thread_create(task_id_t task_id, void *data)
{
	oset_threadattr_t *attr = NULL;
    task_map_t *t = taskmap[task_id];
	oset_assert(t);
    int priority = t->info.priority;

	attr = oset_threadattr_create();
	oset_threadattr_detach_set(attr, 0);//JOINABLE
	oset_threadattr_inheritsched_set(attr, 1);//PTHREAD_EXPLICIT_SCHED
	//oset_threadattr_schedpolicy_set(attr, SCHEDPOLICY_RR);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG,"task thread %s sched form: %s",get_task_name(task_id), oset_threadattr_schedpolicy_get(attr));
    if(!strcmp(oset_threadattr_schedpolicy_get(attr),"SCHED_FIFO")){
		if(priority<oset_threadattr_getpriority_min(SCHEDPOLICY_FIFO)) priority=oset_threadattr_getpriority_min(SCHEDPOLICY_FIFO);
		if(priority>oset_threadattr_getpriority_max(SCHEDPOLICY_FIFO)) priority=oset_threadattr_getpriority_max(SCHEDPOLICY_FIFO);
		oset_threadattr_schedparam_set(attr, priority);
	}else if(!strcmp(oset_threadattr_schedpolicy_get(attr),"SCHED_RR")){
		if(priority<oset_threadattr_getpriority_min(SCHEDPOLICY_RR)) priority=oset_threadattr_getpriority_min(SCHEDPOLICY_RR);
		if(priority>oset_threadattr_getpriority_max(SCHEDPOLICY_RR)) priority=oset_threadattr_getpriority_max(SCHEDPOLICY_RR);
		oset_threadattr_schedparam_set(attr, priority);
	}
    t->thread = oset_threadplus_create(attr, t->info.func, data);
    oset_expect_or_return_val(t->thread, OSET_ERROR);
    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Create thread Task %s success", get_task_name(task_id));

    /*todo*/
    /*CPU_SET*/
    /*pthread_attr_setaffinity_np*/
	return OSET_OK;
}

