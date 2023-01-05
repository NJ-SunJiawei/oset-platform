/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#ifndef GNB_LAYER_TASK_H_
#define GNB_LAYER_TASK_H_

#include "oset-core.h"
#include "band_helper_2c.h"
#include "channel_2c.h"
#include "gnb_config_parser.h"
#include "phy_nr_config.h"
#include "rrc_nr_config.h"

#ifdef __cplusplus
extern "C" {
#endif
#define SRSENB_MAX_UES   64

typedef struct gnb_manager_s{
	bool                   running;
	oset_apr_memory_pool_t *app_pool;
	oset_timer_mgr_t       *app_timer;
	band_helper_t          *band_helper;
    all_args_t             args;
	phy_cfg_t	           phy_cfg;
	rrc_cfg_t	           rrc_cfg;
	rrc_nr_cfg_t           rrc_nr_cfg;

	/*UL/DL channel emulator*/
	channel_t              *dl_channel;
	channel_t              *ul_channel;
}gnb_manager_t;

void gnb_manager_init(void);
void gnb_manager_destory(void);
void gnb_layer_tasks_args_init(void);
int gnb_layer_tasks_create(void);
void gnb_layer_tasks_destory(void);

#ifdef __cplusplus
}
#endif

#endif

