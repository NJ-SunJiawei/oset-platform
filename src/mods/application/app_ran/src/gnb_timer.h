/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef GNB_TIMER_H_
#define GNB_TIMER_H_

#include "lib/common/time.h"

#ifdef __cplusplus
extern "C" {
#endif

void *gnb_timer_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
