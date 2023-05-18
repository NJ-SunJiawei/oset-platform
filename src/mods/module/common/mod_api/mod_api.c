/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#include "sset-core.h"
#include "version.h"
#ifdef __GLIBC__
#include <malloc.h> /* mallinfo() */
#endif

OSET_MODULE_LOAD_FUNCTION(mod_api_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_api_shutdown);
OSET_MODULE_DEFINITION(libmod_api, mod_api_load, mod_api_shutdown, NULL);

static oset_apr_mutex_t *reload_mutex = NULL;

struct cb_helper {
	uint32_t row_process;
	oset_stream_handle_t *stream;
};

struct stream_format {
	char *http;           /* http cmd (from xmlrpc)                                              */
	char *query;          /* http query (cmd args)                                               */
	oset_bool_t api;    /* flag: define content type for http reply e.g. text/html or text/xml */
	oset_bool_t html;   /* flag: format as html                                                */
	char *nl;             /* newline to use: html "<br>\n" or just "\n"                          */
};
typedef struct stream_format stream_format;


#define SYSTEM_SYNTAX "<command>"
OSET_STANDARD_API(system_function)
{
	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", SYSTEM_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	if (oset_stream_system(cmd, stream) < 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Failed to execute command: %s", cmd);
	}

	return OSET_STATUS_SUCCESS;
}


#define SYSTEM_SYNTAX "<command>"
OSET_STANDARD_API(bg_system_function)
{
	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", SYSTEM_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Executing command: %s", cmd);
	if (oset_system(cmd, OSET_FALSE) < 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Failed to execute command: %s", cmd);
	}
	stream->write_function(stream, "+OK\n");
	return OSET_STATUS_SUCCESS;
}

#define SPAWN_SYNTAX "<command>"
OSET_STANDARD_API(spawn_stream_function)
{
	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", SPAWN_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	if (oset_stream_spawn(cmd, OSET_FALSE, OSET_TRUE, stream) < 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Failed to execute command: %s", cmd);
	}

	return OSET_STATUS_SUCCESS;
}

#define SPAWN_SYNTAX "<command>"
OSET_STANDARD_API(spawn_function)
{
	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", SPAWN_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Executing command: %s", cmd);
	if (oset_spawn(cmd, OSET_TRUE) < 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Failed to execute command: %s", cmd);
	}
	stream->write_function(stream, "+OK\n");
	return OSET_STATUS_SUCCESS;
}

#define SPAWN_SYNTAX "<command>"
OSET_STANDARD_API(bg_spawn_function)
{
	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", SPAWN_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Executing command: %s", cmd);
	if (oset_spawn(cmd, OSET_FALSE) < 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Failed to execute command: %s", cmd);
	}
	stream->write_function(stream, "+OK\n");
	return OSET_STATUS_SUCCESS;
}


#define ALIAS_SYNTAX "[add|stickyadd] <alias> <command> | del [<alias>|*]"
OSET_STANDARD_API(alias_function)
{
	oset_status_t status;

	if ((status = oset_console_set_alias(cmd)) == OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK\n");
	} else {
		stream->write_function(stream, "-USAGE: %s\n", ALIAS_SYNTAX);
	}

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(banner_function)
{
	stream->write_function(stream, "%s", oset_core_banner());
	return OSET_STATUS_SUCCESS;
}

static oset_apr_thread_rwlock_t *bgapi_rwlock = NULL;
struct bg_job {
	char *cmd;
	char *arg;
	char uuid_str[OSET_UUID_FORMATTED_LENGTH + 1];
	oset_apr_memory_pool_t *pool;
};

static void *OSET_THREAD_FUNC bgapi_exec(oset_apr_thread_t *thread, void *obj)
{
	struct bg_job *job = (struct bg_job *) obj;
	oset_stream_handle_t stream = { 0 };
	oset_status_t status;
	char *reply, *freply = NULL;
	oset_event_t *event;
	char *arg;
	oset_apr_memory_pool_t *pool;

	if (!job) {
		return NULL;
	}

	oset_apr_thread_rwlock_rdlock(bgapi_rwlock);

	pool = job->pool;

	OSET_STANDARD_STREAM(stream);

	if ((arg = strchr(job->cmd, ' '))) {
		*arg++ = '\0';
	}

	if ((status = oset_api_execute(job->cmd, arg, NULL, &stream)) == OSET_STATUS_SUCCESS) {
		reply = stream.data;
	} else {
		freply = oset_mprintf("%s: Command not found!\n", job->cmd);
		reply = freply;
	}

	if (!reply) {
		reply = "Command returned no output!";
	}

	if (oset_event_create(&event, OSET_EVENT_BACKGROUND_JOB) == OSET_STATUS_SUCCESS) {
		oset_event_add_header_string(event, OSET_STACK_BOTTOM, "Job-UUID", job->uuid_str);
		oset_event_add_header_string(event, OSET_STACK_BOTTOM, "Job-Command", job->cmd);
		if (arg) {
			oset_event_add_header_string(event, OSET_STACK_BOTTOM, "Job-Command-Arg", arg);
		}

		oset_event_add_body(event, "%s", reply);
		oset_event_fire(&event);
	}

	oset_safe_free(stream.data);
	oset_safe_free(freply);

	job = NULL;
	oset_core_destroy_memory_pool(&pool);
	pool = NULL;

	oset_apr_thread_rwlock_unlock(bgapi_rwlock);

	return NULL;
}

OSET_STANDARD_API(bgapi_function)
{
	struct bg_job *job;
	oset_uuid_t uuid;
	oset_apr_memory_pool_t *pool;
	oset_apr_thread_t *thread;
	oset_apr_threadattr_t *thd_attr = NULL;
	
	const char *p, *arg = cmd;
	char my_uuid[OSET_UUID_FORMATTED_LENGTH + 1] = ""; 

	if (!cmd) {
		stream->write_function(stream, "-ERR Invalid syntax\n");
		return OSET_STATUS_SUCCESS;
	}

	if (!strncasecmp(cmd, "uuid:", 5)) {
		p = cmd + 5;
		if ((arg = strchr(p, ' ')) && *arg++) {
			oset_apr_copy_string(my_uuid, p, arg - p);
		}
	}

	if (zstr(arg)) {
		stream->write_function(stream, "-ERR Invalid syntax\n");
		return OSET_STATUS_SUCCESS;
	}

	oset_core_new_memory_pool(&pool);
	job = oset_core_alloc(pool, sizeof(*job));
	job->cmd = oset_core_strdup(pool, arg);
	job->pool = pool;

	if (*my_uuid) {
		oset_apr_copy_string(job->uuid_str, my_uuid, strlen(my_uuid)+1);
	} else {
		oset_uuid_get(&uuid);
		oset_uuid_format(job->uuid_str, &uuid);
	}

	oset_apr_threadattr_create(&thd_attr, job->pool);
	oset_apr_threadattr_detach_set(thd_attr, 1);
	oset_apr_threadattr_stacksize_set(thd_attr, OSET_THREAD_STACKSIZE);
	stream->write_function(stream, "+OK Job-UUID: %s\n", job->uuid_str);
	oset_apr_thread_create(&thread, thd_attr, bgapi_exec, job, job->pool);

	return OSET_STATUS_SUCCESS;
}

#define COMPLETE_SYNTAX "add <word>|del [<word>|*]"
OSET_STANDARD_API(complete_function)
{
	oset_status_t status;

	if ((status = oset_console_set_complete(cmd)) == OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK\n");
	} else {
		stream->write_function(stream, "-USAGE: %s\n", COMPLETE_SYNTAX);
	}

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(console_complete_function)
{
	const char *p, *cursor = NULL;
	int c;

	if (zstr(cmd)) {
		cmd = " ";
	}

	if ((p = strstr(cmd, "c="))) {
		p += 2;
		c = atoi(p);
		if ((p = strchr(p, ';'))) {
			cmd = p + 1;
			cursor = cmd + c;
		}
	}

	oset_console_complete(cmd, cursor, NULL, stream, NULL);
	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(console_complete_xml_function)
{
	const char *p, *cursor = NULL;
	int c;
	oset_xml_t xml = oset_xml_new("complete");
	char *sxml;

	if (zstr(cmd)) {
		cmd = " ";
	}

	if ((p = strstr(cmd, "c="))) {
		p += 2;
		c = atoi(p);
		if ((p = strchr(p, ';'))) {
			cmd = p + 1;
			cursor = cmd + c;
		}
	}

	oset_console_complete(cmd, cursor, NULL, NULL, xml);

	sxml = oset_xml_toxml(xml, OSET_TRUE);
	stream->write_function(stream, "%s", sxml);
	free(sxml);

	oset_xml_free(xml);

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(db_cache_function)
{
	int argc;
	char *mydata = NULL, *argv[2];

	if (zstr(cmd)) {
		goto error;
	}

	mydata = strdup(cmd);
	oset_assert(mydata);

	argc = oset_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));

	if (argc < 1) {
		goto error;
	}
	if (argv[0] && oset_stristr("status", argv[0])) {
		oset_cache_db_status(stream);
		goto ok;
	} else {
		goto error;
	}

  error:
	stream->write_function(stream, "%s", "parameter missing\n");
  ok:
	oset_safe_free(mydata);
	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(echo_function)
{
	stream->write_function(stream, "%s", cmd);
	return OSET_STATUS_SUCCESS;
}


OSET_STANDARD_API(escape_function)
{
	int len;
	char *mycmd;

	if (zstr(cmd)) {
		return OSET_STATUS_SUCCESS;
	}

	len = (int)strlen(cmd) * 2 + 1;
	mycmd = malloc(len);

	stream->write_function(stream, "%s", oset_escape_string(cmd, mycmd, len));

	oset_safe_free(mycmd);
	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(shutdown_function)
{
	sset_session_ctl_t command = SCSC_SHUTDOWN;
	int arg = 0;

	stream->write_function(stream, "+OK\n");
	sset_core_session_ctl(command, &arg);

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(version_function)
{
	if (zstr(cmd)) {
		stream->write_function(stream, "OSET Version %s\n", SSET_VERSION);
		goto end;
	}
end:
	return OSET_STATUS_SUCCESS;
}

struct holder {
	char * delim;
	oset_stream_handle_t *stream;
	uint32_t count;
	int print_title;
	oset_xml_t xml;
	cJSON *json;
	int rows;
	int justcount;
	stream_format *format;
};

static stream_format set_format(stream_format *format, oset_stream_handle_t *stream)
{
	format->nl = "\n";
	if (stream->param_event && (format->http = oset_event_get_header(stream->param_event, "HTTP-URI"))) {
		format->query = oset_event_get_header(stream->param_event, "HTTP-QUERY");
		if (oset_event_get_header(stream->param_event, "HTTP-API")) {
			format->api = OSET_TRUE;
		}
		if (!strncasecmp(format->http, "/webapi/", 8)) {
			format->nl = "<br>\n";
			format->html = OSET_TRUE;
		}
	}

	return *format;
}

static int show_as_xml_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct holder *holder = (struct holder *) pArg;
	oset_xml_t row, field;
	int x, f_off = 0;
	char id[50];

	if (holder->count == 0) {
		if (!(holder->xml = oset_xml_new("result"))) {
			return -1;
		}
	}

	if (holder->justcount) {
		if (zstr(argv[0])) {
			holder->count = 0;
		} else {
			holder->count = (uint32_t) atoi(argv[0]);
		}
		return 0;
	}

	if (!(row = oset_xml_add_child_d(holder->xml, "row", holder->rows++))) {
		return -1;
	}

	oset_snprintf(id, sizeof(id), "%d", holder->rows);

	oset_xml_set_attr_d_buf(row, "row_id", id);

	for (x = 0; x < argc; x++) {
		char *name = columnNames[x];
		char *val = oset_str_nil(argv[x]);

		if (!name) {
			name = "undefined";
		}

		if ((field = oset_xml_add_child_d(row, name, f_off++))) {
			oset_xml_set_txt_d(field, val);
		} else {
			return -1;
		}
	}

	holder->count++;

	return 0;
}

static int show_callback(void *pArg, int argc, char **argv, char **columnNames)
{
	struct holder *holder = (struct holder *) pArg;
	int x;

	if (holder->justcount) {
		if (zstr(argv[0])) {
			holder->count = 0;
		} else {
			holder->count = (uint32_t) atoi(argv[0]);
		}
		return 0;
	}

	if (holder->print_title && holder->count == 0) {
		if (holder->format && holder->format->html) {
			holder->stream->write_function(holder->stream, "\n<tr>");
		}

		for (x = 0; x < argc; x++) {
			char *name = columnNames[x];
			if (!name) {
				name = "undefined";
			}

			if (holder->format && holder->format->html) {
				holder->stream->write_function(holder->stream, "<td>");
				holder->stream->write_function(holder->stream, "<b>%s</b>%s", name, x == (argc - 1) ? "</td></tr>\n" : "</td><td>");
			} else {
				holder->stream->write_function(holder->stream, "%s%s", name, x == (argc - 1) ? "\n" : holder->delim);
			}
		}
	}

	if (holder->format && holder->format->html) {
		holder->stream->write_function(holder->stream, "<tr bgcolor=%s>", holder->count % 2 == 0 ? "eeeeee" : "ffffff");
	}

	for (x = 0; x < argc; x++) {
		char *val = oset_str_nil(argv[x]);


		if (holder->format && holder->format->html) {
			char aval[512];

			oset_amp_encode(val, aval, sizeof(aval));
			holder->stream->write_function(holder->stream, "<td>");
			holder->stream->write_function(holder->stream, "%s%s", aval, x == (argc - 1) ? "</td></tr>\n" : "</td><td>");
		} else {
			holder->stream->write_function(holder->stream, "%s%s", val, x == (argc - 1) ? "\n" : holder->delim);
		}
	}

	holder->count++;
	return 0;
}

#define SHOW_SYNTAX "application|api|timer|aliases|complete|modules|tasks|limits|status"
OSET_STANDARD_API(show_function)
{
	char sql[1024];
	char *errmsg;
	oset_cache_db_handle_t *db;
	struct holder holder = { 0 };
	int help = 0;
	char *mydata = NULL, *argv[6] = { 0 };
	char *command = NULL, *as = NULL;
	oset_core_flag_t cflags = oset_core_flags();
	oset_status_t status = OSET_STATUS_SUCCESS;
	int html = 0;
	char *nl = "\n";
	stream_format format = { 0 };

	holder.format = &format;
	set_format(holder.format, stream);
	html = holder.format->html; /* html is just a shortcut */

	if (!(cflags & SCF_USE_SQL)) {
		stream->write_function(stream, "-ERR SQL disabled, no data available!\n");
		return OSET_STATUS_SUCCESS;
	}

	if (oset_core_db_handle(&db) != OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "%s", "-ERR Database error!\n");
		return OSET_STATUS_SUCCESS;
	}

	holder.justcount = 0;

	if (cmd && *cmd && (mydata = strdup(cmd))) {
		oset_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
		command = argv[0];
		if (argv[2] && !strcasecmp(argv[1], "as")) {
			as = argv[2];
		}
	}

	holder.print_title = 1;

	if (!command) {
		stream->write_function(stream, "-USAGE: %s\n", SHOW_SYNTAX);
		goto end;
	/* for those of us that keep on typing at the CLI: "show status" instead of "status" */
	} else if (!strncasecmp(command, "status", 6)) {
		if (!as) {
			as = argv[1];
		}
		oset_api_execute(command, as, NULL, stream);
		goto end;
	/* If you change the field qty or order of any of these select          */
	/* statements, you must also change show_callback and friends to match! */
	} else if (!strncasecmp(command, "timer", 5) ||
			   !strncasecmp(command, "limit", 5)) {
		if (end_of(command) == 's') {
			end_of(command) = '\0';
		}
		oset_apr_snprintfv(sql, sizeof(sql), "select type, name, ikey from interfaces where hostname='%q' and type = '%q' order by type,name", oset_core_get_hostname(), command);
	} else if (!strncasecmp(command, "module", 6)) {
		if (argv[1] && strcasecmp(argv[1], "as")) {
			oset_apr_snprintfv(sql, sizeof(sql), "select distinct type, name, ikey, filename from interfaces where hostname='%q' and ikey = '%q' order by type,name",
					oset_core_get_hostname(), argv[1]);
		} else {
			oset_apr_snprintfv(sql, sizeof(sql), "select distinct type, name, ikey, filename from interfaces where hostname='%q' order by type,name", oset_core_get_hostname());
		}
	} else if (!strcasecmp(command, "tasks")) {
		oset_apr_snprintfv(sql, sizeof(sql), "select * from %q where hostname='%q'", command, oset_core_get_hostname());
	} else if (!strcasecmp(command, "application") || !strcasecmp(command, "api")) {
		if (argv[1] && strcasecmp(argv[1], "as")) {
			oset_apr_snprintfv(sql, sizeof(sql),
					"select name, description, syntax, ikey from interfaces where hostname='%q' and type = '%q' and description != '' and name = '%q' order by type,name",
					oset_core_get_hostname(), command, argv[1]);
		} else {
			oset_apr_snprintfv(sql, sizeof(sql), "select name, description, syntax, ikey from interfaces where hostname='%q' and type = '%q' and description != '' order by type,name", oset_core_get_hostname(), command);
		}
	/* moved refreshable webpage show commands i.e. show calls|registrations|channels||detailed_calls|bridged_calls|detailed_bridged_calls */
	} else if (!strcasecmp(command, "aliases")) {
		oset_apr_snprintfv(sql, sizeof(sql), "select * from aliases where hostname='%q' order by alias", oset_core_get_osetname());
	} else if (!strcasecmp(command, "complete")) {
		oset_apr_snprintfv(sql, sizeof(sql), "select * from complete where hostname='%q' order by a1,a2,a3,a4,a5,a6,a7,a8,a9,a10", oset_core_get_osetname());
	} else if (!strncasecmp(command, "help", 4)) {
		char *cmdname = NULL;

		help = 1;
		holder.print_title = 0;
		if ((cmdname = strchr(command, ' ')) && strcasecmp(cmdname, "as")) {
			*cmdname++ = '\0';
			oset_apr_snprintfv(sql, sizeof(sql),
							"select name, syntax, description, ikey from interfaces where hostname='%q' and type = 'api' and name = '%q' order by name",
							oset_core_get_hostname(), cmdname);
		} else {
			oset_apr_snprintfv(sql, sizeof(sql), "select name, syntax, description, ikey from interfaces where hostname='%q' and type = 'api' order by name", oset_core_get_hostname());
		}
	} else {
			stream->write_function(stream, "-USAGE: %s\n", SHOW_SYNTAX);
			goto end;
	}

	holder.stream = stream;
	holder.count = 0;

	if (html) {
		nl = holder.format->nl;
		if (!as || strcasecmp(as,"xml")) {
			/* don't bother cli with heading and timestamp */
			stream->write_function(stream, "<h1>OM %s %s</h1>\n", command, holder.justcount?"(count)":"");
			stream->write_function(stream, "%s%s", oset_event_get_header(stream->param_event,"Event-Date-Local"), nl);
		}
		holder.stream->write_function(holder.stream, "<table cellpadding=1 cellspacing=4 border=1>\n");
	}

	if (!as) {
		as = "delim";
		holder.delim = ",";
	}

	/* oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "SQL: %s", sql); */

	if (!strcasecmp(as, "delim") || !strcasecmp(as, "csv")) {
		if (zstr(holder.delim)) {
			if (!(holder.delim = argv[3])) {
				holder.delim = ",";
			}
		}
		oset_cache_db_execute_sql_callback(db, sql, show_callback, &holder, &errmsg);
		if (html) {
			holder.stream->write_function(holder.stream, "</table>");
		}

		if (errmsg) {
			stream->write_function(stream, "-ERR SQL error [%s]\n", errmsg);
			free(errmsg);
			errmsg = NULL;
		} else if (help) {
			if (holder.count == 0)
				stream->write_function(stream, "-ERR No such command\n");
		} else {
			stream->write_function(stream, "%s%u total.%s", nl, holder.count, nl);
		}
	} else if (!strcasecmp(as, "xml")) {
		oset_cache_db_execute_sql_callback(db, sql, show_as_xml_callback, &holder, &errmsg);

		if (errmsg) {
			stream->write_function(stream, "-ERR SQL error [%s]\n", errmsg);
			free(errmsg);
			errmsg = NULL;
		}

		if (holder.xml) {
			char count[50];
			char *xmlstr;
			oset_snprintf(count, sizeof(count), "%d", holder.count);

			oset_xml_set_attr(holder.xml, "row_count", count);
			xmlstr = oset_xml_toxml(holder.xml, OSET_FALSE);
			oset_xml_free(holder.xml);

			if (xmlstr) {
				holder.stream->write_function(holder.stream, "%s", xmlstr);
				free(xmlstr);
			} else {
				holder.stream->write_function(holder.stream, "<result row_count=\"0\"/>\n");
			}
		} else {
			holder.stream->write_function(holder.stream, "<result row_count=\"0\"/>\n");
		}
	}else {
		holder.stream->write_function(holder.stream, "-ERR Cannot find format %s\n", as);
		goto end;
	}

  end:

	oset_safe_free(mydata);
	oset_cache_db_release_db_handle(&db);

	return status;
}


OSET_STANDARD_API(help_function)
{
	char showcmd[1024];
	int all = 0;
	if (zstr(cmd)) {
		sprintf(showcmd, "help");
		all = 1;
	} else {
		oset_snprintf(showcmd, sizeof(showcmd) - 1, "help %s", cmd);
	}

	if (all) {
		stream->write_function(stream, "\nValid Commands:\n\n");
	}

	show_function(showcmd, session, stream);

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(hostname_api_function)
{
	stream->write_function(stream, "%s", oset_core_get_hostname());
	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(osetname_api_function)
{
	stream->write_function(stream, "%s", oset_core_get_osetname());
	return OSET_STATUS_SUCCESS;
}


OSET_STANDARD_API(gethost_api_function)
{
	struct sockaddr_in sa;
	struct hostent *he;
	const char *ip;
	char buf[50] = "";

	if (!zstr(cmd)) {
		he = gethostbyname(cmd);

		if (he) {
			memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
			ip = oset_inet_ntop2(AF_INET, &sa.sin_addr, buf, sizeof(buf));
			stream->write_function(stream, "%s", ip);
			return OSET_STATUS_SUCCESS;
		}
	}

	stream->write_function(stream, "-ERR");

	return OSET_STATUS_SUCCESS;
}

#define GETENV_SYNTAX "<name>"
OSET_STANDARD_API(getenv_function)
{
	const char *var = NULL;

	if (cmd) {
		var = getenv((char *)cmd);
	}

	stream->write_function(stream, "%s", var ? var : "_undef_");

	return OSET_STATUS_SUCCESS;
}

#define LOAD_SYNTAX "<mod_name>"
OSET_STANDARD_API(load_function)
{
	const char *err;

	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", LOAD_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	oset_apr_mutex_lock(reload_mutex);

	if (oset_xml_reload(&err) == OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK Reloading XML\n");
	}

	if (oset_loadable_module_load_module((char *) OSET_GLOBAL_dirs.mod_dir, (char *) cmd, OSET_TRUE, &err) == OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK\n");
	} else {
		stream->write_function(stream, "-ERR [%s]\n", err);
	}

	oset_apr_mutex_unlock(reload_mutex);

	return OSET_STATUS_SUCCESS;
}

#define UNLOAD_SYNTAX "[-f] <mod_name>"
OSET_STANDARD_API(unload_function)
{
	const char *err;
	oset_bool_t force = OSET_FALSE;
	const char *p = cmd;

	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", UNLOAD_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}


	if (*p == '-') {
		p++;
		while (p && *p) {
			switch (*p) {
			case ' ':
				cmd = p + 1;
				goto end;
			case 'f':
				force = OSET_TRUE;
				break;
			default:
				break;
			}
			p++;
		}
	}
  end:

	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", UNLOAD_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	oset_apr_mutex_lock(reload_mutex);

	if (oset_loadable_module_unload_module((char *) OSET_GLOBAL_dirs.mod_dir, (char *) cmd, force, &err) == OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK\n");
	} else {
		stream->write_function(stream, "-ERR [%s]\n", err);
	}

	oset_apr_mutex_unlock(reload_mutex);

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(reload_function)
{
	const char *err;
	oset_bool_t force = OSET_FALSE;
	const char *p = cmd;

	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", UNLOAD_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	if (*p == '-') {
		p++;
		while (p && *p) {
			switch (*p) {
			case ' ':
				cmd = p + 1;
				goto end;
			case 'f':
				force = OSET_TRUE;
				break;
			default:
				break;
			}
			p++;
		}
	}
  end:

	if (zstr(cmd)) {
		stream->write_function(stream, "-USAGE: %s\n", UNLOAD_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	oset_apr_mutex_lock(reload_mutex);

	if (oset_xml_reload(&err) == OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK Reloading XML\n");
	}

	if (oset_loadable_module_unload_module((char *) OSET_GLOBAL_dirs.mod_dir, (char *) cmd, force, &err) == OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK module unloaded\n");
	} else {
		stream->write_function(stream, "-ERR unloading module [%s]\n", err);
	}

	if (oset_loadable_module_load_module((char *) OSET_GLOBAL_dirs.mod_dir, (char *) cmd, OSET_TRUE, &err) == OSET_STATUS_SUCCESS) {
		stream->write_function(stream, "+OK module loaded\n");
	} else {
		stream->write_function(stream, "-ERR loading module [%s]\n", err);
	}

	oset_apr_mutex_unlock(reload_mutex);

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(reload_xml_function)
{
	const char *err = "";

	oset_xml_reload(&err);
	stream->write_function(stream, "+OK [%s]\n", err);

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(module_exists_function)
{
	if (!zstr(cmd)) {
		if (oset_loadable_module_exists(cmd) == OSET_STATUS_SUCCESS) {
			stream->write_function(stream, "true");
		} else {
			stream->write_function(stream, "false");
		}
	}

	return OSET_STATUS_SUCCESS;
}

#define LOG_SYNTAX "<level> <message>"
OSET_STANDARD_API(log_function)
{
	char *level, *log_str;

	if (cmd && (level = strdup(cmd))) {
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
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), ltype, "log input %s", log_str);

		oset_safe_free(level);
		stream->write_function(stream, "+OK\n");
	} else {
		stream->write_function(stream, "-ERR\n");
	}

	return OSET_STATUS_SUCCESS;
}

struct api_task {
	uint32_t recur;
	char cmd[];
};

static void sch_api_callback(oset_scheduler_task_t *task)
{
	char *cmd, *arg = NULL;
	oset_stream_handle_t stream = { 0 };
	struct api_task *api_task = (struct api_task *) task->cmd_arg;
	oset_assert(task);

	cmd = strdup(api_task->cmd);
	oset_assert(cmd);

	if ((arg = strchr(cmd, ' '))) {
		*arg++ = '\0';
	}

	OSET_STANDARD_STREAM(stream);
	oset_api_execute(cmd, arg, NULL, &stream);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "Command %s(%s):\n%s", cmd, oset_str_nil(arg), oset_str_nil((char *) stream.data));
	oset_safe_free(stream.data);
	oset_safe_free(cmd);

	if (api_task->recur) {
		task->runtime = oset_epoch_time_now(NULL) + api_task->recur;
	}
}


#define SCHED_SYNTAX "[+@]<time> <group_name> <command_string>[&]"
OSET_STANDARD_API(sched_api_function)
{
	char *tm = NULL, *dcmd, *group;
	time_t when;
	struct api_task *api_task = NULL;
	uint32_t recur = 0;
	int flags = SSHF_FREE_ARG;

	if (!cmd) {
		goto bad;
	}
	tm = strdup(cmd);
	oset_assert(tm != NULL);

	if ((group = strchr(tm, ' '))) {
		uint32_t id;

		*group++ = '\0';

		if ((dcmd = strchr(group, ' '))) {
			*dcmd++ = '\0';

			if (*tm == '+') {
				when = oset_epoch_time_now(NULL) + atol(tm + 1);
			} else if (*tm == '@') {
				recur = (uint32_t) atol(tm + 1);
				when = oset_epoch_time_now(NULL) + recur;
			} else {
				when = atol(tm);
			}

			oset_zmalloc(api_task, sizeof(*api_task) + strlen(dcmd) + 1);
			oset_apr_copy_string(api_task->cmd, dcmd, strlen(dcmd) + 1);
			api_task->recur = recur;
			if (end_of(api_task->cmd) == '&') {
				end_of(api_task->cmd) = '\0';
				flags |= SSHF_OWN_THREAD;
			}


			id = oset_scheduler_add_task(when, sch_api_callback, (char *) __OSET_FUNC__, group, 0, api_task, flags);
			stream->write_function(stream, "+OK Added: %u\n", id);
			goto good;
		}
	}

  bad:

	stream->write_function(stream, "-ERR Invalid syntax. USAGE: %s\n", SCHED_SYNTAX);

  good:

	oset_safe_free(tm);
	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(sched_del_function)
{
	uint32_t cnt = 0;

	if (!cmd) {
		stream->write_function(stream, "-ERR Invalid syntax\n");
		return OSET_STATUS_SUCCESS;
	}

	if (oset_is_digit_string(cmd)) {
		int64_t tmp;
		tmp = (uint32_t) atoi(cmd);
		if (tmp > 0) {
			cnt = oset_scheduler_del_task_id((uint32_t) tmp);
		}
	} else {
		cnt = oset_scheduler_del_task_group(cmd);
	}

	stream->write_function(stream, "+OK Deleted: %u\n", cnt);

	return OSET_STATUS_SUCCESS;
}

#define UNSCHED_SYNTAX "<task_id>"
OSET_STANDARD_API(unsched_api_function)
{
	uint32_t id;

	if (!cmd) {
		stream->write_function(stream, "-ERR Invalid syntax. USAGE: %s\n", UNSCHED_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	if ((id = (uint32_t) atol(cmd))) {
		stream->write_function(stream, "%s\n", oset_scheduler_del_task_id(id) ? "+OK" : "-ERR No such id");
	}

	return OSET_STATUS_SUCCESS;
}

#define SQL_ESCAPE_SYNTAX "<string>"
OSET_STANDARD_API(sql_escape)
{
	if (!cmd) {
		stream->write_function(stream, "-USAGE: %s\n", SQL_ESCAPE_SYNTAX);
	} else {
		stream->write_function(stream, "%q", cmd);
	}

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(status_function)
{
	oset_core_time_duration_t duration = { 0 };
	int sps = 0, last_sps = 0, max_sps = 0, max_sps_fivemin = 0;
	int sessions_peak = 0, sessions_peak_fivemin = 0; /* Max Concurrent Sessions buffers */
	oset_bool_t html = OSET_FALSE;	/* shortcut to format.html	*/
	char * nl = "\n";					/* shortcut to format.nl	*/
	stream_format format = { 0 };
	oset_size_t cur = 0, max = 0;

	set_format(&format, stream);

	if (format.api) {
		format.html = OSET_TRUE;
		format.nl = "<br>\n";
	}

	if (format.html) {
		/* set flag to allow refresh of webpage if web request contained kv-pair refresh=xx  */
		oset_event_add_header_string(stream->param_event, OSET_STACK_BOTTOM, "HTTP-REFRESH", "true");
		if (format.api) {
			/* "Overwrite" default "api" Content-Type: text/plain */
			stream->write_function(stream, "Content-Type: text/html\r\n\r\n");
		}
	}

	html = format.html;
	nl = format.nl;

	if (html) {
		/* don't bother cli with heading and timestamp */
		stream->write_function(stream, "%sset-platform Status%s", "<h1>", "</h1>\n");
		stream->write_function(stream, "%s%s", oset_event_get_header(stream->param_event,"Event-Date-Local"), nl);
	}


	oset_core_measure_time(oset_core_uptime(), &duration);
	stream->write_function(stream,
						"UP %u year%s, %u day%s, %u hour%s, %u minute%s, %u second%s, %u millisecond%s, %u microsecond%s%s",
						duration.yr,  duration.yr  == 1 ? "" : "s", duration.day, duration.day == 1 ? "" : "s",
						duration.hr,  duration.hr  == 1 ? "" : "s", duration.min, duration.min == 1 ? "" : "s",
						duration.sec, duration.sec == 1 ? "" : "s", duration.ms , duration.ms  == 1 ? "" : "s", duration.mms,
						duration.mms == 1 ? "" : "s", nl);

	stream->write_function(stream, "%" OSET_SIZE_T_FMT " session(s) since startup%s", sset_core_session_id() - 1, nl);
	sset_core_session_ctl(SCSC_SESSIONS_PEAK, &sessions_peak);
	sset_core_session_ctl(SCSC_SESSIONS_PEAK_FIVEMIN, &sessions_peak_fivemin);
	stream->write_function(stream, "%d session(s) - peak %d, last 5min %d %s", sset_core_session_count(), sessions_peak, sessions_peak_fivemin, nl);
	sset_core_session_ctl(SCSC_LAST_SPS, &last_sps);
	sset_core_session_ctl(SCSC_SPS, &sps);
	sset_core_session_ctl(SCSC_SPS_PEAK, &max_sps);
	sset_core_session_ctl(SCSC_SPS_PEAK_FIVEMIN, &max_sps_fivemin);
	stream->write_function(stream, "%d session(s) per Sec out of max %d, peak %d, last 5min %d %s", last_sps, sps, max_sps, max_sps_fivemin, nl);
	stream->write_function(stream, "%d session(s) max%s", sset_core_session_limit(0), nl);
	stream->write_function(stream, "min idle cpu %0.2f/%0.2f%s", oset_core_min_idle_cpu(-1.0), oset_core_idle_cpu(), nl);

	if (oset_core_get_stacksizes(&cur, &max) == OSET_STATUS_SUCCESS) {		stream->write_function(stream, "Current Stack Size/Max %ldK/%ldK\n", cur / 1024, max / 1024);
	}
	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(strftime_tz_api_function)
{
	char *format = NULL;
	const char *tz_name = NULL;
	char date[80] = "";
	char *mycmd = NULL, *p;
	oset_time_t when = 0;

	if (cmd) mycmd = strdup(cmd);

	if (!zstr(mycmd)) {
		tz_name = mycmd;

		if ((format = strchr(mycmd, ' '))) {
			*format++ = '\0';

			if ((p = strchr(format, '|'))) {
				*p++ = '\0';
				when = atol(format);
				format = p;
			}
		}
	}

	if (zstr(format)) {
		format = "%Y-%m-%d %T";
	}

	if (format && oset_apr_strftime_tz(tz_name, format, date, sizeof(date), when * 1000000) == OSET_STATUS_SUCCESS) {	/* The lookup of the zone may fail. */
		stream->write_function(stream, "%s", date);
	} else {
		stream->write_function(stream, "-ERR Invalid timezone/format\n");
	}

	oset_safe_free(mycmd);

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_API(msleep_function)
{
	if (cmd) {
		long ms = atol(cmd);
		oset_yield(ms * 1000);
	}

	stream->write_function(stream, "+OK");

	return OSET_STATUS_SUCCESS;
}

#define UPTIME_SYNTAX "[us|ms|s|m|h|d|microseconds|milliseconds|seconds|minutes|hours|days]"
OSET_STANDARD_API(uptime_function)
{
	oset_time_t scale;

	if (zstr(cmd)) {
		/* default to seconds */
		scale = 1000000;
	}
	else if (!strcasecmp(cmd, "microseconds") || !strcasecmp(cmd, "us")) {
		scale = 1;
	}
	else if (!strcasecmp(cmd, "milliseconds") || !strcasecmp(cmd, "ms")) {
		scale = 1000;
	}
	else if (!strcasecmp(cmd, "seconds") || !strcasecmp(cmd, "s")) {
		scale = 1000000;
	}
	else if (!strcasecmp(cmd, "minutes") || !strcasecmp(cmd, "m")) {
		scale = 60000000;
	}
	else if (!strcasecmp(cmd, "hours") || !strcasecmp(cmd, "h")) {
		scale = 3600000000;
	}
	else if (!strcasecmp(cmd, "days") || !strcasecmp(cmd, "d")) {
		scale = 86400000000;
	}
	else {
		stream->write_function(stream, "-USAGE: %s\n", UPTIME_SYNTAX);
		return OSET_STATUS_SUCCESS;
	}

	stream->write_function(stream, "%u\n", oset_core_uptime() / scale);
	return OSET_STATUS_SUCCESS;
}

OSET_DECLARE(const char *) oset_memory_usage_stream(oset_stream_handle_t *stream)
{
	const char *status = NULL;
#ifdef __GLIBC__
/*
 *  The mallinfo2() function was added in glibc 2.33.
 *  https://man7.org/linux/man-pages/man3/mallinfo.3.html
 */
#if defined(__GLIBC_PREREQ) && __GLIBC_PREREQ(2, 33)
	struct mallinfo2 mi;

	mi = mallinfo2();

	stream->write_function(stream, "Total non-mmapped bytes (arena):       %" OSET_SIZE_T_FMT "\n", mi.arena);
	stream->write_function(stream, "# of free chunks (ordblks):            %" OSET_SIZE_T_FMT "\n", mi.ordblks);
	stream->write_function(stream, "# of free fastbin blocks (smblks):     %" OSET_SIZE_T_FMT "\n", mi.smblks);
	stream->write_function(stream, "# of mapped regions (hblks):           %" OSET_SIZE_T_FMT "\n", mi.hblks);
	stream->write_function(stream, "Bytes in mapped regions (hblkhd):      %" OSET_SIZE_T_FMT "\n", mi.hblkhd);
	stream->write_function(stream, "Max. total allocated space (usmblks):  %" OSET_SIZE_T_FMT "\n", mi.usmblks);
	stream->write_function(stream, "Free bytes held in fastbins (fsmblks): %" OSET_SIZE_T_FMT "\n", mi.fsmblks);
	stream->write_function(stream, "Total allocated space (uordblks):      %" OSET_SIZE_T_FMT "\n", mi.uordblks);
	stream->write_function(stream, "Total free space (fordblks):           %" OSET_SIZE_T_FMT "\n", mi.fordblks);
	stream->write_function(stream, "Topmost releasable block (keepcost):   %" OSET_SIZE_T_FMT "\n", mi.keepcost);
#else
	struct mallinfo mi;

	mi = mallinfo();

	stream->write_function(stream, "Total non-mmapped bytes (arena):       %u\n", mi.arena);
	stream->write_function(stream, "# of free chunks (ordblks):            %u\n", mi.ordblks);
	stream->write_function(stream, "# of free fastbin blocks (smblks):     %u\n", mi.smblks);
	stream->write_function(stream, "# of mapped regions (hblks):           %u\n", mi.hblks);
	stream->write_function(stream, "Bytes in mapped regions (hblkhd):      %u\n", mi.hblkhd);
	stream->write_function(stream, "Max. total allocated space (usmblks):  %u\n", mi.usmblks);
	stream->write_function(stream, "Free bytes held in fastbins (fsmblks): %u\n", mi.fsmblks);
	stream->write_function(stream, "Total allocated space (uordblks):      %u\n", mi.uordblks);
	stream->write_function(stream, "Total free space (fordblks):           %u\n", mi.fordblks);
	stream->write_function(stream, "Topmost releasable block (keepcost):   %u\n", mi.keepcost);

#endif

	oset_goto_status(NULL, done);
#else
#ifdef WIN32
	/* Based on: https://docs.microsoft.com/en-us/windows/win32/memory/enumerating-a-heap and https://docs.microsoft.com/en-us/windows/win32/memory/getting-process-heaps */
	PHANDLE aHeaps;
	SIZE_T BytesToAllocate;
	DWORD HeapsIndex;
	DWORD HeapsLength;
	DWORD NumberOfHeaps;
	HRESULT Result;
	HANDLE hDefaultProcessHeap;
	size_t CommittedSizeTotal = 0;
	size_t UnCommittedSizeTotal = 0;
	size_t SizeTotal = 0;
	size_t OverheadTotal = 0;

	NumberOfHeaps = GetProcessHeaps(0, NULL);
	Result = SIZETMult(NumberOfHeaps, sizeof(*aHeaps), &BytesToAllocate);
	if (Result != S_OK) {
		oset_goto_status("SIZETMult failed.", done);
	}

	hDefaultProcessHeap = GetProcessHeap();
	if (hDefaultProcessHeap == NULL) {
		oset_goto_status("Failed to retrieve the default process heap", done);
	}

	aHeaps = (PHANDLE)HeapAlloc(hDefaultProcessHeap, 0, BytesToAllocate);
	if (aHeaps == NULL) {
		oset_goto_status("HeapAlloc failed to allocate space for heaps", done);
	}

	HeapsLength = NumberOfHeaps;
	NumberOfHeaps = GetProcessHeaps(HeapsLength, aHeaps);

	if (NumberOfHeaps == 0) {
		oset_goto_status("Failed to retrieve heaps", cleanup);
	} else if (NumberOfHeaps > HeapsLength) {
		/*
		 * Compare the latest number of heaps with the original number of heaps.
		 * If the latest number is larger than the original number, another
		 * component has created a new heap and the buffer is too small.
		 */
		oset_goto_status("Another component created a heap between calls.", cleanup);
	}

	stream->write_function(stream, "Process has %d heaps.\n", HeapsLength);
	for (HeapsIndex = 0; HeapsIndex < HeapsLength; ++HeapsIndex) {
		PROCESS_HEAP_ENTRY Entry;
		HANDLE hHeap = aHeaps[HeapsIndex];

		stream->write_function(stream, "Heap %d at address: %#p.\n", HeapsIndex, aHeaps[HeapsIndex]);

		/* Lock the heap to prevent other threads from accessing the heap during enumeration. */
		if (HeapLock(hHeap) == FALSE) {
			oset_goto_status("Failed to lock heap.", cleanup);
		}

		Entry.lpData = NULL;
		while (HeapWalk(hHeap, &Entry) != FALSE) {
			if ((Entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0) {
			} else if ((Entry.wFlags & PROCESS_HEAP_REGION) != 0) {
				CommittedSizeTotal += Entry.Region.dwCommittedSize;
				UnCommittedSizeTotal += Entry.Region.dwUnCommittedSize;
			}

			SizeTotal += Entry.cbData;
			OverheadTotal += Entry.cbOverhead;
		}

		/* Unlock the heap to allow other threads to access the heap after enumeration has completed. */
		if (HeapUnlock(hHeap) == FALSE) {
			abort();
		}
	}

	stream->write_function(stream, "Committed bytes:   %" OSET_SIZE_T_FMT "\n", CommittedSizeTotal);
	stream->write_function(stream, "Uncommited bytes:  %" OSET_SIZE_T_FMT "\n", UnCommittedSizeTotal);
	stream->write_function(stream, "Size:              %" OSET_SIZE_T_FMT "\n", SizeTotal);
	stream->write_function(stream, "Overhead:          %" OSET_SIZE_T_FMT"\n", OverheadTotal);

cleanup:
	HeapFree(hDefaultProcessHeap, 0, aHeaps);
#else
	oset_goto_status("Memory usage statistics is not implemented on the current platform.", done);
#endif
#endif
done:
	return status;
}


OSET_STANDARD_API(memory_function)
{
	const char *err;
	if (!(err = oset_memory_usage_stream(stream))) {
		stream->write_function(stream, "+OK\n");
	} else {
		stream->write_function(stream, "-ERR %s\n", err);
	}

	return OSET_STATUS_SUCCESS;
}


OSET_STANDARD_API(json_function)
{
	cJSON *jcmd = NULL, *format = NULL;
	const char *message = "";
	char *response = NULL;

	if (zstr(cmd)) {
		message = "No JSON supplied.";
		goto err;
	}

	jcmd = cJSON_Parse(cmd);

	if (!jcmd) {
		message = "Parse error.";
		goto err;
	}

	format = cJSON_GetObjectItem(jcmd, "format");

	oset_json_api_execute(jcmd, session, NULL);


	if (format && format->valuestring && !strcasecmp(format->valuestring, "pretty")) {
		response = cJSON_Print(jcmd);
	} else {
		response = cJSON_PrintUnformatted(jcmd);
	}

	stream->write_function(stream, "%s\n", oset_str_nil(response));

	oset_safe_free(response);

	cJSON_Delete(jcmd);

	return OSET_STATUS_SUCCESS;

 err:

	stream->write_function(stream, "-ERR %s\n", message);


	return OSET_STATUS_SUCCESS;
}

#define UUID_LOGLEVEL_SYNTAX "<uuid> <level>"
OSET_STANDARD_API(uuid_loglevel)
{
	oset_core_session_t *tsession = NULL;
	char *uuid = NULL, *text = NULL;
	int b = 0;

	if (!zstr(cmd) && (uuid = strdup(cmd))) {
		if ((text = strchr(uuid, ' '))) {
			*text++ = '\0';

			if (!strncasecmp(text, "-b", 2)) {
				b++;
				if ((text = strchr(text, ' '))) {
					*text++ = '\0';
				}
			}
		}
	}

	if (zstr(uuid) || zstr(text)) {
		stream->write_function(stream, "-USAGE: %s\n", UUID_LOGLEVEL_SYNTAX);
	} else {
		oset_log2_level_t level = oset_log2_str2level(text);

		if (level == OSET_LOG2_INVALID) {
			stream->write_function(stream, "-ERR Invalid log level!\n");
		} else if ((tsession = oset_core_session_locate(uuid))) {

			oset_core_session_set_loglevel(tsession, level);

			stream->write_function(stream, "+OK\n");
		} else {
			stream->write_function(stream, "-ERR No such session %s!\n", uuid);
		}
	}

	oset_safe_free(uuid);
	return OSET_STATUS_SUCCESS;
}


OSET_STANDARD_JSON_API(json_status_function)
{
	cJSON *o, *oo, *reply = cJSON_CreateObject();
	oset_core_time_duration_t duration = { 0 };
	int sps = 0, last_sps = 0, max_sps = 0, max_sps_fivemin = 0;
	int sessions_peak = 0, sessions_peak_fivemin = 0; /* Max Concurrent Sessions buffers */
	oset_size_t cur = 0, max = 0;

	oset_core_measure_time(oset_core_uptime(), &duration);

	sset_core_session_ctl(SCSC_SESSIONS_PEAK, &sessions_peak);
	sset_core_session_ctl(SCSC_SESSIONS_PEAK_FIVEMIN, &sessions_peak_fivemin);
	sset_core_session_ctl(SCSC_LAST_SPS, &last_sps);
	sset_core_session_ctl(SCSC_SPS, &sps);
	sset_core_session_ctl(SCSC_SPS_PEAK, &max_sps);
	sset_core_session_ctl(SCSC_SPS_PEAK_FIVEMIN, &max_sps_fivemin);

	cJSON_AddItemToObject(reply, "systemStatus",  cJSON_CreateString(oset_test_flag((&runtime), SCF_SHUTTING_DOWN) ? "ready" : "not ready"));

	o = cJSON_CreateObject();
	cJSON_AddItemToObject(o, "years", cJSON_CreateNumber(duration.yr));
	cJSON_AddItemToObject(o, "days", cJSON_CreateNumber(duration.day));
	cJSON_AddItemToObject(o, "hours", cJSON_CreateNumber(duration.hr));
	cJSON_AddItemToObject(o, "minutes", cJSON_CreateNumber(duration.min));
	cJSON_AddItemToObject(o, "seconds", cJSON_CreateNumber(duration.sec));
	cJSON_AddItemToObject(o, "milliseconds", cJSON_CreateNumber(duration.ms));
	cJSON_AddItemToObject(o, "microseconds", cJSON_CreateNumber(duration.mms));

	cJSON_AddItemToObject(reply, "uptime", o);
	cJSON_AddItemToObject(reply, "version", cJSON_CreateString(SSET_VERSION));

	o = cJSON_CreateObject();
	cJSON_AddItemToObject(reply, "sessions", o);

	oo = cJSON_CreateObject();
	cJSON_AddItemToObject(o, "count", oo);

	cJSON_AddItemToObject(oo, "total", cJSON_CreateNumber((double)(sset_core_session_id() - 1)));
	cJSON_AddItemToObject(oo, "active", cJSON_CreateNumber(sset_core_session_count()));
	cJSON_AddItemToObject(oo, "peak", cJSON_CreateNumber(sessions_peak));
	cJSON_AddItemToObject(oo, "peak5Min", cJSON_CreateNumber(sessions_peak_fivemin));
	cJSON_AddItemToObject(oo, "limit", cJSON_CreateNumber(sset_core_session_limit(0)));



	oo = cJSON_CreateObject();
	cJSON_AddItemToObject(o, "rate", oo);
	cJSON_AddItemToObject(oo, "current", cJSON_CreateNumber(last_sps));
	cJSON_AddItemToObject(oo, "max", cJSON_CreateNumber(sps));
	cJSON_AddItemToObject(oo, "peak", cJSON_CreateNumber(max_sps));
	cJSON_AddItemToObject(oo, "peak5Min", cJSON_CreateNumber(max_sps_fivemin));


	o = cJSON_CreateObject();
	cJSON_AddItemToObject(reply, "idleCPU", o);

	cJSON_AddItemToObject(o, "used", cJSON_CreateNumber(oset_core_min_idle_cpu(-1.0)));
	cJSON_AddItemToObject(o, "allowed", cJSON_CreateNumber(oset_core_idle_cpu()));


	if (oset_core_get_stacksizes(&cur, &max) == OSET_STATUS_SUCCESS) {
		o = cJSON_CreateObject();
		cJSON_AddItemToObject(reply, "stackSizeKB", o);

		cJSON_AddItemToObject(o, "current", cJSON_CreateNumber((double)(cur / 1024)));
		cJSON_AddItemToObject(o, "max", cJSON_CreateNumber((double)(max / 1024)));
	}


	*json_reply = reply;

	return OSET_STATUS_SUCCESS;
}

OSET_STANDARD_JSON_API(json_api_function)
{
	cJSON *data, *cmd, *arg, *reply;
	oset_stream_handle_t stream = { 0 };
	oset_status_t status = OSET_STATUS_SUCCESS;

	data = cJSON_GetObjectItem(json, "data");

	cmd = cJSON_GetObjectItem(data, "cmd");
	arg = cJSON_GetObjectItem(data, "arg");

	if (cmd && !cmd->valuestring) {
		cmd = NULL;
	}

	if (arg && !arg->valuestring) {
		arg = NULL;
	}

	reply = cJSON_CreateObject();

	OSET_STANDARD_STREAM(stream);

	if (cmd && (status = oset_api_execute(cmd->valuestring, arg ? arg->valuestring : NULL, session, &stream)) == OSET_STATUS_SUCCESS) {
		cJSON_AddItemToObject(reply, "message", cJSON_CreateString((char *) stream.data));
	} else {
		cJSON_AddItemToObject(reply, "message", cJSON_CreateString("INVALID CALL"));
	}

	oset_safe_free(stream.data);

	*json_reply = reply;

	return status;

}


OSET_STANDARD_API(show_pkbuf_status)
{
	oset_default_pkbuf_static();

	stream->write_function(stream, "+OK\n");
	return OSET_STATUS_SUCCESS;
}


OSET_STANDARD_API(acl_function)
{
	int argc;
	char *mydata = NULL, *argv[3];

	if (!cmd) {
		goto error;
	}

	mydata = strdup(cmd);
	oset_sys_assert(mydata);

	argc = oset_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));

	if (argc < 1) {
		stream->write_function(stream, "%s", "acl <ip>\n");
		goto error;
	}

	if (sset_insert_auth_network_list_ip(argv[0], "authorization.auto")) {
		goto ok;
	}

  error:

	stream->write_function(stream, "false\n");

  ok:

	oset_safe_free(mydata);

	return OSET_STATUS_SUCCESS;
}


OSET_MODULE_LOAD_FUNCTION(mod_api_load)
{
	oset_api_interface_t *commands_api_interface;
	oset_json_api_interface_t *json_api_interface;
	int use_system_commands = 1;

	if (oset_true(oset_core_get_variable("disable_system_api_commands"))) {
		use_system_commands = 0;
	}

	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

	oset_apr_thread_rwlock_create(&bgapi_rwlock, pool);
	oset_apr_mutex_init(&reload_mutex, OSET_MUTEX_NESTED, pool);

	if (use_system_commands) {
		OSET_ADD_API(commands_api_interface, "bg_system", "Execute a system command in the background", bg_system_function, SYSTEM_SYNTAX);
		OSET_ADD_API(commands_api_interface, "system", "Execute a system command", system_function, SYSTEM_SYNTAX);
		OSET_ADD_API(commands_api_interface, "bg_spawn", "Execute a spawn command in the background", bg_spawn_function, SPAWN_SYNTAX);
		OSET_ADD_API(commands_api_interface, "spawn", "Execute a spawn command without capturing it's output", spawn_function, SPAWN_SYNTAX);
		OSET_ADD_API(commands_api_interface, "spawn_stream", "Execute a spawn command and capture it's output", spawn_stream_function, SPAWN_SYNTAX);
	}

	OSET_ADD_API(commands_api_interface, "acl", "Compare an ip to an acl list", acl_function, "<ip>");

	OSET_ADD_API(commands_api_interface, "show", "Show various reports", show_function, SHOW_SYNTAX);
	OSET_ADD_API(commands_api_interface, "pkbuf", "Show pkbuf status", show_pkbuf_status, "");

	OSET_ADD_API(commands_api_interface, "...", "Shutdown", shutdown_function, "");
	OSET_ADD_API(commands_api_interface, "shutdown", "Shutdown", shutdown_function, "");
	OSET_ADD_API(commands_api_interface, "version", "Version", version_function, "");
	OSET_ADD_API(commands_api_interface, "help", "Show help for all the api commands", help_function, "");
	OSET_ADD_API(commands_api_interface, "load", "Load Module", load_function, LOAD_SYNTAX);
	OSET_ADD_API(commands_api_interface, "unload", "Unload module", unload_function, UNLOAD_SYNTAX);
	OSET_ADD_API(commands_api_interface, "reload", "Reload module", reload_function, UNLOAD_SYNTAX);
	OSET_ADD_API(commands_api_interface, "reloadxml", "Reload XML", reload_xml_function, "");
	OSET_ADD_API(commands_api_interface, "module_exists", "Check if module exists", module_exists_function, "<module>");
	OSET_ADD_API(commands_api_interface, "status", "Show current status", status_function, "");
	OSET_ADD_API(commands_api_interface, "memory", "Memory usage statistics", memory_function, "memory");
	OSET_ADD_API(commands_api_interface, "sched_api", "Schedule an api command", sched_api_function, SCHED_SYNTAX);
	OSET_ADD_API(commands_api_interface, "sched_del", "Delete a scheduled task", sched_del_function, "<task_id>|<group_id>");
	OSET_ADD_API(commands_api_interface, "unsched_api", "Unschedule an api command", unsched_api_function, UNSCHED_SYNTAX);
	OSET_ADD_API(commands_api_interface, "bgapi", "Execute an api command in a thread", bgapi_function, "<command>[ <arg>]");
	OSET_ADD_API(commands_api_interface, "echo", "Echo", echo_function, "<data>");
	OSET_ADD_API(commands_api_interface, "console_complete", "", console_complete_function, "<line>");
    OSET_ADD_API(commands_api_interface, "console_complete_xml", "", console_complete_xml_function, "<line>");


	OSET_ADD_API(commands_api_interface, "log", "Log", log_function, LOG_SYNTAX);

	OSET_ADD_API(commands_api_interface, "escape", "Escape a string", escape_function, "<data>");
	OSET_ADD_API(commands_api_interface, "sql_escape", "Escape a string to prevent sql injection", sql_escape, SQL_ESCAPE_SYNTAX);
	OSET_ADD_API(commands_api_interface, "hostname", "Return the system hostname", hostname_api_function, "");
	OSET_ADD_API(commands_api_interface, "osetname", "Return the oset name", osetname_api_function, "");
	OSET_ADD_API(commands_api_interface, "gethost", "gethostbyname", gethost_api_function, "");
	OSET_ADD_API(commands_api_interface, "getenv", "getenv", getenv_function, GETENV_SYNTAX);
	OSET_ADD_API(commands_api_interface, "alias", "Alias", alias_function, ALIAS_SYNTAX);	
	OSET_ADD_API(commands_api_interface, "banner", "Return the system banner", banner_function, "");
	OSET_ADD_API(commands_api_interface, "complete", "Complete", complete_function, COMPLETE_SYNTAX);
	OSET_ADD_API(commands_api_interface, "db_cache", "Manage db cache", db_cache_function, "status");
	OSET_ADD_API(commands_api_interface, "strftime_tz", "Display formatted time of timezone", strftime_tz_api_function, "<timezone_name> [<epoch>|][format string]");
	OSET_ADD_API(commands_api_interface, "msleep", "Sleep N milliseconds", msleep_function, "<milliseconds>");
	OSET_ADD_API(commands_api_interface, "uptime", "Show uptime", uptime_function, UPTIME_SYNTAX);
	OSET_ADD_API(commands_api_interface, "json", "JSON API", json_function, "JSON");

	OSET_ADD_API(commands_api_interface, "uuid_loglevel", "Set loglevel on session", uuid_loglevel, UUID_LOGLEVEL_SYNTAX);

	OSET_ADD_JSON_API(json_api_interface, "status", "JSON status API", json_status_function, "");
	OSET_ADD_JSON_API(json_api_interface, "fsapi", "JSON FSAPI Gateway", json_api_function, "");

	oset_console_set_complete("add alias add");
	oset_console_set_complete("add alias stickyadd");
	oset_console_set_complete("add alias del");
	oset_console_set_complete("add complete add");
	oset_console_set_complete("add complete del");
	oset_console_set_complete("add db_cache status");
	oset_console_set_complete("add load ::console::list_available_modules");
	oset_console_set_complete("add reload ::console::list_loaded_modules");
	oset_console_set_complete("add reloadxml");
	oset_console_set_complete("add show aliases");
	oset_console_set_complete("add show api");
	oset_console_set_complete("add show application");
	oset_console_set_complete("add show complete");
	oset_console_set_complete("add show interfaces");
	oset_console_set_complete("add show interface_types");
	oset_console_set_complete("add show tasks");
	oset_console_set_complete("add show modules");
	oset_console_set_complete("add show status");
	oset_console_set_complete("add show timer");
	oset_console_set_complete("add shutdown");
	oset_console_set_complete("add sql_escape");
	oset_console_set_complete("add unload ::console::list_loaded_modules");
	oset_console_set_complete("add uptime ms");
	oset_console_set_complete("add uptime s");
	oset_console_set_complete("add uptime m");
	oset_console_set_complete("add uptime h");
	oset_console_set_complete("add uptime d");
	oset_console_set_complete("add uptime microseconds");
	oset_console_set_complete("add uptime milliseconds");
	oset_console_set_complete("add uptime seconds");
	oset_console_set_complete("add uptime minutes");
	oset_console_set_complete("add uptime hours");
	oset_console_set_complete("add uptime days");
	oset_console_set_complete("add version");
	oset_console_set_complete("add ...");
	oset_console_set_complete("add getcputime");

	oset_console_set_complete("add uuid_loglevel ::console::list_uuid console");
	oset_console_set_complete("add uuid_loglevel ::console::list_uuid alert");
	oset_console_set_complete("add uuid_loglevel ::console::list_uuid crit");
	oset_console_set_complete("add uuid_loglevel ::console::list_uuid err");
	oset_console_set_complete("add uuid_loglevel ::console::list_uuid warning");
	oset_console_set_complete("add uuid_loglevel ::console::list_uuid notice");
	oset_console_set_complete("add uuid_loglevel ::console::list_uuid info");
	oset_console_set_complete("add uuid_loglevel ::console::list_uuid debug");

	/* indicate that the module should continue to be loaded */
	return OSET_STATUS_NOUNLOAD;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_api_shutdown)
{
	int x;

	for (x = 30; x > 0; x--) {
		if (oset_apr_thread_rwlock_trywrlock(bgapi_rwlock) == OSET_STATUS_SUCCESS) {
			oset_apr_thread_rwlock_unlock(bgapi_rwlock);
			break;
		}
		if (x == 30) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "Waiting for bgapi threads.");
		}
		oset_yield(1000000);
	}

	if (!x) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Giving up waiting for bgapi threads.");
	}

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
