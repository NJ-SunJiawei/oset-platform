/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#include "oset-core.h"
#include <stdlib.h>
#include <syslog.h>

#define DEFAULT_IDENT    "om"
#define DEFAULT_FACILITY LOG_USER
#define DEFAULT_LEVEL    "warning"
#define DEFAULT_FORMAT   "[message]"
#define MAX_LENGTH       1024

OSET_MODULE_LOAD_FUNCTION(mod_syslog_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_syslog_shutdown);
OSET_MODULE_DEFINITION(libmod_syslog, mod_syslog_load, mod_syslog_shutdown, NULL);

static oset_status_t load_config(void);
static oset_log2_level_t log_level;

static struct {
	char *ident;
	char *format;
	int facility;
	oset_bool_t log_uuid;
} globals;

struct _facility_table_entry {
	char *description;
	int facility;
};

OSET_DECLARE_GLOBAL_STRING_FUNC(set_global_ident, globals.ident);
OSET_DECLARE_GLOBAL_STRING_FUNC(set_global_format, globals.format);

oset_status_t set_global_facility(const char *facility)
{
	const struct _facility_table_entry facilities[] = {
		{"auth", LOG_AUTH},
#if !defined (__SVR4) && !defined (__sun)
		{"authpriv", LOG_AUTHPRIV},
		{"ftp", LOG_FTP},
#endif
		{"cron", LOG_CRON},
		{"daemon", LOG_DAEMON},
		{"kern", LOG_KERN},
		{"local0", LOG_LOCAL0},
		{"local1", LOG_LOCAL1},
		{"local2", LOG_LOCAL2},
		{"local3", LOG_LOCAL3},
		{"local4", LOG_LOCAL4},
		{"local5", LOG_LOCAL5},
		{"local6", LOG_LOCAL6},
		{"local7", LOG_LOCAL7},
		{"lpr", LOG_LPR},
		{"mail", LOG_MAIL},
		{"news", LOG_NEWS},
		{"syslog", LOG_SYSLOG},
		{"user", LOG_USER},
		{"uucp", LOG_UUCP},
		{NULL, 0}
	};
	const struct _facility_table_entry *entry = facilities;

	while (!zstr(entry->description)) {
		if (!strcasecmp(entry->description, facility)) {
			globals.facility = entry->facility;
			return OSET_STATUS_SUCCESS;
		}
		entry++;
	}

	return OSET_STATUS_FALSE;
}

static oset_loadable_module_interface_t console_module_interface = {
	/*.module_name */ modname,
	/*.endpoint_interface */ NULL,
	/*.timer_interface */ NULL,
	/*.dialplan_interface */ NULL,
	/*.codec_interface */ NULL,
	/*.application_interface */ NULL,
	/*.api_interface */ NULL,
	/*.file_interface */ NULL
};


static int find_unprintable(const char *s)
{
	const char *p;

	for(p = s; p && *p; p++) {
		if (*p < 9 || *p == 27) {
			return 1;
		}
	}

	return 0;
}

static oset_status_t mod_syslog_logger(const oset_log2_node_t *node, oset_log2_level_t level)
{
	int syslog_level;

	switch (level) {
	case OSET_LOG2_DEBUG:
		syslog_level = LOG_DEBUG;
		break;
	case OSET_LOG2_INFO:
		syslog_level = LOG_INFO;
		break;
	case OSET_LOG2_NOTICE:
		syslog_level = LOG_NOTICE;
		break;
	case OSET_LOG2_WARNING:
		syslog_level = LOG_WARNING;
		break;
	case OSET_LOG2_ERROR:
		syslog_level = LOG_ERR;
		break;
	case OSET_LOG2_CRIT:
		syslog_level = LOG_CRIT;
		break;
	case OSET_LOG2_ALERT:
		syslog_level = LOG_ALERT;
		break;
	default:
		syslog_level = LOG_NOTICE;
		break;
	}

	/* don't log blank lines */
	if (!zstr(node->data) && (strspn(node->data, " \t\r\n") < strlen(node->data)) && !find_unprintable(node->data)) {
		if (globals.log_uuid && !zstr(node->userdata)) {
			syslog(syslog_level, "%s %s", node->userdata, node->data);
		} else {
			syslog(syslog_level, "%s", node->data);
		}
	}

	return OSET_STATUS_SUCCESS;
}

static oset_status_t load_config(void)
{
	char *cf = "syslog.conf";
	oset_xml_t cfg, xml, settings, param;

	/* default log level */
	log_level = OSET_LOG2_WARNING;

	/* default facility */
	globals.facility = DEFAULT_FACILITY;
	globals.log_uuid = OSET_TRUE;

	if (!(xml = oset_xml_open_cfg(cf, &cfg, NULL))) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Open of %s failed", cf);
	} else {
		if ((settings = oset_xml_child(cfg, "settings"))) {
			for (param = oset_xml_child(settings, "param"); param; param = param->next) {
				char *var = (char *) oset_xml_attr_soft(param, "name");
				char *val = (char *) oset_xml_attr_soft(param, "value");

				if (!strcmp(var, "ident")) {
					set_global_ident(val);
				} else if (!strcmp(var, "format")) {
					set_global_format(val);
				} else if (!strcmp(var, "facility")) {
					set_global_facility(val);
				} else if (!strcasecmp(var, "loglevel") && !zstr(val)) {
					log_level = oset_log2_str2level(val);
					if (log_level == OSET_LOG2_INVALID) {
						log_level = OSET_LOG2_WARNING;
					}
				} else if (!strcasecmp(var, "uuid")) {
					globals.log_uuid = oset_true(val);
				}
			}
		}
		oset_xml_free(xml);
	}

	if (zstr(globals.ident)) {
		set_global_ident(DEFAULT_IDENT);
	}
	if (zstr(globals.format)) {
		set_global_format(DEFAULT_FORMAT);
	}
	return 0;
}

OSET_MODULE_LOAD_FUNCTION(mod_syslog_load)
{
	oset_status_t status;
	*module_interface = &console_module_interface;

	memset(&globals, 0, sizeof(globals));

	if ((status = load_config()) != OSET_STATUS_SUCCESS) {
		return status;
	}

	openlog(globals.ident, LOG_PID, globals.facility);

	setlogmask(LOG_UPTO(LOG_DEBUG));
	oset_log2_bind_logger(mod_syslog_logger, log_level, OSET_FALSE);

	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_syslog_shutdown)
{
	closelog();

	oset_safe_free(globals.ident);
	oset_safe_free(globals.format);

	oset_log2_unbind_logger(mod_syslog_logger);

	return OSET_STATUS_SUCCESS;
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
