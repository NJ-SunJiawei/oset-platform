/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#include "oset-core.h"

OSET_MODULE_LOAD_FUNCTION(mod_@template@_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_@template@_shutdown);
OSET_MODULE_RUNTIME_FUNCTION(mod_@template@_runtime);
OSET_MODULE_DEFINITION(libapp_@template@, mod_@template@_load, mod_@template@_shutdown, mod_@template@_runtime);

static struct {
	int shutdown;
} globals;


OSET_STANDARD_API(hello_function)
{
	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", "hello <strings>");
		return OSET_STATUS_SUCCESS;
	}

	stream->write_function(stream, "hello: %s", cmd);
	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "hello: %s", cmd);
	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_LOAD_FUNCTION(mod_@template@_load)
{
	oset_api_interface_t *commands_api_interface = NULL;

	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "@template@ module Load Success.");

	OSET_ADD_API(commands_api_interface, "hello", "hello demo", hello_function, "<strings>");
	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_RUNTIME_FUNCTION(mod_@template@_runtime)
{
	while(!globals.shutdown) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "@template@ module running.");
		oset_msleep(5000);
	}
	return OSET_STATUS_SUCCESS;	
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_@template@_shutdown)
{
	globals.shutdown = 1;
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "@template@ module Shutdown Success.");
	return OSET_STATUS_SUCCESS;
}

