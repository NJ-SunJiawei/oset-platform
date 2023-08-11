/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "lib/pdcp/pdcp_entity_base.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-libpdcpBase"

void pdcp_entity_base_init(pdcp_entity_base *base, uint32_t         lcid_, uint16_t rnti_, oset_apr_memory_pool_t *usepool)
{
	base->rnti                 = rnti_;
	base->lcid                 = lcid_;
	base->integrity_direction  = DIRECTION_NONE;
	base->encryption_direction = DIRECTION_NONE;
}

