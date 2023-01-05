/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-mac"

	
typedef struct mac_manager_s{
	oset_apr_memory_pool_t *app_pool;
}mac_manager_t;


static mac_manager_t mac_manager = {0};

rrc_manager_t *mac_manager_self(void)
{
	return &mac_manager;
}

static void mac_manager_init(void)
{
	mac_manager.app_pool = gnb_manager_self()->app_pool;

}

static void mac_manager_destory(void)
{
	mac_manager.app_pool = NULL; /*app_pool release by openset process*/
}

