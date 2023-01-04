/************************************************************************
 *File name: gnb.c
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#include "gnb_common.h"
#include "gnb_manager.h"
#include "radio.h"
#include "phy.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "gnb"

OSET_MODULE_LOAD_FUNCTION(app_gnb_load);
OSET_MODULE_SHUTDOWN_FUNCTION(app_gnb_shutdown);
OSET_MODULE_DEFINITION(libapp_gnb, app_gnb_load, app_gnb_shutdown, NULL);

static oset_apr_memory_pool_t *module_pool = NULL;

OSET_MODULE_LOAD_FUNCTION(app_gnb_load)
{
	oset_api_interface_t *api_interface;

	module_pool = pool;
	gnb_manager_self()->app_pool = module_pool;

	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

    /*todo CPU_SET*/

	if (mlockall((uint32_t)MCL_CURRENT | (uint32_t)MCL_FUTURE) == -1) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Failed to `mlockall`: {}", errno);
	}

	gnb_manager_init();
	rf_manager_init();
	phy_manager_init();


	task_queue_init(tasks_info);
	create_gnb_layer_tasks();
	radio_init();
	phy_init();



	//OSET_ADD_API(api_interface, "gnb_trace", "Show gnb layer trace", show_gnb_layer_trace, "");
	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(app_gnb_shutdown)
{
	gnb_manager_self()->running = false;
	task_queue_termination();//stop queue
	radio_destory();
	phy_destory();
	destory_gnb_layer_tasks();
	task_queue_end(tasks_info);


	phy_manager_destory();
	rf_manager_destory();
	gnb_manager_destory();
	return OSET_STATUS_SUCCESS;
}


