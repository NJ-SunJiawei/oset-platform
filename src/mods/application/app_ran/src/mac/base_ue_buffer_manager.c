/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/base_ue_buffer_manager.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-baseUe"


void base_ue_buffer_manager_init(base_ue_buffer_manager *base_ue, uint16_t rnti_)
{
	base_ue->rnti = rnti_;
	memset(base_ue->lcg_bsr, 0, MAX_NOF_LCGS*sizeof(int));
}


