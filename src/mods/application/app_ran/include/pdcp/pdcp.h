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

#include "lib/pdcp/pdcp_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    oset_lnode_t     lnode;
	oset_apr_memory_pool_t	 *usepool;
	uint16_t	rnti;
	pdcp_t      pdcp;
}pdcp_user_interface;

typedef struct pdcp_manager_s{
	oset_apr_memory_pool_t   *app_pool;
	oset_apr_mutex_t         *mutex;
	oset_apr_thread_cond_t   *cond;

	oset_hash_t            *users;//std::map<uint32_t, user_interface> users
	oset_list_t 		   pdcp_ue_list;
}pdcp_manager_t;

pdcp_manager_t *pdcp_manager_self(void);
int pdcp_init(void);
int pdcp_destory(void);
void pdcp_user_interface_set_rnti(uint16_t rnti, pdcp_user_interface *user);
pdcp_user_interface *pdcp_user_interface_find_by_rnti(uint16_t rnti);
///////////////////////////////////////////////////////////////////////////////////////
void API_pdcp_rrc_add_user(uint16_t rnti);
void API_pdcp_rrc_rem_user(uint16_t rnti);

#ifdef __cplusplus
}
#endif

#endif
