#include "oset-core.h"
#include "om-message.h"


static om_transfer_order_massive_t *build_m_order(int type, char *buf)
{ 
	om_transfer_order_massive_t *t_order = NULL; 
    t_order = (om_transfer_order_massive_t *)oset_calloc(1, sizeof(om_transfer_order_massive_t));

	memcpy(t_order->system_id, buf, strlen(buf) + 1);
    t_order->msg_type =  type;
	t_order->order = (int)START;

	return t_order;
}

static char *omc_json_m_order_encode(om_transfer_order_massive_t *order)
{
    char  *context = NULL;
	cJSON *item = NULL;
	oset_sys_assert(order);

    item = cJSON_CreateObject();

	if (cJSON_AddStringToObject(item, "systemId", order->system_id) == NULL) {
	    oset_error_tt("omc->pod:[order.systemId] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item, "messageType", order->msg_type) == NULL) {
	    oset_error_tt("omc->pod:[order.messageType] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item, "order", order->order) == NULL) {
	    oset_error_tt("omc->pod:[order.order] json encode failed"); 
		goto end;
	}

end:
	if (item) {
		context = cJSON_Print(item);
		oset_sys_assert(context);
	    oset_warn_tt("omc->pod send :\n %s", context);
		cJSON_Delete(item);
	}
    return context;
}


static om_transfer_order_c_t *build_cp_order(int type, int del_flag, char *buf)
{
    char suci[MAX_SUCI_SIZE] = "460070000000005";
 
	om_transfer_order_c_t *t_order = NULL; 
    t_order = (om_transfer_order_c_t *)oset_calloc(1, sizeof(om_transfer_order_c_t));

	memcpy(t_order->header.system_id, buf, strlen(buf) + 1);
    if(del_flag){
		memcpy(t_order->header.pod_id, buf, strlen(buf) + 1);
		memcpy(t_order->header.container_id, buf, strlen(buf) + 1);
		memcpy(t_order->header.node_id, buf, strlen(buf) + 1);
	}

    t_order->msg_type =  type;
	memcpy(t_order->c_order.suci, suci, strlen(suci) + 1);
	memcpy(t_order->c_order.others, "test", MAX_OTHER_SIZE);
	t_order->c_order.order = (int)AUTOSETUP;

	return t_order;
}


static char *omc_json_cp_order_encode(om_transfer_order_c_t *order)
{
    char  *context = NULL;
	cJSON *item = NULL;
	cJSON *item_cp = NULL;
	oset_sys_assert(order);

    item = cJSON_CreateObject();

	if (cJSON_AddStringToObject(item, "systemId", order->header.system_id) == NULL) {
	    oset_error_tt("omc->pod:[order.header.systemId] json encode failed"); 
		goto end;
	}

    if(strlen(order->header.pod_id)){
		if (cJSON_AddStringToObject(item, "podId", order->header.pod_id) == NULL) {
			oset_error_tt("omc->pod:[order.header.podId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "podId") == NULL) {
			oset_error_tt("omc->pod:[order.cOrder.podId] json encode failed"); 
			goto end;
		}
	}

	if (cJSON_AddStringToObject(item, "containerId", order->header.container_id) == NULL) {
	    oset_error_tt("pod->omc:[order.header.containerId] json encode failed"); 
		goto end;
	}

	if (cJSON_AddStringToObject(item, "nodeId", order->header.node_id) == NULL) {
	    oset_error_tt("omc->pod:[order.header.nodeId] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item, "messageType", order->msg_type) == NULL) {
	    oset_error_tt("omc->pod:[order.messageType] json encode failed"); 
		goto end;
	}

    item_cp = cJSON_CreateObject();

    if(strlen(order->c_order.suci)){
		if (cJSON_AddStringToObject(item_cp, "suci", order->c_order.suci) == NULL) {
			oset_error_tt("omc->pod:[order.cOrder.suci] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item_cp, "suci") == NULL) {
			oset_error_tt("omc->pod:[order.cOrder.suci] json encode failed"); 
			goto end;
		}
	}

	if (cJSON_AddStringToObject(item_cp, "others", order->c_order.others) == NULL) {
	    oset_error_tt("omc->pod:[order.cOrder.suci] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_cp, "order", order->c_order.order) == NULL) {
	    oset_error_tt("omc->pod:[order.cOrder.order] json encode failed"); 
		goto end;
	}

    cJSON_AddItemToObject(item, "cOrder", item_cp);
    if (item->child == NULL) {
	    oset_error_tt("omc->pod:[order.cOrder] json encode failed"); 
        goto end;
    }

end:
	if (item) {
		context = cJSON_Print(item);
		oset_sys_assert(context);
	    oset_warn_tt("omc->pod send :\n %s", context);
		cJSON_Delete(item);
	}
    return context;
}


static om_transfer_order_u_t *build_up_order(int type, int del_flag, char *buf)
{
    char suci[MAX_SUCI_SIZE] = "460070000000005";
 
	om_transfer_order_u_t *t_order = NULL; 
    t_order = (om_transfer_order_u_t *)oset_calloc(1, sizeof(om_transfer_order_u_t));

	memcpy(t_order->header.system_id, buf, strlen(buf) + 1);
    if(del_flag){
		memcpy(t_order->header.pod_id, buf, strlen(buf) + 1);
		memcpy(t_order->header.container_id, buf, strlen(buf) + 1);
		memcpy(t_order->header.node_id, buf, strlen(buf) + 1);
	}

    t_order->msg_type =  type;
	memcpy(t_order->u_order.suci, suci, strlen(suci) + 1);
	memcpy(t_order->u_order.target_ip, "127.0.0.1", MAX_IP_SIZE);
	memcpy(t_order->u_order.others, "test", MAX_OTHER_SIZE);
	memcpy(t_order->u_order.src_dir_path, "/home/sset", MAX_DIR_SIZE);
	memcpy(t_order->u_order.target_dir_path, "/home/973", MAX_DIR_SIZE);

	t_order->u_order.order = (int)DO_PING;

	return t_order;
}


static char *omc_json_up_order_encode(om_transfer_order_u_t *order)
{
    char  *context = NULL;
	cJSON *item = NULL;
	cJSON *item_up = NULL;
	oset_sys_assert(order);

    item = cJSON_CreateObject();

	if (cJSON_AddStringToObject(item, "systemId", order->header.system_id) == NULL) {
	    oset_error_tt("omc->pod:[order.header.systemId] json encode failed"); 
		goto end;
	}

    if(strlen(order->header.pod_id)){
		if (cJSON_AddStringToObject(item, "podId", order->header.pod_id) == NULL) {
			oset_error_tt("omc->pod:[order.header.podId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "podId") == NULL) {
			oset_error_tt("omc->pod:[order.cOrder.podId] json encode failed"); 
			goto end;
		}
	}

	if (cJSON_AddStringToObject(item, "containerId", order->header.container_id) == NULL) {
	    oset_error_tt("pod->omc:[order.header.containerId] json encode failed"); 
		goto end;
	}

	if (cJSON_AddStringToObject(item, "nodeId", order->header.node_id) == NULL) {
	    oset_error_tt("omc->pod:[order.header.nodeId] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item, "messageType", order->msg_type) == NULL) {
	    oset_error_tt("omc->pod:[order.messageType] json encode failed"); 
		goto end;
	}

    item_up = cJSON_CreateObject();

    if(strlen(order->u_order.suci)){
		if (cJSON_AddStringToObject(item_up, "suci", order->u_order.suci) == NULL) {
			oset_error_tt("omc->pod:[order.uOrder.suci] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item_up, "suci") == NULL) {
			oset_error_tt("omc->pod:[order.uOrder.suci] json encode failed"); 
			goto end;
		}
	}

    if(strlen(order->u_order.target_ip)){
		if (cJSON_AddStringToObject(item_up, "targetIp", order->u_order.target_ip) == NULL) {
			oset_error_tt("omc->pod:[order.uOrder.targetIp] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item_up, "targetIp") == NULL) {
			oset_error_tt("omc->pod:[order.uOrder.targetIp] json encode failed"); 
			goto end;
		}
	}

	if (cJSON_AddStringToObject(item_up, "others", order->u_order.others) == NULL) {
	    oset_error_tt("omc->pod:[order.uOrder.others] json encode failed"); 
		goto end;
	}

	if (cJSON_AddStringToObject(item_up, "sourceDirPath", order->u_order.src_dir_path) == NULL) {
	    oset_error_tt("omc->pod:[order.uOrder.sourceDirPath] json encode failed"); 
		goto end;
	}

	if (cJSON_AddStringToObject(item_up, "targetDirPath", order->u_order.target_dir_path) == NULL) {
	    oset_error_tt("omc->pod:[order.uOrder.targetDirPath] json encode failed"); 
		goto end;
	}


	if (cJSON_AddNumberToObject(item_up, "order", order->u_order.order) == NULL) {
	    oset_error_tt("omc->pod:[order.uOrder.order] json encode failed"); 
		goto end;
	}

    cJSON_AddItemToObject(item, "uOrder", item_up);
    if (item->child == NULL) {
	    oset_error_tt("omc->pod:[order.uOrder] json encode failed"); 
        goto end;
    }

end:
	if (item) {
		context = cJSON_Print(item);
		oset_sys_assert(context);
	    oset_warn_tt("omc->pod send :\n %s", context);
		cJSON_Delete(item);
	}
    return context;
}


int main(int argc, const char *const argv[])
{
    oset_sock_t *udp2;
    oset_sockaddr_t  *addr2;
    oset_socknode_t  *node2;
    //char str[1024];
    //ssize_t size;

    oset_pkbuf_config_t config;

    oset_core_initialize();
    oset_pkbuf_default_init(&config);
    oset_pkbuf_default_create(&config);

    oset_getaddrinfo(&addr2, AF_INET, "10.37.6.28", 20086, 0);  //omc
    node2 = oset_socknode_new(addr2);
    udp2 = oset_udp_client(node2);
	
    char buf[MAX_ID_SIZE] = "29b30ba6-3840-49e7-b317-ccd3";
	char *context0 = NULL;
    om_transfer_order_c_t * corder = build_cp_order(OM_MSG_DL_COMMAND_CP,1, buf);
    oset_info_tt("==>%s", "send to pod1");
	oset_info_tt("sendc1==>%s", corder->header.system_id);
    oset_info_tt("sendc1==>%s", corder->header.pod_id);	
	oset_info_tt("sendc1==>%s", corder->header.container_id);	
	oset_info_tt("sendc1==>%s", corder->header.node_id);	
	oset_info_tt("sendc1==>%d", corder->msg_type);
	oset_info_tt("sendc1==>%s", corder->c_order.suci);
	oset_info_tt("sendc1==>%s", corder->c_order.others);
	oset_info_tt("sendc1==>%d", corder->c_order.order);
    context0 = omc_json_cp_order_encode(corder); //down omc command

	char *context1 = NULL;
    om_transfer_order_u_t * uorder = build_up_order(OM_MSG_DL_COMMAND_UP,1, buf);
    oset_info_tt("==>%s", "send to pod1");
	oset_info_tt("sendu1==>%s", uorder->header.system_id);
    oset_info_tt("sendu1==>%s", uorder->header.pod_id);	
	oset_info_tt("sendu1==>%s", uorder->header.container_id);	
	oset_info_tt("sendu1==>%s", uorder->header.node_id);	
	oset_info_tt("sendu1==>%d", uorder->msg_type);
	oset_info_tt("sendu1==>%s", uorder->u_order.suci);
	oset_info_tt("sendu1==>%s", uorder->u_order.target_ip);
	oset_info_tt("sendu1==>%s", uorder->u_order.others);
	oset_info_tt("sendu1==>%s", uorder->u_order.src_dir_path);
	oset_info_tt("sendu1==>%s", uorder->u_order.target_dir_path);
	oset_info_tt("sendu1==>%d", uorder->u_order.order);
    context1 = omc_json_up_order_encode(uorder); //down omc command


    char buf1[MAX_ID_SIZE] = "29b30ba6-3840-49e7-b317-ccd4";
	char *context2 = NULL;
    om_transfer_order_c_t * corder1 = build_cp_order(OM_MSG_DL_COMMAND_CP,0, buf1);
    oset_info_tt("==>%s", "send to pod2");
	oset_info_tt("sendc2==>%s", corder1->header.system_id);
    oset_info_tt("sendc2==>%s", corder1->header.pod_id);	
	oset_info_tt("sendc2==>%s", corder1->header.container_id);	
	oset_info_tt("sendc2==>%s", corder1->header.node_id);	
	oset_info_tt("sendc2==>%d", corder1->msg_type);
	oset_info_tt("sendc2==>%s", corder1->c_order.suci);
	oset_info_tt("sendc2==>%s", corder1->c_order.others);
	oset_info_tt("sendc2==>%d", corder1->c_order.order);
    context2 = omc_json_cp_order_encode(corder1); //down omc command
    
	char *context3 = NULL;
    om_transfer_order_u_t * uorder1 = build_up_order(OM_MSG_DL_COMMAND_UP,0, buf1);
    oset_info_tt("==>%s", "send to pod2");
	oset_info_tt("sendu2==>%s", uorder1->header.system_id);
    oset_info_tt("sendu2==>%s", uorder1->header.pod_id);	
	oset_info_tt("sendu2==>%s", uorder1->header.container_id);	
	oset_info_tt("sendu2==>%s", uorder1->header.node_id);	
	oset_info_tt("sendu2==>%d", uorder1->msg_type);
	oset_info_tt("sendu2==>%s", uorder1->u_order.suci);
	oset_info_tt("sendu2==>%s", uorder1->u_order.target_ip);
	oset_info_tt("sendu2==>%s", uorder1->u_order.others);
	oset_info_tt("sendu2==>%s", uorder1->u_order.src_dir_path);
	oset_info_tt("sendu2==>%s", uorder1->u_order.target_dir_path);
	oset_info_tt("sendu2==>%d", uorder1->u_order.order);
    context3 = omc_json_up_order_encode(uorder1); //down omc command


	char *context4 = NULL;
    om_transfer_order_massive_t * morder = build_m_order(OM_MSG_DL_COMMAND_MASSIVE, buf1);
    oset_info_tt("==>%s", "send to pod2");
	oset_info_tt("sendm==>%s", morder->system_id);	
	oset_info_tt("sendm==>%d", morder->msg_type);
	oset_info_tt("sendm==>%d", morder->order);
    context4 = omc_json_m_order_encode(morder); //down omc command


    int i = 0;

	//oset_send(udp2->fd, context4, strlen(context4), 0);  //ccd4 m

    while(1)
	{
         //oset_send(udp2->fd, context0, strlen(context0), 0);  //ccd3 c
         oset_msleep(10);
         //oset_send(udp2->fd, context1, strlen(context1), 0);  //ccd3 u
         oset_msleep(10);
         //oset_send(udp2->fd, context2, strlen(context2), 0);  //ccd4 c b
         oset_msleep(10);
         oset_send(udp2->fd, context3, strlen(context3), 0);  //ccd4 u b
         oset_msleep(10000);
	     oset_info_tt("%d", i++);
		 if(i == 1000) i = 0;
	}
    oset_safe_free(context0);
    oset_safe_free(context1);
    oset_safe_free(context2);
    oset_safe_free(context3);
    oset_safe_free(context4);

	oset_free(corder);
	oset_free(uorder);
	oset_free(corder1);
	oset_free(uorder1);
	oset_free(morder);

    oset_socknode_free(node2);

    oset_pkbuf_default_destroy();
    oset_core_terminate();
}
