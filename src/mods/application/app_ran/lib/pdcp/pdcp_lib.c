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

void pdcp_lib_init(pdcp_lib_t *pdcp)
{
	pdcp->pdcp_array = oset_hash_make();
	pdcp->pdcp_array_mrb = oset_hash_make();
	oset_apr_mutex_init(&pdcp->cache_mutex, pdcp->usepool);
	oset_stl_array_init(pdcp->valid_lcids_cached);
	pdcp->metrics_tp = 0;
}


void pdcp_lib_stop(pdcp_lib_t *pdcp)
{
	oset_hash_destroy(pdcp->pdcp_array);
	oset_hash_destroy(pdcp->pdcp_array_mrb);
	oset_apr_mutex_destroy(pdcp->cache_mutex);
	oset_stl_array_term(pdcp->valid_lcids_cached);
}

