/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "channel_2c.h"
#include "txrx.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "txrx"


void txrx_init(void)
{
	phy_common* worker_com = &phy_manager_self()->workers_common;

  // Instantiate UL channel emulator
  if (worker_com->params.ul_channel_args.enable) {
    gnb_manager_self()->ul_channel = channel_create(worker_com->params.ul_channel_args));
  }

  if (OSET_ERROR == task_thread_create(TASK_RXTX, NULL)) {
	oset_error("Create task for gNB rxtx failed");
	return OSET_ERROR;
  }


  return OSET_OK;
}

