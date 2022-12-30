/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef TXRX_H_
#define TXRX_H_

#ifdef __cplusplus
extern "C" {
#endif

void txrx_init(void);
void txrx_task_init(void);
void *gnb_txrx_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
