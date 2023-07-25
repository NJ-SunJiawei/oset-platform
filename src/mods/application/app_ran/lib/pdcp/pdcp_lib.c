/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "lib/pdcp/pdcp_lib.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-libpdcp"

void pdcp_lib_init(pdcp_t *pdcp)
{
	oset_apr_mutex_init(&pdcp->cache_mutex, pdcp->usepool);
}


void pdcp_lib_stop(pdcp_t *pdcp);
{

}

