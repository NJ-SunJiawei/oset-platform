/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/

#include "sset-core.h"

OSET_MODULE_LOAD_FUNCTION(mod_app_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_app_shutdown);
OSET_MODULE_DEFINITION(libmod_app, mod_app_load, mod_app_shutdown, NULL);

#define LOG_LONG_DESC "Logs a variable for the application."

OSET_STANDARD_APP(log_function)
{
	char *level, *log_str;

	if (data && (level = strdup(data))) {
		oset_log2_level_t ltype = OSET_LOG2_DEBUG;

		if ((log_str = strchr(level, ' '))) {
			*log_str++ = '\0';
			ltype = oset_log2_str2level(level);
		} else {
			log_str = level;
			ltype = oset_log2_str2level(level);
		}
		if (ltype == OSET_LOG2_INVALID) {
			ltype = OSET_LOG2_DEBUG;
		}

		sset_core_session_ctl(SCSC_LOGLEVEL, &ltype);
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), ltype, "%s", log_str);
		oset_safe_free(level);
	}
}

OSET_STANDARD_APP(system_session_function)
{
	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Executing command: %s", data);
	if (oset_system(data, OSET_TRUE) < 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Failed to execute command: %s", data);
	}
}

OSET_STANDARD_APP(bgsystem_session_function)
{
	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Executing command: %s", data);
	if (oset_system(data, OSET_FALSE) < 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Failed to execute command: %s", data);
	}
}


OSET_MODULE_LOAD_FUNCTION(mod_app_load)
{
	oset_application_interface_t *app_interface;
	*module_interface = oset_loadable_module_create_module_interface(pool, modname);
	OSET_ADD_APP(app_interface, "log", "Logs to the logger", LOG_LONG_DESC, log_function, "<log_level> <log_string>", SAF_ZOMBIE_EXEC);
	OSET_ADD_APP(app_interface, "system", "Execute a system command", "Execute a system command", system_session_function, "<command>", SAF_ZOMBIE_EXEC);
	OSET_ADD_APP(app_interface, "bgsystem", "Execute a system command in the background", "Execute a background system command", bgsystem_session_function, "<command>", SAF_ZOMBIE_EXEC);
	return OSET_STATUS_NOUNLOAD;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_app_shutdown)
{
	return OSET_STATUS_SUCCESS;
}



