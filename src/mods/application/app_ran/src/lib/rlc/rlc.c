/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "lib/rlc/rlc.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librlc"

static void rlc_lib_init(rlc_t *rlc, uint32_t lcid_)
{
	rlc->default_lcid = lcid_;

	reset_metrics();

	// create default RLC_TM bearer for SRB0
	add_bearer(rlc->default_lcid, rlc_config_t());
}


void rlc_init(rlc_t *rlc, uint32_t lcid_, bsr_callback_t bsr_callback_)
{
	rlc->bsr_callback = bsr_callback_;
	rlc_lib_init(rlc, lcid_);
}

