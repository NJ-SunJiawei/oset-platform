/************************************************************************
 *File name: om-routine.c
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/

#include "om-public.h"
#include "asyncProducer.h"


OSET_DECLARE(uint32_t *) system_sid_find_by_idnex(uint32_t id ,uint32_t idnex)
{
    return oset_pool_find(&om_self()->om_system_id_pool[id], idnex);
}

OSET_DECLARE(uint32_t *) sess_sid_find_by_idnex(uint32_t id ,uint32_t idnex)
{
    return oset_pool_find(&om_self()->om_sess_id_pool[id], idnex);
}

OSET_DECLARE(void) om_system_destory_debug(om_system_t *system, const char *file, const char *func, int line)
{
	oset_apr_memory_pool_t *pool = NULL;
	oset_event_t *event = NULL;
	oset_lnode2_t *lnode = NULL;
	om_session_t *node = NULL;

	uint32_t id = 0; 

	oset_sys_assert(system);
	id = system->thread_id;

	oset_log2_printf(OSET_CHANNEL_ID_LOG, OSET_LOG2_DOMAIN, file, func, line, oset_core_session_get_uuid(system->common), OSET_LOG2_NOTICE, "Thread[%u]:destory system [%ld][%s]",\
				                                                system->thread_id,oset_core_session_get_id(system->common), oset_core_session_get_uuid(system->common));
	oset_apr_mutex_lock(runtime.session_hash_mutex);
	session_manager.others_count[0]--;
	oset_apr_mutex_unlock(runtime.session_hash_mutex);

	if (oset_event_create(&event, OSET_EVENT_CHANNEL_DESTROY) == OSET_STATUS_SUCCESS) {
		oset_event_add_header(event, OSET_STACK_BOTTOM, "thread-id", "%u", system->thread_id);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "system-idnex", "%ld", oset_core_session_get_id(system->common));
		oset_event_add_header_string(event, OSET_STACK_BOTTOM, "system-uuid", oset_core_session_get_uuid(system->common));
		oset_event_add_header_string(event, OSET_STACK_BOTTOM, "system-id", system->system_id);
		oset_event_fire(&event);
	}
	//oset_scheduler_del_task_group(oset_core_session_get_uuid(system->common));
	if(system->step_task_id) oset_scheduler_del_task_id(system->step_task_id);
	if(system->pre_rp_task_id) oset_scheduler_del_task_id(system->pre_rp_task_id);
	if(system->static_task_id) oset_scheduler_del_task_id(system->static_task_id);

	if(system->all_sess_list->count){
		oset_list2_for_each(system->all_sess_list, lnode) {
			node = (om_session_t *)lnode->data;
		    om_session_destory(node);
		}
	}

	oset_list2_free(system->all_sess_list);

	for(int i = 0 ;i < MAX_NODE; i++){
		oset_list2_free(system->node_sess_list[i]);
		oset_free(system->node_id[i]);
	}

	oset_pool_free(&om_self()->om_system_id_pool[id], system_sid_find_by_idnex(id,system->idnex));

    oset_list_remove(&om_self()->system_list[id], system);
    oset_core_hash_delete(om_self()->system_hashtable[id], system->system_id);
    oset_core_hash_delete(om_self()->system_uuid_hashtable[id], system->common->uuid_str);

	pool = system->common->pool;
	system = NULL;
	
	oset_core_destroy_memory_pool(&pool);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[system Delete] Thread[%u]:Number of system is now %d", id, oset_list_count(&om_self()->system_list[id]));
}


OSET_STANDARD_SCHED_FUNC(send_pod_timestep_msg)
{
	pod_timestep_data_t *step_msg = NULL;
	om_system_t *system = NULL;
	oset_lnode2_t *lnode = NULL;	
	om_session_t *node = NULL;	
    char buf[OSET_ADDRSTRLEN] = {0};
    //static int step_num = 0;

    system = (om_system_t *)task->cmd_arg;
	oset_sys_assert(system);

    if(system->step_num > om_self()->max_step_num) system->step_num = 0;

	if(system->all_sess_list->count){
		oset_list2_for_each(system->all_sess_list, lnode) {
			node = (om_session_t *)lnode->data;
			step_msg = (pod_timestep_data_t *)oset_calloc(1, sizeof(pod_timestep_data_t));
			memcpy(&step_msg->header, node->p, sizeof(om_msg_header_t));
            step_msg->msg_type = (int)OM_MSG_TIMESTEP;
            step_msg->step_num = system->step_num;
			om_udp_sendto(node->pod_sock, &node->pod_from, (void *)step_msg, sizeof(pod_timestep_data_t));
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "thread[%u]:send_pod_timestep_msg() success, step = %d, dest pod_id[%s], addr[%s:%d]", system->thread_id, system->step_num, node->p->pod_id, OSET_ADDR(&node->pod_from, buf), OSET_PORT(&node->pod_from));
			oset_free(step_msg);
		}
		system->step_num++;
	}
}

OSET_DECLARE(void) om_get_system_by_uuid(uint32_t id, char* uuid, oset_stream_handle_t *stream)
{
	om_system_t *system = NULL;
	oset_lnode2_t *lnode = NULL;	
	om_session_t *node = NULL;	
    char buf[OSET_ADDRSTRLEN] = {0};

    system = oset_core_hash_find(om_self()->system_uuid_hashtable[id], uuid);
    if(system){
		if(system->all_sess_list->count){
			stream->write_function(stream, "Session Number of the system is %ld:\n",system->all_sess_list->count);
			oset_list2_for_each(system->all_sess_list, lnode) {
				node = (om_session_t *)lnode->data;
				stream->write_function(stream, "pod_type[%d], pod_id[%s], addr[%s:%d]\n",node->pod_type ,node->p->pod_id, OSET_ADDR(&node->pod_from, buf), OSET_PORT(&node->pod_from));
			}
		}
    }
}

static char *omc_json_pre_report_encode(pod_pre_report_t *pre_rpt)
{
    char  *report = NULL;
	cJSON *item = NULL;
	oset_sys_assert(pre_rpt);

    item = cJSON_CreateObject();

    if(pre_rpt->header.system_id){
		if (cJSON_AddStringToObject(item, "systemId", pre_rpt->header.system_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.header.systemId] json encode failed"); 
			goto end;
		}
	}else{
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "system_id is NULL");
		goto end;
	}

    if(pre_rpt->header.pod_id && strlen(pre_rpt->header.pod_id)){
		if (cJSON_AddStringToObject(item, "podId", pre_rpt->header.pod_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.header.podId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "podId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.header.podId] json encode failed"); 
			goto end;
		}
	}


    if(pre_rpt->header.container_id && strlen(pre_rpt->header.container_id)){
		if (cJSON_AddStringToObject(item, "containerId", pre_rpt->header.container_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.header.containerId] json encode failed"); 
			goto end;
		}
    }else{
		if (cJSON_AddNullToObject(item, "containerId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.header.containerId] json encode failed"); 
			goto end;
		}
	}


    if(pre_rpt->header.node_id && strlen(pre_rpt->header.node_id)){
		if (cJSON_AddStringToObject(item, "nodeId", pre_rpt->header.node_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.header.nodeId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "nodeId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.header.nodeId] json encode failed"); 
			goto end;
		}
	}


	if (cJSON_AddNumberToObject(item, "ueNum", pre_rpt->ue_pod_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.ueNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item, "gnbNum", pre_rpt->gnb_pod_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.gnbNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item, "cnNum", pre_rpt->cn_pod_num) == NULL) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om->omc:[pre-report.cnNum] json encode failed"); 
		goto end;
	}

end:
	if (item) {
		report = cJSON_Print(item);
		//report = cJSON_PrintUnformatted(item);
		oset_sys_assert(report);
		oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "om->omc pre-report send:\n%s\n\n", report);
		cJSON_Delete(item);
	}
	return report;
}



OSET_STANDARD_SCHED_FUNC(send_pre_report_msg)
{
	om_system_t *system = NULL;
	oset_lnode2_t *lnode = NULL;	
	om_session_t *node = NULL;	
	int ue_pod_num = 0;
	int gnb_pod_num = 0;
	int cn_pod_num = 0;
	pod_pre_report_t *pre_report = NULL;
	char *pre_context = NULL;

	system = (om_system_t *)task->cmd_arg;
	oset_sys_assert(system);

	if(system->all_sess_list->count){
		oset_list2_for_each(system->all_sess_list, lnode) {
			node = (om_session_t *)lnode->data;
			if(OM_POD_TYPE_UE == node->pod_type){
				ue_pod_num++;
			}else if(OM_POD_TYPE_GNB == node->pod_type){
				gnb_pod_num++;
			}else if(OM_POD_TYPE_CN == node->pod_type){
				cn_pod_num++;
			}

		}
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "pod pre report : ue-pod[%d]  gnb-pod[%d] cn-pod[%d]", ue_pod_num, gnb_pod_num, cn_pod_num);
	}

	if(ue_pod_num || gnb_pod_num || cn_pod_num){
		pre_report = (pod_pre_report_t *)oset_calloc(1, sizeof(pod_pre_report_t));
		memcpy(pre_report->header.system_id, system->system_id, strlen(system->system_id) + 1);
		memset(pre_report->header.pod_id, 0, MAX_ID_SIZE);
		memset(pre_report->header.container_id, 0, MAX_ID_SIZE);
		memset(pre_report->header.node_id, 0, MAX_ID_SIZE);
		pre_report->ue_pod_num = ue_pod_num;
		pre_report->gnb_pod_num = gnb_pod_num;
		pre_report->cn_pod_num = cn_pod_num;	
		pre_context = omc_json_pre_report_encode(pre_report);
		sendProbeMessage(om_self()->mq_producer, OM_PRE_REPORT, pre_report->header.system_id, pre_report->header.node_id, pre_context);
		oset_safe_free(pre_context);
		oset_free(pre_report);
	}
	
	ue_pod_num = 0;
	gnb_pod_num = 0;
	cn_pod_num = 0;

}


OSET_DECLARE(om_system_t *) om_create_system(uint32_t id, char* system_id, oset_apr_memory_pool_t **pool)
{
	oset_apr_memory_pool_t *usepool;
	om_system_t *system = NULL;
	uint32_t *t_system = NULL;
	oset_uuid_t uuid;

	if (runtime.min_idle_time > 0 && runtime.profile_time < runtime.min_idle_time) {
		return NULL;
	}

	if (pool && *pool) {
		usepool = *pool;
		*pool = NULL;
	} else {
		oset_core_new_memory_pool(&usepool);
	}
	system = oset_core_alloc(usepool, sizeof(*system));
	system->common = oset_core_alloc(usepool, sizeof(oset_core_session_t));

	system->common->pool = usepool;
	//oset_core_memory_pool_set_data(system->common->pool, "__system", system);

    system->thread_id = id;
    memcpy(system->system_id, system_id ,strlen(system_id) + 1);

    oset_pool_alloc(&om_self()->om_system_id_pool[id], &t_system);
    oset_sys_assert(t_system);
	system->idnex = oset_pool_index(&om_self()->om_system_id_pool[id], t_system);
	t_system = (uint32_t *)system;
    system->common->id = system->idnex;

	oset_apr_uuid_get(&uuid);
    oset_apr_uuid_format(system->common->uuid_str, &uuid);

    system->all_sess_list = oset_list2_create();

	for(int i = 0 ;i < MAX_NODE; i++){
		system->node_sess_list[i] = oset_list2_create();
	}
    oset_list_add(&om_self()->system_list[id], system);

    oset_core_hash_insert(om_self()->system_hashtable[id], system->system_id, system);

    oset_core_hash_insert(om_self()->system_uuid_hashtable[id], system->common->uuid_str, system);

	system->step_task_id =oset_scheduler_add_task(om_self()->time_of_step, send_pod_timestep_msg, "timestep", oset_core_session_get_uuid(system->common), 0, (void *)system, SSHF_NONE);

	system->pre_rp_task_id =oset_scheduler_add_task(om_self()->time_of_pre_report, send_pre_report_msg, "pod-num-report", "pre-report", 0, (void *)system, SSHF_NONE);  

	oset_apr_mutex_lock(runtime.session_hash_mutex);
	session_manager.others_count[0]++;
	oset_apr_mutex_unlock(runtime.session_hash_mutex);

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[system Added] Thread[%u]:Number of system is now %d", id, oset_list_count(&om_self()->system_list[id]));
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[system Added] Thread[%u]:system idnex = %u;uuid = %s", id, system->idnex, system->common->uuid_str);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG,"[system Added] system uuid = %s [system_id = %s]", system->common->uuid_str,system->system_id);

	return system;
}

static void om_timer_msg_send(om_timer_e timer_id, om_session_t *session)
{
    om_inner_worker_message_t *timer_msg = NULL;
    int rv = OSET_ERROR;
    int id = 0;
 
    oset_sys_assert(session);
    id = session->thread_id;

	timer_msg = (om_inner_worker_message_t *)oset_ring_buf_get(om_self()->worker_buf[id]);
    oset_sys_assert(timer_msg);
    timer_msg->e_type = OM_WORKER_E_TIMER;
    timer_msg->m_type = timer_id;

    timer_msg->thread_id = id;
    timer_msg->u.sess = session;

    rv = oset_ring_queue_put(om_self()->worker_queue[id], (uint8_t *)timer_msg, sizeof(om_inner_worker_message_t));
    if (rv != OSET_OK) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "oset_ring_queue_put(worker_buf[%d]) err",id);
		oset_ring_buf_ret((uint8_t *)timer_msg);
    }

}


static void del_sess_timer_expire(void *data)
{
    om_timer_msg_send(OM_TIMER_HB_TIMEOUT, data);
}


OSET_DECLARE(void) om_session_destory_debug(om_session_t *session, const char *file, const char *func, int line)
{
	oset_event_t *event = NULL;
	oset_lnode2_t *lnode = NULL;
	om_system_t *system = NULL;	
	om_session_t *t_session = NULL;

	uint32_t id = 0; 

    oset_sys_assert(session);
	system = session->system;
    id = session->thread_id; 

    oset_sys_assert(system);


	oset_log2_printf(OSET_CHANNEL_ID_LOG, OSET_LOG2_DOMAIN, file, func, line, oset_core_session_get_uuid(session->common), OSET_LOG2_NOTICE, "[session Delete] Thread[%u]:system [%ld] (destory session [%ld][%s])",\
					 session->thread_id,oset_core_session_get_id(system->common), oset_core_session_get_id(session->common), oset_core_session_get_uuid(session->common));

	 oset_apr_mutex_lock(runtime.session_hash_mutex);
	 oset_core_hash_delete(session_manager.session_table, session->common->uuid_str);
	 if (session_manager.session_count) {
		 session_manager.session_count--;
		 if (session_manager.session_count == 0) {
			 if (oset_test_flag((&runtime), SCF_SYNC_CLOCK_REQUESTED)) {
				 oset_apr_time_sync();
				 oset_clear_flag((&runtime), SCF_SYNC_CLOCK_REQUESTED);
			 }
		 }
	 }
	 oset_apr_mutex_unlock(runtime.session_hash_mutex);

	 if (oset_event_create(&event, OSET_EVENT_CHANNEL_DESTROY) == OSET_STATUS_SUCCESS) {
		 oset_event_add_header(event, OSET_STACK_BOTTOM, "thread-id", "%u", session->thread_id);
		 oset_event_add_header(event, OSET_STACK_BOTTOM, "session-idnex", "%ld", oset_core_session_get_id(session->common));
		 oset_event_add_header_string(event, OSET_STACK_BOTTOM, "session-uuid", oset_core_session_get_uuid(session->common));
		 oset_event_add_header_string(event, OSET_STACK_BOTTOM, "system-id", session->p->system_id);
		 oset_event_add_header_string(event, OSET_STACK_BOTTOM, "pod-id", session->p->pod_id);
		 oset_event_add_header_string(event, OSET_STACK_BOTTOM, "container-id", session->p->container_id);
		 oset_event_add_header_string(event, OSET_STACK_BOTTOM, "node-id", session->p->node_id);
		 oset_event_add_header(event, OSET_STACK_BOTTOM, "pod-type", "%d", session->pod_type);
		 oset_event_fire(&event);
	 }

	 if (session->common->private_hash) oset_core_hash_destroy(&session->common->private_hash);
	 oset_apr_mutex_lock(session->common->profile_mutex);
	 oset_event_destroy(&session->common->variables);
	 oset_apr_mutex_unlock(session->common->profile_mutex);

     oset_apr_thread_rwlock_destroy(session->common->rwlock);

	 oset_pool_free(&om_self()->om_sess_id_pool[id], sess_sid_find_by_idnex(id,session->idnex));
	 oset_timer_delete(session->del_timer);

	 oset_list2_for_each(system->all_sess_list, lnode){
	 	t_session = (om_session_t *)lnode->data;
	     if(!strcmp(t_session->common->uuid_str, session->common->uuid_str)){
		 	oset_list2_remove(system->all_sess_list, lnode);
			break;
		 }
	 }

	 if(session->p->node_id){
		 for(int i = 0; i < system->node_sq ;i++){
			 if(!strcmp(session->p->node_id, system->node_id[i])){
				 oset_list2_for_each(system->node_sess_list[i], lnode){
				 	 t_session = (om_session_t *)lnode->data;
					 if(!strcmp(t_session->common->uuid_str, session->common->uuid_str)){
						oset_list2_remove(system->node_sess_list[i], lnode);
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "i[%d]:node_id[%s]:session num[%ld]",i,system->node_id[i],system->node_sess_list[i]->count);
						goto next;
					 }
				 }
			 }
		 }
	 }
	 
next:
	 for(int j = 0; j < system->node_sq ;j++){
		 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "[node static] Thread[%u]:system [%ld]:node_id [%s], list_num [%ld]",\
						  session->thread_id,oset_core_session_get_id(system->common), system->node_id[j],system->node_sess_list[j]->count);
	 }

	 oset_list_remove(&om_self()->session_list[id], session);
	 char *new_key = oset_msprintf("%s-%s-%s-%s",session->p->system_id,session->p->pod_id,session->p->container_id,session->p->node_id);
	 oset_core_hash_delete(om_self()->session_hashtable[id], new_key);
	 oset_free(new_key);
	 oset_core_hash_delete(om_self()->session_uuid_hashtable[id], session->common->uuid_str);

     oset_core_destroy_memory_pool(&session->common->pool);
	 session = NULL;

	 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[session Delete] Thread[%u]:system [%ld]:session num[%ld]",\
					  system->thread_id,oset_core_session_get_id(system->common), system->all_sess_list->count);

}



OSET_DECLARE(om_session_t *) om_create_session(uint32_t id, oset_sock_t *sock, oset_sockaddr_t from, om_msg_header_t *p, int pod_type, om_system_t * system, oset_apr_memory_pool_t **pool)
{
	oset_apr_memory_pool_t *usepool = NULL;
	om_session_t *session = NULL;
	uint32_t *t_session = NULL;
	oset_uuid_t uuid;
    int  have_flag = 0;
	uint32_t count = 0;
	int32_t sps = 0;

    oset_sys_assert(p);
    oset_sys_assert(system);

	oset_apr_mutex_lock(runtime.throttle_mutex);
	count = session_manager.session_count;
	sps = --runtime.sps;
	oset_apr_mutex_unlock(runtime.throttle_mutex);
	
	if (sps <= 0) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Throttle Error! %d\n", session_manager.session_count);
		//return NULL;
	}

	if ((count + 1) > session_manager.session_limit) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Over session Limit! %d\n", session_manager.session_limit/om_self()->worker_thread_num);
		return NULL;
	}

	if (pool && *pool) {
		usepool = *pool;
		*pool = NULL;
	} else {
		oset_core_new_memory_pool(&usepool);
	}

	session = oset_core_alloc(usepool, sizeof(*session));
	session->common = oset_core_alloc(usepool, sizeof(oset_core_session_t));

	session->system = system;

	session->common->pool = usepool;
	//oset_core_memory_pool_set_data(session->common->pool, "__session", session);

    oset_pool_alloc(&om_self()->om_sess_id_pool[id], &t_session);
    oset_sys_assert(t_session);
	session->idnex = oset_pool_index(&om_self()->om_sess_id_pool[id], t_session);
	t_session = (uint32_t *)session;
    session->common->id = session->idnex;

	oset_apr_uuid_get(&uuid);
    oset_apr_uuid_format(session->common->uuid_str, &uuid);

	//oset_session_set_variable(session->common, "uuid", session->uuid_str);

	oset_event_create_plain(&session->common->variables, OSET_EVENT_CHANNEL_DATA);
	oset_core_hash_init(&session->common->private_hash);
	oset_apr_mutex_init(&session->common->profile_mutex, OSET_MUTEX_NESTED, usepool);

	oset_apr_thread_rwlock_create(&session->common->rwlock, usepool);

    session->thread_id = id;
    session->pod_type = pod_type;

	//oset_core_session_strdup(session->common, p->system_id);

	session->p = oset_core_alloc(usepool, sizeof(om_msg_header_t));
    memcpy(session->p, p ,sizeof(om_msg_header_t));


    if(session->p->node_id){
		have_flag = 0;
        for(int i = 0; i < system->node_sq ;i++){
            if(!zstr(system->node_id[i]) && !strcmp(session->p->node_id, system->node_id[i])){
				oset_list2_add(system->node_sess_list[i], session);
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "[node Added] 1 %p:%s",system->node_sess_list[i],system->node_id[i]);

				have_flag = 1;
				break;
			}
		}

        if(!have_flag){
    	    system->node_id[system->node_sq] = oset_strdup(session->p->node_id);
			oset_list2_add(system->node_sess_list[system->node_sq], session);
		    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "[node Added] 2 %p:%s",system->node_sess_list[system->node_sq],system->node_id[system->node_sq]);
		    system->node_sq++;
        }
	}

	for(int j = 0; j < system->node_sq ;j++){
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "[node static] Thread[%u]:system [%ld]:node_id [%s], list_num [%ld]",\
						 session->thread_id,oset_core_session_get_id(system->common), system->node_id[j],system->node_sess_list[j]->count);
	}

	session->pod_from = from; //oset_strdup;oset_sys_strdup
	session->pod_sock = sock;

	oset_list2_add(system->all_sess_list, session);

    oset_list_add(&om_self()->session_list[id], session);

	char *new_key = oset_msprintf("%s-%s-%s-%s",p->system_id,p->pod_id,p->container_id,p->node_id);
    oset_core_hash_insert(om_self()->session_hashtable[id], new_key, session);
	oset_free(new_key);
    oset_core_hash_insert(om_self()->session_uuid_hashtable[id], session->common->uuid_str, session);

    session->del_timer = oset_timer_add(om_self()->worker_timer[id], del_sess_timer_expire, session);

	oset_timer_start(session->del_timer, oset_time_from_sec(om_self()->time_of_hb_check));

	oset_apr_mutex_lock(runtime.session_hash_mutex);
	oset_core_hash_insert(session_manager.session_table, session->common->uuid_str, session);
	session_manager.session_count++;

	if (session_manager.session_count > (uint32_t)runtime.sessions_peak) {
		runtime.sessions_peak = session_manager.session_count;
	}
	if (session_manager.session_count > (uint32_t)runtime.sessions_peak_fivemin) {
		runtime.sessions_peak_fivemin = session_manager.session_count;
	}
	oset_apr_mutex_unlock(runtime.session_hash_mutex);

	//oset_session_set_variable_printf(session->common, "session_id", "%u", session->common.id);

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[session Added] Thread[%u]:system [%ld](create session idnex[%ld] uuid[%s])",\
					 session->thread_id,oset_core_session_get_id(system->common), oset_core_session_get_id(session->common), oset_core_session_get_uuid(session->common));
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[session Added] Thread[%u]:system [%ld]:session num[%ld]",\
					 session->thread_id,oset_core_session_get_id(system->common), system->all_sess_list->count);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "[session Added static]:====>>>\n[system uuid[%s]]:\n[session uuid[%s]]:\n[system_id = %s]\n[pod_id = %s]\n[container_id = %s]\n[node_id = %s]",\
					oset_core_session_get_uuid(system->common),oset_core_session_get_uuid(session->common),\
					session->p->system_id,session->p->pod_id, session->p->container_id, session->p->node_id);



	return session;
}


OSET_DECLARE(void) timer_msg_deal(uint32_t id, om_session_t *session)
{
	om_system_t * system = NULL;

    oset_sys_assert(session);

	system = session->system;
    oset_sys_assert(system);

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[Time] Thread[%u]:session(%ld:uuid[%s]) pod_id[%s] hb timeout", id,\
		                                                oset_core_session_get_id(session->common),\
		                                                oset_core_session_get_uuid(session->common),\
		                                                session->p->pod_id);

	om_session_destory(session);

	if(0 == system->all_sess_list->count)
	{
	   om_system_destory(system);
	}
}


OSET_DECLARE(void) hb_msg_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, pod_heartbeat_data_t *hb)
{
	om_msg_header_t *header = NULL;
	om_system_t * system, *new_system = NULL;
	om_session_t * session, *new_session = NULL;	

	header = (om_msg_header_t *)(hb);
    oset_sys_assert(header->system_id);

	char *new_key = oset_msprintf("%s-%s-%s-%s",header->system_id,header->pod_id,header->container_id,header->node_id);

	system = (om_system_t *)oset_core_hash_find(om_self()->system_hashtable[id], header->system_id);
	if(NULL == system){
		new_system = om_create_system(id, header->system_id, NULL);
		if(NULL == new_system){
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Thread work[%u]:om_create_system[%s] new failed",id, header->system_id);
            return;
		}
		session = (om_session_t *)oset_core_hash_find(om_self()->session_hashtable[id], new_key);
		if(NULL == session){
			new_session = om_create_session(id, pod_sock, pod_from, header, hb->hb_type, new_system, NULL);
			oset_sys_assert(new_session);
		}else{
			oset_timer_start(session->del_timer, oset_time_from_sec(om_self()->time_of_hb_check));
			session->pod_from = pod_from;
		}
	}else{
        //system context update;
		session = (om_session_t *)oset_core_hash_find(om_self()->session_hashtable[id], new_key);
		if(NULL == session){
			new_session = om_create_session(id, pod_sock, pod_from, header, hb->hb_type, system, NULL);
			oset_sys_assert(new_session);

		}else{
			oset_timer_start(session->del_timer, oset_time_from_sec(om_self()->time_of_hb_check));
			session->pod_from = pod_from;
		}
	}
    oset_free(new_key);
}



static om_transfer_order_c_t *omc_json_cp_order_decode(void *data)
{
	om_transfer_order_c_t *t_omc = NULL;

    const char *system_id = NULL;
    const char *pod_id = NULL;
    const char *container_id = NULL;
    const char *node_id = NULL;
    const char *suci = NULL;
	const char *others = NULL;

	cJSON *jdata = NULL;
	cJSON *cdata = NULL;
	cJSON *item = NULL;
    char *context = NULL;

    t_omc = (om_transfer_order_c_t *)oset_calloc(1, sizeof(om_transfer_order_c_t));

	jdata = cJSON_Parse((char *)data);
	context = cJSON_Print(jdata);
	oset_sys_assert(context);
	//context = cJSON_PrintUnformatted(jdata);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_INFO, "omc->pod cp_order receive:\n%s\n\n", context);
    oset_safe_free(context);

	system_id = cJSON_GetObjectCstr(jdata, "systemId");
	pod_id = cJSON_GetObjectCstr(jdata, "podId");
	container_id = cJSON_GetObjectCstr(jdata, "containerId");
	node_id = cJSON_GetObjectCstr(jdata, "nodeId");

	if(system_id) memcpy(t_omc->header.system_id , system_id, strlen(system_id) + 1);
	if(pod_id) memcpy(t_omc->header.pod_id , pod_id, strlen(pod_id) + 1);
	if(container_id) memcpy(t_omc->header.container_id , container_id, strlen(container_id) + 1);
	if(node_id) memcpy(t_omc->header.node_id , node_id, strlen(node_id) + 1);

	item = cJSON_GetObjectItem(jdata, "messageType");
	if (!cJSON_IsNumber(item)){
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc->pod:[cp_order.messageType] json decode failed"); 
		cJSON_Delete(jdata);
        oset_free(t_omc);
		return NULL;
	}

    t_omc->msg_type = item->valueint;

	cdata = cJSON_GetObjectItem(jdata, "cOrder");
	suci = cJSON_GetObjectCstr(cdata, "suci");
	others = cJSON_GetObjectCstr(cdata, "others");


	if(suci) memcpy(t_omc->c_order.suci , suci, strlen(suci) + 1);
	if(others) memcpy(t_omc->c_order.others , others, strlen(others) + 1);

	item = cJSON_GetObjectItem(cdata, "order");
	if (!cJSON_IsNumber(item)){
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc->pod:[cp_order.cOrder.order] json decode failed"); 
		cJSON_Delete(jdata);
	    oset_free(t_omc);
		return NULL;
	}
    t_omc->c_order.order = item->valueint;

	cJSON_Delete(jdata);
	return t_omc;
}


static char *omc_json_cp_report_encode(om_report_data_c_t *pod_cp_rpt)
{
    char  *report = NULL;
	cJSON *item = NULL;
	cJSON *item_cp = NULL;
	oset_sys_assert(pod_cp_rpt);

    item = cJSON_CreateObject();

    if(pod_cp_rpt->header.system_id){
		if (cJSON_AddStringToObject(item, "systemId", pod_cp_rpt->header.system_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.header.systemId] json encode failed"); 
			goto end;
		}
	}else{
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "system_id is NULL");
		goto end;
	}

    if(pod_cp_rpt->header.pod_id && strlen(pod_cp_rpt->header.pod_id)){
		if (cJSON_AddStringToObject(item, "podId", pod_cp_rpt->header.pod_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.header.podId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "podId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.header.podId] json encode failed"); 
			goto end;
		}
	}


    if(pod_cp_rpt->header.container_id && strlen(pod_cp_rpt->header.container_id)){
		if (cJSON_AddStringToObject(item, "containerId", pod_cp_rpt->header.container_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.header.containerId] json encode failed"); 
			goto end;
		}
    }else{
		if (cJSON_AddNullToObject(item, "containerId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.header.containerId] json encode failed"); 
			goto end;
		}
	}


    if(pod_cp_rpt->header.node_id && strlen(pod_cp_rpt->header.node_id)){
		if (cJSON_AddStringToObject(item, "nodeId", pod_cp_rpt->header.node_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.header.nodeId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "nodeId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.header.nodeId] json encode failed"); 
			goto end;
		}
	}


	if (cJSON_AddNumberToObject(item, "messageType", pod_cp_rpt->msg_type) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.messageType] json encode failed"); 
		goto end;
	}

	if (cJSON_AddBoolToObject(item, "successFlag", pod_cp_rpt->success_flag) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.successFlag] json encode failed"); 
		goto end;
	}

    if(pod_cp_rpt->failure_cause && strlen(pod_cp_rpt->failure_cause)){
		if (cJSON_AddStringToObject(item, "failureCause", pod_cp_rpt->failure_cause) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.failureCause] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "failureCause") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.failureCause] json encode failed"); 
			goto end;
		}
	}

    item_cp = cJSON_CreateObject();

    if(pod_cp_rpt->ue_cp_data.suci && strlen(pod_cp_rpt->ue_cp_data.suci)){
		if (cJSON_AddStringToObject(item_cp, "suci", pod_cp_rpt->ue_cp_data.suci) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.cData.suci] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item_cp, "suci") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.cData.suci] json encode failed"); 
			goto end;
		}
	}

    if(pod_cp_rpt->ue_cp_data.ip && strlen(pod_cp_rpt->ue_cp_data.ip)){
		if (cJSON_AddStringToObject(item_cp, "ip", pod_cp_rpt->ue_cp_data.ip) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.cData.ip] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item_cp, "ip") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.cData.ip] json encode failed");
			goto end;
		}
	}


    cJSON_AddItemToObject(item, "cData", item_cp);
    if (item->child == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[creport.cData] json encode failed"); 
        goto end;
    }

end:
	if (item) {
		report = cJSON_Print(item);
		//report = cJSON_PrintUnformatted(item);

		oset_sys_assert(report);
	    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "pod->omc cp_report send:\n%s\n\n", report);
		cJSON_Delete(item);
	}
    return report;
}


static om_report_data_c_t *build_err_cp_report(om_transfer_order_c_t *cp_order, int type)
{
	om_report_data_c_t *t_err_report = NULL; 
	
    t_err_report = (om_report_data_c_t *)oset_calloc(1, sizeof(om_report_data_c_t));
	memcpy(t_err_report, &cp_order->header, sizeof(om_msg_header_t));
    t_err_report->msg_type =  type;
	t_err_report->success_flag = FALSE;
	memcpy(t_err_report->failure_cause, "COMMON_FAILURE", MAX_CAUSE_LEN);

    if(cp_order->c_order.suci) memcpy(t_err_report->ue_cp_data.suci, cp_order->c_order.suci, strlen(cp_order->c_order.suci) + 1);
    
	return t_err_report;
}


OSET_DECLARE(void) cp_order_deal(uint32_t id, oset_sock_t *omc_sock, oset_sockaddr_t omc_from, void *order)
{
	om_transfer_order_c_t *cp_order = NULL;
	om_report_data_c_t * err_ack = NULL;
	oset_lnode2_t *lnode = NULL;
	om_session_t *session, *node = NULL;
	om_system_t *system = NULL;
	char *context = NULL;
    char buf[OSET_ADDRSTRLEN] = {0};

	cp_order = omc_json_cp_order_decode(order);
	if(NULL == cp_order){
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "thread[%u]:cp_order_deal() error", id); 
		oset_free(cp_order);
        return;	
	}
    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%s\n", "receive from OMC");
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%s\n", cp_order->header.system_id);
    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%s\n", cp_order->header.pod_id);	
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%s\n", cp_order->header.container_id);	
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%s\n", cp_order->header.node_id);	
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%d\n", cp_order->msg_type);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%s\n", cp_order->c_order.suci);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%s\n", cp_order->c_order.others);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "cp_order decode :%d\n", cp_order->c_order.order);

	oset_sys_assert(cp_order->header.system_id);

	system = (om_system_t *)oset_core_hash_find(om_self()->system_hashtable[id], cp_order->header.system_id);
	if(system) 	oset_scheduler_del_task_id(system->pre_rp_task_id);

    if(DESTORY  == cp_order->c_order.order){
        if(system){
			om_system_destory(system);
			oset_free(cp_order);
            return;
	    }
	}

    if(strlen(cp_order->header.system_id) && strlen(cp_order->header.pod_id) &&\
		    strlen(cp_order->header.container_id) && strlen(cp_order->header.node_id)){
		char *new_key = oset_msprintf("%s-%s-%s-%s",cp_order->header.system_id,cp_order->header.pod_id,cp_order->header.container_id,cp_order->header.node_id);
		
		session = (om_session_t *)oset_core_hash_find(om_self()->session_hashtable[id], new_key);
		if(NULL == session){
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "thread[%u]:cp_order_deal() find session_hashtable failed", id); 
		
			err_ack = build_err_cp_report(cp_order, OM_MSG_DL_COMMAND_CP);
			context = omc_json_cp_report_encode(err_ack);
			sendProbeMessage(om_self()->mq_producer, OM_C_REPORT, err_ack->header.system_id, err_ack->header.pod_id, context);
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:om->omc build err_cp_report sendProbeMessage success", id); 
			oset_safe_free(context);
			oset_free(err_ack);
		}else{
			session->omc_sock = omc_sock;
			session->omc_from = omc_from;
			om_udp_sendto(session->pod_sock, &session->pod_from, (void *)cp_order, sizeof(om_transfer_order_c_t));
			oset_hex_print(OSET_LOG2_DEBUG, (unsigned char *)cp_order, sizeof(om_transfer_order_c_t));
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:cp_order_deal() send cp_order success, dest pod_id[%s], addr[%s:%d]", id, session->p->pod_id, OSET_ADDR(&session->pod_from, buf), OSET_PORT(&session->pod_from)); 
		}
		oset_free(new_key);
	}else{
		if(NULL== system){
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "thread[%u]:cp_order_deal() find system_hashtable failed", id); 
		
			err_ack = build_err_cp_report(cp_order, OM_MSG_DL_COMMAND_CP);
			context = omc_json_cp_report_encode(err_ack);
			sendProbeMessage(om_self()->mq_producer, OM_C_REPORT, err_ack->header.system_id, err_ack->header.pod_id, context);
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:om->omc build err_cp_report sendProbeMessage success", id); 
			oset_safe_free(context);
			oset_free(err_ack);
		}else{
			if(system->all_sess_list->count){
				oset_list2_for_each(system->all_sess_list, lnode) {
					node = (om_session_t *)lnode->data;
					node->omc_sock = omc_sock;
					node->omc_from = omc_from;
					om_udp_sendto(node->pod_sock, &node->pod_from, (void *)cp_order, sizeof(om_transfer_order_c_t));
					oset_hex_print(OSET_LOG2_DEBUG, (unsigned char *)cp_order, sizeof(om_transfer_order_c_t));
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:cp_order_deal() broadcast cp_order success, dest pod_id[%s], addr[%s:%d]", id, node->p->pod_id, OSET_ADDR(&node->pod_from, buf), OSET_PORT(&node->pod_from));
				    oset_msleep(50);
				}
			}
		}
	}
	oset_free(cp_order);
}


OSET_DECLARE(void) cp_report_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, om_report_data_c_t *pod_cp_rpt, uint32_t len)
{
    char *context = NULL;

    oset_sys_assert(pod_cp_rpt);
	oset_hex_print(OSET_LOG2_DEBUG, (unsigned char *)pod_cp_rpt, len);
	context = omc_json_cp_report_encode(pod_cp_rpt);
	sendProbeMessage(om_self()->mq_producer, OM_C_REPORT, pod_cp_rpt->header.system_id, pod_cp_rpt->header.pod_id, context);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:om->omc cp_report_deal() sendProbeMessage success", id); 
    oset_safe_free(context);
}

static om_transfer_order_u_t *omc_json_up_order_decode(void *data)
{
	om_transfer_order_u_t *t_omc = NULL;

    const char *system_id = NULL;
    const char *pod_id = NULL;
    const char *container_id = NULL;
    const char *node_id = NULL;
    const char *suci = NULL;
    const char *target_ip = NULL;
	const char *others = NULL;
	const char *src_dir_path = NULL;
	const char *target_dir_path = NULL;


	cJSON *jdata = NULL;
	cJSON *cdata = NULL;
	cJSON *item = NULL;
    char *context = NULL;

    t_omc = (om_transfer_order_u_t *)oset_calloc(1, sizeof(om_transfer_order_u_t));

	jdata = cJSON_Parse((char *)data);
	context = cJSON_Print(jdata);
	oset_sys_assert(context);
	//context = cJSON_PrintUnformatted(jdata);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_INFO, "omc->pod up_order receive:\n%s\n\n", context);
    oset_safe_free(context);

	system_id = cJSON_GetObjectCstr(jdata, "systemId");
	pod_id = cJSON_GetObjectCstr(jdata, "podId");
	container_id = cJSON_GetObjectCstr(jdata, "containerId");
	node_id = cJSON_GetObjectCstr(jdata, "nodeId");

	if(system_id) memcpy(t_omc->header.system_id , system_id, strlen(system_id) + 1);
	if(pod_id) memcpy(t_omc->header.pod_id , pod_id, strlen(pod_id) + 1);
	if(container_id) memcpy(t_omc->header.container_id , container_id, strlen(container_id) + 1);
	if(node_id) memcpy(t_omc->header.node_id , node_id, strlen(node_id) + 1);

	item = cJSON_GetObjectItem(jdata, "messageType");
	if (!cJSON_IsNumber(item)){
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc->pod:[up_order.messageType] json decode failed"); 
		cJSON_Delete(jdata);
        oset_free(t_omc);
		return NULL;
	}

    t_omc->msg_type = item->valueint;

	cdata = cJSON_GetObjectItem(jdata, "uOrder");
	suci = cJSON_GetObjectCstr(cdata, "suci");
	target_ip = cJSON_GetObjectCstr(cdata, "targetIp");
	others = cJSON_GetObjectCstr(cdata, "others");
	src_dir_path = cJSON_GetObjectCstr(cdata, "sourceDirPath");
	target_dir_path = cJSON_GetObjectCstr(cdata, "targetDirPath");

	if(suci) memcpy(t_omc->u_order.suci , suci, strlen(suci) + 1);
	if(target_ip) memcpy(t_omc->u_order.target_ip , target_ip, strlen(target_ip) + 1);
	if(others) memcpy(t_omc->u_order.others , others, strlen(others) + 1);
	if(src_dir_path) memcpy(t_omc->u_order.src_dir_path , src_dir_path, strlen(src_dir_path) + 1);
	if(target_dir_path) memcpy(t_omc->u_order.target_dir_path , target_dir_path, strlen(target_dir_path) + 1);


	item = cJSON_GetObjectItem(cdata, "order");
	if (!cJSON_IsNumber(item)){
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc->pod:[up_order.uOrder.order] json decode failed"); 
		cJSON_Delete(jdata);
	    oset_free(t_omc);
		return NULL;
	}
    t_omc->u_order.order = item->valueint;

	cJSON_Delete(jdata);
	return t_omc;
}

static char *omc_json_up_report_encode(om_report_data_u_t *pod_up_rpt)
{
    char  *report = NULL;
	cJSON *item = NULL;
	cJSON *item_up = NULL;
	oset_sys_assert(pod_up_rpt);

    item = cJSON_CreateObject();

    if(pod_up_rpt->header.system_id){
		if (cJSON_AddStringToObject(item, "systemId", pod_up_rpt->header.system_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.header.systemId] json encode failed"); 
			goto end;
		}
	}else{
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "system_id is NULL");
		goto end;
	}

    if(pod_up_rpt->header.pod_id && strlen(pod_up_rpt->header.pod_id)){
		if (cJSON_AddStringToObject(item, "podId", pod_up_rpt->header.pod_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.header.podId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "podId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.header.podId] json encode failed"); 
			goto end;
		}
	}


    if(pod_up_rpt->header.container_id && strlen(pod_up_rpt->header.container_id)){
		if (cJSON_AddStringToObject(item, "containerId", pod_up_rpt->header.container_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.header.containerId] json encode failed"); 
			goto end;
		}
    }else{
		if (cJSON_AddNullToObject(item, "containerId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.header.containerId] json encode failed"); 
			goto end;
		}
	}


    if(pod_up_rpt->header.node_id && strlen(pod_up_rpt->header.node_id)){
		if (cJSON_AddStringToObject(item, "nodeId", pod_up_rpt->header.node_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.header.nodeId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "nodeId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.header.nodeId] json encode failed"); 
			goto end;
		}
	}


	if (cJSON_AddNumberToObject(item, "messageType", pod_up_rpt->msg_type) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.messageType] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item, "noOfUe", pod_up_rpt->noOfUe) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.noOfUe] json encode failed"); 
		goto end;
	}

	cJSON *u_data_lists = cJSON_AddArrayToObject(item, "uData");
	if (u_data_lists == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.uData] json encode failed"); 
		goto end;
	}

	for (int i = 0; i < pod_up_rpt->noOfUe; i++) {
		item_up = cJSON_CreateObject();

	    if(pod_up_rpt->ue_up_data[i].suci && strlen(pod_up_rpt->ue_up_data[i].suci)){
			if (cJSON_AddStringToObject(item_up, "suci", pod_up_rpt->ue_up_data[i].suci) == NULL) {
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.ueUpData.suci] json encode failed"); 
				goto end;
			}
		}else{
			if (cJSON_AddNullToObject(item_up, "suci") == NULL) {
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.ueUpData.suci] json encode failed"); 
				goto end;
			}
		}

		if (cJSON_AddNumberToObject(item_up, "type", pod_up_rpt->ue_up_data[i].type) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.ueUpData.type] json encode failed"); 
			goto end;
		}

		if (cJSON_AddBoolToObject(item_up, "successFlag", pod_up_rpt->ue_up_data[i].success_flag) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.ueUpData.successFlag] json encode failed"); 
			goto end;
		}

	    if(pod_up_rpt->ue_up_data[i].result_size){
			if (cJSON_AddStringToObject(item_up, "result", pod_up_rpt->ue_up_data[i].result) == NULL) {
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.ueUpData.result] json encode failed"); 
				goto end;
			}
		}else{
			if (cJSON_AddNullToObject(item_up, "result") == NULL) {
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ureport.ueUpData.result] json encode failed"); 
				goto end;
			}
		}

	    cJSON_AddItemToArray(u_data_lists, item_up);
	}



end:
	if (item) {
		report = cJSON_Print(item);
		//report = cJSON_PrintUnformatted(item);

		oset_sys_assert(report);
	    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "pod->omc cp_report send:\n%s\n\n", report);
		cJSON_Delete(item);
	}
    return report;
}


static om_report_data_u_t *build_err_up_report(om_transfer_order_u_t *up_order, int type)
{
	om_report_data_u_t *t_err_report = NULL; 
	
    t_err_report = (om_report_data_u_t *)oset_calloc(1, sizeof(om_report_data_u_t));
	memcpy(t_err_report, &up_order->header, sizeof(om_msg_header_t));
    t_err_report->msg_type =  type;
	t_err_report->noOfUe = 1;

    if(up_order->u_order.suci) memcpy(t_err_report->ue_up_data[0].suci, up_order->u_order.suci, strlen(up_order->u_order.suci) + 1);

    t_err_report->ue_up_data[0].success_flag = FALSE;
    t_err_report->ue_up_data[0].type = up_order->u_order.order;
	
	return t_err_report;
}


OSET_DECLARE(void) up_order_deal(uint32_t id, oset_sock_t *omc_sock, oset_sockaddr_t omc_from, void *order)
{
	om_transfer_order_u_t *up_order = NULL;
	om_report_data_u_t * err_ack = NULL;
	oset_lnode2_t *lnode = NULL;
	om_session_t *session, *node = NULL;
	om_system_t *system = NULL;
	char *context = NULL;
    char buf[OSET_ADDRSTRLEN] = {0};

	up_order = omc_json_up_order_decode(order);
	if(NULL == up_order){
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "thread[%u]:up_order_deal() error", id); 
		oset_free(up_order);
        return;	
	}
    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", "receive from OMC");
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->header.system_id);
    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->header.pod_id);	
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->header.container_id);	
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->header.node_id);	
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%d\n", up_order->msg_type);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->u_order.suci);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->u_order.target_ip);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->u_order.others);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->u_order.src_dir_path);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%s\n", up_order->u_order.target_dir_path);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "up_order decode :%d\n", up_order->u_order.order);
	//oset_hex_print(OSET_LOG2_DEBUG, (unsigned char *)up_order, sizeof(om_transfer_order_u_t));

	oset_sys_assert(up_order->header.system_id);

	system = (om_system_t *)oset_core_hash_find(om_self()->system_hashtable[id], up_order->header.system_id);
	if(system) 	oset_scheduler_del_task_id(system->pre_rp_task_id);

    if(strlen(up_order->header.system_id) && strlen(up_order->header.pod_id) &&\
		    strlen(up_order->header.container_id) && strlen(up_order->header.node_id)){
		char *new_key = oset_msprintf("%s-%s-%s-%s",up_order->header.system_id,up_order->header.pod_id,up_order->header.container_id,up_order->header.node_id);
		
		session = (om_session_t *)oset_core_hash_find(om_self()->session_hashtable[id], new_key);
		if(NULL == session){
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "thread[%u]:up_order_deal() find session_hashtable failed", id); 
		
			err_ack = build_err_up_report(up_order, OM_MSG_DL_COMMAND_UP);
			context = omc_json_up_report_encode(err_ack);
			sendProbeMessage(om_self()->mq_producer, OM_U_REPORT, err_ack->header.system_id, err_ack->header.pod_id, context);
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:om->omc build err_up_report sendProbeMessage success", id); 
			oset_safe_free(context);
			oset_free(err_ack);
		}else{
			session->omc_sock = omc_sock;
			session->omc_from = omc_from;
			om_udp_sendto(session->pod_sock, &session->pod_from, (void *)up_order, sizeof(om_transfer_order_u_t));
			oset_hex_print(OSET_LOG2_DEBUG, (unsigned char *)up_order, sizeof(om_transfer_order_u_t));
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:up_order_deal() send up_order success, dest pod_id[%s], addr[%s:%d]", id, session->p->pod_id, OSET_ADDR(&session->pod_from, buf), OSET_PORT(&session->pod_from)); 
		}
		oset_free(new_key);
	}else{
		if(NULL== system){
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "thread[%u]:up_order_deal() find system_hashtable failed", id); 
		
			err_ack = build_err_up_report(up_order, OM_MSG_DL_COMMAND_UP);
			context = omc_json_up_report_encode(err_ack);
			sendProbeMessage(om_self()->mq_producer, OM_U_REPORT, err_ack->header.system_id, err_ack->header.pod_id, context);
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:om->omc build err_up_report sendProbeMessage success", id); 
			oset_safe_free(context);
			oset_free(err_ack);
		}else{
			if(system->all_sess_list->count){
				oset_list2_for_each(system->all_sess_list, lnode) {
					node = (om_session_t *)lnode->data;
					node->omc_sock = omc_sock;
					node->omc_from = omc_from;
					om_udp_sendto(node->pod_sock, &node->pod_from, (void *)up_order, sizeof(om_transfer_order_u_t));
					oset_hex_print(OSET_LOG2_DEBUG, (unsigned char *)up_order, sizeof(om_transfer_order_u_t));
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:cp_order_deal() broadcast up_order success, dest pod_id[%s], addr[%s:%d]", id, node->p->pod_id, OSET_ADDR(&node->pod_from, buf), OSET_PORT(&node->pod_from));
				    oset_msleep(100);
				}
			}
		}
	}
	oset_free(up_order);
}


OSET_DECLARE(void) up_report_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, om_report_data_u_t *pod_up_rpt, uint32_t len)
{
    char *context = NULL;

    oset_sys_assert(pod_up_rpt);
	oset_hex_print(OSET_LOG2_DEBUG, (unsigned char *)pod_up_rpt, len);
	context = omc_json_up_report_encode(pod_up_rpt);
	sendProbeMessage(om_self()->mq_producer, OM_U_REPORT, pod_up_rpt->header.system_id, pod_up_rpt->header.pod_id, context);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "thread[%u]:om->omc up_report_deal() sendProbeMessage success", id); 
    oset_safe_free(context);
}


