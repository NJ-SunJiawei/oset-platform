/************************************************************************
 *File name: om-session.c
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/

#include "sset-core.h"
#include "om-public.h"
#include "asyncProducer.h"


#define PRIORITY_LEVEL_1 1
#define PRIORITY_LEVEL_2 2
#define PRIORITY_LEVEL_3 3

static om_public_t self = {0};

static OSET_POOL(pool_listen, om_event_t);
static OSET_POOL(pool_omc, om_event_t);
static OSET_POOL(pool_pod, om_event_t);

static const char *om_event_get_name(om_event_t *e)
{
    switch (e->id) {
    case OM_LISTEN_MSG:
        return "OM_LISTEN_MSG";
    case OM_OMC_MSG:
        return "OM_OMC_MSG";
    case OM_POD_MSG:
        return "OM_POD_MSG";

    case OM_OMC_MSG_UP:
        return "OM_OMC_MSG_UP";
    case OM_OMC_MSG_DOWN:
        return "OM_OMC_MSG_DOWN";
	case OM_OMC_MSG_TIMER:
		return "OM_OMC_MSG_TIMER";

    case OM_POD_MSG_UP:
        return "OM_POD_MSG_UP";
    case OM_POD_MSG_DOWN:
        return "OM_POD_MSG_DOWN";
	case OM_POD_MSG_TIMER:
		return "OM_POD_MSG_TIMER";

    default: 
       break;
    }

    return "UNKNOWN_EVENT";
}

static size_t hash_calculate(char *key, size_t klen)
{
    size_t hash = 0;
    size_t len = klen;
    while( len ) {
        (len) --;
        hash = hash * 33 + key[ len ];
    }
    return hash;
}


OSET_DECLARE(void) om_resource_default_config(void)
{
	self.max_pod_num = 100;
	self.worker_thread_num = 2;
	self.worker_thread_que_size = 200;
	self.pod_thread_que_size = 200;
	self.omc_thread_que_size = 100;
	self.time_of_step = 5;
	self.time_of_pre_report = 3;
	self.time_of_massive_report = 5;
	self.max_step_num = 100;
}

OSET_DECLARE(void) om_event_init(void)
{

    oset_pool_init(&pool_listen, (self.pod_thread_que_size + self.omc_thread_que_size));
    oset_pool_init(&pool_omc, self.omc_thread_que_size);
    oset_pool_init(&pool_pod, self.pod_thread_que_size);
}

OSET_DECLARE(void) om_event_final(void)
{
    oset_pool_final(&pool_pod);
    oset_pool_final(&pool_omc);
    oset_pool_final(&pool_listen);
}

OSET_DECLARE(void) om_omc_event_free(om_event_t *e)
{
    oset_sys_assert(e);
    oset_pool_free(&pool_omc, e);
}

OSET_DECLARE(om_event_t *) om_omc_event_new(om_event_e id)
{
    om_event_t *e = NULL;

    oset_pool_alloc(&pool_omc, &e);
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));

    e->id = id;

    return e;
}

OSET_DECLARE(void) om_pod_event_free(om_event_t *e)
{
    oset_sys_assert(e);
    oset_pool_free(&pool_pod, e);
}


OSET_DECLARE(om_event_t *) om_pod_event_new(om_event_e id)
{
    om_event_t *e = NULL;

    oset_pool_alloc(&pool_pod, &e);
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));

    e->id = id;

    return e;
}

OSET_DECLARE(void) om_listen_event_free(om_event_t *e)
{
    oset_sys_assert(e);
    oset_pool_free(&pool_listen, e);
}


OSET_DECLARE(om_event_t *) om_listen_event_new(om_event_e id)
{
    om_event_t *e = NULL;

    oset_pool_alloc(&pool_listen, &e);
    if (!e) return NULL;
    memset(e, 0, sizeof(*e));

    e->id = id;

    return e;
}


OSET_DECLARE(om_public_t *) om_self()
{
    return &self;
}

OSET_DECLARE(int) om_sess_resource_init(void)
{
	uint32_t id = 0;

#define MAX_NUM_OF_TIMER        4
#define MAX_SOCKET_NUM_OF_PER_POD   2
#define MAX_POD_NUM_OF_PER_SYSTEM   4

	self.listen_queue = oset_ring_queue_create(self.omc_thread_que_size + self.pod_thread_que_size);
	self.pod_queue = oset_ring_queue_create(self.pod_thread_que_size);
	self.omc_queue = oset_ring_queue_create(self.omc_thread_que_size);

    for(id = 0; id < self.worker_thread_num; id++)
    {
 	    self.worker_queue[id] = oset_ring_queue_create(self.worker_thread_que_size);
		if(NULL == self.worker_queue[id])
		{
		    oset_ring_queue_destroy(self.pod_queue);
		    oset_ring_queue_destroy(self.omc_queue);
	        return OSET_ERROR;	
		}

		self.worker_buf[id] = oset_ring_buf_create(self.worker_thread_que_size, sizeof(om_inner_worker_message_t));
		if(NULL == self.worker_buf[id])
		{
		    oset_ring_queue_destroy(self.pod_queue);
		    oset_ring_queue_destroy(self.omc_queue);
		    oset_ring_queue_destroy(self.worker_queue[id]);
	        return OSET_ERROR;	
		}

		oset_core_hash_init(&self.system_hashtable[id]);
		oset_core_hash_init(&self.system_uuid_hashtable[id]);
		oset_core_hash_init(&self.session_hashtable[id]);
		oset_core_hash_init(&self.session_uuid_hashtable[id]);

		oset_list_init(&self.system_list[id]);
		oset_list_init(&self.session_list[id]);
	
		oset_pool_init(&self.om_sess_id_pool[id], self.worker_thread_que_size * MAX_POD_NUM_OF_PER_SYSTEM);
		oset_pool_init(&self.om_system_id_pool[id], self.worker_thread_que_size);
		self.worker_timer[id] = oset_timer_mgr_create(self.worker_thread_que_size* MAX_NUM_OF_TIMER); 
    }
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "pod thread que size[%u], omc thread que size[%u].", self.pod_thread_que_size, self.omc_thread_que_size);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "worker thread num [%u], worker thread que size[%u].", self.worker_thread_num, self.worker_thread_que_size);

    if((NULL != self.mq_server_ip) && (NULL != self.mq_port)){
		const char *addr = oset_msprintf("%s:%s", self.mq_server_ip,self.mq_port);
		CProducer *omProducer = createAndStartProducer(self.mq_group_id, addr, 0);
		if (omProducer == NULL) return OSET_ERROR;
		self.mq_producer = omProducer;
		oset_free((void *)addr);
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "om create rtmq procuder success.IP[%s],PORT[%s],GROUP_ID[%s]",\
															self.mq_server_ip, self.mq_port, self.mq_group_id);
	}else{
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "om create rtmq procuder failed.IP[%s],PORT[%s],GROUP_ID[%s]",\
															self.mq_server_ip, self.mq_port, self.mq_group_id);
        return OSET_ERROR;
	}

	/*self.omc_timer = oset_timer_mgr_create(self.omc_thread_que_size* MAX_NUM_OF_TIMER);    
	oset_sys_assert(self.omc_timer);
	self.pod_timer = oset_timer_mgr_create(self.pod_thread_que_size* MAX_NUM_OF_TIMER);
	oset_sys_assert(self.pod_timer);*/

    self.pollset = oset_pollset_create(self.max_pod_num * MAX_SOCKET_NUM_OF_PER_POD);
    oset_sys_assert(self.pollset);
    oset_list_init(&self.udp_server_list);

	om_event_init();

	self.running = 1;
    return OSET_OK; 
}


OSET_DECLARE(void) om_sess_resource_destory(void)
{	
	self.running = 0;

	//clear session

    for(uint32_t id = 0; id < self.worker_thread_num; id++)
    {
		 oset_ring_queue_destroy(self.worker_queue[id]);
		 oset_ring_buf_destroy(self.worker_buf[id]);

		 oset_core_hash_destroy(&self.system_hashtable[id]);
		 oset_core_hash_destroy(&self.system_uuid_hashtable[id]);

		 oset_core_hash_destroy(&self.session_hashtable[id]);
		 oset_core_hash_destroy(&self.session_uuid_hashtable[id]);

		 oset_pool_final(&self.om_sess_id_pool[id]);
		 oset_pool_final(&self.om_system_id_pool[id]);
		 if (self.worker_timer[id])  oset_timer_mgr_destroy(self.worker_timer[id]);
    }
	oset_ring_queue_destroy(self.omc_queue);
	oset_ring_queue_destroy(self.pod_queue);
	oset_ring_queue_destroy(self.listen_queue);

    if(self.mq_producer) stopAndDestroyProducer(self.mq_producer);
    if(self.pollset) oset_pollset_destroy(self.pollset);
    //if(self.omc_timer) oset_timer_mgr_destroy(self.omc_timer);
    //if(self.pod_timer) oset_timer_mgr_destroy(self.pod_timer);
}


OSET_DECLARE(oset_sock_t *) om_udp_server(oset_socknode_t *node)
{
    char buf[OSET_ADDRSTRLEN];
    oset_sock_t *om_sock;
    oset_sys_assert(node);

    om_sock = oset_udp_server(node);
    if (om_sock) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "om_udp_server() success [%s]:%d",OSET_ADDR(node->addr, buf), OSET_PORT(node->addr));
    }
    return om_sock;
}

OSET_DECLARE(void) om_sockaddr4_add(void)
{
    int ret = OSET_ERROR;
    oset_sockaddr_t *addr = NULL;
    ret = oset_addaddrinfo(&addr, AF_UNSPEC, self.om_sock.self_ip, self.om_sock.udp_port_omc, 0);
    oset_sys_assert(ret == OSET_OK);
	if (addr) {
		oset_socknode_add(&self.udp_server_list, AF_INET, addr);
		oset_freeaddrinfo(addr);
        addr = NULL;
	}

    ret = oset_addaddrinfo(&addr, AF_UNSPEC, self.om_sock.self_ip, self.om_sock.udp_port_pod, 0);
    oset_sys_assert(ret == OSET_OK);	
	if (addr) {
		oset_socknode_add(&self.udp_server_list, AF_INET, addr);
		oset_freeaddrinfo(addr);
	    addr = NULL;
	}

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[Added] Number of om udp server is now %d", oset_list_count(&self.udp_server_list));

}


static void om_recv_udp_msg(short when, oset_socket_t fd, void *data)
{
    int rv = OSET_ERROR;
    om_event_t *e = NULL;
	oset_sock_t *tmp_sock = NULL;
    oset_pkbuf_t *pkbuf = NULL;
    ssize_t size;
	oset_sockaddr_t from;

    oset_sys_assert(data);
    oset_sys_assert(fd != INVALID_SOCKET);

	tmp_sock = (oset_sock_t *)data;

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG1, "receive udp msg, local receive port[%d]",OSET_PORT(&(tmp_sock->local_addr)));

	pkbuf = oset_pkbuf_alloc(NULL, OM_MAX_PKT_LEN);
	oset_sys_assert(pkbuf);
	oset_pkbuf_put(pkbuf, OM_MAX_PKT_LEN);
	
	size = oset_recvfrom(tmp_sock->fd, pkbuf->data, pkbuf->len, 0, &from);
	if (size <= 0) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "pod oset_recvfrom() failed [%s]", strerror(errno));
		oset_pkbuf_free(pkbuf);
		return;
	}
	oset_pkbuf_trim(pkbuf, size);

    e = om_listen_event_new(OM_LISTEN_MSG);
    oset_sys_assert(e);

	e->sock = tmp_sock;
    e->from = from;
    e->pkbuf = pkbuf;

    rv = oset_ring_queue_put(self.listen_queue, (uint8_t *)e, sizeof(*e));
    if (rv != OSET_OK) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "oset_ring_queue_put(pod_queue) err");
		oset_pkbuf_free(pkbuf);
        om_listen_event_free(e);
    }    

}


OSET_DECLARE(int) om_udp_server_open(void)
{
    oset_socknode_t *node = NULL;
    oset_sock_t *sock = NULL;

    /* udp Server */
    oset_list_for_each(&self.udp_server_list, node) {
        sock = om_udp_server(node);
        if (!sock) return OSET_ERROR;
        
        node->poll = oset_pollset_add(self.pollset, OSET_POLLIN, sock->fd, om_recv_udp_msg, sock);
        oset_sys_assert(node->poll);
    }
    return OSET_OK;
}

OSET_DECLARE(void) om_udp_server_close(void)
{
    oset_socknode_t *node = NULL;

    oset_socknode_remove_all(&self.udp_server_list);

    oset_list_for_each(&self.udp_server_list, node) {
        if (node->poll) oset_pollset_remove(node->poll);
        oset_closesocket(node->sock->fd);
    }
}

OSET_DECLARE(int) om_udp_sendto(oset_sock_t *sock, oset_sockaddr_t *dst, void *data, size_t len)
{
    ssize_t sent;

    oset_sys_assert(sock);
    oset_sys_assert(dst);
    oset_sys_assert(data);

    sent = oset_sendto(sock->fd, data, len, 0, dst);
    if (sent < 0 || sent != len) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "oset_sendto() failed [%s]", strerror(errno));
        return OSET_ERROR;
    }
    return OSET_OK;
}

static void om_remove_all_system(uint32_t id)
{
    om_system_t *system = NULL, *next = NULL;;

    oset_list_for_each_safe(&self.system_list[id], next, system) {
        //clear session
	    om_system_destory(system);
    }
}

static void om_task_termination(void)
{
    oset_ring_queue_term(self.listen_queue);
    oset_ring_queue_term(self.omc_queue);
    oset_ring_queue_term(self.pod_queue);

    for(uint32_t id = 0; id < self.worker_thread_num; id++){
        oset_ring_queue_term(self.worker_queue[id]);
	    om_remove_all_system(id);
    }
    oset_pollset_notify(self.pollset);
}


OSET_DECLARE(void) om_sess_termination(void)
{
	om_udp_server_close();
	//oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "om_task_termination waiting 3s!!!");
	oset_msleep(100);
	oset_scheduler_del_task_group("omc-massive-order");
	om_task_termination();
	om_event_final();
}

static void deal_om_worker_handle_thread(om_inner_worker_message_t *pkg, uint32_t len)
{
    om_inner_worker_message_t *msg = NULL;

    oset_sys_assert(pkg);

	msg = (om_inner_worker_message_t *)pkg;
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG1, "%s():[%u]", __func__, msg->e_type);

    switch (msg->e_type) {
		case OM_WORKER_E_HANDLE:
			oset_sys_assert(msg->u.udp.pkbuf);		
		    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG1, "Worker thread[%u]:OM_WORKER_MSG_HANDLE:msg_type[%d]", msg->thread_id, msg->m_type);
			switch (msg->m_type) {
			case OM_MSG_HRAETBEAT:
				hb_msg_deal(msg->thread_id, msg->sock, msg->u.udp.from, (pod_heartbeat_data_t *)msg->u.udp.pkbuf->data);
				break;
			
			case OM_MSG_DL_COMMAND_CP:
                cp_order_deal(msg->thread_id, msg->sock, msg->u.udp.from, (void *)msg->u.udp.pkbuf->data);
				break;
			
			case OM_MSG_DL_COMMAND_UP:
                up_order_deal(msg->thread_id, msg->sock, msg->u.udp.from, (void *)msg->u.udp.pkbuf->data);
				break;
				
			case OM_MSG_DL_COMMAND_MASSIVE:
                massive_order_deal(msg->thread_id, msg->sock, msg->u.udp.from, (void *)msg->u.udp.pkbuf->data);
				break;
				
			case OM_MSG_UL_REPORT_CP:
                cp_report_deal(msg->thread_id, msg->sock, msg->u.udp.from, (om_report_data_c_t *)msg->u.udp.pkbuf->data, msg->u.udp.pkbuf->len);
				break;
			
			case OM_MSG_UL_REPORT_UP:
                up_report_deal(msg->thread_id, msg->sock, msg->u.udp.from, (om_report_data_u_t *)msg->u.udp.pkbuf->data, msg->u.udp.pkbuf->len);
				break;
			
			case OM_MSG_UL_MASSIVE_UE:
				ue_static_deal(msg->thread_id, msg->sock, msg->u.udp.from, (om_report_ue_data_t *)msg->u.udp.pkbuf->data);
				break;
			
			case OM_MSG_UL_MASSIVE_GNB:
				gnb_static_deal(msg->thread_id, msg->sock, msg->u.udp.from, (om_report_gnb_data_t *)msg->u.udp.pkbuf->data);
				break;			
			default:
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Worker thread[%u]:No handler for msg type %d", msg->thread_id, msg->m_type);
				break;
			}
	        oset_pkbuf_free(msg->u.udp.pkbuf);
            break;
        case OM_WORKER_E_TIMER:
			 switch (msg->m_type) {
				case OM_WORKER_TIMER_HB_LOST:
					timer_msg_deal(msg->thread_id, (om_session_t *)msg->u.sess);
					break;			
				default:
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Worker thread[%u]:No handler for timer type %d", msg->thread_id, msg->m_type);
					break;
				}
            break;
		default:
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Worker thread[%u]:No handler for event type %d", msg->thread_id, msg->e_type);
			break;
	}
}


//static void *om_worker_deal_thread(oset_apr_thread_t *thread, void *obj)
static void *om_worker_handle_thread(oset_threadplus_t *thread, void *data)
{
	//oset_core_thread_session_t *ts = obj;
	uint32_t id = 0;
    int rv = OSET_ERROR;
	uint8_t *pkt = NULL;
	uint32_t len = 0;

    //id = *(uint32_t *)(ts->objs[0]);
    id = *(uint32_t *)(data);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "om_worker_deal_thread[%p][id = %d] running", thread, id);

    while(self.running) {
        oset_timer_mgr_expire(self.worker_timer[id]);
		// 处理超时,不能阻塞
		rv = oset_ring_queue_try_get(self.worker_queue[id], &pkt, &len);
		if(rv != OSET_OK)
		{
	       if (rv == OSET_DONE)
		   	   break;

		   if (rv == OSET_RETRY){
			   //oset_usleep(500);
			   continue;
		   }
		}
		deal_om_worker_handle_thread((om_inner_worker_message_t *)pkt, len);
		oset_ring_buf_ret(pkt);
		pkt = NULL;
		len = 0;
    }

	//oset_apr_thread_exit(thread, 0);
	return NULL;
}


static int omc_json_order_pre_decode(void *order, om_msg_header_t **header)
{
	om_msg_header_t *t_header = NULL;

    const char *system_id = NULL;
    const char *pod_id = NULL;
    const char *container_id = NULL;
    const char *node_id = NULL;
	char *context = NULL;
    int   msg_type = 0;
	cJSON *jdata = NULL;
	cJSON *item = NULL;


    t_header = (om_msg_header_t *)oset_calloc(1, sizeof(om_msg_header_t));

	jdata = cJSON_Parse((char *)order);

	context = cJSON_Print(jdata);
	if(NULL == context){
		oset_free(t_header);
		cJSON_Delete(jdata);
	    return OSET_ERROR;
	}
	//context = cJSON_PrintUnformatted(jdata);
	//oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "omc->pod receive:\n\n %s", content);
    oset_safe_free(context);
	
	system_id = cJSON_GetObjectCstr(jdata, "systemId");
	pod_id = cJSON_GetObjectCstr(jdata, "podId");
	container_id = cJSON_GetObjectCstr(jdata, "containerId");
	node_id = cJSON_GetObjectCstr(jdata, "nodeId");
	if(system_id) memcpy(t_header->system_id , system_id, strlen(system_id) + 1);
	if(pod_id) memcpy(t_header->pod_id , pod_id, strlen(pod_id) + 1);
	if(container_id) memcpy(t_header->container_id , container_id, strlen(container_id) + 1);
	if(node_id) memcpy(t_header->node_id , node_id, strlen(node_id) + 1);

    *header = t_header;

	item = cJSON_GetObjectItem(jdata, "messageType");
	if (!cJSON_IsNumber(item)){
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc->pod:[messageType] json pre decode failed"); 
		cJSON_Delete(jdata);
	    oset_free(t_header);
		return OSET_ERROR;
	}
	msg_type = item->valueint;

	cJSON_Delete(jdata);
	return msg_type;
}

static void deal_om_omc_handle_thread(om_event_t *e)
{

	int rv = OSET_ERROR;
	oset_pkbuf_t *pkbuf = NULL;
	om_inner_worker_message_t *omc_msg = NULL;
	om_msg_header_t *header = NULL;
	uint32_t id = 0;
    int msg_type = -1;

	oset_sys_assert(e);
	pkbuf = e->pkbuf;
    oset_sys_assert(pkbuf);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG1, "%s(): %s", __func__, om_event_get_name(e));

	switch (e->id) {
	case OM_OMC_MSG:
        msg_type = omc_json_order_pre_decode((void *)pkbuf->data, &header);
		if(OSET_ERROR == msg_type){
			oset_pkbuf_free(pkbuf);
			return;
		}

		id = hash_calculate(header->system_id,strlen(header->system_id))%self.worker_thread_num;

        oset_free(header);
		omc_msg = (om_inner_worker_message_t *)oset_ring_buf_get(self.worker_buf[id]);
		if(NULL == omc_msg) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc oset_ring_buf_get(worker_buf[%d]) err",id);
			oset_pkbuf_free(pkbuf);
			return;
		}

		omc_msg->e_type = OM_WORKER_E_HANDLE;
		omc_msg->m_type = msg_type;
		omc_msg->thread_id = id;
		omc_msg->sock = e->sock;
		omc_msg->u.udp.from = e->from;
		omc_msg->u.udp.pkbuf = pkbuf;

		rv = oset_ring_queue_put(self.worker_queue[id], (uint8_t *)omc_msg, sizeof(om_inner_worker_message_t));
		if (rv != OSET_OK) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc oset_ring_queue_put(pod_queue[%d]) err",id);
			oset_ring_buf_ret((uint8_t *)omc_msg);
			oset_pkbuf_free(pkbuf);
		}	 
		break;

	default:
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc No handler for event %s", om_event_get_name(e));
		break;
	}

}


//static void *om_omc_handle_thread(oset_apr_thread_t *thread, void *obj)
static void *om_omc_handle_thread(oset_threadplus_t *thread, void *data)
{
    int rv = OSET_ERROR;
	om_event_t *e = NULL;
	uint32_t len = 0;

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "om_omc_handle_thread[%p] running", thread);

    while(self.running) {
		//oset_timer_mgr_expire(self.omc_timer);
		rv = oset_ring_queue_try_get(self.omc_queue, (uint8_t **)&e, &len);
		if(rv != OSET_OK)
		{
	       if (rv == OSET_DONE)
		   	   break;

		   if (rv == OSET_RETRY){
		       continue;
		   }
		}
		deal_om_omc_handle_thread(e);
		om_omc_event_free(e);
		e = NULL;
		len = 0;
    }
	//oset_apr_thread_exit(thread, 0);
    return NULL;
}


static void deal_om_pod_handle_thread(om_event_t *e)
{
	int rv = OSET_ERROR;
	oset_pkbuf_t *pkbuf = NULL;
	om_inner_worker_message_t *pod_msg = NULL;
	om_msg_header_t *header = NULL;
	uint32_t id = 0;

	oset_sys_assert(e);
	pkbuf = e->pkbuf;
	oset_sys_assert(pkbuf);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG1, "%s(): %s", __func__, om_event_get_name(e));

	switch (e->id) {
	case OM_POD_MSG:
		header = (om_msg_header_t *)(pkbuf->data);

	    id = hash_calculate(header->system_id,strlen(header->system_id))%self.worker_thread_num;

	    pod_msg = (om_inner_worker_message_t *)oset_ring_buf_get(self.worker_buf[id]);
		if(NULL == pod_msg) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod oset_ring_buf_get(worker_buf[%d]) err",id);
	        oset_pkbuf_free(pkbuf);
			return;
		}

		pod_msg->e_type = OM_WORKER_E_HANDLE;
		pod_msg->m_type = *(int *)(pkbuf->data + sizeof(*header));
		pod_msg->thread_id = id;
		pod_msg->sock = e->sock;
		pod_msg->u.udp.from = e->from;
		pod_msg->u.udp.pkbuf = pkbuf;

	    rv = oset_ring_queue_put(self.worker_queue[id], (uint8_t *)pod_msg, sizeof(om_inner_worker_message_t));
	    if (rv != OSET_OK) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod oset_ring_queue_put(pod_queue[%d]) err",id);
			oset_ring_buf_ret((uint8_t *)pod_msg);
	        oset_pkbuf_free(pkbuf);
	    }    
	    break;

	default:
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod No handler for event %s", om_event_get_name(e));
		break;
	}
}


//static void *om_pod_handle_thread(oset_apr_thread_t *thread, void *obj)
static void *om_pod_handle_thread(oset_threadplus_t *thread, void *data)
{
	int rv = OSET_ERROR;
	om_event_t *e = NULL;
	uint32_t len = 0;

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "om_pod_handle_thread[%p] running", thread);

	while(self.running) {
		//oset_timer_mgr_expire(self.pod_timer);
		rv = oset_ring_queue_try_get(self.pod_queue, (uint8_t **)&e, &len);
		if(rv != OSET_OK)
		{
	       if (rv == OSET_DONE)
		   	   break;

		   if (rv == OSET_RETRY){
			   continue;
		   }

		}
		deal_om_pod_handle_thread(e);
		om_pod_event_free(e);
		e = NULL;
		len = 0;
	}
	//oset_apr_thread_exit(thread, 0);
	return NULL;
}


static void deal_om_main_listen_thread(om_event_t *e)
 {
	 int rv = OSET_ERROR;
	 om_event_t *tmp_e = NULL;

	 oset_sys_assert(e);
	 oset_sys_assert(e->pkbuf);
	 oset_sys_assert(e->sock->fd != INVALID_SOCKET);

	 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG1, "%s(): %s", __func__, om_event_get_name(e));

	 switch (e->id) {
	 case OM_LISTEN_MSG:
		 if(OSET_PORT(&(e->sock->local_addr)) == self.om_sock.udp_port_pod){
			 tmp_e = om_pod_event_new(OM_POD_MSG);
			 oset_sys_assert(tmp_e);

			 tmp_e->sock = e->sock;
			 tmp_e->pkbuf = e->pkbuf;
			 tmp_e->from = e->from;

			 rv = oset_ring_queue_put(self.pod_queue, (uint8_t *)tmp_e, sizeof(*tmp_e));
			 if (rv != OSET_OK) {
				 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "oset_ring_queue_put(pod_queue) err");
				 oset_pkbuf_free(e->pkbuf);
				 om_pod_event_free(tmp_e);
			 }	  
		 }else if(OSET_PORT(&(e->sock->local_addr)) == self.om_sock.udp_port_omc){
			 tmp_e = om_omc_event_new(OM_OMC_MSG);
			 oset_sys_assert(tmp_e);
			 
			 tmp_e->sock = e->sock;
			 tmp_e->pkbuf = e->pkbuf;
		     tmp_e->from = e->from;

			 rv = oset_ring_queue_put(self.omc_queue, (uint8_t *)tmp_e, sizeof(*tmp_e));
			 if (rv != OSET_OK) {
				 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "oset_ring_queue_put(omc_queue) err");
				 oset_pkbuf_free(e->pkbuf);
				 om_omc_event_free(tmp_e);
			 } 
		 }else{
			 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "unkown error udp msg, local receive port[%d]",OSET_PORT(&(e->sock->local_addr)));
		 }		 
		 break;

	 default:
		 oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "No handler for event %s", om_event_get_name(e));
		 break;
	 }
 }


//static void *om_main_listen_thread(oset_apr_thread_t *thread, void *obj)
static void *om_main_listen_thread(oset_threadplus_t *thread, void *data)
{
	int rv = OSET_ERROR;
	om_event_t *e = NULL;
	uint32_t len = 0;

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "om_main_listen_thread[%p] running", thread);

	while(self.running) {
	    //oset_pollset_poll(self.pollset, oset_time_from_msec(100));
	    oset_pollset_poll(self.pollset, OSET_INFINITE_TIME);
	    for ( ;; ) {
			rv = oset_ring_queue_try_get(self.listen_queue, (uint8_t **)&e, &len);
			if(rv != OSET_OK)
			{
		       if (rv == OSET_DONE)
	               goto done;

			   if (rv == OSET_RETRY){		   	
			       break;
			   }

			}
			deal_om_main_listen_thread(e);
	        om_listen_event_free(e);
	        e = NULL;
	        len = 0;
	    }
	}
	done:
	//oset_apr_thread_exit(thread, 0);
	return NULL;
}



 OSET_DECLARE(int) om_add_thread_task(oset_apr_memory_pool_t *pool)
 {
	 int rv = OSET_ERROR;

	 /*for (uint32_t i = 0; i < self.worker_thread_num; i++) {
		 oset_core_launch_thread(om_worker_handle_thread, (void *)&i, pool);
		 oset_msleep(10);
	 }	 
	 oset_core_launch_thread(om_pod_handle_thread, NULL, pool);
	 oset_msleep(10);

	 oset_core_launch_thread(om_omc_handle_thread, NULL, pool);
	 oset_msleep(10);

	 oset_core_launch_thread(om_main_listen_thread, NULL, pool);*/


	 /*use thread pools*/
	for (uint32_t i = 0; i < self.worker_thread_num; i++) {
		 rv = oset_threadpool_push(runtime.thrp, om_worker_handle_thread, (void *)&i, PRIORITY_LEVEL_3, "worker_deal_thread");
		 oset_sys_assert(OSET_OK == rv);
		 oset_msleep(10);
	 }

	 rv = oset_threadpool_push(runtime.thrp, om_pod_handle_thread, NULL, PRIORITY_LEVEL_2, "pod_handle_thread");
	 oset_sys_assert(OSET_OK == rv);
	 oset_msleep(10);

	 rv = oset_threadpool_push(runtime.thrp, om_omc_handle_thread, NULL, PRIORITY_LEVEL_2, "omc_handle_thread");
	 oset_sys_assert(OSET_OK == rv);
	 oset_msleep(10);

	 rv = oset_threadpool_push(runtime.thrp, om_main_listen_thread, NULL, PRIORITY_LEVEL_1, "main_listen_thread");
	 oset_sys_assert(OSET_OK == rv);
	 return OSET_OK;
 }
