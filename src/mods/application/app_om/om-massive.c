/************************************************************************
 *File name: om-massive.c
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/
#include "om-public.h"
#include "asyncProducer.h"

static char *omc_json_ue_static_report_encode(om_report_ue_data_t *ue_rpt)
{
    char  *report = NULL;
	cJSON *item = NULL;
	cJSON *item_ue  = NULL;
	oset_sys_assert(ue_rpt);

    item = cJSON_CreateObject();

    if(ue_rpt->header.system_id){
		if (cJSON_AddStringToObject(item, "systemId", ue_rpt->header.system_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.header.systemId] json encode failed"); 
			goto end;
		}
	}else{
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "system_id is NULL");
		goto end;
	}

    if(ue_rpt->header.pod_id && strlen(ue_rpt->header.pod_id)){
		if (cJSON_AddStringToObject(item, "podId", ue_rpt->header.pod_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.header.podId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "podId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.header.podId] json encode failed"); 
			goto end;
		}
	}


    if(ue_rpt->header.container_id && strlen(ue_rpt->header.container_id)){
		if (cJSON_AddStringToObject(item, "containerId", ue_rpt->header.container_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.header.containerId] json encode failed"); 
			goto end;
		}
    }else{
		if (cJSON_AddNullToObject(item, "containerId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.header.containerId] json encode failed"); 
			goto end;
		}
	}


    if(ue_rpt->header.node_id && strlen(ue_rpt->header.node_id)){
		if (cJSON_AddStringToObject(item, "nodeId", ue_rpt->header.node_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.header.nodeId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "nodeId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.header.nodeId] json encode failed"); 
			goto end;
		}
	}


	if (cJSON_AddNumberToObject(item, "messageType", ue_rpt->msg_type) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.messageType] json encode failed"); 
		goto end;
	}



    item_ue = cJSON_CreateObject();

	if (cJSON_AddNumberToObject(item_ue, "timeStep", ue_rpt->ue_static.time_step) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.staticDataUe.timeStep] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_ue, "rrcReqNum", ue_rpt->ue_static.rrc_req_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.staticDataUe.rrcReqNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_ue, "rrcSetupCompleteNum", ue_rpt->ue_static.rrc_setup_cpt_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.staticDataUe.rrcSetupCompleteNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_ue, "rrcRejNum", ue_rpt->ue_static.rrc_rej_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.staticDataUe.rrcRejNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_ue, "abnormalRelNum", ue_rpt->ue_static.abnormal_rel_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.staticDataUe.abnormalRelNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_ue, "averageBler", ue_rpt->ue_static.average_bler) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.staticDataUe.averageBler] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_ue, "averageLatency", ue_rpt->ue_static.average_latency) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.staticDataUe.averageLatency] json encode failed"); 
		goto end;
	}

    cJSON_AddItemToObject(item, "staticDataUe", item_ue);
    if (item->child == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[ue-massive.staticDataUe] json encode failed"); 
        goto end;
    }

end:
	if (item) {
		report = cJSON_Print(item);
		//report = cJSON_PrintUnformatted(item);
		oset_sys_assert(report);
	    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "pod->omc ue_massive_report send:\n%s\n\n", report);
		cJSON_Delete(item);
	}
    return report;
}


static char *omc_json_gnb_static_report_encode(om_report_gnb_data_t *gnb_rpt)
{
    char  *report = NULL;
	cJSON *item = NULL;
	cJSON *item_gnb  = NULL;
	oset_sys_assert(gnb_rpt);

    item = cJSON_CreateObject();

    if(gnb_rpt->header.system_id){
		if (cJSON_AddStringToObject(item, "systemId", gnb_rpt->header.system_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.header.systemId] json encode failed"); 
			goto end;
		}
	}else{
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "system_id is NULL");
		goto end;
	}

    if(gnb_rpt->header.pod_id && strlen(gnb_rpt->header.pod_id)){
		if (cJSON_AddStringToObject(item, "podId", gnb_rpt->header.pod_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.header.podId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "podId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.header.podId] json encode failed"); 
			goto end;
		}
	}


    if(gnb_rpt->header.container_id && strlen(gnb_rpt->header.container_id)){
		if (cJSON_AddStringToObject(item, "containerId", gnb_rpt->header.container_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.header.containerId] json encode failed"); 
			goto end;
		}
    }else{
		if (cJSON_AddNullToObject(item, "containerId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.header.containerId] json encode failed"); 
			goto end;
		}
	}


    if(gnb_rpt->header.node_id && strlen(gnb_rpt->header.node_id)){
		if (cJSON_AddStringToObject(item, "nodeId", gnb_rpt->header.node_id) == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.header.nodeId] json encode failed"); 
			goto end;
		}
	}else{
		if (cJSON_AddNullToObject(item, "nodeId") == NULL) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.header.nodeId] json encode failed"); 
			goto end;
		}
	}


	if (cJSON_AddNumberToObject(item, "messageType", gnb_rpt->msg_type) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.messageType] json encode failed"); 
		goto end;
	}



    item_gnb = cJSON_CreateObject();

	if (cJSON_AddNumberToObject(item_gnb, "timeStep", gnb_rpt->gnb_static.time_step) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.staticDataGnb.timeStep] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_gnb, "rrcReqNum", gnb_rpt->gnb_static.rrc_req_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.staticDataGnb.rrcReqNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_gnb, "rrcSetupCompleteNum", gnb_rpt->gnb_static.rrc_setup_cpt_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.staticDataGnb.rrcSetupCompleteNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_gnb, "rrcRejNum", gnb_rpt->gnb_static.rrc_rej_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.staticDataGnb.rrcRejNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_gnb, "abnormalRelNum", gnb_rpt->gnb_static.abnormal_rel_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.staticDataGnb.abnormalRelNum] json encode failed"); 
		goto end;
	}

	if (cJSON_AddNumberToObject(item_gnb, "activeNum", gnb_rpt->gnb_static.active_num) == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.staticDataGnb.averageBler] json encode failed"); 
		goto end;
	}

    cJSON_AddItemToObject(item, "staticDataGnb", item_gnb);
    if (item->child == NULL) {
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod->omc:[gnb-massive.staticDataGnb] json encode failed"); 
        goto end;
    }

end:
	if (item) {
		report = cJSON_Print(item);
		//report = cJSON_PrintUnformatted(item);

		oset_sys_assert(report);
	    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "pod->omc gnb_massive_report send:\n%s\n\n", report);
		cJSON_Delete(item);
	}
    return report;
}


static om_transfer_order_massive_t *omc_json_massive_order_decode(void *data)
{
	om_transfer_order_massive_t *t_omc = NULL;

	const char *system_id = NULL;

	cJSON *jdata = NULL;
	cJSON *item = NULL;
	char *context = NULL;

	t_omc = (om_transfer_order_massive_t *)oset_calloc(1, sizeof(om_transfer_order_massive_t));

	jdata = cJSON_Parse((char *)data);
	context = cJSON_Print(jdata);
	oset_sys_assert(context);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "omc->pod massive_order receive:\n%s\n\n", context);
	oset_safe_free(context);

	system_id = cJSON_GetObjectCstr(jdata, "systemId");

	if(system_id) memcpy(t_omc->system_id , system_id, strlen(system_id) + 1);

	item = cJSON_GetObjectItem(jdata, "messageType");
	if (!cJSON_IsNumber(item)){
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc->pod:[massive_order.messageType] json decode failed"); 
		cJSON_Delete(jdata);
		oset_free(t_omc);
		return NULL;
	}

	t_omc->msg_type = item->valueint;

	item = cJSON_GetObjectItem(jdata, "order");
	if (!cJSON_IsNumber(item)){
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc->pod:[massive_order.order] json decode failed"); 
		cJSON_Delete(jdata);
		oset_free(t_omc);
		return NULL;
	}
	t_omc->order = item->valueint;

	cJSON_Delete(jdata);
	return t_omc;
}

OSET_STANDARD_SCHED_FUNC(send_massive_static_msg)
{
	om_system_t *system = NULL;
	oset_lnode2_t *lnode = NULL;
	om_session_t *node = NULL;
	om_report_ue_data_t *ue_report = NULL;
	om_report_gnb_data_t *gnb_report = NULL;
	char *ue_context = NULL;
	char *gnb_context = NULL;
    bool ue_fg, gnb_fg = FALSE;
	report_static_data_ue_t ue_static = {0};
	report_static_data_gnb_t gnb_static = {0};

    system = (om_system_t *)task->cmd_arg;
	oset_sys_assert(system);

	for(int i = 0; i < system->node_sq ;i++){
		if(system->node_sess_list[i]->count){
            memset(&ue_static, 0, sizeof(report_static_data_ue_t));
            memset(&gnb_static, 0, sizeof(report_static_data_gnb_t));

			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "massive_order node_id [%d]:%s", i, system->node_id[i]);

			oset_list2_for_each(system->node_sess_list[i], lnode){
				node = (om_session_t *)lnode->data;
				if(NULL == node) continue;
                if(OM_POD_TYPE_UE == node->pod_type){
				    //no need timestep;
				    ue_static.time_step = node->ue_static.time_step;
                    ue_static.rrc_req_num += node->ue_static.rrc_req_num;
                    ue_static.rrc_setup_cpt_num += node->ue_static.rrc_setup_cpt_num;					
                    ue_static.rrc_rej_num += node->ue_static.rrc_rej_num;
                    ue_static.abnormal_rel_num += node->ue_static.abnormal_rel_num;
                    ue_static.average_bler += node->ue_static.average_bler;
                    ue_static.average_latency += node->ue_static.average_latency;
                    ue_fg = TRUE;			    
				}else if(OM_POD_TYPE_GNB == node->pod_type){
					//no need timestep;
				    gnb_static.time_step = node->gnb_static.time_step;
                    gnb_static.rrc_req_num += node->gnb_static.rrc_req_num;
                    gnb_static.rrc_setup_cpt_num += node->gnb_static.rrc_setup_cpt_num;					
                    gnb_static.rrc_rej_num += node->gnb_static.rrc_rej_num;
                    gnb_static.abnormal_rel_num += node->gnb_static.abnormal_rel_num;
                    gnb_static.active_num += node->gnb_static.active_num;
				    gnb_fg = TRUE;
				}				
			}	

			if(ue_fg){
				ue_report = (om_report_ue_data_t *)oset_calloc(1, sizeof(om_report_ue_data_t));
				memcpy(ue_report->header.system_id, system->system_id, strlen(system->system_id) + 1);
			    memset(ue_report->header.pod_id, 0, MAX_ID_SIZE);
			    memset(ue_report->header.container_id, 0, MAX_ID_SIZE);
				memcpy(ue_report->header.node_id, system->node_id[i], strlen(system->node_id[i]) + 1);
				ue_report->msg_type = OM_MSG_UL_MASSIVE_UE;
				ue_report->ue_static = ue_static;
				
				oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "ue report :%s\n", ue_report->header.system_id);
				oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "ue report :%s\n", ue_report->header.pod_id); 
				oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "ue report :%s\n", ue_report->header.container_id);	
				oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "ue report :%s\n", ue_report->header.node_id);

				ue_context = omc_json_ue_static_report_encode(ue_report);
				sendProbeMessage(om_self()->mq_producer, OM_UE_STATIC_REPORT, ue_report->header.system_id, ue_report->header.node_id, ue_context);
				oset_safe_free(ue_context);
				oset_free(ue_report);
				ue_fg = FALSE;
			}
			
			if(gnb_fg){
				gnb_report = (om_report_gnb_data_t *)oset_calloc(1, sizeof(om_report_gnb_data_t));
				memcpy(gnb_report->header.system_id, system->system_id, strlen(system->system_id) + 1);
			    memset(gnb_report->header.pod_id, 0, MAX_ID_SIZE);
			    memset(gnb_report->header.container_id, 0, MAX_ID_SIZE);
				memcpy(gnb_report->header.node_id, system->node_id[i], strlen(system->node_id[i]) + 1);
				gnb_report->msg_type = OM_MSG_UL_MASSIVE_GNB;
				gnb_report->gnb_static = gnb_static;

				oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "gnb report :%s\n", gnb_report->header.system_id);
				oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "gnb report :%s\n", gnb_report->header.pod_id); 
				oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "gnb report :%s\n", gnb_report->header.container_id);	
				oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_DEBUG, "gnb report :%s\n", gnb_report->header.node_id);

				gnb_context = omc_json_gnb_static_report_encode(gnb_report);
				sendProbeMessage(om_self()->mq_producer, OM_GNB_STATIC_REPORT, gnb_report->header.system_id, gnb_report->header.node_id, gnb_context);
				oset_safe_free(gnb_context); 
				oset_free(gnb_report);
				gnb_fg = FALSE;
			}
		}
	}	
}


OSET_DECLARE(void) massive_order_deal(uint32_t id, oset_sock_t *omc_sock, oset_sockaddr_t omc_from, void *order)
{
	om_transfer_order_massive_t *massive_order = NULL;
	om_system_t *system = NULL;

	massive_order = omc_json_massive_order_decode(order);
	if(NULL == massive_order){
	    oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "thread[%u]:massive_order_deal() error", id); 
		oset_free(massive_order);
        return;	
	}
    oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_INFO, "massive_order decode :%s\n", "receive from OMC");
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_INFO, "massive_order decode :%s\n", massive_order->system_id);	
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_INFO, "massive_order decode :%d\n", massive_order->msg_type);
	oset_log2_printf(OSET_CHANNEL_LOG_CLEAN, OSET_LOG2_INFO, "massive_order decode :%d\n", massive_order->order);

	oset_sys_assert(massive_order->system_id);
	system = (om_system_t *)oset_core_hash_find(om_self()->system_hashtable[id], massive_order->system_id);
	if(NULL== system){
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "thread[%u]:massive_order_deal() find system_hashtable failed", id); 
	}else{
		oset_scheduler_del_task_id(system->pre_rp_task_id);

        if(START == massive_order->order){
	        system->static_task_id =oset_scheduler_add_task(om_self()->time_of_massive_report, send_massive_static_msg, "massive-report", "omc-massive-order", 0, (void *)system, SSHF_NONE);   
		}else if(STOP == massive_order->order){
            oset_scheduler_del_task_id(system->static_task_id);
		}else{
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "thread[%u]:massive_order_deal() unkown order type %d", id, massive_order->order); 
		}
	}
	oset_free(massive_order);
}



OSET_DECLARE(void) ue_static_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, om_report_ue_data_t *ue_static_rpt)
{
    om_report_ue_data_t *ue_data = NULL;
	om_session_t * session = NULL;

	oset_sys_assert(ue_static_rpt);
	ue_data = (om_report_ue_data_t *)ue_static_rpt;

	char *new_key = oset_msprintf("%s-%s-%s-%s",ue_data->header.system_id,ue_data->header.pod_id,ue_data->header.container_id,ue_data->header.node_id);

	session = (om_session_t *)oset_core_hash_find(om_self()->session_hashtable[id], new_key);
	if(session){
		if(OM_POD_TYPE_UE == session->pod_type){
			memset(&session->ue_static, 0, sizeof(report_static_data_ue_t));
			session->ue_static = ue_data->ue_static;
		}
	}

    oset_free(new_key);
}

OSET_DECLARE(void) gnb_static_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, om_report_gnb_data_t *gnb_static_rpt)
{

	om_report_gnb_data_t *gnb_data = NULL;
	om_session_t * session = NULL;

	oset_sys_assert(gnb_static_rpt);
	gnb_data = (om_report_gnb_data_t *)gnb_static_rpt;

	char *new_key = oset_msprintf("%s-%s-%s-%s",gnb_data->header.system_id,gnb_data->header.pod_id,gnb_data->header.container_id,gnb_data->header.node_id);

	session = (om_session_t *)oset_core_hash_find(om_self()->session_hashtable[id], new_key);
	if(session){
		if(OM_POD_TYPE_GNB == session->pod_type){
			memset(&session->gnb_static, 0, sizeof(report_static_data_gnb_t));
			session->gnb_static = gnb_data->gnb_static;
		}
	}

    oset_free(new_key);
}

