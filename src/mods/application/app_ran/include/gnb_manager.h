/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#ifndef GNB_LAYER_TASK_H_
#define GNB_LAYER_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif
#define SRSENB_MAX_UES   64

void gnb_manager_init(void);
void gnb_manager_destory(void);
int create_gnb_layer_tasks(void);
void destory_gnb_layer_tasks(void);

#ifdef __cplusplus
}
#endif

#endif

