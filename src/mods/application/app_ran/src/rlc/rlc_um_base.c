/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/
#include "rlc/rlc_um_base.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlcUmBase"

void rlc_um_base_init(rlc_um_base *base, uint32_t lcid_, uint16_t rnti_)
{
	rlc_common_init(&base->common, NULL, rnti_, lcid_, (rlc_mode_t)um, NULL);
	base->rx_enabled = false;
	base->tx_enabled = false;
	oset_thread_mutex_init(&base->metrics_mutex);
	base->metrics = {0};
}

void rlc_um_base_stop(rlc_um_base *base)
{
	rlc_common_destory(&base->common);
	base->rx_enabled = false;
	base->tx_enabled = false;
	oset_thread_mutex_destroy(&base->metrics_mutex);
	base->metrics = {0};
}

