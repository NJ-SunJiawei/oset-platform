/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/

#ifndef PDCP_H_
#define PDCP_H_

#include "lib/pdcp/pdcp.h"
#include "lib/pdcp/pdcp_interface_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint16_t	rnti;
	pdcp_t      pdcp;
}pdcp_user_interface;

typedef struct pdcp_manager_s{
	oset_apr_memory_pool_t   *app_pool;
	oset_apr_mutex_t         *mutex;
	oset_apr_thread_cond_t   *cond;

	OSET_POOL(ue_pool, pdcp_user_interface); //rnti user context
	oset_hash_t            *users;//std::map<uint32_t, user_interface> users
	oset_list_t 		   pdcp_ue_list;
}pdcp_manager_t;

pdcp_manager_t *pdcp_manager_self(void);
int pdcp_init(void);
int pdcp_destory(void);


#ifdef __cplusplus
}
#endif

#endif
