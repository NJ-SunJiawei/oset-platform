/************************************************************************
 *File name: sset-core.h
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/
#ifndef OM_CORE_H
#define OM_CORE_H

#include "oset-core.h"

#define OM_PREFIX_DIR "/usr/local/install/sset"
#define OSET_DEFAULT_DIR_PERMS OSET_FPROT_UREAD | OSET_FPROT_UWRITE | OSET_FPROT_UEXECUTE | OSET_FPROT_GREAD | OSET_FPROT_GEXECUTE

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "openSET"

OSET_BEGIN_EXTERN_C

typedef enum {
	//SCSC_PAUSE_ALL,
	SCSC_HUPALL,
	SCSC_SHUTDOWN,
	SCSC_CHECK_RUNNING,
	SCSC_LOGLEVEL,
	SCSC_SPS,
	SCSC_LAST_SPS,
	SCSC_RECLAIM,
	SCSC_SYNC_CLOCK,
	SCSC_CANCEL_SHUTDOWN,
	SCSC_SEND_SIGHUP,
	SCSC_DEBUG_LEVEL,
	SCSC_FLUSH_DB_HANDLES,
	SCSC_SHUTDOWN_NOW,
	SCSC_REINCARNATE_NOW,
	SCSC_CALIBRATE_CLOCK,
	SCSC_SAVE_HISTORY,
	SCSC_CRASH,
	SCSC_MIN_IDLE_CPU,
	SCSC_VERBOSE_EVENTS,
	SCSC_SHUTDOWN_CHECK,
	//SCSC_PAUSE_CHECK,
	//SCSC_READY_CHECK,
	SCSC_THREADED_SYSTEM_EXEC,
	SCSC_SYNC_CLOCK_WHEN_IDLE,
	SCSC_DEBUG_SQL,
	SCSC_SQL,
	SCSC_API_EXPANSION,
	SCSC_SPS_PEAK,
	SCSC_SPS_PEAK_FIVEMIN,
	SCSC_SESSIONS_PEAK,
	SCSC_SESSIONS_PEAK_FIVEMIN,
	SCSC_SHUTDOWN_CAUSE
}sset_session_ctl_t;
	
OSET_DECLARE(const char *) oset_core_banner(void);

OSET_DECLARE(int32_t) sset_core_session_ctl(sset_session_ctl_t cmd, void *val);
OSET_DECLARE(int) sset_core_session_sync_clock(void);
OSET_DECLARE(oset_session_manager_t *) sset_sess_manager(void);

OSET_DECLARE(oset_status_t) sset_core_init(_In_ oset_core_flag_t flags, _In_ oset_bool_t console, _Out_ const char **err);
OSET_DECLARE(oset_status_t) sset_core_init_and_modload(_In_ oset_core_flag_t flags, _In_ oset_bool_t console, _Out_ const char **err);
OSET_DECLARE(oset_status_t) sset_core_destroy(void);
OSET_DECLARE(void) sset_core_runtime_loop(int bg);
OSET_DECLARE(void) sset_core_set_globals(void);


OSET_DECLARE(uint32_t) sset_core_session_count(void);
OSET_DECLARE(oset_size_t) sset_core_session_id(void);
OSET_DECLARE(uint32_t) sset_core_session_limit(uint32_t new_limit);

void sset_core_sess_manager_init(oset_apr_memory_pool_t *pool);

OSET_DECLARE(oset_bool_t) sset_insert_auth_network_list_ip(const char *ip_str, const char *list_name);
OSET_DECLARE(oset_bool_t) sset_check_network_list_ip_port_token(const char *ip_str, int port, const char *list_name, const char **token);
OSET_DECLARE(oset_bool_t) sset_check_network_list_ip_token(const char *ip_str, const char *list_name, const char **token);
#define sset_check_network_list_ip(_ip_str, _list_name) sset_check_network_list_ip_token(_ip_str, _list_name, NULL)
OSET_DECLARE(void) sset_load_network_lists(oset_bool_t reload);


OSET_END_EXTERN_C

#endif

