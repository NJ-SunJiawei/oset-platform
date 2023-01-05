/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef RRC_H_
#define RRC_H_

#include "oset-core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rrc_manager_s{
	oset_apr_memory_pool_t *app_pool;
}rrc_manager_t;

void *gnb_rrc_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
