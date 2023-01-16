/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#ifndef GNB_TASK_INTERFACE_H_
#define GNB_TASK_INTERFACE_H_

/*3gPP message def*/
#include "def/phy_messages_types.h"
#include "def/mac_messages_types.h"
//#include "def/rlc_messages_types.h"
//#include "def/pdcp_messages_types.h"
//#include "def/rrc_messages_types.h"
//#include "def/ngap_messages_types.h"
//#include "def/nas_messages_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************/
typedef enum task_priorities_e {
    TASK_PRIORITY_MAX       = 100,
    TASK_PRIORITY_MAX_LEAST = 85,
    TASK_PRIORITY_MED_PLUS  = 70,
    TASK_PRIORITY_MED       = 55,
    TASK_PRIORITY_MED_LEAST = 40,
    TASK_PRIORITY_MIN_PLUS  = 25,
    TASK_PRIORITY_MIN       = 10,
} task_priorities_t;

typedef struct {
    task_priorities_t priority;
    unsigned int queue_size;
    char name[256];
    oset_threadplus_start_t func;
} task_info_t;

#define FOREACH_TASK(TASK_DEF) \
  TASK_DEF(TASK_UNKNOWN,  TASK_PRIORITY_MED,       10,  NULL)  \
  TASK_DEF(TASK_TIMER,    TASK_PRIORITY_MED_LEAST, 200, NULL)  \
  TASK_DEF(TASK_PRACH,	  TASK_PRIORITY_MED,       200, NULL)  \
  TASK_DEF(TASK_TXRX,     TASK_PRIORITY_MIN_PLUS,  200, NULL)  \
  TASK_DEF(TASK_MAC, 	  TASK_PRIORITY_MED,       200, NULL)  \
  TASK_DEF(TASK_RRC,      TASK_PRIORITY_MED,       200, NULL)  \
  TASK_DEF(TASK_NGAP,     TASK_PRIORITY_MED,       200, NULL)  \
  TASK_DEF(TASK_SCTP,     TASK_PRIORITY_MED,       200, NULL)  \
  TASK_DEF(TASK_MAX,      TASK_PRIORITY_MED,       200, NULL)

#define TASK_DEF(TaskID, pRIO, qUEUEsIZE, FuNc)          { pRIO, qUEUEsIZE, #TaskID, FuNc},
static const task_info_t tasks_info[] = {
  FOREACH_TASK(TASK_DEF)
};


#define TASK_ENUM(TaskID, pRIO, qUEUEsIZE, FuNc,) TaskID,
typedef enum {
  FOREACH_TASK(TASK_ENUM)
} task_id_t;

typedef struct task_map_s {
  task_info_t info;
  int         task_id;
  oset_threadplus_t   *thread;
  oset_ring_queue_t   *msg_queue;
} task_map_t;

/*************************************************************/

typedef struct msg_empty_s {
} msg_empty_t;

typedef struct {
    long  timer_id;
    void *arg;
} timer_expired_t;

typedef struct phy_time_s {
    uint32_t frame;
    uint8_t  slot;
} phy_time_t;


typedef enum message_priorities_e {
    MESSAGE_PRIORITY_MAX       = 100,
    MESSAGE_PRIORITY_MAX_LEAST = 85,
    MESSAGE_PRIORITY_MED_PLUS  = 70,
    MESSAGE_PRIORITY_MED       = 55,
    MESSAGE_PRIORITY_MED_LEAST = 40,
    MESSAGE_PRIORITY_MIN_PLUS  = 25,
    MESSAGE_PRIORITY_MIN       = 10,
} message_priorities_t;


#define FOREACH_MSG(INTERNAL_MSG)         \
  INTERNAL_MSG(TIMER_HAS_EXPIRED,  MESSAGE_PRIORITY_MED, timer_expired_t, timer_expired_message) \
  INTERNAL_MSG(MESSAGE_TEST,       MESSAGE_PRIORITY_MED, msg_empty_t, message_test)

typedef enum {
#define MESSAGE_DEF(iD, pRIO, sTRUCT, fIELDnAME) iD,
  FOREACH_MSG(MESSAGE_DEF)
#include "all_msg_def.h"
#undef MESSAGE_DEF
  MESSAGES_ID_MAX,
} msg_ids_t;

typedef union msg_s {
#define MESSAGE_DEF(iD, pRIO, sTRUCT, fIELDnAME) sTRUCT fIELDnAME;
  FOREACH_MSG(MESSAGE_DEF)
#include "all_msg_def.h"
#undef MESSAGE_DEF
} msg_body_t;

typedef struct msg_header_s {
    msg_ids_t  message_id;           /**< Unique message id as referenced in enum MessagesIds */
    task_id_t  ori_task_id;         /**< ID of the sender task */
    task_id_t  det_task_id;         /**< ID of the destination task */
    phy_time_t phy_time;            /**phy slot time*/
    uint32_t   size;            /**< Message size (not including header size) */
} msg_header_t;

typedef struct message_info_s {
    int id;
    message_priorities_t priority;
    uint32_t   size;
    const char name[256];
} message_info_t;

static const message_info_t messages_info[] = {
#define MESSAGE_DEF(iD, pRIO, sTRUCT, fIELDnAME) { iD, pRIO, sizeof(sTRUCT), #iD },
  FOREACH_MSG(MESSAGE_DEF)
#include "all_msg_def.h"
#undef MESSAGE_DEF
};

typedef struct msg_def_s {
    msg_header_t  msg_header; /*< Message header */
    msg_body_t    msg_body;
} msg_def_t;

int task_queue_init(const task_info_t *tasks);
int task_queue_end(const task_info_t *tasks);
void task_queue_termination();
msg_def_t *task_alloc_msg(task_id_t origin_task_id, msg_ids_t message_id);
int task_send_msg(task_id_t destination_task_id, msg_def_t *message);
int task_thread_create(task_id_t task_id, void *data);

#ifdef __cplusplus
}
#endif

#endif
