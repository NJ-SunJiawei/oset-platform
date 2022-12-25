#include "oset-core.h"
#include "om-message.h"

static oset_thread_t *thread;

static void thread_func1(void *data)
{
    char str[1024] = {0};
    oset_sockaddr_t sa;
    om_transfer_order_c_t *corder = NULL;
    om_transfer_order_u_t *uorder = NULL;
    pod_timestep_data_t   *step = NULL;
    char buf[OSET_ADDRSTRLEN] = {0};
    oset_sock_t *udp = (oset_sock_t *)data;

    int type =0;

    oset_info_tt("%s", " thread_func receive running");
    while (oset_recvfrom(udp->fd, str, 2048, 0, &sa)) {
         oset_info_tt("==>%s%s:%d", "receive from OM ",OSET_ADDR(&sa, buf), OSET_PORT(&sa));

         type = *(int *)(str + sizeof(om_msg_header_t));
	      
         if(type == OM_MSG_DL_COMMAND_CP){
	         corder  = (om_transfer_order_c_t *)str;

	         oset_info_tt("receive c==>%s", corder->header.system_id);
	         oset_info_tt("receive c==>%s", corder->header.pod_id);	
	         oset_info_tt("receive c==>%s", corder->header.container_id);	
	         oset_info_tt("receive c==>%s", corder->header.node_id);	
	         oset_info_tt("receive c==>%d", corder->msg_type);
	         oset_info_tt("receive c==>%s", corder->c_order.suci);
	         oset_info_tt("receive c==>%s", corder->c_order.others);
	         oset_info_tt("receive c==>%d", corder->c_order.order);
		 }else if(type == OM_MSG_DL_COMMAND_UP){
	         uorder  = (om_transfer_order_u_t *)str;

	         oset_info_tt("receive u==>%s", uorder->header.system_id);
	         oset_info_tt("receive u==>%s", uorder->header.pod_id);	
	         oset_info_tt("receive u==>%s", uorder->header.container_id);	
	         oset_info_tt("receive u==>%s", uorder->header.node_id);	
	         oset_info_tt("receive u==>%d", uorder->msg_type);
	         oset_info_tt("receive u==>%s", uorder->u_order.suci);
	         oset_info_tt("receive u==>%s", uorder->u_order.others);
	         oset_info_tt("receive u==>%s", uorder->u_order.src_dir_path);
	         oset_info_tt("receive u==>%s", uorder->u_order.target_dir_path);
	         oset_info_tt("receive u==>%d", uorder->u_order.order);
		 }else if(type == OM_MSG_TIMESTEP){
	         step  = (pod_timestep_data_t *)str;

	         oset_info_tt("receive t==>%s", step->header.system_id);
	         oset_info_tt("receive t==>%s", step->header.pod_id);	
	         oset_info_tt("receive t==>%s", step->header.container_id);	
	         oset_info_tt("receive t==>%s", step->header.node_id);	
	         oset_info_tt("receive t==>%d", step->msg_type);
	         oset_info_tt("receive t==>%d", step->step_num);
		 }else{
	         oset_info_tt("receive err typr[%d]", type);
		 }
	     oset_msleep(100);
    }
}

int main(int argc, const char *const argv[])
{
    oset_sock_t *udp1;
    oset_sockaddr_t *addr1;
    oset_socknode_t *node1;
	
    oset_pkbuf_config_t config;

    pod_heartbeat_data_t  hb1 = {};
    pod_heartbeat_data_t  hb2 = {};
    pod_heartbeat_data_t  hb3 = {};
    pod_heartbeat_data_t  hb4 = {};
    pod_heartbeat_data_t  hb5 = {};
    pod_heartbeat_data_t  hb6 = {};

    om_report_ue_data_t   u1 = {};
    om_report_ue_data_t   u2 = {};
    om_report_gnb_data_t  g1 = {};
    om_report_gnb_data_t  g2 = {};

    oset_core_initialize();
    oset_pkbuf_default_init(&config);
    oset_pkbuf_default_create(&config);

    oset_getaddrinfo(&addr1, AF_INET, "10.37.6.28", 20087, 0);  //pod
    node1 = oset_socknode_new(addr1);
    udp1 = oset_udp_client(node1);

    memcpy(hb1.header.system_id, "29b30ba6-3840-49e7-b317-ccd3", MAX_ID_SIZE);
    memcpy(hb1.header.pod_id, "29b30ba6-3840-49e7-b317-ccd3", MAX_ID_SIZE);
    memcpy(hb1.header.container_id, "29b30ba6-3840-49e7-b317-ccd3", MAX_ID_SIZE);
    memcpy(hb1.header.node_id, "29b30ba6-3840-49e7-b317-ccd3", MAX_ID_SIZE);
	hb1.msg_type = OM_MSG_HRAETBEAT;
	hb1.hb_type = OM_POD_TYPE_UE;


    memcpy(hb2.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(hb2.header.pod_id, "39b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(hb2.header.container_id, "39b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(hb2.header.node_id, "39b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
	hb2.msg_type = OM_MSG_HRAETBEAT;
	hb2.hb_type = OM_POD_TYPE_UE;

    memcpy(hb3.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(hb3.header.pod_id, "39b30ba6-3840-49e7-b317-ccd5", MAX_ID_SIZE);
    memcpy(hb3.header.container_id, "39b30ba6-3840-49e7-b317-ccd5", MAX_ID_SIZE);
    memcpy(hb3.header.node_id, "39b30ba6-3840-49e7-b317-ccd5", MAX_ID_SIZE);
	hb3.msg_type = OM_MSG_HRAETBEAT;
	hb3.hb_type = OM_POD_TYPE_GNB;

    memcpy(hb4.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(hb4.header.pod_id, "39b30ba6-3840-49e7-b317-ccd6", MAX_ID_SIZE);
    memcpy(hb4.header.container_id, "39b30ba6-3840-49e7-b317-ccd6", MAX_ID_SIZE);
    memcpy(hb4.header.node_id, "39b30ba6-3840-49e7-b317-ccd6", MAX_ID_SIZE);
	hb4.msg_type = OM_MSG_HRAETBEAT;
	hb4.hb_type = OM_POD_TYPE_UE;

    memcpy(hb5.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(hb5.header.pod_id, "39b30ba6-3840-49e7-b317-ccd7", MAX_ID_SIZE);
    memcpy(hb5.header.container_id, "39b30ba6-3840-49e7-b317-ccd7", MAX_ID_SIZE);
    memcpy(hb5.header.node_id, "39b30ba6-3840-49e7-b317-ccd7", MAX_ID_SIZE);
	hb5.msg_type = OM_MSG_HRAETBEAT;
	hb5.hb_type = OM_POD_TYPE_GNB;

    memcpy(hb6.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(hb6.header.pod_id, "39b30ba6-3840-49e7-b317-ccd8", MAX_ID_SIZE);
    memcpy(hb6.header.container_id, "39b30ba6-3840-49e7-b317-ccd8", MAX_ID_SIZE);
    memcpy(hb6.header.node_id, "39b30ba6-3840-49e7-b317-ccd8", MAX_ID_SIZE);
	hb6.msg_type = OM_MSG_HRAETBEAT;
	hb6.hb_type = OM_POD_TYPE_CN;


    memcpy(u1.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(u1.header.pod_id, "39b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(u1.header.container_id, "39b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(u1.header.node_id, "39b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
	u1.msg_type = OM_MSG_UL_MASSIVE_UE;
	u1.ue_static.time_step = 1;
	u1.ue_static.rrc_req_num = 5;
	u1.ue_static.rrc_setup_cpt_num= 5;
	u1.ue_static.rrc_rej_num = 1;	
	
    memcpy(u2.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(u2.header.pod_id, "39b30ba6-3840-49e7-b317-ccd6", MAX_ID_SIZE);
    memcpy(u2.header.container_id, "39b30ba6-3840-49e7-b317-ccd6", MAX_ID_SIZE);
    memcpy(u2.header.node_id, "39b30ba6-3840-49e7-b317-ccd6", MAX_ID_SIZE);
	u2.msg_type = OM_MSG_UL_MASSIVE_UE;
	u2.ue_static.time_step = 1;
	u2.ue_static.rrc_req_num = 5;
	u2.ue_static.rrc_setup_cpt_num= 5;
	u2.ue_static.rrc_rej_num = 1;

    memcpy(g1.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(g1.header.pod_id, "39b30ba6-3840-49e7-b317-ccd5", MAX_ID_SIZE);
    memcpy(g1.header.container_id, "39b30ba6-3840-49e7-b317-ccd5", MAX_ID_SIZE);
    memcpy(g1.header.node_id, "39b30ba6-3840-49e7-b317-ccd5", MAX_ID_SIZE);
	g1.msg_type = OM_MSG_UL_MASSIVE_GNB;
	g1.gnb_static.time_step = 1;
	g1.gnb_static.rrc_req_num = 5;
	g1.gnb_static.rrc_setup_cpt_num= 5;
	g1.gnb_static.rrc_rej_num = 1;

    memcpy(g2.header.system_id, "29b30ba6-3840-49e7-b317-ccd4", MAX_ID_SIZE);
    memcpy(g2.header.pod_id, "39b30ba6-3840-49e7-b317-ccd7", MAX_ID_SIZE);
    memcpy(g2.header.container_id, "39b30ba6-3840-49e7-b317-ccd7", MAX_ID_SIZE);
    memcpy(g2.header.node_id, "39b30ba6-3840-49e7-b317-ccd7", MAX_ID_SIZE);
	g2.msg_type = OM_MSG_UL_MASSIVE_GNB;
	g2.gnb_static.time_step = 1;
	g2.gnb_static.rrc_req_num = 5;
	g2.gnb_static.rrc_setup_cpt_num= 5;
	g2.gnb_static.rrc_rej_num = 1;

	
    int i = 0;
    thread = oset_thread_create(thread_func1, udp1);
    while(1)
	{
         oset_send(udp1->fd, &hb1, sizeof(pod_heartbeat_data_t), 0);  //up om hb
         oset_msleep(10);
         oset_send(udp1->fd, &hb2, sizeof(pod_heartbeat_data_t), 0);
         oset_send(udp1->fd, &u1, sizeof(om_report_ue_data_t), 0);
         oset_msleep(100);
         oset_send(udp1->fd, &hb3, sizeof(pod_heartbeat_data_t), 0);
         oset_send(udp1->fd, &g1, sizeof(om_report_gnb_data_t), 0);
         oset_msleep(100);
         oset_send(udp1->fd, &hb4, sizeof(pod_heartbeat_data_t), 0);
         oset_send(udp1->fd, &u2, sizeof(om_report_ue_data_t), 0);
         oset_msleep(100);
         oset_send(udp1->fd, &hb5, sizeof(pod_heartbeat_data_t), 0);
         oset_send(udp1->fd, &g2, sizeof(om_report_gnb_data_t), 0);
         oset_msleep(100);
         oset_send(udp1->fd, &hb6, sizeof(pod_heartbeat_data_t), 0);
         oset_msleep(5000);
         oset_info_tt("%d", i++);
		 if(i == 1000) i = 0;
	}
	oset_thread_destroy(thread);

    oset_socknode_free(node1);
    oset_pkbuf_default_destroy();
    oset_core_terminate();
}
