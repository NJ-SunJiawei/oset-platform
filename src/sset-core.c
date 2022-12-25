/************************************************************************
 *File name: sset-core.c
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#include "version.h"
#include "sset-core.h"

OSET_DECLARE(const char *) oset_core_banner(void)
{

	return ("\n"
			".=============================================================.\n"
		    "|  _________       _________      _________      ___________  |\n"
		    "| |  _____  |     |  _____  |    |  _______|    |___     ___| |\n"
		    "| | |     | |     | |     |_|    | |                 | |      |\n"
		    "| | |     | |     | |_______     | |_______          | |      |\n"
     		"| | |     | |     |_______  |    |  _______|         | |      |\n"
	    	"| | |     | |      _      | |    | |                 | |      |\n"
		    "| | |_____| |     | |_____| |    | |_______          | |      |\n"
		    "| |_________|     |_________|    |_________|         |_|      |\n"
		    "|                                                             |\n"
			".=============================================================.\n"
			"| SSET Platform create by SSET protocol team                  |\n"
			".=============================================================.\n"
			"\n");
}

static void sset_load_core_config(const char *file);

static void send_heartbeat(void)
{
	oset_event_t *event;
	oset_core_time_duration_t duration;

	oset_core_measure_time(oset_core_uptime(), &duration);

	if (oset_event_create(&event, OSET_EVENT_HEARTBEAT) == OSET_STATUS_SUCCESS) {
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Event-Info", "System Ready");
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Up-Time",
								"%u year%s, "
								"%u day%s, "
								"%u hour%s, "
								"%u minute%s, "
								"%u second%s, "
								"%u millisecond%s, "
								"%u microsecond%s",
								duration.yr, duration.yr == 1 ? "" : "s",
								duration.day, duration.day == 1 ? "" : "s",
								duration.hr, duration.hr == 1 ? "" : "s",
								duration.min, duration.min == 1 ? "" : "s",
								duration.sec, duration.sec == 1 ? "" : "s",
								duration.ms, duration.ms == 1 ? "" : "s", duration.mms, duration.mms == 1 ? "" : "s");

		oset_event_add_header(event, OSET_STACK_BOTTOM, "Uptime-msec", "%"OSET_TIME_T_FMT, oset_core_uptime() / 1000);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Session-Count", "%u", sset_core_session_count());
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Max-Sessions", "%u", sset_core_session_limit(0));
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Session-Per-Sec", "%u", runtime.sps);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Session-Per-Sec-Last", "%u", runtime.sps_last);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Session-Per-Sec-Max", "%u", runtime.sps_peak);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Session-Per-Sec-FiveMin", "%u", runtime.sps_peak_fivemin);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Session-Since-Startup", "%" OSET_SIZE_T_FMT, sset_core_session_id() - 1);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Session-Peak-Max", "%u", runtime.sessions_peak);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Session-Peak-FiveMin", "%u", runtime.sessions_peak_fivemin);
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Idle-CPU", "%f", oset_core_idle_cpu());
		oset_event_fire(&event);
	}
}


OSET_STANDARD_SCHED_FUNC(heartbeat_callback)
{
	send_heartbeat();

	/* reschedule this task */
	//task->runtime = oset_epoch_time_now(NULL) + runtime.event_heartbeat_interval;
}


OSET_DECLARE(oset_size_t) sset_core_session_id(void)
{
	return session_manager.session_id;
}


OSET_DECLARE(uint32_t) sset_core_session_count(void)
{
	return session_manager.session_count;
}


OSET_DECLARE(uint32_t) sset_core_session_limit(uint32_t new_limit)
{
	if (new_limit) {
		session_manager.session_limit = new_limit;
	}

	return session_manager.session_limit;
}

OSET_DECLARE(oset_session_manager_t *) sset_sess_manager(void)
{
    return &session_manager;
}

OSET_DECLARE(int) sset_core_session_sync_clock(void)
{
	int doit = 0;

	oset_apr_mutex_lock(runtime.session_hash_mutex);
	if (session_manager.session_count == 0) {
		doit = 1;
	} else {
		oset_set_flag((&runtime), SCF_SYNC_CLOCK_REQUESTED);
	}
	oset_apr_mutex_unlock(runtime.session_hash_mutex);

	if (doit)  {
		oset_apr_time_sync();
	}

	return doit;

}

void sset_core_sess_manager_init(oset_apr_memory_pool_t *pool)
{
	memset(&session_manager, 0, sizeof(session_manager));
	session_manager.session_limit = 1000;
	session_manager.session_id = 1;
	session_manager.memory_pool = pool;
	oset_core_hash_init(&session_manager.session_table);
	oset_apr_mutex_init(&session_manager.mutex, OSET_MUTEX_DEFAULT, session_manager.memory_pool);
	oset_apr_thread_cond_create(&session_manager.cond, session_manager.memory_pool);
	oset_apr_queue_create(&session_manager.thread_queue, 100000, session_manager.memory_pool);
}

void sset_core_sess_manager_uninit(void)
{
	oset_apr_queue_term(session_manager.thread_queue);
	oset_apr_mutex_lock(session_manager.mutex);
	if (session_manager.running)
		oset_apr_thread_cond_timedwait(session_manager.cond, session_manager.mutex, 10000000);
	oset_apr_mutex_unlock(session_manager.mutex);
	oset_core_hash_destroy(&session_manager.session_table);
}

typedef struct {
	oset_apr_memory_pool_t *pool;
	oset_hashtable_t *hash;
} sset_ip_list_t;
static sset_ip_list_t IP_LIST = { 0 };

OSET_DECLARE(oset_bool_t) sset_insert_auth_network_list_ip(const char *ip_str, const char *list_name)
{
    oset_network_list_t *list;
	if (!list_name) {
	    return OSET_FALSE;
	}

	if ((list = oset_core_hash_find(IP_LIST.hash, list_name))) {
		oset_network_list_add_cidr(list, ip_str, OSET_TRUE);
		return OSET_TRUE;
	}
	return OSET_FALSE;
}


OSET_DECLARE(oset_bool_t) sset_check_network_list_ip_port_token(const char *ip_str, int port, const char *list_name, const char **token)
{
	oset_network_list_t *list;
	ip_t  ip, mask, net;
	uint32_t bits;
	char *ipv6 = strchr(ip_str,':');
	oset_bool_t ok = OSET_FALSE;
	char *ipv4 = NULL;

	if (!list_name) {
		return OSET_FALSE;
	}

	if ((ipv4 = oset_network_ipv4_mapped_ipv6_addr(ip_str))) {
		ip_str = ipv4;
		ipv6 = NULL;
	}

	oset_apr_mutex_lock(runtime.global_mutex);
	if (ipv6) {
		oset_inet_pton2(AF_INET6, ip_str, &ip);
	} else {
		oset_inet_pton2(AF_INET, ip_str, &ip);
		ip.v4 = htonl(ip.v4);
	}

	if ((list = oset_core_hash_find(IP_LIST.hash, list_name))) {
		if (ipv6) {
			ok = oset_network_list_validate_ip6_port_token(list, ip, port, token);
		} else {
			ok = oset_network_list_validate_ip_port_token(list, ip.v4, port, token);
		}
	} else if (strchr(list_name, '/')) {
		if (strchr(list_name, ',')) {
			char *list_name_dup = strdup(list_name);
			char *argv[32];
			int argc;

			oset_sys_assert(list_name_dup);

			if ((argc = oset_separate_string(list_name_dup, ',', argv, (sizeof(argv) / sizeof(argv[0]))))) {
				int i;
				for (i = 0; i < argc; i++) {
					oset_parse_cidr(argv[i], &net, &mask, &bits);
					if (ipv6) {
						if ((ok = oset_testv6_subnet(ip, net, mask))){
							break;
						}
					} else {
						if ((ok = oset_test_subnet(ip.v4, net.v4, mask.v4))) {
							break;
						}
					}
				}
			}
			free(list_name_dup);
		} else {
			oset_parse_cidr(list_name, &net, &mask, &bits);

			if (ipv6) {
				ok = oset_testv6_subnet(ip, net, mask);
			} else {
				ok = oset_test_subnet(ip.v4, net.v4, mask.v4);
			}
		}
	}

	oset_safe_free(ipv4);
	oset_apr_mutex_unlock(runtime.global_mutex);

	return ok;
}

OSET_DECLARE(oset_bool_t) sset_check_network_list_ip_token(const char *ip_str, const char *list_name, const char **token)
{
	return sset_check_network_list_ip_port_token(ip_str, 0, list_name, token);
}


OSET_DECLARE(void) sset_load_network_lists(oset_bool_t reload)
{
	oset_network_list_t *rfc_list, *list;
	char guess_ip[16] = "";
	int mask = 0;
	char guess_mask[16] = "";
	char *tmp_name;
	struct in_addr in;

	oset_find_local_ip(guess_ip, sizeof(guess_ip), &mask, AF_INET);
	in.s_addr = mask;
	oset_set_string(guess_mask, inet_ntoa(in));

	oset_apr_mutex_lock(runtime.global_mutex);

	if (IP_LIST.hash) {
		oset_core_hash_destroy(&IP_LIST.hash);
	}

	if (IP_LIST.pool) {
		oset_core_destroy_memory_pool(&IP_LIST.pool);
	}

	memset(&IP_LIST, 0, sizeof(IP_LIST));
	oset_core_new_memory_pool(&IP_LIST.pool);
	oset_core_hash_init(&IP_LIST.hash);

	tmp_name = "any_v4.auto";
	oset_network_list_create(&rfc_list, tmp_name, OSET_FALSE, IP_LIST.pool);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Created ip list %s default (allow)", tmp_name);
	oset_network_list_add_cidr(rfc_list, "0.0.0.0/0", OSET_TRUE);
	oset_core_hash_insert(IP_LIST.hash, tmp_name, rfc_list);


	tmp_name = "any_v6.auto";
	oset_network_list_create(&rfc_list, tmp_name, OSET_FALSE, IP_LIST.pool);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Created ip list %s default (allow)", tmp_name);
	oset_network_list_add_cidr(rfc_list, "::/0", OSET_TRUE);
	oset_core_hash_insert(IP_LIST.hash, tmp_name, rfc_list);


	tmp_name = "authorization.auto";
	oset_network_list_create(&rfc_list, tmp_name, OSET_FALSE, IP_LIST.pool);
	oset_log2_printf(OSET_CHANNEL_LOG,OSET_LOG2_NOTICE, "Created ip list %s default (deny)", tmp_name);
	oset_network_list_add_cidr(rfc_list, "127.0.0.0/8", OSET_TRUE);
	oset_network_list_add_cidr(rfc_list, "::1/128", OSET_TRUE);
	if (oset_network_list_add_host_mask(rfc_list, guess_ip, guess_mask, OSET_TRUE) == OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Adding %s/%s (allow) to list %s", guess_ip, guess_mask, tmp_name);
	}
	oset_core_hash_insert(IP_LIST.hash, tmp_name, rfc_list);


	tmp_name = "loopback.auto";
	oset_network_list_create(&rfc_list, tmp_name, OSET_FALSE, IP_LIST.pool);
	oset_log2_printf(OSET_CHANNEL_LOG,OSET_LOG2_NOTICE, "Created ip list %s default (deny)", tmp_name);
	oset_network_list_add_cidr(rfc_list, "127.0.0.0/8", OSET_TRUE);
	oset_network_list_add_cidr(rfc_list, "::1/128", OSET_TRUE);
	oset_core_hash_insert(IP_LIST.hash, tmp_name, rfc_list);

	tmp_name = "localnet.auto";
	oset_network_list_create(&list, tmp_name, OSET_FALSE, IP_LIST.pool);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Created ip list %s default (deny)", tmp_name);

	if (oset_network_list_add_host_mask(list, guess_ip, guess_mask, OSET_TRUE) == OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Adding %s/%s (allow) to list %s", guess_ip, guess_mask, tmp_name);
	}
	oset_core_hash_insert(IP_LIST.hash, tmp_name, list);

	oset_apr_mutex_unlock(runtime.global_mutex);
}


static void sset_load_core_config(const char *file)
{
	oset_xml_t xml = NULL, cfg = NULL;

	if ((xml = oset_xml_open_cfg(file, &cfg, NULL))) {
		oset_xml_t settings, param;

		if ((settings = oset_xml_child(cfg, "settings"))) {
			for (param = oset_xml_child(settings, "param"); param; param = param->next) {
				const char *var = oset_xml_attr_soft(param, "name");
				const char *val = oset_xml_attr_soft(param, "value");

				if (!strcasecmp(var, "loglevel")) {
					int level;
					if (*val > 47 && *val < 58) {
						level = atoi(val);
					} else {
						level = oset_log2_str2level(val);
					}

					if (level != OSET_LOG2_INVALID) {
						sset_core_session_ctl(SCSC_LOGLEVEL, &level);
					}
#ifdef HAVE_SETRLIMIT
				} else if (!strcasecmp(var, "dump-cores") && oset_true(val)) {
					struct rlimit rlp;
					memset(&rlp, 0, sizeof(rlp));
					rlp.rlim_cur = RLIM_INFINITY;
					rlp.rlim_max = RLIM_INFINITY;
					setrlimit(RLIMIT_CORE, &rlp);
#endif
				} else if (!strcasecmp(var, "debug-level")) {
					int tmp = atoi(val);
					if (tmp > -1 && tmp < 11) {
						sset_core_session_ctl(SCSC_DEBUG_LEVEL, &tmp);
					}
				} else if (!strcasecmp(var, "pool-size-128")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 65536*100) {
						runtime.defconfig.cluster_128_pool = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pool-size-128 must be between 0 and 65536*100");
					}
				} else if (!strcasecmp(var, "pool-size-256")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 16384*100) {
						runtime.defconfig.cluster_256_pool = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pool-size-256 must be between 0 and 16384*100");
					}
				} else if (!strcasecmp(var, "pool-size-512")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 4096*100) {
						runtime.defconfig.cluster_512_pool = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pool-size-512 must be between 0 and 4096*100");
					}
				} else if (!strcasecmp(var, "pool-size-1024")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 1024*100) {
						runtime.defconfig.cluster_1024_pool = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pool-size-1024 must be between 0 and 1024*100");
					}
				} else if (!strcasecmp(var, "pool-size-2048")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 512*100) {
						runtime.defconfig.cluster_2048_pool = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pool-size-2048 must be between 0 and 512*100");
					}
				} else if (!strcasecmp(var, "pool-size-8192")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 128*100) {
						runtime.defconfig.cluster_8192_pool = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pool-size-8192 must be between 0 and 128*100");
					}
				} else if (!strcasecmp(var, "pool-size-1024*1024")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 8*100) {
						runtime.defconfig.cluster_big_pool = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "pool-size-1024*1024 must be between 0 and 8*100");
					}
				} else if (!strcasecmp(var, "thread-pool-size")) {
					int tmp = atoi(val);

					if (tmp > 0 && tmp < 100) {
						runtime.max_thread_pool_size = (int) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "thread-pool-size must be between 0 and 100");
					}
				} else if (!strcasecmp(var, "max-db-handles")) {
					long tmp = atol(val);

					if (tmp > 4 && tmp < 5001) {
						runtime.max_db_handles = (uint32_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "max-db-handles must be between 5 and 5000");
					}
				} else if (!strcasecmp(var, "db-handle-timeout")) {
					long tmp = atol(val);

					if (tmp > 0 && tmp < 5001) {
						runtime.db_handle_timeout = (uint32_t) tmp * 1000000;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "db-handle-timeout must be between 1 and 5000");
					}

				} else if (!strcasecmp(var, "event-heartbeat-interval")) {
					long tmp = atol(val);

					if (tmp > 0) {
						runtime.event_heartbeat_interval = (uint32_t) tmp;
					} else {
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "heartbeat-interval must be a greater than 0");
					}

				} else if (!strcasecmp(var, "multiple-registrations")) {
					runtime.multiple_registrations = oset_true(val);
				} else if (!strcasecmp(var, "auto-create-schemas")) {
					if (oset_true(val)) {
						oset_set_flag((&runtime), SCF_AUTO_SCHEMAS);
					} else {
						oset_clear_flag((&runtime), SCF_AUTO_SCHEMAS);
					}
				} else if (!strcasecmp(var, "session-thread-pool")) {
					if (oset_true(val)) {
						oset_set_flag((&runtime), SCF_SESSION_THREAD_POOL);
					} else {
						oset_clear_flag((&runtime), SCF_SESSION_THREAD_POOL);
					}
				} else if (!strcasecmp(var, "auto-clear-sql")) {
					if (oset_true(val)) {
						oset_set_flag((&runtime), SCF_CLEAR_SQL);
					} else {
						oset_clear_flag((&runtime), SCF_CLEAR_SQL);
					}
				} else if (!strcasecmp(var, "api-expansion")) {
					if (oset_true(val)) {
						oset_set_flag((&runtime), SCF_API_EXPANSION);
					} else {
						oset_clear_flag((&runtime), SCF_API_EXPANSION);
					}
				} else if (!strcasecmp(var, "enable-early-hangup") && oset_true(val)) {
					oset_set_flag((&runtime), SCF_EARLY_HANGUP);
				} else if (!strcasecmp(var, "colorize-console") && oset_true(val)) {
					runtime.colorize_console = OSET_TRUE;
				} else if (!strcasecmp(var, "core-db-pre-trans-execute") && !zstr(val)) {
					runtime.core_db_pre_trans_execute = oset_core_strdup(runtime.memory_pool, val);
				} else if (!strcasecmp(var, "core-db-post-trans-execute") && !zstr(val)) {
					runtime.core_db_post_trans_execute = oset_core_strdup(runtime.memory_pool, val);
				} else if (!strcasecmp(var, "core-db-inner-pre-trans-execute") && !zstr(val)) {
					runtime.core_db_inner_pre_trans_execute = oset_core_strdup(runtime.memory_pool, val);
				} else if (!strcasecmp(var, "core-db-inner-post-trans-execute") && !zstr(val)) {
					runtime.core_db_inner_post_trans_execute = oset_core_strdup(runtime.memory_pool, val);
				}else if (!strcasecmp(var, "sessions-per-second") && !zstr(val)) {
					oset_core_sessions_per_second(atoi(val));
				}else if (!strcasecmp(var, "enable-use-system-time")) {
					oset_apr_time_set_use_system_time(oset_true(val));
				} else if (!strcasecmp(var, "enable-monotonic-timing")) {
					oset_apr_time_set_monotonic(oset_true(val));
				} else if (!strcasecmp(var, "enable-softtimer-timerfd")) {
					int ival = 0;
					if (val) {
						if (oset_true(val)) {
							ival = 2;
						} else {
							if (strcasecmp(val, "broadcast")) {
								ival = 1;
							} else if (strcasecmp(val, "fd-per-timer")) {
								ival = 2;
							}
						}
					}
					oset_apr_time_set_timerfd(ival);
				} else if (!strcasecmp(var, "enable-clock-nanosleep")) {
					oset_apr_time_set_nanosleep(oset_true(val));
				} else if (!strcasecmp(var, "enable-cond-yield")) {
					oset_apr_time_set_cond_yield(oset_true(val));
				} else if (!strcasecmp(var, "enable-timer-matrix")) {
					oset_apr_time_set_matrix(oset_true(val));
				} else if (!strcasecmp(var, "max-sessions") && !zstr(val)) {
					sset_core_session_limit(atoi(val));
				} else if (!strcasecmp(var, "verbose-channel-events") && !zstr(val)) {
					int v = oset_true(val);
					if (v) {
						oset_set_flag((&runtime), SCF_VERBOSE_EVENTS);
					} else {
						oset_clear_flag((&runtime), SCF_VERBOSE_EVENTS);
					}
				} else if (!strcasecmp(var, "threaded-system-exec") && !zstr(val)) {
					int v = oset_true(val);
					if (v) {
						oset_set_flag((&runtime), SCF_THREADED_SYSTEM_EXEC);
					} else {
						oset_clear_flag((&runtime), SCF_THREADED_SYSTEM_EXEC);
					}
				} else if (!strcasecmp(var, "spawn-instead-of-system") && !zstr(val)) {
					int v = oset_true(val);
					if (v) {
						oset_core_set_variable("spawn_instead_of_system", "true");
					} else {
						oset_core_set_variable("spawn_instead_of_system", "false");
					}
				} else if (!strcasecmp(var, "min-idle-cpu") && !zstr(val)) {
					oset_core_min_idle_cpu(atof(val));
				} else if (!strcasecmp(var, "tipping-point") && !zstr(val)) {
					runtime.tipping_point = atoi(val);
				} else if (!strcasecmp(var, "cpu-idle-smoothing-depth") && !zstr(val)) {
					runtime.cpu_idle_smoothing_depth = atoi(val);
				} else if (!strcasecmp(var, "events-use-dispatch") && !zstr(val)) {
					runtime.events_use_dispatch = oset_true(val);
				} else if (!strcasecmp(var, "initial-event-threads") && !zstr(val)) {
					int tmp;

					if (!runtime.events_use_dispatch) {
						runtime.events_use_dispatch = 1;
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING,
										  "Implicitly setting events-use-dispatch based on usage of this initial-event-threads parameter.");
					}

					tmp = atoi(val);

					if (tmp > runtime.cpu_count / 2) {
						tmp = runtime.cpu_count / 2;
						oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "This value cannot be higher than %d so setting it to that value",
										  runtime.cpu_count / 2);
					}

					if (tmp < 1) {
						tmp = 1;
						oset_log2_printf(OSET_CHANNEL_LOG,OSET_LOG2_WARNING, "This value cannot be lower than 1 so setting it to that level");
					}

					oset_event_launch_dispatch_threads(tmp);

				} else if (!strcasecmp(var, "1ms-timer") && oset_true(val)) {
					runtime.microseconds_per_tick = 1000;
				} else if (!strcasecmp(var, "timer-affinity") && !zstr(val)) {
					if (!strcasecmp(val, "disabled")) {
						runtime.timer_affinity = -1;
					} else {
						runtime.timer_affinity = atoi(val);
					}
				} else if (!strcasecmp(var, "udp-port-usage-robustness") && oset_true(val)) {
					runtime.port_alloc_flags |= SPF_ROBUST_UDP;
				} else if (!strcasecmp(var, "core-db-name") && !zstr(val)) {
					runtime.dbname = oset_core_strdup(runtime.memory_pool, val);
				} else if (!strcasecmp(var, "core-db-dsn") && !zstr(val)) {
					runtime.odbc_dsn = oset_core_strdup(runtime.memory_pool, val);
				} else if (!strcasecmp(var, "core-non-sqlite-db-required") && !zstr(val)) {
					oset_set_flag((&runtime), SCF_CORE_NON_SQLITE_DB_REQ);
				} else if (!strcasecmp(var, "core-dbtype") && !zstr(val)) {
					if (!strcasecmp(val, "MSSQL")) {
						runtime.odbc_dbtype = DBTYPE_MSSQL;
					} else {
						runtime.odbc_dbtype = DBTYPE_DEFAULT;
					}
				} else if (!strcasecmp(var, "osetname") && !zstr(val)) {
					runtime.osetname = oset_core_strdup(runtime.memory_pool, val);
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Set osetname to %s", runtime.osetname);
				}else if (!strcasecmp(var, "caller-profile-soft-variables-uses-prefix") && !zstr(val)) {
					int v = oset_true(val);
					if (v) {
						oset_set_flag((&runtime), SCF_CPF_SOFT_PREFIX);
					} else {
						oset_clear_flag((&runtime), SCF_CPF_SOFT_PREFIX);
					}
				} else if (!strcasecmp(var, "event-channel-key-separator") && !zstr(val)) {
					runtime.event_channel_key_separator = oset_core_strdup(runtime.memory_pool, val);
				}
			}
		}

		if (runtime.event_channel_key_separator == NULL) {
			runtime.event_channel_key_separator = oset_core_strdup(runtime.memory_pool, ".");
		}

		if ((settings = oset_xml_child(cfg, "variables"))) {
			for (param = oset_xml_child(settings, "variable"); param; param = param->next) {
				const char *var = oset_xml_attr_soft(param, "name");
				const char *val = oset_xml_attr_soft(param, "value");
				if (var && val) {
					oset_core_set_variable(var, val);
				}
			}
		}

		oset_xml_free(xml);
	}

}


OSET_DECLARE(void) sset_core_runtime_loop(int bg)
{
	if (bg) {
		while (runtime.running) {
			oset_yield(1000000);
		}
	} else {
		/* wait for console input */
		oset_console_loop();
	}
}

OSET_DECLARE(oset_status_t) sset_core_destroy(void)
{
	oset_event_t *event;

	if (oset_event_create(&event, OSET_EVENT_SHUTDOWN) == OSET_STATUS_SUCCESS) {
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Event-Info", "System Shutting Down");
		oset_event_fire(&event);
	}

	oset_set_flag((&runtime), SCF_SHUTTING_DOWN);
	//oset_scheduler_del_task_id(runtime.hb_task_id);

	//oset_core_session_hupall(runtime.shutdown_cause);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "End existing sessions.");

	oset_scheduler_task_thread_stop();

	oset_loadable_module_shutdown();
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "Clean up modules.");

	oset_xml_destroy();
	oset_console_shutdown();

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "Closing Event Engine.");
	oset_event_shutdown();

	sset_core_sess_manager_uninit();
	oset_core_memory_stop();

	if (runtime.console && runtime.console != stdout && runtime.console != stderr) {
		fclose(runtime.console);
		runtime.console = NULL;
	}

	oset_safe_free(OSET_GLOBAL_dirs.base_dir);
	oset_safe_free(OSET_GLOBAL_dirs.mod_dir);
	oset_safe_free(OSET_GLOBAL_dirs.conf_dir);
	oset_safe_free(OSET_GLOBAL_dirs.log_dir);
	oset_safe_free(OSET_GLOBAL_dirs.db_dir);
	oset_safe_free(OSET_GLOBAL_dirs.run_dir);
	oset_safe_free(OSET_GLOBAL_dirs.temp_dir);

	oset_safe_free(OSET_GLOBAL_filenames.conf_name);

	oset_event_destroy(&runtime.global_vars);

	if (IP_LIST.hash) {
		oset_core_hash_destroy(&IP_LIST.hash);
	}

	if (IP_LIST.pool) {
		oset_core_destroy_memory_pool(&IP_LIST.pool);
	}

	oset_threadpool_destory(runtime.thrp);

    oset_pkbuf_default_destroy();
	oset_core_terminate();
    //oset_pkbuf_final();

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "Finalizing Shutdown.");
	oset_log2_shutdown();

	if (runtime.memory_pool) {
		apr_pool_destroy(runtime.memory_pool);
	}

	sqlite3_shutdown();

	return oset_test_flag((&runtime), SCF_RESTART) ? OSET_STATUS_RESTART : OSET_STATUS_SUCCESS;
}


OSET_DECLARE(oset_status_t) sset_core_init(oset_core_flag_t flags, oset_bool_t console, const char **err)
{
	oset_uuid_t uuid;
	char guess_ip4[256];
	char guess_ip6[256];
	int mask = 0;
	struct in_addr in;
    apr_status_t rv;

	if (runtime.runlevel > 0) {
		/* one per customer */
		return OSET_STATUS_SUCCESS;
	}

    oset_pkbuf_default_init(&runtime.defconfig);

	memset(&runtime, 0, sizeof(runtime));
	gethostname(runtime.hostname, sizeof(runtime.hostname));

	runtime.max_thread_pool_size = 10;
	runtime.shutdown_cause = OSET_CAUSE_SYSTEM_SHUTDOWN;
	runtime.max_db_handles = 50;
	runtime.db_handle_timeout = 5000000;
	runtime.event_heartbeat_interval = 20;

	runtime.runlevel++;
	runtime.events_use_dispatch = 1; 

	runtime.dbname = "core";
	oset_set_flag((&runtime), SCF_AUTO_SCHEMAS);
	oset_set_flag((&runtime), SCF_CLEAR_SQL);
	oset_set_flag((&runtime), SCF_API_EXPANSION);
	oset_set_flag((&runtime), SCF_SESSION_THREAD_POOL);
	if (flags & SCF_LOG_DISABLE) {
		runtime.hard_log_level = OSET_LOG2_DISABLE;
		flags &= ~SCF_LOG_DISABLE;
	} else {
		runtime.hard_log_level = OSET_LOG2_DEBUG;
	}
	runtime.odbc_dbtype = DBTYPE_DEFAULT;
	runtime.dbname = NULL;
	runtime.cpu_count = sysconf (_SC_NPROCESSORS_ONLN);

	if (!runtime.cpu_count) runtime.cpu_count = 1;

	if (sqlite3_initialize() != SQLITE_OK) {
		*err = "FATAL ERROR! Could not initialize SQLite\n";
		return OSET_STATUS_MEMERR;
	}

	if (!(runtime.memory_pool = oset_core_memory_init())) {
		*err = "FATAL ERROR! Could not allocate memory pool\n";
		return OSET_STATUS_MEMERR;
	}
	oset_sys_assert(runtime.memory_pool != NULL);

	oset_apr_dir_make_recursive(OSET_GLOBAL_dirs.base_dir, OSET_DEFAULT_DIR_PERMS, runtime.memory_pool);
	oset_apr_dir_make_recursive(OSET_GLOBAL_dirs.mod_dir, OSET_DEFAULT_DIR_PERMS, runtime.memory_pool);
	oset_apr_dir_make_recursive(OSET_GLOBAL_dirs.conf_dir, OSET_DEFAULT_DIR_PERMS, runtime.memory_pool);
	oset_apr_dir_make_recursive(OSET_GLOBAL_dirs.log_dir, OSET_DEFAULT_DIR_PERMS, runtime.memory_pool);
	oset_apr_dir_make_recursive(OSET_GLOBAL_dirs.run_dir, OSET_DEFAULT_DIR_PERMS, runtime.memory_pool);
	oset_apr_dir_make_recursive(OSET_GLOBAL_dirs.db_dir, OSET_DEFAULT_DIR_PERMS, runtime.memory_pool);
	oset_apr_dir_make_recursive(OSET_GLOBAL_dirs.temp_dir, OSET_DEFAULT_DIR_PERMS, runtime.memory_pool);

	oset_apr_mutex_init(&runtime.uuid_mutex, OSET_MUTEX_NESTED, runtime.memory_pool);

	oset_apr_mutex_init(&runtime.throttle_mutex, OSET_MUTEX_NESTED, runtime.memory_pool);

	oset_apr_mutex_init(&runtime.session_hash_mutex, OSET_MUTEX_NESTED, runtime.memory_pool);
	oset_apr_mutex_init(&runtime.global_mutex, OSET_MUTEX_NESTED, runtime.memory_pool);

	oset_apr_thread_rwlock_create(&runtime.global_var_rwlock, runtime.memory_pool);
	sset_core_set_globals();
	sset_core_sess_manager_init(runtime.memory_pool);
	oset_event_create_plain(&runtime.global_vars, OSET_EVENT_CHANNEL_DATA);
	runtime.flags |= flags;
	runtime.sps_total = 30;

	*err = NULL;

	if (console) {
		runtime.console = stdout;
	}

	oset_core_set_variable("hostname", runtime.hostname);
	oset_find_local_ip(guess_ip4, sizeof(guess_ip4), &mask, AF_INET);
	oset_core_set_variable("local_ip_v4", guess_ip4);
	in.s_addr = mask;
	oset_core_set_variable("local_mask_v4", inet_ntoa(in));

	oset_find_local_ip(guess_ip6, sizeof(guess_ip6), NULL, AF_INET6);
	oset_core_set_variable("local_ip_v6", guess_ip6);

	oset_core_set_variable("base_dir", OSET_GLOBAL_dirs.base_dir);
	oset_core_set_variable("conf_dir", OSET_GLOBAL_dirs.conf_dir);
	oset_core_set_variable("log_dir", OSET_GLOBAL_dirs.log_dir);
	oset_core_set_variable("run_dir", OSET_GLOBAL_dirs.run_dir);
	oset_core_set_variable("db_dir", OSET_GLOBAL_dirs.db_dir);
	oset_core_set_variable("mod_dir", OSET_GLOBAL_dirs.mod_dir);
	oset_core_set_variable("temp_dir", OSET_GLOBAL_dirs.temp_dir);


	oset_console_init(runtime.memory_pool);
	oset_event_init(runtime.memory_pool);

	if (oset_xml_init(runtime.memory_pool, err) != OSET_STATUS_SUCCESS) {
		/* allow missing configuration if MINIMAL */
		if (!(flags & SCF_MINIMAL)) {
			apr_terminate();
		    oset_core_terminate();
		    //oset_pkbuf_final();
			return OSET_STATUS_MEMERR;
		}
	}
	oset_log2_init(runtime.memory_pool, runtime.colorize_console);

	runtime.tipping_point = 0;
	runtime.timer_affinity = -1;
	runtime.microseconds_per_tick = 20000;

	if (flags & SCF_MINIMAL) return OSET_STATUS_SUCCESS;

	sset_load_core_config("core.conf");

    oset_pkbuf_default_create(&runtime.defconfig);

    /*include apr threadpool,use apr_thread_pool_push scheduler depend on priority */
    rv = oset_threadpool_create(&runtime.thrp, runtime.max_thread_pool_size, runtime.max_thread_pool_size);
    oset_sys_assert(OSET_OK == rv);

    /*scheduler depend on time*/
	oset_scheduler_task_thread_start();

	runtime.running = 1;
	runtime.initiated = oset_mono_micro_time_now();

    /*timer */

	//runtime.hb_task_id = oset_scheduler_add_task(oset_epoch_time_now(NULL), heartbeat_callback, "heartbeat", "core", 0, NULL, SSHF_NONE | SSHF_NO_DEL);
	runtime.hb_task_id = oset_scheduler_add_task(runtime.event_heartbeat_interval, heartbeat_callback, "heartbeat", "core", 0, NULL, SSHF_NONE | SSHF_NO_DEL);

	oset_apr_uuid_get(&uuid);
	oset_apr_uuid_format(runtime.uuid_str, &uuid);
	oset_core_set_variable("core_uuid", runtime.uuid_str);

	return OSET_STATUS_SUCCESS;
}

OSET_DECLARE(oset_status_t) sset_core_init_and_modload(oset_core_flag_t flags, oset_bool_t console, const char **err)
{
	oset_event_t *event;

	if (sset_core_init(flags, console, err) != OSET_STATUS_SUCCESS) {
		return OSET_STATUS_GENERR;
	}

	if (runtime.runlevel > 1) {
		/* one per customer */
		return OSET_STATUS_SUCCESS;
	}

	runtime.runlevel++;
	runtime.events_use_dispatch = 1; 

	oset_core_set_signal_handlers();
	sset_load_network_lists(OSET_FALSE);

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "Bringing up environment.");
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "Loading Modules.");
	if (oset_loadable_module_init(OSET_TRUE) != OSET_STATUS_SUCCESS) {
		*err = "Cannot load modules";
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "Error: %s", *err);
		return OSET_STATUS_GENERR;
	}

	oset_core_set_signal_handlers();

	if (oset_event_create(&event, OSET_EVENT_STARTUP) == OSET_STATUS_SUCCESS) {
		oset_event_add_header(event, OSET_STACK_BOTTOM, "Event-Info", "System Ready");
		oset_event_fire(&event);
	}

    if(console){
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CONSOLE, "%s%s%s%s%s\n",
						  OSET_SEQ_DEFAULT_COLOR,
						  OSET_SEQ_FYELLOW, OSET_SEQ_BBLUE,
						  oset_core_banner(),
						  OSET_SEQ_DEFAULT_COLOR);
	}else{
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "\n%s",oset_core_banner());
	}

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO,
					  "OSET Version %s \nOSET Started >>> Max Sessions [%u]  Session Rate [%d]  SQL [%s]\n",
					  SSETOM_VERSION,
					  sset_core_session_limit(0),
					  oset_core_sessions_per_second(0), oset_test_flag((&runtime), SCF_USE_SQL) ? "Enabled" : "Disabled");

#ifdef HAVE_SYSTEMD
	sd_notifyf(0, "READY=1\n"
		"MAINPID=%lu\n", (unsigned long) getpid());
#endif
	return OSET_STATUS_SUCCESS;

}




