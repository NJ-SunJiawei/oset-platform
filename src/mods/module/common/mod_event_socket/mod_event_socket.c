/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.05
************************************************************************/
#include "oset-core.h"
#include "sset-core.h"

#define CMD_BUFLEN 1024 * 1000
#define MAX_QUEUE_LEN 100000
#define MAX_MISSED 500
OSET_MODULE_LOAD_FUNCTION(mod_event_socket_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_event_socket_shutdown);
OSET_MODULE_RUNTIME_FUNCTION(mod_event_socket_runtime);
OSET_MODULE_DEFINITION(libmod_event_socket, mod_event_socket_load, mod_event_socket_shutdown, mod_event_socket_runtime);

static char *MARKER = "1";

typedef enum {
	LFLAG_AUTHED = (1 << 0),
	LFLAG_RUNNING = (1 << 1),
	LFLAG_EVENTS = (1 << 2),
	LFLAG_LOG = (1 << 3),
	LFLAG_FULL = (1 << 4),
	LFLAG_MYEVENTS = (1 << 5),
	LFLAG_SESSION = (1 << 6),
	LFLAG_ASYNC = (1 << 7),
	LFLAG_STATEFUL = (1 << 8),
	LFLAG_OUTBOUND = (1 << 9),
	LFLAG_LINGER = (1 << 10),
	LFLAG_HANDLE_DISCO = (1 << 11),
	LFLAG_CONNECTED = (1 << 12),
	LFLAG_RESUME = (1 << 13),
	LFLAG_AUTH_EVENTS = (1 << 14),
	LFLAG_ALL_EVENTS_AUTHED = (1 << 15),
	LFLAG_ALLOW_LOG = (1 << 16)
} event_flag_t;

typedef enum {
	EVENT_FORMAT_PLAIN,
	EVENT_FORMAT_XML,
	EVENT_FORMAT_JSON
} event_format_t;

struct listener {
	oset_apr_socket_t *sock;
	oset_apr_queue_t *event_queue;
	oset_apr_queue_t *log_queue;
	oset_apr_memory_pool_t *pool;
	event_format_t format;
	oset_apr_mutex_t *flag_mutex;
	oset_apr_mutex_t *filter_mutex;
	uint32_t flags;
	oset_log2_level_t level;
	char *ebuf;
	uint8_t event_list[OSET_EVENT_ALL + 1];
	uint8_t allowed_event_list[OSET_EVENT_ALL + 1];
	oset_hashtable_t *event_hash;
	oset_hashtable_t *allowed_event_hash;
	oset_hashtable_t *allowed_api_hash;
	oset_apr_thread_rwlock_t *rwlock;
	oset_core_session_t *session;
	int lost_events;
	int lost_logs;
	time_t last_flush;
	time_t expire_time;
	uint32_t timeout;
	uint32_t id;
	oset_apr_sockaddr_t *sa;
	char remote_ip[50];
	oset_port_t remote_port;
	oset_event_t *filters;
	time_t linger_timeout;
	struct listener *next;
	oset_apr_pollfd_t *pollfd;
	uint8_t lock_acquired;
	uint8_t finished;
};

typedef struct listener listener_t;

static struct {
	oset_apr_mutex_t *listener_mutex;
	oset_event_node_t *node;
	int debug;
} globals;

static struct {
	oset_apr_socket_t *sock;
	oset_apr_mutex_t *sock_mutex;
	listener_t *listeners;
	uint8_t ready;
} listen_list;

#define MAX_ACL 100

static struct {
	oset_apr_mutex_t *mutex;
	char *ip;
	uint16_t port;
	char *password;
	int done;
	int threads;
	char *acl[MAX_ACL];
	uint32_t acl_count;
	uint32_t id;
	int stop_on_bind_error;
} prefs;


static const char *format2str(event_format_t format)
{
	switch (format) {
	case EVENT_FORMAT_PLAIN:
		return "plain";
	case EVENT_FORMAT_XML:
		return "xml";
	case EVENT_FORMAT_JSON:
		return "json";
	}

	return "invalid";
}

static void remove_listener(listener_t *listener);
static void kill_listener(listener_t *l, const char *message);
static void kill_all_listeners(void);

static uint32_t next_id(void)
{
	uint32_t id;
	oset_apr_mutex_lock(globals.listener_mutex);
	id = ++prefs.id;
	oset_apr_mutex_unlock(globals.listener_mutex);
	return id;
}

OSET_DECLARE_GLOBAL_STRING_FUNC(set_pref_ip, prefs.ip);
OSET_DECLARE_GLOBAL_STRING_FUNC(set_pref_pass, prefs.password);

static void *OSET_THREAD_FUNC listener_run(oset_apr_thread_t *thread, void *obj);
static oset_status_t launch_listener_thread(listener_t *listener);

static oset_status_t socket_logger(const oset_log2_node_t *node, oset_log2_level_t level)
{
	listener_t *l;
	oset_status_t qstatus;
	oset_apr_mutex_lock(globals.listener_mutex);
	for (l = listen_list.listeners; l; l = l->next) {
		if (oset_test_flag(l, LFLAG_LOG) && l->level >= node->level) {
			oset_log2_node_t *dnode = oset_log2_node_dup(node);
			qstatus = oset_apr_queue_trypush(l->log_queue, dnode); 
			if (qstatus == OSET_STATUS_SUCCESS) {
				if (l->lost_logs) {
					int ll = l->lost_logs;
					l->lost_logs = 0;
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Lost [%d] log lines! Log Queue size: [%u/%u]", ll, oset_apr_queue_size(l->log_queue), MAX_QUEUE_LEN);
				}
			} else {
				char errbuf[512] = {0};
				unsigned int qsize = oset_apr_queue_size(l->log_queue);
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, 
						"Log enqueue ERROR [%d] | [%s] Queue size: [%u/%u] %s", 
						(int)qstatus, oset_apr_strerror(qstatus, errbuf, sizeof(errbuf)), qsize, MAX_QUEUE_LEN, (qsize == MAX_QUEUE_LEN)?"Max queue size reached":"");
				oset_log2_node_free(&dnode);
				if (++l->lost_logs > MAX_MISSED) {
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, 
							"Killing listener because of too many lost log lines. Lost [%d] Queue size [%u/%u]!", l->lost_logs, qsize, MAX_QUEUE_LEN);
					kill_listener(l, "killed listener because of lost log lines");
				}
			}
		}
	}
	oset_apr_mutex_unlock(globals.listener_mutex);

	return OSET_STATUS_SUCCESS;
}

static void flush_listener(listener_t *listener, oset_bool_t flush_log, oset_bool_t flush_events)
{
	void *pop;

	if (flush_log && listener->log_queue) {
		while (oset_apr_queue_trypop(listener->log_queue, &pop) == OSET_STATUS_SUCCESS) {
			oset_log2_node_t *dnode = (oset_log2_node_t *) pop;
			if (dnode) {
				oset_log2_node_free(&dnode);
			}
		}
	}

	if (flush_events && listener->event_queue) {
		while (oset_apr_queue_trypop(listener->event_queue, &pop) == OSET_STATUS_SUCCESS) {
			oset_event_t *pevent = (oset_event_t *) pop;
			if (!pop)
				continue;
			oset_event_destroy(&pevent);
		}
	}
}

static oset_status_t expire_listener(listener_t ** listener)
{
	listener_t *l;

	if (!listener || !*listener)
		return OSET_STATUS_FALSE;
	l = *listener;

	if (!l->expire_time) {
		l->expire_time = oset_epoch_time_now(NULL);
	}

	if (oset_apr_thread_rwlock_trywrlock(l->rwlock) != OSET_STATUS_SUCCESS) {
		return OSET_STATUS_FALSE;
	}

	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(l->session), OSET_LOG2_CRIT, "Stateful Listener %u has expired", l->id);

	flush_listener(*listener, OSET_TRUE, OSET_TRUE);
	oset_core_hash_destroy(&l->event_hash);

	if (l->allowed_event_hash) {
		oset_core_hash_destroy(&l->allowed_event_hash);
	}

	if (l->allowed_api_hash) {
		oset_core_hash_destroy(&l->allowed_api_hash);
	}


	oset_apr_mutex_lock(l->filter_mutex);
	if (l->filters) {
		oset_event_destroy(&l->filters);
	}

	oset_apr_mutex_unlock(l->filter_mutex);
	oset_apr_thread_rwlock_unlock(l->rwlock);
	oset_core_destroy_memory_pool(&l->pool);

	*listener = NULL;
	return OSET_STATUS_SUCCESS;
}

static void event_handler(oset_event_t *event)
{
	oset_event_t *clone = NULL;
	listener_t *l, *lp, *last = NULL;
	time_t now = oset_epoch_time_now(NULL);
	oset_status_t qstatus;

	oset_sys_assert(event != NULL);

	if (!listen_list.ready) {
		return;
	}

	oset_apr_mutex_lock(globals.listener_mutex);

	lp = listen_list.listeners;

	while (lp) {
		int send = 0;

		l = lp;
		lp = lp->next;

		if (oset_test_flag(l, LFLAG_STATEFUL) && (l->expire_time || (l->timeout && now - l->last_flush > l->timeout))) {
			if (expire_listener(&l) == OSET_STATUS_SUCCESS) {
				if (last) {
					last->next = lp;
				} else {
					listen_list.listeners = lp;
				}
				continue;
			}
		}

		if (l->expire_time || !oset_test_flag(l, LFLAG_EVENTS)) {
			last = l;
			continue;
		}

		if (l->event_list[OSET_EVENT_ALL]) {
			send = 1;
		} else if ((l->event_list[event->event_id])) {
			if (event->event_id != OSET_EVENT_CUSTOM || !event->subclass_name || (oset_core_hash_find(l->event_hash, event->subclass_name))) {
				send = 1;
			}
		}

		if (send) {
			oset_apr_mutex_lock(l->filter_mutex);

			if (l->filters && l->filters->headers) {
				oset_event_header_t *hp;
				const char *hval;

				send = 0;

				for (hp = l->filters->headers; hp; hp = hp->next) {
					if ((hval = oset_event_get_header(event, hp->name))) {
						const char *comp_to = hp->value;
						int pos = 1, cmp = 0;

						while (comp_to && *comp_to) {
							if (*comp_to == '+') {
								pos = 1;
							} else if (*comp_to == '-') {
								pos = 0;
							} else if (*comp_to != ' ') {
								break;
							}
							comp_to++;
						}

						if (send && pos) {
							continue;
						}

						if (!comp_to) {
							continue;
						}

						if (*hp->value == '/') {
							oset_regex_t *re = NULL;
							int ovector[30];
							cmp = !!oset_regex_perform(hval, comp_to, &re, ovector, sizeof(ovector) / sizeof(ovector[0]));
							oset_regex_safe_free(re);
						} else {
							cmp = !strcasecmp(hval, comp_to);
						}

						if (cmp) {
							if (pos) {
								send = 1;
							} else {
								send = 0;
								break;
							}
						}
					}
				}
			}

			oset_apr_mutex_unlock(l->filter_mutex);
		}

		if (send && oset_test_flag(l, LFLAG_MYEVENTS)) {
			char *uuid = oset_event_get_header(event, "unique-id");
			if (!uuid || (l->session && strcmp(uuid, oset_core_session_get_uuid(l->session)))) {
				send = 0;
			}
			if (!strcmp(oset_core_session_get_uuid(l->session), oset_event_get_header_nil(event, "Job-Owner-UUID"))) {
			    send = 1;
			}
		}

		if (send) {
			if (oset_event_dup(&clone, event) == OSET_STATUS_SUCCESS) {
				qstatus = oset_apr_queue_trypush(l->event_queue, clone); 
				if (qstatus == OSET_STATUS_SUCCESS) {
					if (l->lost_events) {
						int le = l->lost_events;
						l->lost_events = 0;
						oset_log2_printf(OSET_CHANNEL_SESSION_LOG(l->session), OSET_LOG2_CRIT, "Lost [%d] events! Event Queue size: [%u/%u]", le, oset_apr_queue_size(l->event_queue), MAX_QUEUE_LEN);
					}
				} else {
					char errbuf[512] = {0};
					unsigned int qsize = oset_apr_queue_size(l->event_queue);
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, 
							"Event enqueue ERROR [%d] | [%s] | Queue size: [%u/%u] %s", 
							(int)qstatus, oset_apr_strerror(qstatus, errbuf, sizeof(errbuf)), qsize, MAX_QUEUE_LEN, (qsize == MAX_QUEUE_LEN)?"Max queue size reached":"");
					if (++l->lost_events > MAX_MISSED) {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Killing listener because of too many lost events. Lost [%d] Queue size[%u/%u]", l->lost_events, qsize, MAX_QUEUE_LEN);
						kill_listener(l, "killed listener because of lost events");
					}
					oset_event_destroy(&clone);
				}
			} else {
				oset_log2_printf(OSET_CHANNEL_SESSION_LOG(l->session), OSET_LOG2_ERROR, "Memory Error!");
			}
		}
		last = l;
	}
	oset_apr_mutex_unlock(globals.listener_mutex);
}

OSET_STANDARD_APP(socket_function)
{
	char *host, *port_name, *path;
	oset_apr_socket_t *new_sock = NULL;
	oset_apr_sockaddr_t *sa;
	oset_port_t port = 8084;
	listener_t *listener;
	unsigned int argc = 0, x = 0;
	char *argv[80] = { 0 };
	char *hosts[50] = { 0 };
	unsigned int hosts_count = 0;
	oset_status_t connected = OSET_STATUS_FALSE;
	char *mydata;
	char errbuf[512] = {0};


	if (data && (mydata = oset_core_session_strdup(session, data))) {
		argc = oset_separate_string(mydata, ' ', argv, (sizeof(argv) / sizeof(argv[0])));
	}

	if (argc < 1) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Parse Error!");
		return;
	}

	hosts_count = oset_split(argv[0], '|', hosts);

	for(x = 0; x < hosts_count; x++) {
		host = hosts[x];

		if (zstr(host)) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Missing Host!");
			continue;
		}

		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_NOTICE, "Trying host: %s", host);

		if ((port_name = strrchr(host, ':'))) {
			*port_name++ = '\0';
			port = (oset_port_t) atoi(port_name);
		}

		if ((path = strchr((port_name ? port_name : host), '/'))) {
			*path++ = '\0';
			oset_session_set_variable(session, "socket_path", path);
		}

		oset_session_set_variable(session, "socket_host", host);

		if (oset_apr_sockaddr_info_get(&sa, host, OSET_UNSPEC, port, 0, oset_core_session_get_pool(session)) != OSET_STATUS_SUCCESS) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Socket Error!");
			continue;
		}

		if (oset_apr_socket_create(&new_sock, oset_apr_sockaddr_get_family(sa), SOCK_STREAM, OSET_PROTO_TCP, oset_core_session_get_pool(session))
				!= OSET_STATUS_SUCCESS) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Socket Error!");
			continue;
		}

		oset_apr_socket_opt_set(new_sock, OSET_SO_KEEPALIVE, 1);
		oset_apr_socket_opt_set(new_sock, OSET_SO_TCP_NODELAY, 1);
		oset_apr_socket_opt_set(new_sock, OSET_SO_TCP_KEEPIDLE, 30);
		oset_apr_socket_opt_set(new_sock, OSET_SO_TCP_KEEPINTVL, 30);

		if ((connected = oset_apr_socket_connect(new_sock, sa)) == OSET_STATUS_SUCCESS) {
			break;
		}

		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Socket Error: %s", oset_apr_strerror(errno, errbuf, sizeof(errbuf)));
	}//end hosts loop

	if (connected != OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Socket Error!");
		return;
	}

	if (!(listener = oset_core_session_alloc(session, sizeof(*listener)))) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Memory Error");
		return;
	}

	oset_apr_thread_rwlock_create(&listener->rwlock, oset_core_session_get_pool(session));
	oset_apr_queue_create(&listener->event_queue, MAX_QUEUE_LEN, oset_core_session_get_pool(session));
	oset_apr_queue_create(&listener->log_queue, MAX_QUEUE_LEN, oset_core_session_get_pool(session));

	listener->sock = new_sock;
	listener->pool = oset_core_session_get_pool(session);
	listener->format = EVENT_FORMAT_PLAIN;
	listener->session = session;
	oset_set_flag(listener, LFLAG_ALLOW_LOG);

	oset_apr_socket_create_pollset(&listener->pollfd, listener->sock, OSET_APR_POLLIN | OSET_APR_POLLERR, listener->pool);

	oset_apr_mutex_init(&listener->flag_mutex, OSET_MUTEX_NESTED, listener->pool);
	oset_apr_mutex_init(&listener->filter_mutex, OSET_MUTEX_NESTED, listener->pool);

	oset_core_hash_init(&listener->event_hash);
	oset_set_flag(listener, LFLAG_AUTHED);
	oset_set_flag(listener, LFLAG_OUTBOUND);
	for (x = 1; x < argc; x++) {
		if (argv[x] && !strcasecmp(argv[x], "full")) {
			oset_set_flag(listener, LFLAG_FULL);
		} else if (argv[x] && !strcasecmp(argv[x], "async")) {
			oset_set_flag(listener, LFLAG_ASYNC);
		}
	}

	if (oset_test_flag(listener, LFLAG_ASYNC)) {
		if (launch_listener_thread(listener) != OSET_STATUS_SUCCESS) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Failed to start listener");
			return;
		}

		/* Wait until listener_thread acquires session read lock */
		while (!listener->lock_acquired && !listener->finished) {
			oset_cond_next();
		}

		while (!listener->finished && !oset_test_flag(listener, LFLAG_CONNECTED)) {
			oset_cond_next();
		}

		return;
	} else {
		listener_run(NULL, (void *) listener);
	}
}


static void close_socket(oset_apr_socket_t ** sock)
{
	oset_apr_mutex_lock(listen_list.sock_mutex);
	if (*sock) {
		oset_apr_socket_shutdown(*sock, OSET_SHUTDOWN_READWRITE);
		oset_apr_socket_close(*sock);
		*sock = NULL;
	}
	oset_apr_mutex_unlock(listen_list.sock_mutex);
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_event_socket_shutdown)
{
	int sanity = 0;

	prefs.done = 1;

	kill_all_listeners();
	oset_log2_unbind_logger(socket_logger);

	close_socket(&listen_list.sock);

	while (prefs.threads) {
		oset_yield(100000);
		kill_all_listeners();
		if (++sanity >= 200) {
			break;
		}
	}

	oset_event_unbind(&globals.node);

	oset_safe_free(prefs.ip);
	oset_safe_free(prefs.password);

	return OSET_STATUS_SUCCESS;
}

static void add_listener(listener_t *listener)
{
	/* add me to the listeners so I get events */
	oset_apr_mutex_lock(globals.listener_mutex);
	listener->next = listen_list.listeners;
	listen_list.listeners = listener;
	oset_apr_mutex_unlock(globals.listener_mutex);
}

static void remove_listener(listener_t *listener)
{
	listener_t *l, *last = NULL;

	oset_apr_mutex_lock(globals.listener_mutex);
	for (l = listen_list.listeners; l; l = l->next) {
		if (l == listener) {
			if (last) {
				last->next = l->next;
			} else {
				listen_list.listeners = l->next;
			}
		}
		last = l;
	}
	oset_apr_mutex_unlock(globals.listener_mutex);
}

static void send_disconnect(listener_t *listener, const char *message)
{

	char disco_buf[512] = "";
	oset_size_t len, mlen;

	if (zstr(message)) {
		message = "Disconnected.";
	}

	mlen = strlen(message);

	if (listener->session) {
		oset_snprintf(disco_buf, sizeof(disco_buf), "Content-Type: text/disconnect-notice\n"
						"Controlled-Session-UUID: %s\n"
						"Content-Disposition: disconnect" "Content-Length: %d\n\n", oset_core_session_get_uuid(listener->session), (int)mlen);
	} else {
		oset_snprintf(disco_buf, sizeof(disco_buf), "Content-Type: text/disconnect-notice\nContent-Length: %d\n\n", (int)mlen);
	}

	if (!listener->sock) return;

	len = strlen(disco_buf);
	oset_apr_socket_send(listener->sock, disco_buf, &len);
	if (len > 0) {
		len = mlen;
		oset_apr_socket_send(listener->sock, message, &len);
	}
}

static void kill_listener(listener_t *l, const char *message)
{

	if (message) {
		send_disconnect(l, message);
	}

	oset_clear_flag(l, LFLAG_RUNNING);
	if (l->sock) {
		oset_apr_socket_shutdown(l->sock, OSET_SHUTDOWN_READWRITE);
		oset_apr_socket_close(l->sock);
	}

}

static void kill_all_listeners(void)
{
	listener_t *l;

	oset_apr_mutex_lock(globals.listener_mutex);
	for (l = listen_list.listeners; l; l = l->next) {
		kill_listener(l, "The system is being shut down.");
	}
	oset_apr_mutex_unlock(globals.listener_mutex);
}


static listener_t *find_listener(uint32_t id)
{
	listener_t *l, *r = NULL;

	oset_apr_mutex_lock(globals.listener_mutex);
	for (l = listen_list.listeners; l; l = l->next) {
		if (l->id && l->id == id && !l->expire_time) {
			if (oset_apr_thread_rwlock_tryrdlock(l->rwlock) == OSET_STATUS_SUCCESS) {
				r = l;
			}
			break;
		}
	}
	oset_apr_mutex_unlock(globals.listener_mutex);
	return r;
}

static void strip_cr(char *s)
{
	char *p;
	if ((p = strchr(s, '\r')) || (p = strchr(s, '\n'))) {
		*p = '\0';
	}
}


static void xmlize_listener(listener_t *listener, oset_stream_handle_t *stream)
{
	stream->write_function(stream, " <listener>\n");
	stream->write_function(stream, "  <listen-id>%u</listen-id>\n", listener->id);
	stream->write_function(stream, "  <format>%s</format>\n", format2str(listener->format));
	stream->write_function(stream, "  <timeout>%u</timeout>\n", listener->timeout);
	stream->write_function(stream, " </listener>\n");
}

OSET_STANDARD_API(event_sink_function)
{
	char *http = NULL;
	char *wcmd = NULL;
	char *format = NULL;
	listener_t *listener = NULL;

	if (stream->param_event) {
		http = oset_event_get_header(stream->param_event, "http-host");
		wcmd = oset_event_get_header(stream->param_event, "command");
		format = oset_event_get_header(stream->param_event, "format");
	}

	if (!http) {
		stream->write_function(stream, "This is a web application!\n");
		return OSET_STATUS_SUCCESS;
	}

	if (!format) {
		format = "xml";
	}

	if (oset_stristr("json", format)) {
		stream->write_function(stream, "Content-Type: application/json\n\n");
	} else {
		stream->write_function(stream, "Content-Type: text/xml\n\n");

		stream->write_function(stream, "<?xml version=\"1.0\"?>\n");
		stream->write_function(stream, "<root>\n");
	}

	if (!wcmd) {
		stream->write_function(stream, "<data><reply type=\"error\">Missing command parameter!</reply></data>\n");
		goto end;
	}

	if (!strcasecmp(wcmd, "filter")) {
		char *action = oset_event_get_header(stream->param_event, "action");
		char *header_name = oset_event_get_header(stream->param_event, "header-name");
		char *header_val = oset_event_get_header(stream->param_event, "header-val");
		char *id = oset_event_get_header(stream->param_event, "listen-id");
		uint32_t idl = 0;

		if (id) {
			idl = (uint32_t) atol(id);
		}

		if (!(listener = find_listener(idl))) {
			stream->write_function(stream, "<data><reply type=\"error\">Invalid Listen-ID</reply></data>\n");
			goto end;
		}

		if (zstr(action)) {
			stream->write_function(stream, "<data><reply type=\"error\">Invalid Syntax</reply></data>\n");
			goto end;
		}

		oset_apr_mutex_lock(listener->filter_mutex);
		if (!listener->filters) {
			oset_event_create_plain(&listener->filters, OSET_EVENT_CLONE);
		}

		if (!strcasecmp(action, "delete")) {
			if (zstr(header_val)) {
				stream->write_function(stream, "<data><reply type=\"error\">Invalid Syntax</reply></data>\n");
				goto filter_end;
			}

			if (!strcasecmp(header_val, "all")) {
				oset_event_destroy(&listener->filters);
				oset_event_create_plain(&listener->filters, OSET_EVENT_CLONE);
			} else {
				oset_event_del_header(listener->filters, header_val);
			}
			stream->write_function(stream, "<data>\n <reply type=\"success\">filter deleted.</reply>\n<api-command>\n");
		} else if (!strcasecmp(action, "add")) {
			if (zstr(header_name) || zstr(header_val)) {
				stream->write_function(stream, "<data><reply type=\"error\">Invalid Syntax</reply></data>\n");
				goto filter_end;
			}
			oset_event_add_header_string(listener->filters, OSET_STACK_BOTTOM, header_name, header_val);
			stream->write_function(stream, "<data>\n <reply type=\"success\">filter added.</reply>\n<api-command>\n");
		} else {
			stream->write_function(stream, "<data><reply type=\"error\">Invalid Syntax</reply></data>\n");
		}

	  filter_end:

		oset_apr_mutex_unlock(listener->filter_mutex);

	} else if (!strcasecmp(wcmd, "stop-logging")) {
		char *id = oset_event_get_header(stream->param_event, "listen-id");
		uint32_t idl = 0;

		if (id) {
			idl = (uint32_t) atol(id);
		}

		if (!(listener = find_listener(idl))) {
			stream->write_function(stream, "<data><reply type=\"error\">Invalid Listen-ID</reply></data>\n");
			goto end;
		}

		if (oset_test_flag(listener, LFLAG_LOG)) {
			oset_clear_flag_locked(listener, LFLAG_LOG);
			stream->write_function(stream, "<data><reply type=\"success\">Not Logging</reply></data>\n");
		} else {
			stream->write_function(stream, "<data><reply type=\"error\">Not Logging</reply></data>\n");
		}

		goto end;

	} else if (!strcasecmp(wcmd, "set-loglevel")) {
		char *loglevel = oset_event_get_header(stream->param_event, "loglevel");
		char *id = oset_event_get_header(stream->param_event, "listen-id");
		uint32_t idl = 0;

		if (id) {
			idl = (uint32_t) atol(id);
		}

		if (!(listener = find_listener(idl))) {
			stream->write_function(stream, "<data><reply type=\"error\">Invalid Listen-ID</reply></data>\n");
			goto end;
		}

		if (loglevel) {
			oset_log2_level_t ltype = oset_log2_str2level(loglevel);
			if (ltype != OSET_LOG2_INVALID) {
				listener->level = ltype;
				oset_set_flag(listener, LFLAG_LOG);
				stream->write_function(stream, "<data><reply type=\"success\">Log Level %s</reply></data>\n", loglevel);
			} else {
				stream->write_function(stream, "<data><reply type=\"error\">Invalid Level</reply></data>\n");
			}
		} else {
			stream->write_function(stream, "<data><reply type=\"error\">Invalid Syntax</reply></data>\n");
		}

		goto end;

	} else if (!strcasecmp(wcmd, "create-listener")) {
		char *events = oset_event_get_header(stream->param_event, "events");
		char *loglevel = oset_event_get_header(stream->param_event, "loglevel");
		oset_apr_memory_pool_t *pool;
		char *next, *cur;
		uint32_t count = 0, key_count = 0;
		uint8_t custom = 0;
		char *edup;

		if (zstr(events) && zstr(loglevel)) {
			if (oset_stristr("json", format)) {
				stream->write_function(stream, "{\"reply\": \"error\", \"reply_text\":\"Missing parameter!\"}");
			} else {
				stream->write_function(stream, "<data><reply type=\"error\">Missing parameter!</reply></data>\n");
			}
			goto end;
		}

		oset_core_new_memory_pool(&pool);
		listener = oset_core_alloc(pool, sizeof(*listener));
		listener->pool = pool;
		listener->format = EVENT_FORMAT_PLAIN;
		oset_apr_mutex_init(&listener->flag_mutex, OSET_MUTEX_NESTED, listener->pool);
		oset_apr_mutex_init(&listener->filter_mutex, OSET_MUTEX_NESTED, listener->pool);


		oset_core_hash_init(&listener->event_hash);
		oset_set_flag(listener, LFLAG_AUTHED);
		oset_set_flag(listener, LFLAG_STATEFUL);
		oset_set_flag(listener, LFLAG_ALLOW_LOG);
		oset_apr_queue_create(&listener->event_queue, MAX_QUEUE_LEN, listener->pool);
		oset_apr_queue_create(&listener->log_queue, MAX_QUEUE_LEN, listener->pool);

		if (loglevel) {
			oset_log2_level_t ltype = oset_log2_str2level(loglevel);
			if (ltype != OSET_LOG2_INVALID) {
				listener->level = ltype;
				oset_set_flag(listener, LFLAG_LOG);
			}
		}
		oset_apr_thread_rwlock_create(&listener->rwlock, listener->pool);
		listener->id = next_id();
		listener->timeout = 60;
		listener->last_flush = oset_epoch_time_now(NULL);

		if (events) {
			char delim = ',';

			if (oset_stristr("xml", format)) {
				listener->format = EVENT_FORMAT_XML;
			} else if (oset_stristr("json", format)) {
				listener->format = EVENT_FORMAT_JSON;
			} else {
				listener->format = EVENT_FORMAT_PLAIN;
			}

			edup = strdup(events);

			oset_sys_assert(edup);

			if (strchr(edup, ' ')) {
				delim = ' ';
			}

			for (cur = edup; cur; count++) {
				oset_event_types_t type;

				if ((next = strchr(cur, delim))) {
					*next++ = '\0';
				}

				if (custom) {
					oset_core_hash_insert(listener->event_hash, cur, MARKER);
				} else if (oset_name_event(cur, &type) == OSET_STATUS_SUCCESS) {
					key_count++;
					if (type == OSET_EVENT_ALL) {
						uint32_t x = 0;
						for (x = 0; x < OSET_EVENT_ALL; x++) {
							listener->event_list[x] = 1;
						}
					}
					if (type <= OSET_EVENT_ALL) {
						listener->event_list[type] = 1;
					}
					if (type == OSET_EVENT_CUSTOM) {
						custom++;
					}
				}

				cur = next;
			}


			oset_safe_free(edup);

			if (!key_count) {
				oset_core_hash_destroy(&listener->event_hash);
				oset_core_destroy_memory_pool(&listener->pool);
				if (listener->format == EVENT_FORMAT_JSON) {
					stream->write_function(stream, "{\"reply\": \"error\", \"reply_text\":\"No keywords supplied\"}");
				} else {
					stream->write_function(stream, "<data><reply type=\"error\">No keywords supplied</reply></data>\n");
				}
				goto end;
			}
		}

		oset_set_flag_locked(listener, LFLAG_EVENTS);
		add_listener(listener);
		if (listener->format == EVENT_FORMAT_JSON) {
			cJSON *cj, *cjlistener;
			char *p;

			cj = cJSON_CreateObject();
			cjlistener = cJSON_CreateObject();
			cJSON_AddNumberToObject(cjlistener, "listen-id", listener->id);
			cJSON_AddItemToObject(cjlistener, "format", cJSON_CreateString(format2str(listener->format)));
			cJSON_AddNumberToObject(cjlistener, "timeout", listener->timeout);
			cJSON_AddItemToObject(cj, "listener", cjlistener);
			p = cJSON_Print(cj);
			stream->write_function(stream, p);
			oset_safe_free(p);
			cJSON_Delete(cj);
		} else {
			stream->write_function(stream, "<data>\n");
			stream->write_function(stream, " <reply type=\"success\">Listener %u Created</reply>\n", listener->id);
			xmlize_listener(listener, stream);
			stream->write_function(stream, "</data>\n");
		}

		if (globals.debug > 0) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "Creating event-sink listener [%u]", listener->id);
		}

		goto end;
	} else if (!strcasecmp(wcmd, "destroy-listener")) {
		char *id = oset_event_get_header(stream->param_event, "listen-id");
		uint32_t idl = 0;

		if (id) {
			idl = (uint32_t) atol(id);
		}

		if ((listener = find_listener(idl))) {
			if (globals.debug > 0) {
				oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "Destroying event-sink listener [%u]", idl);
			}
			stream->write_function(stream, "<data>\n <reply type=\"success\">listener %u destroyed</reply>\n", listener->id);
			xmlize_listener(listener, stream);
			stream->write_function(stream, "</data>\n");
			listener->expire_time = oset_epoch_time_now(NULL);
			goto end;
		} else {
			if (globals.debug > 0) {
				oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "Request to destroy unknown event-sink listener [%u]", idl);
			}
			stream->write_function(stream, "<data><reply type=\"error\">Can't find listener</reply></data>\n");
			goto end;
		}

	} else if (!strcasecmp(wcmd, "check-listener")) {
		char *id = oset_event_get_header(stream->param_event, "listen-id");
		uint32_t idl = 0;
		void *pop;
		oset_event_t *pevent = NULL;
		cJSON *cj = NULL, *cjevents = NULL;

		if (id) {
			idl = (uint32_t) atol(id);
		}

		if (!(listener = find_listener(idl))) {
			if (oset_stristr("json", format)) {
				stream->write_function(stream, "{\"reply\": \"error\", \"reply_text\":\"Can't find listener\"}");
			} else {
				stream->write_function(stream, "<data><reply type=\"error\">Can't find listener</reply></data>\n");
			}
			goto end;
		}

		listener->last_flush = oset_epoch_time_now(NULL);

		if (listener->format == EVENT_FORMAT_JSON) {
			cJSON *cjlistener;
			cj = cJSON_CreateObject();
			cjlistener = cJSON_CreateObject();
			cJSON_AddNumberToObject(cjlistener, "listen-id", listener->id);
			cJSON_AddItemToObject(cjlistener, "format", cJSON_CreateString(format2str(listener->format)));
			cJSON_AddNumberToObject(cjlistener, "timeout", listener->timeout);
			cJSON_AddItemToObject(cj, "listener", cjlistener);
		} else {
			stream->write_function(stream, "<data>\n <reply type=\"success\">Current Events Follow</reply>\n");
			xmlize_listener(listener, stream);
		}

		if (oset_test_flag(listener, LFLAG_LOG)) {
			stream->write_function(stream, "<log_data>\n");

			while (oset_apr_queue_trypop(listener->log_queue, &pop) == OSET_STATUS_SUCCESS) {
				oset_log2_node_t *dnode = (oset_log2_node_t *) pop;
				size_t encode_len = (strlen(dnode->data) * 3) + 1;
				char *encode_buf = malloc(encode_len);

				oset_sys_assert(encode_buf);

				memset(encode_buf, 0, encode_len);
				oset_url_encode((char *) dnode->data, encode_buf, encode_len);


				stream->write_function(stream,
									   "<log log-level=\"%d\" text-channel=\"%d\" log-file=\"%s\" log-func=\"%s\" log-line=\"%d\" user-data=\"%s\">%s</log>\n",
									   dnode->level, dnode->channel, dnode->file, dnode->func, dnode->line, oset_str_nil(dnode->userdata), encode_buf);
				free(encode_buf);
				oset_log2_node_free(&dnode);
			}

			stream->write_function(stream, "</log_data>\n");
		}

		if (listener->format == EVENT_FORMAT_JSON) {
			cjevents = cJSON_CreateArray();
		} else {
			stream->write_function(stream, "<events>\n");
		}

		while (oset_apr_queue_trypop(listener->event_queue, &pop) == OSET_STATUS_SUCCESS) {
			//char *etype;
			pevent = (oset_event_t *) pop;

			if (listener->format == EVENT_FORMAT_PLAIN) {
				//etype = "plain";
				oset_event_serialize(pevent, &listener->ebuf, OSET_TRUE);
				stream->write_function(stream, "<event type=\"plain\">\n%s</event>", listener->ebuf);
			} else if (listener->format == EVENT_FORMAT_JSON) {
				//etype = "json";
				cJSON *cjevent = NULL;

				oset_event_serialize_json_obj(pevent, &cjevent);
				cJSON_AddItemToArray(cjevents, cjevent);
			} else {
				oset_xml_t xml;
				//etype = "xml";

				if ((xml = oset_event_xmlize(pevent, OSET_VA_NONE))) {
					listener->ebuf = oset_xml_toxml(xml, OSET_FALSE);
					oset_xml_free(xml);
				} else {
					stream->write_function(stream, "<data><reply type=\"error\">XML Render Error</reply></data>\n");
					break;
				}

				stream->write_function(stream, "%s", listener->ebuf);
			}

			oset_safe_free(listener->ebuf);
			oset_event_destroy(&pevent);
		}

		if (listener->format == EVENT_FORMAT_JSON) {
			char *p = "{}";
			cJSON_AddItemToObject(cj, "events", cjevents);
			p = cJSON_Print(cj);
			if (cj && p) stream->write_function(stream, p);
			oset_safe_free(p);
			cJSON_Delete(cj);
			cj = NULL;
		} else {
			stream->write_function(stream, " </events>\n</data>\n");
		}

		if (pevent) {
			oset_event_destroy(&pevent);
		}

		oset_apr_thread_rwlock_unlock(listener->rwlock);
	} else if (!strcasecmp(wcmd, "exec-fsapi")) {
		char *api_command = oset_event_get_header(stream->param_event, "fsapi-command");
		char *api_args = oset_event_get_header(stream->param_event, "fsapi-args");
		oset_event_t *event, *oevent;

		if (!(api_command)) {
			stream->write_function(stream, "<data><reply type=\"error\">INVALID API COMMAND!</reply></data>\n");
			goto end;
		}

		stream->write_function(stream, "<data>\n <reply type=\"success\">Execute API Command</reply>\n<api-command>\n");
		oset_event_create(&event, OSET_EVENT_REQUEST_PARAMS);
		oevent = stream->param_event;
		stream->param_event = event;

		if (!strcasecmp(api_command, "unload") && !strcasecmp(api_args, "mod_event_socket")) {
			api_command = "bgapi";
			api_args = "unload mod_event_socket";
		} else if (!strcasecmp(api_command, "reload") && !strcasecmp(api_args, "mod_event_socket")) {
			api_command = "bgapi";
			api_args = "reload mod_event_socket";
		}

		oset_api_execute(api_command, api_args, NULL, stream);
		stream->param_event = oevent;
		stream->write_function(stream, " </api-command>\n</data>");
	} else {
		stream->write_function(stream, "<data><reply type=\"error\">INVALID COMMAND!</reply></data>\n");
	}

  end:

	if (oset_stristr("json", format)) {
	} else {
		stream->write_function(stream, "</root>\n\n");
	}

	return OSET_STATUS_SUCCESS;
}


OSET_MODULE_LOAD_FUNCTION(mod_event_socket_load)
{
	oset_application_interface_t *app_interface;
	oset_api_interface_t *api_interface;

	memset(&globals, 0, sizeof(globals));

	oset_apr_mutex_init(&globals.listener_mutex, OSET_MUTEX_NESTED, pool);

	memset(&listen_list, 0, sizeof(listen_list));
	oset_apr_mutex_init(&listen_list.sock_mutex, OSET_MUTEX_NESTED, pool);

	if (oset_event_bind_removable(modname, OSET_EVENT_ALL, OSET_EVENT_SUBCLASS_ANY, event_handler, NULL, &globals.node) != OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Couldn't bind!");
		return OSET_STATUS_GENERR;
	}

	oset_log2_bind_logger(socket_logger, OSET_LOG2_DEBUG, OSET_FALSE);//bind log

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = oset_loadable_module_create_module_interface(pool, modname);
	OSET_ADD_APP(app_interface, "socket", "Connect to a socket", "Connect to a socket", socket_function, "<ip>[:<port>]", SAF_NONE);
	OSET_ADD_API(api_interface, "event_sink", "event_sink", event_sink_function, "<web data>");

	/* indicate that the module should continue to be loaded */
	return OSET_STATUS_SUCCESS;
}

static oset_status_t read_packet(listener_t *listener, oset_event_t **event, uint32_t timeout)
{
	oset_size_t mlen, bytes = 0;
	char *mbuf = NULL;
	char buf[1024] = "";
	oset_size_t len;
	oset_status_t status = OSET_STATUS_SUCCESS;
	int count = 0;
	uint32_t elapsed = 0;
	time_t start = 0;
	void *pop;
	char *ptr;
	uint8_t crcount = 0;
	uint32_t max_len = 10485760, block_len = 2048, buf_len = 0;
	int clen = 0;

	*event = NULL;

	if (prefs.done) {
		oset_goto_status(OSET_STATUS_FALSE, end);
	}

	oset_zmalloc(mbuf, block_len);
	oset_sys_assert(mbuf);
	buf_len = block_len;

	start = oset_epoch_time_now(NULL);
	ptr = mbuf;

	while (listener->sock && !prefs.done) {
		uint8_t do_sleep = 1;
		mlen = 1;

		if (bytes == buf_len - 1) {
			char *tmp;
			int pos;

			pos = (int)(ptr - mbuf);
			buf_len += block_len;
			tmp = realloc(mbuf, buf_len);
			oset_sys_assert(tmp);
			mbuf = tmp;
			memset(mbuf + bytes, 0, buf_len - bytes);
			ptr = (mbuf + pos);

		}

		status = oset_apr_socket_recv(listener->sock, ptr, &mlen);

		if (prefs.done || (!OSET_STATUS_IS_BREAK(status) && status != OSET_STATUS_SUCCESS)) {
			oset_goto_status(OSET_STATUS_FALSE, end);
		}

		if (mlen) {
			bytes += mlen;
			do_sleep = 0;

			if (*mbuf == '\r' || *mbuf == '\n') {	/* bah */
				ptr = mbuf;
				mbuf[0] = '\0';
				bytes = 0;
				continue;
			}

			if (*ptr == '\n') {
				crcount++;
			} else if (*ptr != '\r') {
				crcount = 0;
			}
			ptr++;

			if (bytes >= max_len) {
				crcount = 2;
			}

			if (crcount == 2) {
				char *next;
				char *cur = mbuf;
				while (cur) {
					if ((next = strchr(cur, '\r')) || (next = strchr(cur, '\n'))) {
						while (*next == '\r' || *next == '\n') {
							next++;
						}
					}
					count++;
					if (count == 1) {
						oset_event_create(event, OSET_EVENT_CLONE);
						oset_event_add_header_string(*event, OSET_STACK_BOTTOM, "Command", mbuf);
					} else if (cur) {
						char *var, *val;
						var = cur;
						strip_cr(var);
						if (!zstr(var)) {
							if ((val = strchr(var, ':'))) {
								*val++ = '\0';
								while (*val == ' ') {
									val++;
								}
							}
							if (val) {
								oset_event_add_header_string(*event, OSET_STACK_BOTTOM, var, val);
								if (!strcasecmp(var, "content-length")) {
									clen = atoi(val);

									if (clen > 0) {
										char *body;
										char *p;

										oset_zmalloc(body, clen + 1);

										p = body;
										while (clen > 0) {
											mlen = clen;

											status = oset_apr_socket_recv(listener->sock, p, &mlen);

											if (prefs.done || (!OSET_STATUS_IS_BREAK(status) && status != OSET_STATUS_SUCCESS)) {
												free(body);
												oset_goto_status(OSET_STATUS_FALSE, end);
											}

											clen -= (int) mlen;
											p += mlen;
										}

										oset_event_add_body(*event, "%s", body);
										free(body);
									}
								}
							}
						}
					}

					cur = next;
				}
				break;
			}
		}

		if (timeout) {
			elapsed = (uint32_t) (oset_epoch_time_now(NULL) - start);
			if (elapsed >= timeout) {
				oset_clear_flag_locked(listener, LFLAG_RUNNING);
				oset_goto_status(OSET_STATUS_FALSE, end);
			}
		}

		if (!*mbuf) {
			if (oset_test_flag(listener, LFLAG_LOG)) {
				if (oset_apr_queue_trypop(listener->log_queue, &pop) == OSET_STATUS_SUCCESS) {
					oset_log2_node_t *dnode = (oset_log2_node_t *) pop;

					if (dnode->data) {
						oset_snprintf(buf, sizeof(buf),
										"Content-Type: log/data\n"
										"Content-Length: %" OSET_SSIZE_T_FMT "\n"
										"Log-Level: %d\n"
										"Text-Channel: %d\n"
										"Log-File: %s\n"
										"Log-Func: %s\n"
										"Log-Line: %d\n"
										"User-Data: %s\n"
										"\n",
										strlen(dnode->data),
										dnode->level, dnode->channel, dnode->file, dnode->func, dnode->line, oset_str_nil(dnode->userdata)
							);
						len = strlen(buf);
						oset_apr_socket_send(listener->sock, buf, &len);
						len = strlen(dnode->data);
						oset_apr_socket_send(listener->sock, dnode->data, &len);
					}

					oset_log2_node_free(&dnode);
					do_sleep = 0;
				}
			}


			/*if (listener->session) {
				oset_channel_t *chan = oset_core_session_get_channel(listener->session);
				if (oset_channel_get_state(chan) < CS_HANGUP && oset_channel_test_flag(chan, CF_DIVERT_EVENTS)) {
					oset_event_t *e = NULL;
					while (oset_core_session_dequeue_event(listener->session, &e, OSET_TRUE) == OSET_STATUS_SUCCESS) {
						if (oset_apr_queue_trypush(listener->event_queue, e) != OSET_STATUS_SUCCESS) {
							oset_core_session_queue_event(listener->session, &e);
							break;
						}
					}
				}
			}*/

			if (oset_test_flag(listener, LFLAG_EVENTS)) {
				while (oset_apr_queue_trypop(listener->event_queue, &pop) == OSET_STATUS_SUCCESS) {
					char hbuf[512];
					oset_event_t *pevent = (oset_event_t *) pop;
					char *etype;

					do_sleep = 0;
					if (listener->format == EVENT_FORMAT_PLAIN) {
						etype = "plain";
						oset_event_serialize(pevent, &listener->ebuf, OSET_TRUE);
					} else if (listener->format == EVENT_FORMAT_JSON) {
						etype = "json";
						oset_event_serialize_json(pevent, &listener->ebuf);
					} else {
						oset_xml_t xml;
						etype = "xml";

						if ((xml = oset_event_xmlize(pevent, OSET_VA_NONE))) {
							listener->ebuf = oset_xml_toxml(xml, OSET_FALSE);
							oset_xml_free(xml);
						} else {
							oset_log2_printf(OSET_CHANNEL_SESSION_LOG(listener->session), OSET_LOG2_ERROR, "XML ERROR!");
							goto endloop;
						}
					}

					oset_sys_assert(listener->ebuf);

					len = strlen(listener->ebuf);

					oset_snprintf(hbuf, sizeof(hbuf), "Content-Length: %" OSET_SSIZE_T_FMT "\n" "Content-Type: text/event-%s\n" "\n", len, etype);

					len = strlen(hbuf);
					oset_apr_socket_send(listener->sock, hbuf, &len);

					len = strlen(listener->ebuf);
					oset_apr_socket_send(listener->sock, listener->ebuf, &len);

					oset_safe_free(listener->ebuf);

				  endloop:

					oset_event_destroy(&pevent);
				}
			}
		}

		if (oset_test_flag(listener, LFLAG_HANDLE_DISCO) &&
			listener->linger_timeout != (time_t) -1 && oset_epoch_time_now(NULL) > listener->linger_timeout) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(listener->session), OSET_LOG2_DEBUG, "linger timeout, closing socket");
			status = OSET_STATUS_FALSE;
			break;
		}

		if (listener->session && !oset_test_flag(listener, LFLAG_HANDLE_DISCO)) {
			oset_set_flag_locked(listener, LFLAG_HANDLE_DISCO);
			if (oset_test_flag(listener, LFLAG_LINGER)) {
				char disco_buf[512] = "";

				oset_log2_printf(OSET_CHANNEL_SESSION_LOG(listener->session), OSET_LOG2_DEBUG, "Socket Linger %d",(int)listener->linger_timeout);

				oset_snprintf(disco_buf, sizeof(disco_buf), "Content-Type: text/disconnect-notice\n"
								"Controlled-Session-UUID: %s\n"
								"Content-Disposition: linger\n"
								"Linger-Time: %d\n"
								"Content-Length: 0\n\n",
								oset_core_session_get_uuid(listener->session), (int)listener->linger_timeout);


				if (listener->linger_timeout != (time_t) -1) {
					listener->linger_timeout += oset_epoch_time_now(NULL);
				}

				len = strlen(disco_buf);
				oset_apr_socket_send(listener->sock, disco_buf, &len);
			} else {
				status = OSET_STATUS_FALSE;
				break;
			}
		}

		if (do_sleep) {
			int fdr = 0;
			oset_apr_poll(listener->pollfd, 1, &fdr, 20000);
		} else {
			oset_os_yield();
		}
	}

 end:

	oset_safe_free(mbuf);
	return status;

}

struct api_command_struct {
	char *api_cmd;
	char *arg;
	listener_t *listener;
	char uuid_str[OSET_UUID_FORMATTED_LENGTH + 1];
	int bg;
	char bg_owner_uuid_str[OSET_UUID_FORMATTED_LENGTH + 1];
	int ack;
	int console_execute;
	oset_apr_memory_pool_t *pool;
};

static void *OSET_THREAD_FUNC api_exec(oset_apr_thread_t *thread, void *obj)
{

	struct api_command_struct *acs = (struct api_command_struct *) obj;
	oset_stream_handle_t stream = { 0 };
	char *reply, *freply = NULL;
	oset_status_t status;

	oset_apr_mutex_lock(globals.listener_mutex);
	prefs.threads++;
	oset_apr_mutex_unlock(globals.listener_mutex);


	if (!acs) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Internal error.");
		goto cleanup;
	}

	if (!acs->listener || !oset_test_flag(acs->listener, LFLAG_RUNNING) ||
		!acs->listener->rwlock || oset_apr_thread_rwlock_tryrdlock(acs->listener->rwlock) != OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Error! cannot get read lock.");
		acs->ack = -1;
		goto done;
	}

	acs->ack = 1;

	OSET_STANDARD_STREAM(stream);

	if (acs->console_execute) {
		if ((status = oset_console_execute(acs->api_cmd, 0, &stream)) != OSET_STATUS_SUCCESS) {
			stream.write_function(&stream, "-ERR %s Command not found!\n", acs->api_cmd);
		}
	} else {
		status = oset_api_execute(acs->api_cmd, acs->arg, NULL, &stream);
	}

	if (status == OSET_STATUS_SUCCESS) {
		reply = stream.data;
	} else {
		freply = oset_mprintf("-ERR %s Command not found!", acs->api_cmd);
		reply = freply;
	}

	if (!reply) {
		reply = "Command returned no output!";
	}

	if (acs->bg) {
		oset_event_t *event;

		if (oset_event_create(&event, OSET_EVENT_BACKGROUND_JOB) == OSET_STATUS_SUCCESS) {
			oset_event_add_header_string(event, OSET_STACK_BOTTOM, "Job-UUID", acs->uuid_str);
			oset_event_add_header_string(event, OSET_STACK_BOTTOM, "Job-Owner-UUID", acs->bg_owner_uuid_str);
			oset_event_add_header_string(event, OSET_STACK_BOTTOM, "Job-Command", acs->api_cmd);
			if (acs->arg) {
				oset_event_add_header_string(event, OSET_STACK_BOTTOM, "Job-Command-Arg", acs->arg);
			}
			oset_event_add_body(event, "%s", reply);
			oset_event_fire(&event);
		}
	} else {
		oset_size_t rlen, blen;
		char buf[1024] = "";

		if (!(rlen = strlen(reply))) {
			reply = "-ERR no reply";
			rlen = strlen(reply);
		}

		oset_snprintf(buf, sizeof(buf), "Content-Type: api/response\nContent-Length: %" OSET_SSIZE_T_FMT "\n\n", rlen);
		blen = strlen(buf);
		oset_apr_socket_send(acs->listener->sock, buf, &blen);
		oset_apr_socket_send(acs->listener->sock, reply, &rlen);
	}

	oset_safe_free(stream.data);
	oset_safe_free(freply);

	oset_apr_thread_rwlock_unlock(acs->listener->rwlock);

  done:

	if (acs->bg) {
		oset_apr_memory_pool_t *pool = acs->pool;
		if (acs->ack == -1) {
			int sanity = 2000;
			while (acs->ack == -1) {
				oset_cond_next();
				if (--sanity <= 0)
					break;
			}
		}

		acs = NULL;
		oset_core_destroy_memory_pool(&pool);
		pool = NULL;

	}

  cleanup:
	oset_apr_mutex_lock(globals.listener_mutex);
	prefs.threads--;
	oset_apr_mutex_unlock(globals.listener_mutex);

	return NULL;

}

static oset_bool_t auth_api_command(listener_t *listener, const char *api_cmd, const char *arg)
{
	const char *check_cmd = api_cmd;
	char *sneaky_commands[] = { "bgapi", "sched_api", NULL, NULL, NULL, NULL };
	int x = 0;
	char *dup_arg = NULL;
	char *next = NULL;
	oset_bool_t ok = OSET_TRUE;

  top:

	if (!oset_core_hash_find(listener->allowed_api_hash, check_cmd)) {
		ok = OSET_FALSE;
		goto end;
	}

	while (check_cmd) {
		for (x = 0; sneaky_commands[x]; x++) {
			if (!strcasecmp(sneaky_commands[x], check_cmd)) {
				if (check_cmd == api_cmd) {
					if (arg) {
						oset_safe_free(dup_arg);
						dup_arg = strdup(arg);
						oset_sys_assert(dup_arg);
						check_cmd = dup_arg;
						if ((next = strchr(check_cmd, ' '))) {
							*next++ = '\0';
						}
					} else {
						break;
					}
				} else {
					if (next) {
						check_cmd = next;
					} else {
						check_cmd = dup_arg;
					}

					if ((next = strchr(check_cmd, ' '))) {
						*next++ = '\0';
					}
				}
				goto top;
			}
		}
		break;
	}

  end:

	oset_safe_free(dup_arg);
	return ok;

}

static void set_all_custom(listener_t *listener)
{
	oset_console_callback_match_t *events = NULL;
	oset_console_callback_match_node_t *m;

	if (oset_event_get_custom_events(&events) == OSET_STATUS_SUCCESS) {
		for (m = events->head; m; m = m->next) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG1, "ADDING CUSTOM EVENT: %s", m->val);
			oset_core_hash_insert(listener->event_hash, m->val, MARKER);
		}

		oset_console_free_matches(&events);
	}
}

static void set_allowed_custom(listener_t *listener)
{
	oset_hashtable_index_t *hi = NULL;
	const void *var;
	void *val;

	oset_sys_assert(listener->allowed_event_hash);

	for (hi = oset_core_hash_first(listener->allowed_event_hash); hi; hi = oset_core_hash_next(&hi)) {
		oset_core_hash_this(hi, &var, NULL, &val);
		oset_core_hash_insert(listener->event_hash, (char *)var, MARKER);
	}
}

static oset_status_t parse_command(listener_t *listener, oset_event_t **event, char *reply, uint32_t reply_len)
{
	oset_status_t status = OSET_STATUS_SUCCESS;
	char *cmd = NULL;
	char unload_cheat[] = "api bgapi unload mod_event_socket";
	char reload_cheat[] = "api bgapi reload mod_event_socket";

	*reply = '\0';

	if (!event || !*event || !(cmd = oset_event_get_header(*event, "command"))) {
		oset_clear_flag_locked(listener, LFLAG_RUNNING);
		oset_snprintf(reply, reply_len, "-ERR command parse error.");
		goto done;
	}

	if (oset_stristr("unload", cmd) && oset_stristr("mod_event_socket", cmd)) {
		cmd = unload_cheat;
	} else if (oset_stristr("reload", cmd) && oset_stristr("mod_event_socket", cmd)) {
		cmd = reload_cheat;
	}

	if (!strncasecmp(cmd, "exit", 4) || !strncasecmp(cmd, "...", 3)) {
		oset_clear_flag_locked(listener, LFLAG_RUNNING);
		oset_snprintf(reply, reply_len, "+OK bye");
		goto done;
	}

	if (!oset_test_flag(listener, LFLAG_AUTHED)) {
		if (!strncasecmp(cmd, "auth ", 5)) {
			char *pass;
			strip_cr(cmd);

			pass = cmd + 5;

			if (!strcmp(prefs.password, pass)) {
				oset_set_flag_locked(listener, LFLAG_AUTHED);
				oset_snprintf(reply, reply_len, "+OK accepted");
			} else {
				oset_snprintf(reply, reply_len, "-ERR invalid");
				oset_clear_flag_locked(listener, LFLAG_RUNNING);
			}

			goto done;
		}

		if (!strncasecmp(cmd, "userauth ", 9)) {
			const char *passwd;
			const char *allowed_api;
			const char *allowed_events;
			oset_event_t *params;
			char *user = NULL, *domain_name = NULL, *pass = NULL;
			oset_xml_t x_domain = NULL, x_domain_root, x_user = NULL, x_params, x_param, x_group = NULL;
			int authed = 0;
			char *edup = NULL;
			char event_reply[512] = "Allowed-Events: all";
			char api_reply[512] = "Allowed-API: all";
			char log_reply[512] = "";
			int allowed_log = 1;
			char *tmp;

			oset_clear_flag(listener, LFLAG_ALLOW_LOG);

			strip_cr(cmd);

			user = cmd + 9;

			if ((domain_name = strchr(user, '@'))) {
				*domain_name++ = '\0';
			}

			if (domain_name && (pass = strchr(domain_name, ':'))) {
				*pass++ = '\0';
			}

			if ((tmp = strchr(user, ':'))) {
				*tmp++ = '\0';
				pass = tmp;
			}

			if (zstr(user) || zstr(domain_name)) {
				oset_snprintf(reply, reply_len, "-ERR invalid");
				oset_clear_flag_locked(listener, LFLAG_RUNNING);
				goto done;
			}


			passwd = NULL;
			allowed_events = NULL;
			allowed_api = NULL;

			params = NULL;
			x_domain_root = NULL;


			oset_event_create(&params, OSET_EVENT_REQUEST_PARAMS);
			oset_sys_assert(params);
			oset_event_add_header_string(params, OSET_STACK_BOTTOM, "action", "event_socket_auth");

			if (oset_xml_locate_user("id", user, domain_name, NULL, &x_domain_root, &x_domain, &x_user, &x_group, params) == OSET_STATUS_SUCCESS) {
				oset_xml_t list[3];
				int x = 0;

				list[0] = x_domain;
				list[1] = x_group;
				list[2] = x_user;

				for (x = 0; x < 3; x++) {
					if ((x_params = oset_xml_child(list[x], "params"))) {
						for (x_param = oset_xml_child(x_params, "param"); x_param; x_param = x_param->next) {
							const char *var = oset_xml_attr_soft(x_param, "name");
							const char *val = oset_xml_attr_soft(x_param, "value");

							if (!strcasecmp(var, "esl-password")) {
								passwd = val;
							} else if (!strcasecmp(var, "esl-allowed-log")) {
								allowed_log = oset_true(val);
							} else if (!strcasecmp(var, "esl-allowed-events")) {
								allowed_events = val;
							} else if (!strcasecmp(var, "esl-allowed-api")) {
								allowed_api = val;
							}
						}
					}
				}
			} else {
				authed = 0;
				goto bot;
			}

			if (!zstr(passwd) && !zstr(pass) && !strcmp(passwd, pass)) {
				authed = 1;

				if (allowed_events) {
					char delim = ',';
					char *cur, *next;
					int count = 0, custom = 0, key_count = 0;

					oset_set_flag(listener, LFLAG_AUTH_EVENTS);

					oset_snprintf(event_reply, sizeof(event_reply), "Allowed-Events: %s", allowed_events);

					oset_core_hash_init(&listener->allowed_event_hash);

					edup = strdup(allowed_events);

					oset_sys_assert(edup);
					
					if (strchr(edup, ' ')) {
						delim = ' ';
					}

					for (cur = edup; cur; count++) {
						oset_event_types_t type;

						if ((next = strchr(cur, delim))) {
							*next++ = '\0';
						}

						if (custom) {
							oset_core_hash_insert(listener->allowed_event_hash, cur, MARKER);
						} else if (oset_name_event(cur, &type) == OSET_STATUS_SUCCESS) {
							key_count++;
							if (type == OSET_EVENT_ALL) {
								uint32_t x = 0;
								oset_set_flag(listener, LFLAG_ALL_EVENTS_AUTHED);
								for (x = 0; x < OSET_EVENT_ALL; x++) {
									listener->allowed_event_list[x] = 1;
								}
							}
							if (type <= OSET_EVENT_ALL) {
								listener->allowed_event_list[type] = 1;
							}

							if (type == OSET_EVENT_CUSTOM) {
								custom++;
							}
						}

						cur = next;
					}

					oset_safe_free(edup);
				}

				oset_snprintf(log_reply, sizeof(log_reply), "Allowed-LOG: %s", allowed_log ? "true" : "false");

				if (allowed_log) {
					oset_set_flag(listener, LFLAG_ALLOW_LOG);
				}

				if (allowed_api) {
					char delim = ',';
					char *cur, *next;
					int count = 0;

					oset_snprintf(api_reply, sizeof(api_reply), "Allowed-API: %s", allowed_api);

					oset_core_hash_init(&listener->allowed_api_hash);

					edup = strdup(allowed_api);

					if (strchr(edup, ' ')) {
						delim = ' ';
					}

					for (cur = edup; cur; count++) {
						if ((next = strchr(cur, delim))) {
							*next++ = '\0';
						}

						oset_core_hash_insert(listener->allowed_api_hash, cur, MARKER);

						cur = next;
					}

					oset_safe_free(edup);
				}

			}


		  bot:
			oset_event_destroy(&params);

			if (authed) {
				oset_set_flag_locked(listener, LFLAG_AUTHED);
				oset_snprintf(reply, reply_len, "~Reply-Text: +OK accepted\n%s%s%s", event_reply, api_reply, log_reply);
			} else {
				oset_snprintf(reply, reply_len, "-ERR invalid");
				oset_clear_flag_locked(listener, LFLAG_RUNNING);
			}

			if (x_domain_root) {
				oset_xml_free(x_domain_root);
			}

		}

		goto done;
	}

	if (!strncasecmp(cmd, "filter ", 7)) {
		char *header_name = cmd + 7;
		char *header_val = NULL;

		strip_cr(header_name);

		while (header_name && *header_name && *header_name == ' ')
			header_name++;

		if ((header_val = strchr(header_name, ' '))) {
			*header_val++ = '\0';
		}

		oset_apr_mutex_lock(listener->filter_mutex);
		if (!listener->filters) {
			oset_event_create_plain(&listener->filters, OSET_EVENT_CLONE);
			oset_clear_flag(listener->filters, EF_UNIQ_HEADERS);
		}

		if (!strcasecmp(header_name, "delete") && header_val) {
			header_name = header_val;
			if ((header_val = strchr(header_name, ' '))) {
				*header_val++ = '\0';
			}
			if (!strcasecmp(header_name, "all")) {
				oset_event_destroy(&listener->filters);
				oset_event_create_plain(&listener->filters, OSET_EVENT_CLONE);
			} else {
				oset_event_del_header_val(listener->filters, header_name, header_val);
			}
			oset_snprintf(reply, reply_len, "+OK filter deleted. [%s][%s]", header_name, oset_str_nil(header_val));
		} else if (header_val) {
			if (!strcasecmp(header_name, "add")) {
				header_name = header_val;
				if ((header_val = strchr(header_name, ' '))) {
					*header_val++ = '\0';
				}
			}
			oset_event_add_header_string(listener->filters, OSET_STACK_BOTTOM, header_name, header_val);
			oset_snprintf(reply, reply_len, "+OK filter added. [%s]=[%s]", header_name, header_val);
		} else {
			oset_snprintf(reply, reply_len, "-ERR invalid syntax");
		}
		oset_apr_mutex_unlock(listener->filter_mutex);

		goto done;
	}

	if (listener->session && !strncasecmp(cmd, "resume", 6)) {
		oset_set_flag_locked(listener, LFLAG_RESUME);
		oset_session_set_variable(listener->session, "socket_resume", "true");
		oset_snprintf(reply, reply_len, "+OK");
		goto done;
	}

	/*if (listener->session || !strncasecmp(cmd, "myevents ", 9)) {
		if (!strncasecmp(cmd, "connect", 7)) {
			oset_event_t *call_event;
			char *event_str;
			oset_size_t len;

			oset_set_flag_locked(listener, LFLAG_CONNECTED);
			oset_event_create(&call_event, OSET_EVENT_CHANNEL_DATA);

			oset_event_add_header_string(call_event, OSET_STACK_BOTTOM, "Content-Type", "command/reply");
			oset_event_add_header_string(call_event, OSET_STACK_BOTTOM, "Reply-Text", "+OK");
			oset_event_add_header_string(call_event, OSET_STACK_BOTTOM, "Socket-Mode", oset_test_flag(listener, LFLAG_ASYNC) ? "async" : "static");
			oset_event_add_header_string(call_event, OSET_STACK_BOTTOM, "Control", oset_test_flag(listener, LFLAG_FULL) ? "full" : "single-channel");

			oset_event_serialize(call_event, &event_str, OSET_TRUE);
			oset_sys_assert(event_str);
			len = strlen(event_str);
			oset_apr_socket_send(listener->sock, event_str, &len);
			oset_safe_free(event_str);
			oset_event_destroy(&call_event);
			//oset_snprintf(reply, reply_len, "+OK");
			goto done_noreply;
		} else if (!strncasecmp(cmd, "getvar", 6)) {
			char *arg;
			const char *val = "";

			strip_cr(cmd);

			if ((arg = strchr(cmd, ' '))) {
				*arg++ = '\0';
				if (!(val = oset_session_get_variable(listener->session, arg))) {
					val = "";
				}

			}
			oset_snprintf(reply, reply_len, "%s", val);
			goto done;
		} else if (!strncasecmp(cmd, "myevents", 8)) {
			if (oset_test_flag(listener, LFLAG_MYEVENTS)) {
				oset_snprintf(reply, reply_len, "-ERR aready enabled.");
				goto done;
			}

			if (!listener->session) {
				char *uuid;

				if ((uuid = cmd + 9)) {
					char *fmt;
					strip_cr(uuid);

					if ((fmt = strchr(uuid, ' '))) {
						*fmt++ = '\0';
					}

					if (!(listener->session = oset_core_session_locate(uuid))) {
						if (fmt) {
							oset_snprintf(reply, reply_len, "-ERR invalid uuid");
							goto done;
						}
					}

					if ((fmt = strchr(uuid, ' '))) {
						if (!strcasecmp(fmt, "xml")) {
							listener->format = EVENT_FORMAT_XML;
						} else if (!strcasecmp(fmt, "plain")) {
							listener->format = EVENT_FORMAT_PLAIN;
						} else if (!strcasecmp(fmt, "json")) {
							listener->format = EVENT_FORMAT_JSON;
						}
					}

					oset_set_flag_locked(listener, LFLAG_SESSION);
					oset_set_flag_locked(listener, LFLAG_ASYNC);
				}


			}

			listener->event_list[OSET_EVENT_CHANNEL_APPLICATION] = 1;
			listener->event_list[OSET_EVENT_CHANNEL_CREATE] = 1;
			listener->event_list[OSET_EVENT_CHANNEL_DATA] = 1;
			listener->event_list[OSET_EVENT_CHANNEL_DESTROY] = 1;
			listener->event_list[OSET_EVENT_CHANNEL_EXECUTE] = 1;
			listener->event_list[OSET_EVENT_CHANNEL_EXECUTE_COMPLETE] = 1;
			listener->event_list[OSET_EVENT_CHANNEL_UUID] = 1;
			oset_set_flag_locked(listener, LFLAG_MYEVENTS);
			oset_set_flag_locked(listener, LFLAG_EVENTS);
			if (strstr(cmd, "xml") || strstr(cmd, "XML")) {
				listener->format = EVENT_FORMAT_XML;
			}
			if (strstr(cmd, "json") || strstr(cmd, "JSON")) {
				listener->format = EVENT_FORMAT_JSON;
			}
			oset_snprintf(reply, reply_len, "+OK Events Enabled");
			goto done;
		}
	}*/


	/*if (!strncasecmp(cmd, "divert_events", 13)) {
		char *onoff = cmd + 13;

		if (!listener->session) {
			oset_snprintf(reply, reply_len, "-ERR not controlling a session.");
			goto done;
		}

		while (*onoff == ' ') {
			onoff++;
		}

		if (*onoff == '\r' || *onoff == '\n') {
			onoff = NULL;
		} else {
			strip_cr(onoff);
		}

		if (zstr(onoff)) {
			oset_snprintf(reply, reply_len, "-ERR missing value.");
			goto done;
		}


		if (!strcasecmp(onoff, "on")) {
			oset_snprintf(reply, reply_len, "+OK events diverted");
		} else {
			oset_snprintf(reply, reply_len, "+OK events not diverted");
		}

		goto done;

	}*/

	/*if (!strncasecmp(cmd, "sendmsg", 7)) {
		oset_core_session_t *session;
		char *uuid = cmd + 7;
		const char *async_var = oset_event_get_header(*event, "async");
		int async = oset_test_flag(listener, LFLAG_ASYNC);

		if (oset_true(async_var)) {
			async = 1;
		}

		while (*uuid == ' ') {
			uuid++;
		}

		if (*uuid == '\r' || *uuid == '\n') {
			uuid = NULL;
		} else {
			strip_cr(uuid);
		}

		if (zstr(uuid)) {
			uuid = oset_event_get_header(*event, "session-id");
		}

		if (uuid && listener->session && !strcmp(uuid, oset_core_session_get_uuid(listener->session))) {
			uuid = NULL;
		}

		if (zstr(uuid) && listener->session) {
			if (async) {
				if ((status = oset_core_session_queue_private_event(listener->session, event, OSET_FALSE)) == OSET_STATUS_SUCCESS) {
					oset_snprintf(reply, reply_len, "+OK");
				} else {
					oset_snprintf(reply, reply_len, "-ERR memory error");
				}
			} else {
				oset_ivr_parse_event(listener->session, *event);
				oset_snprintf(reply, reply_len, "+OK");
			}
		} else {
			if (!zstr(uuid) && (session = oset_core_session_locate(uuid))) {
				if ((status = oset_core_session_queue_private_event(session, event, OSET_FALSE)) == OSET_STATUS_SUCCESS) {
					oset_snprintf(reply, reply_len, "+OK");
				} else {
					oset_snprintf(reply, reply_len, "-ERR memory error");
				}
				oset_apr_thread_rwlock_unlock(session->rwlock);
			} else {
				oset_snprintf(reply, reply_len, "-ERR invalid session id [%s]", oset_str_nil(uuid));
			}
		}

		goto done;

	}*/

	/*if (!strncasecmp(cmd, "sendevent", 9)) {
		char *ename;
		const char *uuid = NULL;
		char uuid_str[OSET_UUID_FORMATTED_LENGTH + 1];
		oset_uuid_str(uuid_str, sizeof(uuid_str));

		oset_event_add_header_string(*event, OSET_STACK_BOTTOM, "Event-UUID", uuid_str);

		strip_cr(cmd);

		ename = cmd + 9;

		while (ename && (*ename == '\t' || *ename == ' ')) {
			++ename;
		}

		if (ename && (*ename == '\r' || *ename == '\n')) {
			ename = NULL;
		}

		if (ename) {
			oset_event_types_t etype;
			if (oset_name_event(ename, &etype) == OSET_STATUS_SUCCESS) {
				const char *subclass_name = oset_event_get_header(*event, "Event-Subclass");
				(*event)->event_id = etype;

				if (etype == OSET_EVENT_CUSTOM && subclass_name) {
					oset_event_set_subclass_name(*event, subclass_name);
				}
			}
		}

		if ((uuid = oset_event_get_header(*event, "unique-id"))) {
			oset_core_session_t *dsession;

			if ((dsession = oset_core_session_locate(uuid))) {
				oset_core_session_queue_event(dsession, event);
				oset_apr_thread_rwlock_unlock(session->rwlock);
			}
		}

		if (*event) {
			oset_event_prep_for_delivery(*event);
			oset_event_fire(event);
		}
		oset_snprintf(reply, reply_len, "+OK %s", uuid_str);
		goto done;
	} else */if (!strncasecmp(cmd, "api ", 4)) {
		struct api_command_struct acs = { 0 };
		char *console_execute = oset_event_get_header(*event, "console_execute");

		char *api_cmd = cmd + 4;
		char *arg = NULL;
		strip_cr(api_cmd);

		if (listener->allowed_api_hash) {
			char *api_copy = strdup(api_cmd);
			char *arg_copy = NULL;
			int ok = 0;

			oset_sys_assert(api_copy);

			if ((arg_copy = strchr(api_copy, ' '))) {
				*arg_copy++ = '\0';
			}

			ok = auth_api_command(listener, api_copy, arg_copy);
			free(api_copy);

			if (!ok) {
				oset_snprintf(reply, reply_len, "-ERR permission denied");
				status = OSET_STATUS_SUCCESS;
				goto done;
			}
		}

		if (!(acs.console_execute = oset_true(console_execute))) {
			if ((arg = strchr(api_cmd, ' '))) {
				*arg++ = '\0';
			}
		}

		acs.listener = listener;
		acs.api_cmd = api_cmd;
		acs.arg = arg;
		acs.bg = 0;


		api_exec(NULL, (void *) &acs);

		status = OSET_STATUS_SUCCESS;
		goto done_noreply;
	} else if (!strncasecmp(cmd, "bgapi ", 6)) {
		struct api_command_struct *acs = NULL;
		char *api_cmd = cmd + 6;
		char *arg = NULL;
		char *uuid_str = NULL;
		oset_apr_memory_pool_t *pool;
		oset_apr_thread_t *thread;
		oset_apr_threadattr_t *thd_attr = NULL;
		oset_uuid_t uuid;
		int sanity;

		strip_cr(api_cmd);

		if ((arg = strchr(api_cmd, ' '))) {
			*arg++ = '\0';
		}

		if (listener->allowed_api_hash) {
			if (!auth_api_command(listener, api_cmd, arg)) {
				oset_snprintf(reply, reply_len, "-ERR permission denied");
				status = OSET_STATUS_SUCCESS;
				goto done;
			}
		}

		oset_core_new_memory_pool(&pool);
		acs = oset_core_alloc(pool, sizeof(*acs));
		oset_sys_assert(acs);
		acs->pool = pool;
		acs->listener = listener;
		acs->console_execute = 0;

		acs->api_cmd = oset_core_strdup(acs->pool, api_cmd);
		if (arg) {
			acs->arg = oset_core_strdup(acs->pool, arg);
		}
		acs->bg = 1;

		oset_apr_threadattr_create(&thd_attr, acs->pool);
		oset_apr_threadattr_detach_set(thd_attr, 1);
		oset_apr_threadattr_stacksize_set(thd_attr, OSET_THREAD_STACKSIZE);

		if ((uuid_str = oset_event_get_header(*event, "job-uuid"))) {
			oset_apr_copy_string(acs->uuid_str, uuid_str, sizeof(acs->uuid_str));
		} else {
			oset_uuid_get(&uuid);
			oset_uuid_format(acs->uuid_str, &uuid);
		}
		oset_apr_copy_string(acs->bg_owner_uuid_str, oset_core_session_get_uuid(listener->session), sizeof(acs->bg_owner_uuid_str));
		oset_snprintf(reply, reply_len, "~Reply-Text: +OK Job-UUID: %s\nJob-UUID: %s\n", acs->uuid_str, acs->uuid_str);
		oset_apr_thread_create(&thread, thd_attr, api_exec, acs, acs->pool);
		sanity = 2000;
		while (!acs->ack) {
			oset_cond_next();
			if (--sanity <= 0)
				break;
		}
		if (acs->ack == -1) {
			acs->ack--;
		}

		status = OSET_STATUS_SUCCESS;
		goto done_noreply;
	} else if (!strncasecmp(cmd, "log", 3)) {

		char *level_s, *p;
		oset_log2_level_t ltype = OSET_LOG2_DEBUG;

		if (!oset_test_flag(listener, LFLAG_ALLOW_LOG)) {
			oset_snprintf(reply, reply_len, "-ERR permission denied");
			goto done;
		}
		//pull off the first newline/carriage return
		strip_cr(cmd);

		//move past the command
		level_s = cmd + 3;

		while(*level_s == ' ') {
			level_s++;
		}

		if ((p = strchr(level_s, ' '))) {
			*p = '\0';
		}

		//see if we lined up on an argument or not
		if (!zstr(level_s)) {
			ltype = oset_log2_str2level(level_s);
		}

		if (ltype != OSET_LOG2_INVALID) {
			listener->level = ltype;
			oset_set_flag(listener, LFLAG_LOG);
			oset_snprintf(reply, reply_len, "+OK log level %s [%d]", level_s, listener->level);
		} else {
			oset_snprintf(reply, reply_len, "-ERR invalid log level");
		}
	} else if (!strncasecmp(cmd, "linger", 6)) {
		if (listener->session) {
			time_t linger_time = 600; /* sounds reasonable? */
			if (*(cmd+6) == ' ' && *(cmd+7)) { /*how long do you want to linger?*/
				linger_time = (time_t) atoi(cmd+7);
			} else {
				linger_time = (time_t) -1;
			}

			listener->linger_timeout = linger_time;
			oset_set_flag_locked(listener, LFLAG_LINGER);
			if (listener->linger_timeout != (time_t) -1) {
				oset_snprintf(reply, reply_len, "+OK will linger %d seconds", (int)linger_time);
			} else {
				oset_snprintf(reply, reply_len, "+OK will linger");
			}
		} else {
			oset_snprintf(reply, reply_len, "-ERR not controlling a session");
		}
	} else if (!strncasecmp(cmd, "nolinger", 8)) {
		if (listener->session) {
			oset_clear_flag_locked(listener, LFLAG_LINGER);
			oset_snprintf(reply, reply_len, "+OK will not linger");
		} else {
			oset_snprintf(reply, reply_len, "-ERR not controlling a session");
		}
	} else if (!strncasecmp(cmd, "nolog", 5)) {
		flush_listener(listener, OSET_TRUE, OSET_FALSE);
		if (oset_test_flag(listener, LFLAG_LOG)) {
			oset_clear_flag_locked(listener, LFLAG_LOG);
			oset_snprintf(reply, reply_len, "+OK no longer logging");
		} else {
			oset_snprintf(reply, reply_len, "-ERR not loging");
		}
	} else if (!strncasecmp(cmd, "event", 5)) {
		char *next, *cur;
		uint32_t count = 0, key_count = 0;
		uint8_t custom = 0;

		strip_cr(cmd);
		cur = cmd + 5;

		if ((cur = strchr(cur, ' '))) {
			for (cur++; cur; count++) {
				oset_event_types_t type;

				if ((next = strchr(cur, ' '))) {
					*next++ = '\0';
				}

				if (!count) {
					if (!strcasecmp(cur, "xml")) {
						listener->format = EVENT_FORMAT_XML;
						goto end;
					} else if (!strcasecmp(cur, "plain")) {
						listener->format = EVENT_FORMAT_PLAIN;
						goto end;
					} else if (!strcasecmp(cur, "json")) {
						listener->format = EVENT_FORMAT_JSON;
						goto end;
					}
				}


				if (custom) {
					if (!listener->allowed_event_hash || oset_core_hash_find(listener->allowed_event_hash, cur)) {
						oset_core_hash_insert(listener->event_hash, cur, MARKER);
					} else {
						oset_snprintf(reply, reply_len, "-ERR permission denied");
						goto done;
					}
				} else if (oset_name_event(cur, &type) == OSET_STATUS_SUCCESS) {
					if (oset_test_flag(listener, LFLAG_AUTH_EVENTS) && !listener->allowed_event_list[type] &&
						!oset_test_flag(listener, LFLAG_ALL_EVENTS_AUTHED)) {
						oset_snprintf(reply, reply_len, "-ERR permission denied");
						goto done;
					}

					key_count++;
					if (type == OSET_EVENT_ALL) {
						uint32_t x = 0;
						for (x = 0; x < OSET_EVENT_ALL; x++) {
							listener->event_list[x] = 1;
						}

						if (!listener->allowed_event_hash) {
							set_all_custom(listener);
						} else {
							set_allowed_custom(listener);
						}

					}
					if (type <= OSET_EVENT_ALL) {
						listener->event_list[type] = 1;
					}
					if (type == OSET_EVENT_CUSTOM) {
						custom++;
					}
				}

			  end:
				cur = next;
			}
		}

		if (!key_count) {
			oset_snprintf(reply, reply_len, "-ERR no keywords supplied");
			goto done;
		}

		if (!oset_test_flag(listener, LFLAG_EVENTS)) {
			oset_set_flag_locked(listener, LFLAG_EVENTS);
		}

		oset_snprintf(reply, reply_len, "+OK event listener enabled %s", format2str(listener->format));

	} else if (!strncasecmp(cmd, "nixevent", 8)) {
		char *next, *cur;
		uint32_t count = 0, key_count = 0;
		uint8_t custom = 0;

		strip_cr(cmd);
		cur = cmd + 8;

		if ((cur = strchr(cur, ' '))) {
			for (cur++; cur; count++) {
				oset_event_types_t type;

				if ((next = strchr(cur, ' '))) {
					*next++ = '\0';
				}

				if (custom) {
					oset_core_hash_delete(listener->event_hash, cur);
				} else if (oset_name_event(cur, &type) == OSET_STATUS_SUCCESS) {
					uint32_t x = 0;
					key_count++;

					if (type == OSET_EVENT_CUSTOM) {
						custom++;
					} else if (type == OSET_EVENT_ALL) {
						for (x = 0; x <= OSET_EVENT_ALL; x++) {
							listener->event_list[x] = 0;
						}
					} else {
						if (listener->event_list[OSET_EVENT_ALL]) {
							listener->event_list[OSET_EVENT_ALL] = 0;
							for (x = 0; x < OSET_EVENT_ALL; x++) {
								listener->event_list[x] = 1;
							}
						}
						listener->event_list[type] = 0;
					}
				}

				cur = next;
			}
		}

		if (!key_count) {
			oset_snprintf(reply, reply_len, "-ERR no keywords supplied");
			goto done;
		}

		if (!oset_test_flag(listener, LFLAG_EVENTS)) {
			oset_set_flag_locked(listener, LFLAG_EVENTS);
		}

		oset_snprintf(reply, reply_len, "+OK events nixed");

	} else if (!strncasecmp(cmd, "noevents", 8)) {
		flush_listener(listener, OSET_FALSE, OSET_TRUE);

		if (oset_test_flag(listener, LFLAG_EVENTS)) {
			uint8_t x = 0;
			oset_clear_flag_locked(listener, LFLAG_EVENTS);
			for (x = 0; x <= OSET_EVENT_ALL; x++) {
				listener->event_list[x] = 0;
			}
			/* wipe the hash */
			oset_core_hash_destroy(&listener->event_hash);
			oset_core_hash_init(&listener->event_hash);
			oset_snprintf(reply, reply_len, "+OK no longer listening for events");
		} else {
			oset_snprintf(reply, reply_len, "-ERR not listening for events");
		}
	}

  done:

	if (zstr(reply)) {
		oset_snprintf(reply, reply_len, "-ERR command not found");
	}

  done_noreply:

	if (event) {
		oset_event_destroy(event);
	}

	return status;
}

static void *OSET_THREAD_FUNC listener_run(oset_apr_thread_t *thread, void *obj)
{
	listener_t *listener = (listener_t *) obj;
	char buf[1024];
	oset_size_t len;
	oset_status_t status;
	oset_event_t *event;
	char reply[512] = "";
	oset_core_session_t *session = NULL;
	oset_event_t *revent = NULL;
	int locked = 1;

	oset_apr_mutex_lock(globals.listener_mutex);
	prefs.threads++;
	oset_apr_mutex_unlock(globals.listener_mutex);

	oset_sys_assert(listener != NULL);

	if ((session = listener->session)) {
		if ((NULL == session->rwlock) || ((oset_status_t) oset_apr_thread_rwlock_tryrdlock(session->rwlock) != OSET_STATUS_SUCCESS)) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Unable to lock session!");
			locked = 0;
			session = NULL;
			goto done;
		}

		listener->lock_acquired = 1;
	}

	if (!listener->sock) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Listener socket is null!");
		oset_clear_flag_locked(listener, LFLAG_RUNNING);
		goto done;
	}

	oset_apr_socket_opt_set(listener->sock, OSET_SO_TCP_NODELAY, TRUE);
	oset_apr_socket_opt_set(listener->sock, OSET_SO_NONBLOCK, TRUE);

	if (prefs.acl_count && listener->sa && !zstr(listener->remote_ip)) {
		uint32_t x = 0;

		for (x = 0; x < prefs.acl_count; x++) {
			if (!sset_check_network_list_ip(listener->remote_ip, prefs.acl[x])) {
				const char message[] = "Access Denied, go away.";
				int mlen = (int)strlen(message);

				oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_WARNING, "IP %s Rejected by acl \"%s\"", listener->remote_ip,
								  prefs.acl[x]);

				oset_snprintf(buf, sizeof(buf), "Content-Type: text/rude-rejection\nContent-Length: %d\n\n", mlen);
				len = strlen(buf);
				oset_apr_socket_send(listener->sock, buf, &len);
				len = mlen;
				oset_apr_socket_send(listener->sock, message, &len);
				goto done;
			}
		}
	}

	if (globals.debug > 0) {
		if (zstr(listener->remote_ip)) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "Connection Open");
		} else {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "Connection Open from %s:%d", listener->remote_ip,
							  listener->remote_port);
		}
	}

	oset_apr_socket_opt_set(listener->sock, OSET_SO_NONBLOCK, TRUE);
	oset_set_flag_locked(listener, LFLAG_RUNNING);
	add_listener(listener);

	if (session && oset_test_flag(listener, LFLAG_AUTHED)) {
		oset_event_t *ievent = NULL;

		oset_set_flag_locked(listener, LFLAG_SESSION);
		status = read_packet(listener, &ievent, 25);

		if (status != OSET_STATUS_SUCCESS || !ievent) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_CRIT, "Socket Error!");
			oset_clear_flag_locked(listener, LFLAG_RUNNING);
			goto done;
		}


		if (parse_command(listener, &ievent, reply, sizeof(reply)) != OSET_STATUS_SUCCESS) {
			oset_clear_flag_locked(listener, LFLAG_RUNNING);
			goto done;
		}


	} else {
		oset_snprintf(buf, sizeof(buf), "Content-Type: auth/request\n\n");

		len = strlen(buf);
		oset_apr_socket_send(listener->sock, buf, &len);

		while (!oset_test_flag(listener, LFLAG_AUTHED)) {
			status = read_packet(listener, &event, 25);
			if (status != OSET_STATUS_SUCCESS) {
				goto done;
			}
			if (!event) {
				continue;
			}

			if (parse_command(listener, &event, reply, sizeof(reply)) != OSET_STATUS_SUCCESS) {
				oset_clear_flag_locked(listener, LFLAG_RUNNING);
				goto done;
			}
			if (*reply != '\0') {
				if (*reply == '~') {
					oset_snprintf(buf, sizeof(buf), "Content-Type: command/reply\n%s", reply + 1);
				} else {
					oset_snprintf(buf, sizeof(buf), "Content-Type: command/reply\nReply-Text: %s\n\n", reply);
				}
				len = strlen(buf);
				oset_apr_socket_send(listener->sock, buf, &len);
			}
			break;
		}
	}

	while (!prefs.done && oset_test_flag(listener, LFLAG_RUNNING) && listen_list.ready) {
		len = sizeof(buf);
		memset(buf, 0, len);
		status = read_packet(listener, &revent, 0);

		if (status != OSET_STATUS_SUCCESS) {
			break;
		}

		if (!revent) {
			continue;
		}

		if (parse_command(listener, &revent, reply, sizeof(reply)) != OSET_STATUS_SUCCESS) {
			oset_clear_flag_locked(listener, LFLAG_RUNNING);
			break;
		}

		if (revent) {
			oset_event_destroy(&revent);
		}

		if (*reply != '\0') {
			if (*reply == '~') {
				oset_snprintf(buf, sizeof(buf), "Content-Type: command/reply\n%s", reply + 1);
			} else {
				oset_snprintf(buf, sizeof(buf), "Content-Type: command/reply\nReply-Text: %s\n\n", reply);
			}
			len = strlen(buf);
			oset_apr_socket_send(listener->sock, buf, &len);
		}

	}

  done:

	if (revent) {
		oset_event_destroy(&revent);
	}

	remove_listener(listener);

	if (globals.debug > 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "Session complete, waiting for children");
	}

	oset_apr_thread_rwlock_wrlock(listener->rwlock);
	flush_listener(listener, OSET_TRUE, OSET_TRUE);
	oset_apr_mutex_lock(listener->filter_mutex);
	if (listener->filters) {
		oset_event_destroy(&listener->filters);
	}
	oset_apr_mutex_unlock(listener->filter_mutex);

	if (listener->sock) {
		send_disconnect(listener, "Disconnected, goodbye.\n");
		close_socket(&listener->sock);
	}

	oset_apr_thread_rwlock_unlock(listener->rwlock);

	if (globals.debug > 0) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "Connection Closed");
	}

	oset_core_hash_destroy(&listener->event_hash);

	if (listener->allowed_event_hash) {
		oset_core_hash_destroy(&listener->allowed_event_hash);
	}

	if (listener->allowed_api_hash) {
		oset_core_hash_destroy(&listener->allowed_api_hash);
	}

	if (listener->session) {
		oset_clear_flag_locked(listener, LFLAG_SESSION);
		if (locked) {
			oset_apr_thread_rwlock_unlock(listener->session->rwlock);
		}
	} else if (listener->pool) {
		oset_apr_memory_pool_t *pool = listener->pool;
		oset_core_destroy_memory_pool(&pool);
	}

	oset_apr_mutex_lock(globals.listener_mutex);
	prefs.threads--;
	oset_apr_mutex_unlock(globals.listener_mutex);

	listener->finished = 1;

	return NULL;
}


/* Create a thread for the socket and launch it */
static oset_status_t launch_listener_thread(listener_t *listener)
{
	oset_apr_thread_t *thread;
	oset_apr_threadattr_t *thd_attr = NULL;

	oset_apr_threadattr_create(&thd_attr, listener->pool);
	oset_apr_threadattr_detach_set(thd_attr, 1);
	oset_apr_threadattr_stacksize_set(thd_attr, OSET_THREAD_STACKSIZE);
	return oset_apr_thread_create(&thread, thd_attr, listener_run, listener, listener->pool);
}

static int config(void)
{
	char *cf = "event_socket.conf";
	oset_xml_t cfg, xml, settings, param;

	memset(&prefs, 0, sizeof(prefs));

	if (!(xml = oset_xml_open_cfg(cf, &cfg, NULL))) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Open of %s failed", cf);
	} else {
		if ((settings = oset_xml_child(cfg, "settings"))) {
			for (param = oset_xml_child(settings, "param"); param; param = param->next) {
				char *var = (char *) oset_xml_attr_soft(param, "name");
				char *val = (char *) oset_xml_attr_soft(param, "value");

				if (!strcmp(var, "listen-ip")) {
					set_pref_ip(val);
				} else if (!strcmp(var, "debug")) {
					globals.debug = atoi(val);
				} else if (!strcmp(var, "listen-port")) {
					prefs.port = (uint16_t) atoi(val);
				} else if (!strcmp(var, "password")) {
					set_pref_pass(val);
				} else if (!strcasecmp(var, "apply-inbound-acl") && ! zstr(val)) {
					if (prefs.acl_count < MAX_ACL) {
						prefs.acl[prefs.acl_count++] = strdup(val);
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Max acl records of %d reached", MAX_ACL);
					}
				} else if (!strcasecmp(var, "stop-on-bind-error")) {
					prefs.stop_on_bind_error = oset_true(val) ? 1 : 0;
				}
			}
		}
		oset_xml_free(xml);
	}

	if (zstr(prefs.ip)) {
		set_pref_ip("127.0.0.1");
	}

	if (zstr(prefs.password)) {
		set_pref_pass("ClueCon");
	}

	if (!prefs.acl_count) {
		prefs.acl[prefs.acl_count++] = strdup("loopback.auto");
	}

	if (!prefs.port) {
		prefs.port = 8021;
	}

	return 0;
}


OSET_MODULE_RUNTIME_FUNCTION(mod_event_socket_runtime)
{
	oset_apr_memory_pool_t *pool = NULL, *listener_pool = NULL;
	oset_status_t rv;
	oset_apr_sockaddr_t *sa;
	oset_apr_socket_t *inbound_socket = NULL;
	listener_t *listener;
	uint32_t x = 0;
	uint32_t errs = 0;

	if (oset_core_new_memory_pool(&pool) != OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "OH OH no pool");
		return OSET_STATUS_TERM;
	}

	config();

	while (!prefs.done) {
		rv = oset_apr_sockaddr_info_get(&sa, prefs.ip, OSET_UNSPEC, prefs.port, 0, pool);
		if (rv) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Cannot get information about IP address %s", prefs.ip);
			goto fail;
		}
		rv = oset_apr_socket_create(&listen_list.sock, oset_apr_sockaddr_get_family(sa), SOCK_STREAM, OSET_PROTO_TCP, pool);
		if (rv)
			goto sock_fail;
		rv = oset_apr_socket_opt_set(listen_list.sock, OSET_SO_REUSEADDR, 1);
		if (rv)
			goto sock_fail;
#ifdef WIN32
		/* Enable dual-stack listening on Windows (if the listening address is IPv6), it's default on Linux */
		if (oset_apr_sockaddr_get_family(sa) == AF_INET6) {
			rv = oset_apr_socket_opt_set(listen_list.sock, OSET_SO_IPV6_V6ONLY, 0);
			if (rv) goto sock_fail;
		}
#endif
		rv = oset_apr_socket_bind(listen_list.sock, sa);
		if (rv)
			goto sock_fail;
		rv = oset_apr_socket_listen(listen_list.sock, 5);
		if (rv)
			goto sock_fail;
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "Socket up listening on %s:%u", prefs.ip, prefs.port);

		break;
	  sock_fail:
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Socket Error! Could not listen on %s:%u", prefs.ip, prefs.port);
		if (prefs.stop_on_bind_error) {
			prefs.done = 1;
			goto fail;
		}
		oset_yield(100000);
	}

	listen_list.ready = 1;


	while (!prefs.done) {
		if (oset_core_new_memory_pool(&listener_pool) != OSET_STATUS_SUCCESS) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "OH OH no pool");
			goto fail;
		}


		if ((rv = oset_apr_socket_accept(&inbound_socket, listen_list.sock, listener_pool))) {
			if (prefs.done) {
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Shutting Down");
				goto end;
			} else {
				/* I wish we could use strerror_r here but its not defined everywhere =/ */
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Socket Error [%s]", strerror(errno));
				if (++errs > 100) {
					goto end;
				}
			}
		} else {
			errs = 0;
		}


		if (!(listener = oset_core_alloc(listener_pool, sizeof(*listener)))) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Memory Error");
			break;
		}

		oset_apr_thread_rwlock_create(&listener->rwlock, listener_pool);
		oset_apr_queue_create(&listener->event_queue, MAX_QUEUE_LEN, listener_pool);
		oset_apr_queue_create(&listener->log_queue, MAX_QUEUE_LEN, listener_pool);

		listener->sock = inbound_socket;
		listener->pool = listener_pool;
		listener_pool = NULL;
		listener->format = EVENT_FORMAT_PLAIN;
		oset_set_flag(listener, LFLAG_FULL);
		oset_set_flag(listener, LFLAG_ALLOW_LOG);

		oset_apr_mutex_init(&listener->flag_mutex, OSET_MUTEX_NESTED, listener->pool);
		oset_apr_mutex_init(&listener->filter_mutex, OSET_MUTEX_NESTED, listener->pool);

		oset_core_hash_init(&listener->event_hash);
		oset_apr_socket_create_pollset(&listener->pollfd, listener->sock, OSET_APR_POLLIN | OSET_APR_POLLERR, listener->pool);


		if (oset_socket_addr_get(&listener->sa, OSET_TRUE, listener->sock) == OSET_STATUS_SUCCESS && listener->sa) {
			oset_apr_get_addr(listener->remote_ip, sizeof(listener->remote_ip), listener->sa);
			if ((listener->remote_port = oset_apr_sockaddr_get_port(listener->sa))) {
				if (launch_listener_thread(listener) == OSET_STATUS_SUCCESS)
					continue;
			}
		}

		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Error initilizing connection");
		close_socket(&listener->sock);
		expire_listener(&listener);

	}

  end:

	close_socket(&listen_list.sock);

	if (pool) {
		oset_core_destroy_memory_pool(&pool);
	}

	if (listener_pool) {
		oset_core_destroy_memory_pool(&listener_pool);
	}


	for (x = 0; x < prefs.acl_count; x++) {
		oset_safe_free(prefs.acl[x]);
	}

  fail:
	return OSET_STATUS_TERM;
}
