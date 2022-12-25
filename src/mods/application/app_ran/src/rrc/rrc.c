/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "rrc.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "rrc"


typedef struct rrc_manager_s{
	oset_apr_memory_pool_t *app_pool;
}rrc_manager_t;


static rrc_manager_t rrc_manager = {0};

rrc_manager_t *rrc_manager_self(void)
{
	return &rrc_manager;
}

static void rrc_manager_init(void)
{
	rrc_manager.app_pool = gnb_manager_self()->app_pool;

}

static void rrc_manager_destory(void)
{
	rrc_manager.app_pool = NULL; /*app_pool release by openset process*/
}

