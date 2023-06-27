/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "gnb_common.h"
#include "pdcp/pdcp.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-pdcp"

static pdcp_manager_t pdcp_manager = {0};

pdcp_manager_t *pdcp_manager_self(void)
{
	return &pdcp_manager;
}

static void pdcp_manager_init(void)
{
	pdcp_manager.app_pool = gnb_manager_self()->app_pool;
	oset_apr_mutex_init(&pdcp_manager.mutex, OSET_MUTEX_NESTED, pdcp_manager.app_pool);
	oset_apr_thread_cond_create(&pdcp_manager.cond, pdcp_manager.app_pool);
}

static void pdcp_manager_destory(void)
{
	oset_apr_mutex_destroy(pdcp_manager.mutex);
	oset_apr_thread_cond_destroy(pdcp_manager.cond);
	pdcp_manager.app_pool = NULL; /*app_pool release by openset process*/
}

int pdcp_init(void)
{
	pdcp_manager_init();
	oset_pool_init(&pdcp_manager.ue_pool, SRSENB_MAX_UES);
	oset_list_init(&pdcp_manager.pdcp_ue_list);
	pdcp_manager.users = oset_hash_make();
	return OSET_OK;
}

int pdcp_destory(void)
{
	oset_list_empty(&pdcp_manager.pdcp_ue_list);
	oset_hash_destroy(pdcp_manager.users);
	oset_pool_final(&pdcp_manager.ue_pool);
	pdcp_manager_destory();
	return OSET_OK;
}

