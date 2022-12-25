/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#include "oset-core.h"

OSET_MODULE_LOAD_FUNCTION(mod_console_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_console_shutdown);
OSET_MODULE_DEFINITION(libmod_console, mod_console_load, mod_console_shutdown, NULL);

static int RUNNING = 0;

static int COLORIZE = 0;
#ifdef WIN32
static HANDLE hStdout;
static WORD wOldColorAttrs;
static CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

static WORD
#else
static const char *
#endif




	COLORS[] =
	{ OSET_SEQ_DEFAULT_COLOR, OSET_SEQ_FRED, OSET_SEQ_FRED, OSET_SEQ_FRED, OSET_SEQ_FMAGEN, OSET_SEQ_FCYAN, OSET_SEQ_FGREEN,
OSET_SEQ_FYELLOW };


static oset_apr_memory_pool_t *module_pool = NULL;
static oset_hashtable_t *log_hash = NULL;
static uint32_t all_level = 0;
static int32_t hard_log_level = OSET_LOG2_DEBUG;
static oset_bool_t log_uuid = OSET_FALSE;
//static int32_t failed_write = 0;
static oset_bool_t json_log = OSET_FALSE;
static oset_log2_json_format_t json_format = {
	{ NULL, NULL }, // version
	{ "host", NULL }, // host
	{ "timestamp", NULL }, // timestamp
	{ "level", NULL }, // level
	{ "ident", NULL }, // ident
	{ "pid", NULL }, // pid
	{ "session", NULL }, // uuid
	{ "file", NULL }, // file
	{ "line", NULL }, // line
	{ "function", NULL }, // function
	{ "message", NULL }, // full_message
	{ NULL, NULL }, // short_message
	"", // custom_field_prefix
	0.0, // timestamp_divisor
	{ "sequence", NULL } // sequence
};

static char *to_json_string(const oset_log2_node_t *node)
{
	char *json_text = NULL;
	cJSON *json = oset_log2_node_to_json(node, node->level, &json_format, NULL);
	json_text = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	return json_text;
}

static void del_mapping(char *var)
{
	oset_core_hash_insert(log_hash, var, NULL);
}

static void add_mapping(char *var, char *val, int cumlative)
{
	uint32_t m = 0;

	if (cumlative) {
		uint32_t l = oset_log2_str2level(val);
		uint32_t i;

		if (l < 10) {
			for (i = 0; i <= l; i++) {
				m |= (1 << i);
			}
		}
	} else {
		m = oset_log2_str2mask(val);
	}

	if (!strcasecmp(var, "all")) {
		all_level = m | oset_log2_str2mask("console");
		return;
	}

	del_mapping(var);
	oset_core_hash_insert(log_hash, var, (void *) (intptr_t) m);
}

static oset_status_t config_logger(void)
{
	char *cf = "console.conf";
	oset_xml_t cfg, xml, settings, param;

	if (!(xml = oset_xml_open_cfg(cf, &cfg, NULL))) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Open of %s failed", cf);
		return OSET_STATUS_TERM;
	}

	if (log_hash) {
		oset_core_hash_destroy(&log_hash);
	}

	oset_core_hash_init(&log_hash);

	if ((settings = oset_xml_child(cfg, "mappings"))) {
		for (param = oset_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) oset_xml_attr_soft(param, "name");
			char *val = (char *) oset_xml_attr_soft(param, "value");
			add_mapping(var, val, 1);
		}
		for (param = oset_xml_child(settings, "map"); param; param = param->next) {
			char *var = (char *) oset_xml_attr_soft(param, "name");
			char *val = (char *) oset_xml_attr_soft(param, "value");
			add_mapping(var, val, 0);
		}
	}

	// Customize field names or remove fields
	// To remove a field, set its name to empty string
	if ((settings = oset_xml_child(cfg, "json-log-format"))) {
		for (param = oset_xml_child(settings, "format"); param; param = param->next) {
			char *var = (char *) oset_xml_attr_soft(param, "field");
			char *val = (char *) oset_xml_attr_soft(param, "name");
			if (!strcasecmp(var, "host")) {
				json_format.host.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "timestamp")) {
				json_format.timestamp.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "level")) {
				json_format.level.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "ident")) {
				json_format.ident.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "pid")) {
				json_format.pid.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "uuid")) {
				json_format.uuid.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "file")) {
				json_format.file.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "line")) {
				json_format.line.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "function")) {
				json_format.function.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "message")) {
				json_format.full_message.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "short-message")) {
				json_format.short_message.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "sequence")) {
				json_format.sequence.name = zstr(val) ? NULL : oset_core_strdup(module_pool, val);
			}
		}
		for (param = oset_xml_child(settings, "config"); param; param = param->next) {
			char *var = (char *) oset_xml_attr_soft(param, "name");
			char *val = (char *) oset_xml_attr_soft(param, "value");
			if (!strcasecmp(var, "custom-field-prefix")) {
				json_format.custom_field_prefix = oset_core_strdup(module_pool, val);
			} else if (!strcasecmp(var, "timestamp-divisor") && oset_is_number(val)) {
				json_format.timestamp_divisor = strtod(val, NULL);
			}
		}
	}

	if ((settings = oset_xml_child(cfg, "settings"))) {
		for (param = oset_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) oset_xml_attr_soft(param, "name");
			char *val = (char *) oset_xml_attr_soft(param, "value");

			if (!strcasecmp(var, "colorize") && oset_true(val)) {
#ifdef WIN32
				hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
				if (oset_core_get_console() == stdout && hStdout != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) {
					wOldColorAttrs = csbiInfo.wAttributes;
					COLORIZE = 1;
				}
#else
				COLORIZE = 1;
#endif
			} else if (!strcasecmp(var, "loglevel") && !zstr(val)) {
				hard_log_level = oset_log2_str2level(val);
			} else if (!strcasecmp(var, "uuid") && oset_true(val)) {
				log_uuid = OSET_TRUE;
			} else if (!strcasecmp(var, "json") && oset_true(val)) {
				json_log = OSET_TRUE;
			}
		}
	}

	oset_xml_free(xml);

	return OSET_STATUS_SUCCESS;
}

static int can_write(FILE * handle, int ms)
{
#ifndef WIN32
	int aok = 1;
	fd_set can_write;
	int fd;
	struct timeval to;
	int sec, usec;

	sec = ms / 1000;
	usec = ms % 1000;

	fd = fileno(handle);
	memset(&to, 0, sizeof(to));
	FD_ZERO(&can_write);
	FD_SET(fd, &can_write);
	to.tv_sec = sec;
	to.tv_usec = usec;
	if (select(fd + 1, NULL, &can_write, NULL, &to) > 0) {
		aok = FD_ISSET(fd, &can_write);
	} else {
		aok = 0;
	}

	return aok;
#else
	return 1;
#endif
}


static oset_status_t oset_console_logger(const oset_log2_node_t *node, oset_log2_level_t level)
{
	FILE *handle;

	if (!RUNNING) {
		return OSET_STATUS_SUCCESS;
	}
#if 0
	if (failed_write) {
		if ((handle = oset_core_data_channel(OSET_CHANNEL_ID_LOG))) {
			int aok = can_write(handle, 100);
			if (aok) {
				const char *msg = "Failed to write to the console! Logging disabled! RE-enable with the 'console loglevel' command";
#ifdef WIN32
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "%s", msg);
#else
				if (COLORIZE) {
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "%s%s%s", COLORS[1], msg, OSET_SEQ_DEFAULT_COLOR);
				} else {
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "%s", msg);
				}
#endif
				failed_write = 0;
			}
		}
	}
#endif

	if (level > hard_log_level && (node->slevel == OSET_LOG2_UNINIT || level > node->slevel)) {
		return OSET_STATUS_SUCCESS;
	}

	if ((handle = oset_core_data_channel(OSET_CHANNEL_ID_LOG))) {
		size_t mask = 0;
		size_t ok = 0;

		ok = oset_log2_check_mask(all_level, level);

		if (log_hash) {
			if (!ok) {
				mask = (size_t) oset_core_hash_find(log_hash, node->file);
				ok = oset_log2_check_mask(mask, level);
			}

			if (!ok) {
				mask = (size_t) oset_core_hash_find(log_hash, node->func);
				ok = oset_log2_check_mask(mask, level);
			}
		}

		if (ok) {
#ifndef WIN32
			int aok = can_write(handle, 10000);

			if (!aok) {
				//hard_log_level = 0;
				//failed_write++;
				return OSET_STATUS_SUCCESS;
			}
#endif

			if (json_log) {
				char *json_log_str = to_json_string(node);
				if (json_log_str) {
					fprintf(handle, "%s\n", json_log_str);
					oset_safe_free(json_log_str);
				}
			} else if (COLORIZE) {
#ifdef WIN32
				DWORD len = (DWORD) strlen(node->data);
				DWORD outbytes = 0;
				SetConsoleTextAttribute(hStdout, COLORS[node->level]);
				if (log_uuid && !zstr(node->userdata)) {
					WriteFile(hStdout, node->userdata, (DWORD)strlen(node->userdata), &outbytes, NULL);
					WriteFile(hStdout, " ", 1, &outbytes, NULL);
				}
				WriteFile(hStdout, node->data, len, &outbytes, NULL);
				SetConsoleTextAttribute(hStdout, wOldColorAttrs);
#else
				if (log_uuid && !zstr(node->userdata)) {
					fprintf(handle, "%s%s %s%s", COLORS[node->level], node->userdata, node->data, OSET_SEQ_DEFAULT_COLOR);
				} else {
					fprintf(handle, "%s%s%s", COLORS[node->level], node->data, OSET_SEQ_DEFAULT_COLOR);
				}
#endif
			} else if (log_uuid && !zstr(node->userdata)) {
				fprintf(handle, "%s %s", node->userdata, node->data);
			} else {
				fprintf(handle, "%s", node->data);
			}
		}
	}

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(console_api_function)
{
	int argc;
	char *mycmd = NULL, *argv[3] = { 0 };
	oset_status_t status = OSET_STATUS_SUCCESS;
	const char *usage_string = "USAGE:\n"
		"--------------------------------------------------------------------------------\n"
		"console help\n"
		"console loglevel [[0-7] | <loglevel_string>]\n"
		"console uuid [on|off|toggle]\n"
		"console json [on|off|toggle]\n"
		"console colorize [on|off|toggle]\n" "--------------------------------------------------------------------------------\n";
	const char *loglevel_usage_string = "USAGE:\n"
		"--------------------------------------------------------------------------------\n"
		"console loglevel [[0-7] | <loglevel_string>]\n"
		"\n"
		"Set the logging verbosity of the console from 0 (least verbose) to\n"
		"7 (debugging), or specify the loglevel as a string:\n"
		"\n"
		"  0 console\n"
		"  1 alert\n"
		"  2 crit\n"
		"  3 err\n"
		"  4 warning\n" "  5 notice\n" "  6 info\n" "  7 debug\n" "--------------------------------------------------------------------------------\n";
	const char *colorize_usage_string = "USAGE:\n"
		"--------------------------------------------------------------------------------\n"
		"console colorize [on|off|toggle]\n"
		"\n" "Enable, disable, or toggle console coloring.\n" "--------------------------------------------------------------------------------\n";

	if (session)
		return OSET_STATUS_FALSE;

	if (zstr(cmd)) {
		stream->write_function(stream, "%s", usage_string);
		goto done;
	}

	if (!(mycmd = strdup(cmd))) {
		status = OSET_STATUS_MEMERR;
		goto done;
	}

	if (!(argc = oset_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) || !argv[0]) {
		stream->write_function(stream, "%s", usage_string);
		goto done;
	}

	if (!strcasecmp(argv[0], "loglevel")) {
		int level = hard_log_level;

		if (argc > 1) {
			if (!strcasecmp(argv[1], "help")) {
				stream->write_function(stream, "%s", loglevel_usage_string);
				goto done;
			} else if (*argv[1] > 47 && *argv[1] < 58) {
				level = atoi(argv[1]);
			} else {
				level = oset_log2_str2level(argv[1]);
			}
		}
		if (level == OSET_LOG2_INVALID) {
			stream->write_function(stream, "-ERR Invalid console loglevel (%s)!\n\n", argc > 1 ? argv[1] : "");
		} else {
			hard_log_level = level;
			stream->write_function(stream, "+OK console log level set to %s\n", oset_log2_level2str(hard_log_level));
		}

	} else if (!strcasecmp(argv[0], "colorize")) {
		if (argc > 1) {
			if (!strcasecmp(argv[1], "help")) {
				stream->write_function(stream, "%s", colorize_usage_string);
				goto done;
			} else if (!strcasecmp(argv[1], "toggle")) {
				COLORIZE ^= 1;
			} else {
				COLORIZE = oset_true(argv[1]);
			}
		}
		stream->write_function(stream, "+OK console color %s\n", COLORIZE ? "enabled" : "disabled");

	} else if (!strcasecmp(argv[0], "uuid")) {
		if (argc > 1) {
			if (!strcasecmp(argv[1], "toggle")) {
				if (log_uuid) {
					log_uuid = OSET_FALSE;
				} else {
					log_uuid = OSET_TRUE;
				}
			} else {
				log_uuid = oset_true(argv[1]);
			}
		}
		stream->write_function(stream, "+OK console uuid %s\n", log_uuid ? "enabled" : "disabled");
	} else if (!strcasecmp(argv[0], "json")) {
		if (argc > 1) {
			if (!strcasecmp(argv[1], "toggle")) {
				if (json_log) {
					json_log = OSET_FALSE;
				} else {
					json_log = OSET_TRUE;
				}
			} else {
				json_log = oset_true(argv[1]);
			}
		}
		stream->write_function(stream, "+OK console json %s\n", json_log ? "enabled" : "disabled");
	} else {					/* if (!strcasecmp(argv[0], "help")) { */
		stream->write_function(stream, "%s", usage_string);
	}

  done:
	oset_safe_free(mycmd);
	return status;
}

OSET_MODULE_LOAD_FUNCTION(mod_console_load)
{
	oset_api_interface_t *api_interface;


	module_pool = pool;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

	OSET_ADD_API(api_interface, "console", "Console", console_api_function, "loglevel [level]|colorize [on|toggle|off]|uuid [on|toggle|off]|json [on|toggle|off]");
	oset_console_set_complete("add console help");
	oset_console_set_complete("add console loglevel");
	oset_console_set_complete("add console loglevel help");
	oset_console_set_complete("add console loglevel console");
	oset_console_set_complete("add console loglevel alert");
	oset_console_set_complete("add console loglevel crit");
	oset_console_set_complete("add console loglevel err");
	oset_console_set_complete("add console loglevel warning");
	oset_console_set_complete("add console loglevel notice");
	oset_console_set_complete("add console loglevel info");
	oset_console_set_complete("add console loglevel debug");
	oset_console_set_complete("add console colorize");
	oset_console_set_complete("add console colorize help");
	oset_console_set_complete("add console colorize on");
	oset_console_set_complete("add console colorize off");
	oset_console_set_complete("add console colorize toggle");
	oset_console_set_complete("add console uuid on");
	oset_console_set_complete("add console uuid off");
	oset_console_set_complete("add console uuid toggle");
	oset_console_set_complete("add console json on");
	oset_console_set_complete("add console json off");
	oset_console_set_complete("add console json toggle");

	/* setup my logger function */
	oset_log2_bind_logger(oset_console_logger, OSET_LOG2_DEBUG, OSET_TRUE);

	config_logger();
	RUNNING = 1;
	/* indicate that the module should continue to be loaded */
	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_console_shutdown)
{

	oset_log2_unbind_logger(oset_console_logger);
	if (log_hash) {
		oset_core_hash_destroy(&log_hash);
	}

	RUNNING = 0;
	return OSET_STATUS_UNLOAD;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
