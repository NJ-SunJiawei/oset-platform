/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef MAC_H_
#define MAC_H_

#include "oset-core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mac_manager_s{
	oset_apr_memory_pool_t *app_pool;
}mac_manager_t;

void *gnb_mac_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
