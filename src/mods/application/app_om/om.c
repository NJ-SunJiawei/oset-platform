/************************************************************************
 *File name: om.c
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/

#include "oset-core.h"
#include "sset-core.h"
#include "om-public.h"

OSET_MODULE_LOAD_FUNCTION(app_om_load);
OSET_MODULE_SHUTDOWN_FUNCTION(app_om_shutdown);
OSET_MODULE_DEFINITION(libapp_om, app_om_load, app_om_shutdown, NULL);

static oset_apr_memory_pool_t *module_pool = NULL;

static void om_load_core_config(const char *file)
{
	oset_xml_t xml = NULL, cfg = NULL;

	if ((xml = oset_xml_open_cfg(file, &cfg, NULL))) {
		oset_xml_t settings, param;

		if ((settings = oset_xml_child(cfg, "settings"))) {
			for (param = oset_xml_child(settings, "param"); param; param = param->next) {
				const char *var = oset_xml_attr_soft(param, "name");
				const char *val = oset_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "om-udp-addr") && !zstr(val)) {
				   om_self()->om_sock.self_ip = oset_core_strdup(module_pool, val);
				} else if (!strcasecmp(var, "om-udp-port-pod")) {
				   int tmp = atoi(val);
				   if (tmp > 0 && tmp < 65535) {
				       om_self()->om_sock.udp_port_pod = (uint16_t) tmp;
				   } else {
					   oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om-udp-port-pod must be between 0 and 65535");
				   }
				} else if (!strcasecmp(var, "om-udp-port-omc")) {
				   int tmp = atoi(val);
				   if (tmp > 0 && tmp < 65535) {
				       om_self()->om_sock.udp_port_omc = (uint16_t) tmp;
				   } else {
					   oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "om-udp-port-omc must be between 0 and 65535");
				   }
				}else if (!strcasecmp(var, "time-of-pod-hb-check")) {
					oset_time_t tmp = atol(val);

					if (tmp > 0 && tmp < 60) {
						om_self()->time_of_hb_check = (oset_time_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "time-of-pod-hb-check must be between 0 and 60");
					}
				} else if (!strcasecmp(var, "time-of-step")) {
					uint32_t tmp = atoi(val);

					if (tmp > 0 && tmp < 60) {
						om_self()->time_of_step = (uint32_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "time-of-step must be between 0 and 60");
					}
				}else if (!strcasecmp(var, "time-of-pod-num-pre-report")) {
					uint32_t tmp = atoi(val);

					if (tmp > 0 && tmp < 20) {
						om_self()->time_of_pre_report = (uint32_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "time-of-pod-num-pre-report must be between 0 and 60");
					}
				} else if (!strcasecmp(var, "max-step-num")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 20000) {
						om_self()->max_step_num = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "max-step-num must be between 50 and 20000");
					}
				}else if (!strcasecmp(var, "time-of-massive-report")) {
					uint32_t tmp = atoi(val);

					if (tmp > 0 && tmp < 60) {
						om_self()->time_of_massive_report = (uint32_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "time-of-massive-static must be between 0 and 60");
					}
				}else if (!strcasecmp(var, "max-pod-num")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 20000) {
						om_self()->max_pod_num = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "max-pod-num must be between 50 and 20000");
					}
				} else if (!strcasecmp(var, "rocketmq-server-addr") && !zstr(val)) {
				   om_self()->mq_server_ip = oset_core_strdup(module_pool, val);
				} else if (!strcasecmp(var, "rocketmq-server-port") && !zstr(val)) {
				   om_self()->mq_port = oset_core_strdup(module_pool, val);
				} else if (!strcasecmp(var, "rocketmq-group-id") && !zstr(val)) {
				   om_self()->mq_group_id = oset_core_strdup(module_pool, val);
				} else if (!strcasecmp(var, "omc-thread-queue-size")) {
					long tmp = atol(val);

					if (tmp > 0 && tmp < 200000) {
						om_self()->omc_thread_que_size = (uint32_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "omc-thread-queue-size must be between 0 and 200000");
					}
				} else if (!strcasecmp(var, "pod-thread-queue-size")) {
					long tmp = atol(val);

					if (tmp > 0 && tmp < 200000) {
						om_self()->pod_thread_que_size = (uint32_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pod-thread-queue-size must be between 0 and 200000");
					}
				} else if (!strcasecmp(var, "worker-thread-num")) {
					long tmp = atoi(val);

					if (tmp > 0 && tmp < 50) {
						om_self()->worker_thread_num = (uint16_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "worker-thread-num must be between 0 and 50");
					}
				}  else if (!strcasecmp(var, "worker-thread-queue-size")) {
					long tmp = atol(val);

					if (tmp > 0 && tmp < 200000) {
						om_self()->worker_thread_que_size = (uint32_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "worker-thread-queue-size must be between 0 and 200000");
					}
				}
			}
		}

		oset_xml_free(xml);
	}

}

OSET_STANDARD_API(show_om_ring_status)
{
	oset_ring_queue_show(om_self()->listen_queue);
	oset_ring_queue_show(om_self()->omc_queue);
	oset_ring_queue_show(om_self()->pod_queue);

    for(int i = 0; i < om_self()->worker_thread_num; i++){
	    oset_ring_queue_show(om_self()->worker_queue[i]);
	    oset_ring_buf_show(om_self()->worker_buf[i]);		
	}


	stream->write_function(stream, "+OK\n");

	return OSET_STATUS_SUCCESS;
}


OSET_STANDARD_API(show_om_status)
{

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "[ALL][system]:All of system is now %d", session_manager.others_count[0]);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "[ALL][session]:All of session is now %d", session_manager.session_count);

    for(int i = 0;i < om_self()->worker_thread_num ;i ++){
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[Thread[%u]][system]:Number of system is now %d", i, oset_list_count(&om_self()->system_list[i]));
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "[Thread[%u]][session]:Number of session is now %d", i, oset_list_count(&om_self()->session_list[i]));
	}

	stream->write_function(stream, "+OK\n");
	return OSET_STATUS_SUCCESS;
}

#define UUID_SYSTEM_SYNTAX "<thread id> <uuid>"
OSET_STANDARD_API(show_uuid_system_status)
{
	char *mycmd = NULL;
	char *argv[2] = {NULL};
	int argc = 0;

	if (zstr(cmd) || !(mycmd = strdup(cmd))) {
		stream->write_function(stream, "-USAGE: %s\n", UUID_SYSTEM_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	argc = oset_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	if (argc != 2) {
		stream->write_function(stream, "-USAGE: %s\n", UUID_SYSTEM_SYNTAX);
		oset_safe_free(mycmd);
		return OSET_STATUS_SUCCESS;
	}

	om_get_system_by_uuid(atoi(argv[0]),argv[1]);

	stream->write_function(stream, "+OK\n");
	oset_safe_free(mycmd);
	return OSET_STATUS_SUCCESS;
}


OSET_MODULE_LOAD_FUNCTION(app_om_load)
{
	//oset_application_interface_t *app_interface;
	oset_api_interface_t *api_interface;
	char guess_ip4[256];
	int mask = 0;

	module_pool = pool;

	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

	om_resource_default_config();

	om_load_core_config("om.conf");

	if(NULL == om_self()->om_sock.self_ip){
		oset_find_local_ip(guess_ip4, sizeof(guess_ip4), &mask, AF_INET);
		om_self()->om_sock.self_ip = oset_core_strdup(pool, guess_ip4);
	}

	if (om_sess_resource_init() != OSET_OK) {
		return OSET_STATUS_GENERR;
	}

    om_sockaddr4_add();
    om_udp_server_open();
	om_add_thread_task(module_pool);

	OSET_ADD_API(api_interface, "om_ring", "Show om ring queue status", show_om_ring_status, "");
	OSET_ADD_API(api_interface, "om", "Show om status", show_om_status, "");
	OSET_ADD_API(api_interface, "uuid_system", "Show uuid system status", show_uuid_system_status, UUID_SYSTEM_SYNTAX);

	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(app_om_shutdown)
{

	//stop ring
	om_sess_termination();

	om_sess_resource_destory();

	return OSET_STATUS_SUCCESS;
}


