/************************************************************************
 *File name: om-public.h
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/
#ifndef OM_PUBLIC_H
#define OM_PUBLIC_H

#include "oset-core.h"
#include "om-message.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-om"


#define MAX_WORKER_THREAD       200
#define OM_MAX_PKT_LEN          2048


#define OM_PRE_REPORT           "OM-PRE-REPORT"
#define OM_C_REPORT             "OM-C-REPORT"
#define OM_U_REPORT             "OM-U-REPORT"
#define OM_UE_STATIC_REPORT     "OM-UE-STATIC-REPORT"
#define OM_GNB_STATIC_REPORT    "OM-GNB-STATIC-REPORT"


OSET_BEGIN_EXTERN_C

typedef struct omc_address{
    char    *self_ip;
	uint16_t udp_port_omc;
	uint16_t udp_port_pod;
}om_address_t;


#define MAX_NODE 500
typedef struct om_system{
    oset_lnode_t     lnode;
    oset_core_session_t  *common;
	uint32_t        idnex;
    uint32_t        thread_id;
	oset_list2_t    *all_sess_list;
	char            system_id[MAX_ID_SIZE];
	int             step_num;

    /*static report*/
	uint32_t        step_task_id;
	uint32_t        static_task_id;
    uint16_t        node_sq;
	char            *node_id[MAX_NODE];
	oset_list2_t    *node_sess_list[MAX_NODE];

	/*pre pod num report*/
	uint32_t        pre_rp_task_id;
}om_system_t;

typedef struct CProducer CProducer;

typedef struct om_session{
    oset_lnode_t     lnode;
    oset_core_session_t  *common;
	uint32_t    idnex;
	om_system_t *system;
	om_msg_header_t *p;
    int          pod_type;         //UE/WX/GNB/CN
	oset_sock_t     *pod_sock;
    oset_sockaddr_t  pod_from;         //peer addr
	oset_sock_t     *omc_sock;
    oset_sockaddr_t  omc_from;
    uint32_t      thread_id;
	oset_timer_t  *del_timer;
    report_static_data_ue_t   ue_static;
    report_static_data_gnb_t  gnb_static;
}om_session_t;

typedef struct om_public{
	int                  running;
	int                  max_pod_num;    //epoll socket num
    oset_time_t          time_of_hb_check;
    uint32_t             time_of_step;
    int                  max_step_num;
    uint32_t             time_of_massive_report;
    uint32_t             time_of_pre_report;

	uint32_t omc_thread_que_size;
	uint32_t pod_thread_que_size;

	uint16_t worker_thread_num;
	uint32_t worker_thread_que_size;

    oset_ring_queue_t  *listen_queue;
    oset_ring_queue_t  *pod_queue;
    oset_ring_queue_t  *omc_queue;
    oset_ring_queue_t  *worker_queue[MAX_WORKER_THREAD];	
	oset_ring_buf_t    *worker_buf[MAX_WORKER_THREAD];
	oset_hashtable_t   *system_hashtable[MAX_WORKER_THREAD];
	oset_hashtable_t   *system_uuid_hashtable[MAX_WORKER_THREAD];
	oset_list_t         system_list[MAX_WORKER_THREAD];
	oset_hashtable_t   *session_hashtable[MAX_WORKER_THREAD];
	oset_hashtable_t   *session_uuid_hashtable[MAX_WORKER_THREAD];
	oset_list_t         session_list[MAX_WORKER_THREAD];

    OSET_POOL(om_sess_id_pool[MAX_WORKER_THREAD], uint32_t);
    OSET_POOL(om_system_id_pool[MAX_WORKER_THREAD], uint32_t);

    //oset_timer_mgr_t    *omc_timer;
    //oset_timer_mgr_t    *pod_timer;
	oset_timer_mgr_t    *worker_timer[MAX_WORKER_THREAD];

    oset_pollset_t      *pollset;

	om_address_t         om_sock;
    oset_list_t          udp_server_list;
	char      *mq_server_ip;
	char      *mq_port;
    char      *mq_group_id;
	CProducer *mq_producer;
}om_public_t;

/* forward declaration */
typedef enum {
    OM_TIMER_BASE = 0,
    OM_TIMER_HB_TIMEOUT,
    MAX_NUM_OF_OM_TIMER,
} om_timer_e;



typedef enum {
    OM_MSG_BASE = OSET_FSM_USER_SIG,

	OM_LISTEN_MSG,
    OM_OMC_MSG,
	OM_POD_MSG,

    OM_OMC_MSG_UP,
    OM_OMC_MSG_DOWN,
	OM_OMC_MSG_TIMER,

    OM_POD_MSG_UP,
	OM_POD_MSG_DOWN,
	OM_POD_MSG_TIMER,

    OM_MSG_TOP,
} om_event_e;


typedef struct om_event {
    om_event_e id;
    oset_pkbuf_t *pkbuf;
    //int timer_id;
	oset_sockaddr_t from;
	oset_sock_t *sock;
	
    //oset_timer_t *timer;
} om_event_t;

static inline oset_core_session_t *om_system_get_common(om_system_t *system) { return system->common;}
#define OSET_CHANNEL_SYSTEM_LOG(x) OSET_CHANNEL_ID_SESSION, __FILE__, __OSET_FUNC__, __LINE__, (const char*)om_system_get_common(x)


OSET_DECLARE(oset_sock_t *) om_udp_server(oset_socknode_t *node);
OSET_DECLARE(void) om_sockaddr4_add(void);
OSET_DECLARE(int) om_udp_server_open(void);
OSET_DECLARE(void) om_udp_server_close(void);
OSET_DECLARE(int) om_udp_sendto(oset_sock_t *sock, oset_sockaddr_t *dst, void *data, size_t len);

OSET_DECLARE(void) om_resource_default_config(void);
OSET_DECLARE(om_public_t *) om_self();
OSET_DECLARE(int) om_sess_resource_init(void);
OSET_DECLARE(void) om_sess_resource_destory(void);
OSET_DECLARE(om_event_t ) *om_omc_event_new(om_event_e id);
OSET_DECLARE(void) om_omc_event_free(om_event_t *e);
OSET_DECLARE(om_event_t *) om_pod_event_new(om_event_e id);
OSET_DECLARE(void) om_pod_event_free(om_event_t *e);
OSET_DECLARE(om_event_t *) om_listen_event_new(om_event_e id);
OSET_DECLARE(void) om_listen_event_free(om_event_t *e);
OSET_DECLARE(void) om_event_init(void);
OSET_DECLARE(void) om_event_final(void);
OSET_DECLARE(int) om_add_thread_task(oset_apr_memory_pool_t *pool);
OSET_DECLARE(void) om_sess_termination(void);

OSET_DECLARE(uint32_t *) sess_sid_find_by_idnex(uint32_t id ,uint32_t idnex);
OSET_DECLARE(uint32_t *) system_sid_find_by_idnex(uint32_t id ,uint32_t idnex);

OSET_DECLARE(void) om_system_destory_debug( _In_ om_system_t *system, _In_z_ const char *file, _In_z_ const char *func, _In_ int line);
#define om_system_destory(system) om_system_destory_debug(system, __FILE__, __OSET_FUNC__, __LINE__)


OSET_DECLARE(void) om_session_destory_debug(_In_ om_session_t *session, _In_z_ const char *file, _In_z_ const char *func, _In_ int line);
#define om_session_destory(session) om_session_destory_debug(session, __FILE__, __OSET_FUNC__, __LINE__)


OSET_DECLARE(om_system_t *) om_create_system(uint32_t id, char* system_id, oset_apr_memory_pool_t **pool);
OSET_DECLARE(om_session_t *) om_create_session(uint32_t id, oset_sock_t *sock, oset_sockaddr_t from, om_msg_header_t *p, int pod_type, om_system_t * system, oset_apr_memory_pool_t **pool);

OSET_DECLARE(void) om_get_system_by_uuid(uint32_t id, char* uuid);

OSET_DECLARE(void) timer_msg_deal(uint32_t id, om_session_t *session);
OSET_DECLARE(void) hb_msg_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, pod_heartbeat_data_t *hb);
OSET_DECLARE(void) cp_order_deal(uint32_t id, oset_sock_t *omc_sock, oset_sockaddr_t omc_from, void *order);
OSET_DECLARE(void) cp_report_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, om_report_data_c_t *pod_cp_rpt, uint32_t len);
OSET_DECLARE(void) up_order_deal(uint32_t id, oset_sock_t *omc_sock, oset_sockaddr_t omc_from, void *order);
OSET_DECLARE(void) up_report_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, om_report_data_u_t *pod_up_rpt, uint32_t len);

OSET_DECLARE(void) ue_static_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, om_report_ue_data_t *ue_static_rpt);
OSET_DECLARE(void) gnb_static_deal(uint32_t id, oset_sock_t *pod_sock, oset_sockaddr_t pod_from, om_report_gnb_data_t *gnb_static_rpt);

OSET_DECLARE(void) massive_order_deal(uint32_t id, oset_sock_t *omc_sock, oset_sockaddr_t omc_from, void *order);

OSET_END_EXTERN_C

#endif


