/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#include "gnb_common.h"

#include "gnb_timer.h"
#include "rf/radio.h"
#include "phy/phy.h"
#include "phy/prach_work.h"
#include "phy/txrx.h"
#include "mac/mac.h"
#include "rrc/rrc.h"


#define NUM_OF_APP_TIMER      2
static gnb_manager_t gnb_manager = {0};

gnb_manager_t *gnb_manager_self(void)
{
    return &gnb_manager;
}

void gnb_manager_init(void)
{
	gnb_manager.running = true;
	gnb_manager.app_timer = oset_timer_mgr_create(SRSENB_MAX_UES * NUM_OF_APP_TIMER);
	oset_assert(gnb_manager.app_timer);
	gnb_manager.band_helper = band_helper_create();
	gnb_arg_default(&gnb_manager.args);
	gnb_arg_second(&gnb_manager.args);
	parse_cfg_files(&gnb_manager.args, &gnb_manager.rrc_cfg, &gnb_manager.rrc_nr_cfg, &gnb_manager.phy_cfg);
}

void gnb_manager_destory(void)
{

	gnb_manager.running = false;

	oset_lnode2_t *lnode = NULL;
	oset_list2_for_each(gnb_manager.rrc_nr_cfg.cell_list, lnode){
	    rrc_cell_cfg_nr_t * cell = (rrc_cell_cfg_nr_t *)lnode->data;
		DYN_ARRAY_CLEAR(&cell->pdcch_cfg_common.common_search_space_list);
		DYN_ARRAY_CLEAR(&cell->pdcch_cfg_ded.ctrl_res_set_to_add_mod_list);
		DYN_ARRAY_CLEAR(&cell->pdcch_cfg_ded.search_spaces_to_add_mod_list);
	}

	band_helper_destory(gnb_manager.band_helper);//???

	oset_list2_free(gnb_manager.rrc_nr_cfg.cell_list);
	oset_list2_free(gnb_manager.rrc_nr_cfg.five_qi_cfg);
	oset_list2_free(gnb_manager.phy_cfg->phy_cell_cfg_nr);


	oset_timer_mgr_destroy(gnb_manager.app_timer);

	if (gnb_manager.args.phy.dl_channel_args.enable) {
		channel_destory(gnb_manager.dl_channel);
	}

	if (gnb_manager.args.phy.ul_channel_args.enable) {
		channel_destory(gnb_manager.ul_channel);
	}

	gnb_manager.app_pool = NULL; /*app_pool release by openset process*/

}

void gnb_layer_tasks_args_init(void)
{
	task_map_self(TASK_TIMER)->info.func = gnb_timer_task;
	task_map_self(TASK_PRACH)->info.func = gnb_prach_task;
	task_map_self(TASK_TXRX)->info.func = gnb_txrx_task;
	task_map_self(TASK_RRC)->info.func = gnb_rrc_task;
	task_map_self(TASK_MAC)->info.func = gnb_mac_task;
}


int gnb_layer_tasks_create(void)
{
	/*todo bind CPU*/

	if (OSET_ERROR == task_thread_create(TASK_TIMER, NULL)) {
	  oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Create task for gNB Timer failed");
	  return OSET_ERROR;
	}

	//rrc_init must start before phy, rrc_config_phy()
	//rrc_init must start before mac, rrc_config_mac()
	if (OSET_ERROR == task_thread_create(TASK_RRC, NULL)) {
	  oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Create task for gNB RRC failed");
	  return OSET_ERROR;
	}

	if (OSET_ERROR == task_thread_create(TASK_MAC, NULL)) {
	  oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Create task for gNB MAC failed");
	  return OSET_ERROR;
	}

	rf_init();
	phy_init();
    return OSET_OK;
}

void gnb_layer_tasks_destory(void)
{
	phy_destory();
	rf_destory();
	oset_threadplus_destroy(task_map_self(TASK_MAC)->thread);
	oset_threadplus_destroy(task_map_self(TASK_RRC)->thread);
	oset_threadplus_destroy(task_map_self(TASK_TIMER)->thread);

}

