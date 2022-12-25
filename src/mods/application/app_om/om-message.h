/************************************************************************
 *File name: om-message.h
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/
#ifndef OM_MESSAGE_H
#define OM_MESSAGE_H

#define  MAX_SUCI_SIZE  40
#define  MAX_IP_SIZE    40
#define  MAX_ID_SIZE    128
#define  MAX_OTHER_SIZE 128
#define  MAX_DIR_SIZE   512
#define  MAX_CAUSE_LEN  60
#define  OM_MAX_SDU_LEN  8192

OSET_BEGIN_EXTERN_C

/******************common message struct*********************/
typedef enum {
    OM_POD_TYPE_NULL = 0,
	OM_POD_TYPE_UE,
	OM_POD_TYPE_WX,
    OM_POD_TYPE_GNB,
	OM_POD_TYPE_CN,
}om_pod_type_e;

typedef struct om_msg_header{
   char  system_id[MAX_ID_SIZE];
   char  pod_id[MAX_ID_SIZE];
   char  container_id[MAX_ID_SIZE];
   char  node_id[MAX_ID_SIZE];
}om_msg_header_t;


/******************inner message struct*********************/

typedef enum {
	OM_WORKER_E_HANDLE = 1,
	OM_WORKER_E_TIMER,
} om_worker_event_type_e;
	
typedef enum {
	OM_WORKER_TIMER_NULL = 0,
	OM_WORKER_TIMER_HB_LOST,
	OM_WORKER_TIMER_MAX,
} om_worker_timer_type_e;


typedef struct om_session om_session_t;

typedef struct om_inner_worker_message{
	om_worker_event_type_e  e_type;
    int                     m_type;
	uint32_t                thread_id;
	oset_sock_t             *sock;

    union{
	    struct{
			oset_sockaddr_t 	  from; //peer addr
			oset_pkbuf_t		  *pkbuf;
		}udp;
		om_session_t              *sess;                  
	}u;
}om_inner_worker_message_t;



/******************pod message struct*********************/
/******************omc message struct*********************/
typedef enum {
    OM_MSG_NULL = 0,

	OM_MSG_HRAETBEAT = 01,
    OM_MSG_TIMESTEP  = 02,

	OM_MSG_DL_COMMAND_CP =11,
	OM_MSG_DL_COMMAND_UP = 12,
	OM_MSG_DL_COMMAND_MASSIVE =13,

	OM_MSG_UL_REPORT_CP = 21,
	OM_MSG_UL_REPORT_UP = 22,

	OM_MSG_UL_MASSIVE_UE = 31,
	OM_MSG_UL_MASSIVE_GNB = 32,

	OM_MSG_MAX,
} om_msg_type_e;


/********(omc-->om) down direction**********/
/********(om-->pod) down direction*********/

typedef enum {
     AUTOSETUP=1,
     REGISTER,
     PDUSETUP,
	 DEREDISTER,
	 PDURELEASE,
	 DESTORY = 101,
} c_order_type_e;

typedef struct {
    char   suci[MAX_SUCI_SIZE];       //null表示所有suci
    char   others[MAX_OTHER_SIZE];	   //其他参数，null表示没有
    int    order;
}transfer_c_order_t;

typedef struct om_transfer_order_c{
	om_msg_header_t      header;
    int                  msg_type;
    transfer_c_order_t   c_order;
}om_transfer_order_c_t;

typedef enum {
	DO_PING = 1,
	DO_FILE,
	DO_VEDIO,
}u_order_type_t;

typedef struct {
    char   suci[MAX_SUCI_SIZE];      //null表示所有suci
    char   target_ip[MAX_IP_SIZE];   
    char   others[MAX_OTHER_SIZE];	 //其他参数，null表示没有
    char   src_dir_path[MAX_DIR_SIZE];
	char   target_dir_path[MAX_DIR_SIZE];
    int    order;
}transfer_u_order_t;

typedef struct om_transfer_order_u{
	om_msg_header_t      header;
    int                  msg_type;
    transfer_u_order_t   u_order;
}om_transfer_order_u_t;


typedef enum {
	INIT = 0,
	START,
	STOP,
}massive_order_type_t;

typedef struct om_transfer_order_massive{
	char  system_id[MAX_ID_SIZE];
    int   msg_type;
    int   order;
}om_transfer_order_massive_t;


/********(pod-->om) up direction**********/
/********(om-->omc) up direction**********/
typedef struct report_c_data{
   char    suci[MAX_SUCI_SIZE];
   char    ip[MAX_IP_SIZE];
}report_c_data_t;
typedef struct om_report_data_c{
	om_msg_header_t   header;
    int               msg_type;
	bool              success_flag;    //0: Failure 1:Success
    char              failure_cause[MAX_CAUSE_LEN];
	report_c_data_t   ue_cp_data;
}om_report_data_c_t;

typedef struct report_u_data{
    char   suci[MAX_SUCI_SIZE];
    int    type;           //(PING = 1 /VEDIO = 2````)
    bool   success_flag;  //0: Failure 1:Success
    int    result_size;
    char   result[OM_MAX_SDU_LEN];
}report_u_data_t;

#define MAX_UE_NO 10
typedef struct om_report_data_u{
	om_msg_header_t   header;
    int               msg_type;
    int               noOfUe;
    report_u_data_t   ue_up_data[MAX_UE_NO];
}om_report_data_u_t;

typedef struct report_static_data_ue{
	int  time_step;
	int  rrc_req_num;
	int  rrc_setup_cpt_num;
	int  rrc_rej_num;
	int  abnormal_rel_num;
	int  average_bler;
	int  average_latency;
}report_static_data_ue_t;

typedef struct report_static_data_gnb{
	int  time_step;
	int  rrc_req_num;
	int  rrc_setup_cpt_num;
	int  rrc_rej_num;
	int  abnormal_rel_num;
    int  active_num;
}report_static_data_gnb_t;
typedef struct om_report_ue_data{
	om_msg_header_t           header;
    int                       msg_type;
    report_static_data_ue_t   ue_static;
}om_report_ue_data_t;

typedef struct om_report_gnb_data{
	om_msg_header_t           header;
    int                       msg_type;
    report_static_data_gnb_t  gnb_static;
}om_report_gnb_data_t;


/********(pod-->om) up heartbeat**********/
typedef struct pod_heartbeat_data{
	om_msg_header_t   header;
    int               msg_type;
    int               hb_type;
}pod_heartbeat_data_t;

/********(om-->omc) up pre-report**********/
typedef struct pod_pre_report{
	om_msg_header_t   header;
    int               ue_pod_num;
    int               gnb_pod_num;
    int               cn_pod_num; 
}pod_pre_report_t;

/********(om-->pod) down timestep**********/
typedef struct pod_timestep_data{
	om_msg_header_t   header;
    int               msg_type;
	int               step_num;
}pod_timestep_data_t;

OSET_END_EXTERN_C

#endif


