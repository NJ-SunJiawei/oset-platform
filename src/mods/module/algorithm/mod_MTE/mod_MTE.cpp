/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/
#include "oset-core.h"
#include "MTEnvironmentModel.h"

OSET_MODULE_LOAD_FUNCTION(mod_MTE_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_MTE_shutdown);
OSET_MODULE_DEFINITION(libmod_MTE, mod_MTE_load, mod_MTE_shutdown, NULL);

#define MTE_SYNTAX "<distance(m)> <f(Hz)=20e9> <EIRP=34> <G_T=10> <encoding='16QAM'> <bandeidth=200e6>"
OSET_STANDARD_API(MTE_function)
{
	char *mycmd = NULL;
	char *argv[6] = {NULL};
	int argc = 0;
    CHANNELINDEX result;

	if (zstr(cmd) || !(mycmd = strdup(cmd))) {
		stream->write_function(stream, "-USAGE: %s\n", MTE_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	argc = oset_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	if (argc != 6) {
		stream->write_function(stream, "-USAGE: %s\n", MTE_SYNTAX);
		oset_safe_free(mycmd);
		return OSET_STATUS_SUCCESS;
	}

	result = calWirelessChannelModel(atoi(argv[0]),atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),argv[4],atoi(argv[5]));

	stream->write_function(stream, "+OK WirelessChannel CNR[%ld], ber[%ld], delay[%ld]\n",result.m_CNR, result.m_ber, result.m_delay);
	oset_safe_free(mycmd);
	return OSET_STATUS_SUCCESS;
}



OSET_MODULE_LOAD_FUNCTION(mod_MTE_load)
{
	//oset_application_interface_t *app_interface;
	oset_api_interface_t *api_interface;

	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

	OSET_ADD_API(api_interface, "mte", "run MTE modle", MTE_function, MTE_SYNTAX);

	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_MTE_shutdown)
{

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "mod_MTE_shutdown.");
	return OSET_STATUS_SUCCESS;
}

