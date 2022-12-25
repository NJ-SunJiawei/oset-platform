/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/
#include "FSPLModel.h"
#include "oset-core.h"

OSET_MODULE_LOAD_FUNCTION(mod_FSPL_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_FSPL_shutdown);
OSET_MODULE_DEFINITION(libmod_FSPL, mod_FSPL_load, mod_FSPL_shutdown, NULL);

#define FSPL_SYNTAX "<f(Hz)> <distance(m)>"
OSET_STANDARD_API(FSPL_function)
{
	char *mycmd = NULL;
	char *argv[2] = {NULL};
	int argc = 0;
    double dB = 0;

	if (zstr(cmd) || !(mycmd = strdup(cmd))) {
		stream->write_function(stream, "-USAGE: %s\n", FSPL_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	argc = oset_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	if (argc != 2) {
		stream->write_function(stream, "-USAGE: %s\n", FSPL_SYNTAX);
		oset_safe_free(mycmd);
		return OSET_STATUS_SUCCESS;
	}

	dB = calFSPLModel(atoi(argv[0]),atoi(argv[1]));

	stream->write_function(stream, "+OK free space loss %d dB\n",dB);
	oset_safe_free(mycmd);
	return OSET_STATUS_SUCCESS;
}



OSET_MODULE_LOAD_FUNCTION(mod_FSPL_load)
{
	//oset_application_interface_t *app_interface;
	oset_api_interface_t *api_interface;

	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

	OSET_ADD_API(api_interface, "fspl", "run FSPL modle", FSPL_function, FSPL_SYNTAX);

	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_FSPL_shutdown)
{

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "mod_FSPL_shutdown.");
	return OSET_STATUS_SUCCESS;
}

