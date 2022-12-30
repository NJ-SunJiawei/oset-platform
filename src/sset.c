/************************************************************************
 *File name: sset.c
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#ifndef WIN32
#include <poll.h>
#ifdef HAVE_SETRLIMIT
#include <sys/resource.h>
#endif
#endif

#ifdef __linux__
#include <sys/prctl.h>
#endif


#include "sset-core.h"
#include "version.h"


#define PIDFILE "sset.pid"
static char *pfile = PIDFILE;
static int system_ready = 0;

#ifndef OM_PREFIX_DIR
#define OM_PREFIX_DIR "."
#endif


static const char helpmap[] =
	"Usage: OSET [OPTIONS]\n\n"
	"These are the optional arguments you can pass to om:\n"
	"\t-version               -- print the version and exit\n"

	"\t-help                  -- this message\n"
	"\t-c                     -- output to a console and stay in the foreground (default)\n"
	"\t-u [user]              -- specify user to switch to\n"
	"\t-g [group]             -- specify group to switch to\n"


#ifdef HAVE_SETRLIMIT
	"\t-core                  -- dump cores\n"
#endif
	"\t-rp                    -- enable high(realtime) priority settings\n"
	"\t-lp                    -- enable low priority settings\n"
	"\t-np                    -- enable normal priority settings\n"
	"\t-nosql                 -- disable internal sql scoreboard\n"
	"\t-nocal                 -- disable clock calibration\n"
	"\t-nort                  -- disable clock clock_realtime\n"
	"\t-stop                  -- stop sset\n"
	"\t-nc                    -- do not output to a console and background\n"
#ifndef WIN32
	"\t-ncwait                -- do not output to a console and background but wait until the system is ready before exiting (implies -nc)\n"
#endif

	"\n\tOptions to control locations of files:\n"
	"\t-base [basedir]         -- alternate prefix directory\n"
	"\t-cfgname [filename]     -- alternate filename for main configuration file\n"
	"\t-conf [confdir]         -- alternate directory for configuration files\n"
	"\t-log [logdir]           -- alternate directory for logfiles\n"
	"\t-run [rundir]           -- alternate directory for runtime files\n"
	"\t-db [dbdir]             -- alternate directory for the internal database\n"
	"\t-mod [moddir]           -- alternate directory for modules\n";


static void show_version(void)
{
	fprintf(stdout, "SSET VERSION %s\n\n", SSETOM_VERSION);
}


static oset_bool_t is_option(const char *p)
{
	/* skip whitespaces */
	while ((*p == 13) || (*p == 10) || (*p == 9) || (*p == 32) || (*p == 11)) p++;
	return (p[0] == '-');
}


static int oset_kill_background()
{
	FILE *f;					/* FILE handle to open the pid file */
	char path[256] = "";		/* full path of the PID file */
	pid_t pid = 0;				/* pid from the pid file */

	/* set the globals so we can use the global paths. */
	sset_core_set_globals();

	/* get the full path of the pid file. */
	oset_apr_snprintf(path, sizeof(path), "%s%s%s", OSET_GLOBAL_dirs.run_dir, OSET_PATH_SEPARATOR, pfile);

	/* open the pid file */
	if ((f = fopen(path, "r")) == 0) {
		/* pid file does not exist */
		fprintf(stderr, "Cannot open pid file %s.\n", path);
		return 255;
	}

	/* pull the pid from the file */
	if (fscanf(f, "%d", (int *) (intptr_t) & pid) != 1) {
		fprintf(stderr, "Unable to get the pid!\n");

	}

	/* if we have a valid pid */
	if (pid > 0) {

		/* kill the sset running at the pid we found */
		fprintf(stderr, "Killing: %d\n", (int) pid);
#ifdef WIN32
		/* for windows we need the event to signal for shutting down a background FreeSWITCH */
		snprintf(path, sizeof(path), "Global\\sset-om.%d", pid);

		/* open the event so we can signal it */
		shutdown_event = OpenEvent(EVENT_MODIFY_STATE, FALSE, path);

		/* did we successfully open the event */
		if (!shutdown_event) {
			/* we can't get the event, so we can't signal the process to shutdown */
			fprintf(stderr, "ERROR: Can't Shutdown: %d\n", (int) pid);
		} else {
			/* signal the event to shutdown */
			SetEvent(shutdown_event);
			/* cleanup */
			CloseHandle(shutdown_event);
		}
#else
		/* for unix, send the signal to kill. */
		kill(pid, SIGTERM);
#endif
	}

	/* be nice and close the file handle to the pid file */
	fclose(f);

	return 0;
}

static void handle_SIG(int sig)
{
	int32_t arg = 0;
	if (sig) {}
	/* send shutdown signal to the om core */
	sset_core_session_ctl(SCSC_SHUTDOWN, &arg);
	return;
}

static int check_fd(int fd, int ms)
{
	struct pollfd pfds[2] = { { 0 } };
	int s, r = 0, i = 0;

	pfds[0].fd = fd;
	pfds[0].events = POLLIN | POLLERR;
	s = poll(pfds, 1, ms);

	if (s == 0 || s == -1) {
		r = s;
	} else {
		r = -1;

		if ((pfds[0].revents & POLLIN)) {
			if ((i = read(fd, &r, sizeof(r))) > -1) {
				(void)write(fd, &r, sizeof(r));
			}
		}
	}

	return r;
}


static void daemonize(int *fds)
{
	int fd;
	pid_t pid;
	unsigned int sanity = 60;

	if (!fds) {
		switch (fork()) {
		case 0:		/* child process */
			break;
		case -1:
			fprintf(stderr, "Error Backgrounding (fork)! %d - %s\n", errno, strerror(errno));
			exit(EXIT_SUCCESS);
			break;
		default:	/* parent process */
			exit(EXIT_SUCCESS);
		}

		if (setsid() < 0) {
			fprintf(stderr, "Error Backgrounding (setsid)! %d - %s\n", errno, strerror(errno));
			exit(EXIT_SUCCESS);
		}
	}

	pid = oset_fork();

	switch (pid) {
	case 0:		/* child process */
		if (fds) {
			close(fds[0]);
		}
		break;
	case -1:
		fprintf(stderr, "Error Backgrounding (fork2)! %d - %s\n", errno, strerror(errno));
		exit(EXIT_SUCCESS);
		break;
	default:	/* parent process */
		fprintf(stderr, "%d Backgrounding.\n", (int) pid);

		if (fds) {
			char *o;

			close(fds[1]);

			if ((o = oset_env_get("OSET_BG_TIMEOUT"))) {
				int tmp = atoi(o);
				if (tmp > 0) {
					sanity = tmp;
				}
			}

			do {
				system_ready = check_fd(fds[0], 2000);

				if (system_ready == 0) {
					printf("OSET[%d] Waiting for background process pid:%d to be ready.....\n", (int)getpid(), (int) pid);
				}

			} while (--sanity && system_ready == 0);

			shutdown(fds[0], 2);
			close(fds[0]);
			fds[0] = -1;


			if (system_ready < 0) {
				printf("OSET[%d] Error starting system! pid:%d\n", (int)getpid(), (int) pid);
				kill(pid, 9);
				exit(EXIT_FAILURE);
			}

			printf("OSET[%d] System Ready pid:%d\n", (int) getpid(), (int) pid);
		}

		exit(EXIT_SUCCESS);
	}

	if (fds) {
		setsid();
	}
	/* redirect std* to null */
	fd = open("/dev/null", O_RDONLY);
	oset_sys_assert( fd >= 0 );
	if (fd != 0) {
		dup2(fd, 0);
		close(fd);
	}

	fd = open("/dev/null", O_WRONLY);
	oset_sys_assert( fd >= 0 );
	if (fd != 1) {
		dup2(fd, 1);
		close(fd);
	}

	fd = open("/dev/null", O_WRONLY);
	oset_sys_assert( fd >= 0 );
	if (fd != 2) {
		dup2(fd, 2);
		close(fd);
	}
	return;
}


OSET_DECLARE(int32_t) sset_core_session_ctl(sset_session_ctl_t cmd, void *val)
{
	int *intval = (int *) val;
	int oldintval = 0, newintval = 0;

	if (intval) {
		oldintval = *intval;
	}
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "val %d, oldintval %d",*(int *) val, oldintval);

	if (oset_test_flag((&runtime), SCF_SHUTTING_DOWN)) {
		return -1;
	}

	switch (cmd) {
	case SCSC_DEBUG_SQL:
		{
			if (oset_test_flag((&runtime), SCF_DEBUG_SQL)) {
				oset_clear_flag((&runtime), SCF_DEBUG_SQL);
				newintval = 0;
			} else {
				oset_set_flag((&runtime), SCF_DEBUG_SQL);
				newintval = 1;
			}
		}
		break;
	case SCSC_VERBOSE_EVENTS:
		if (intval) {
			if (oldintval > -1) {
				if (oldintval) {
					oset_set_flag((&runtime), SCF_VERBOSE_EVENTS);
				} else {
					oset_clear_flag((&runtime), SCF_VERBOSE_EVENTS);
				}
			}
			newintval = oset_test_flag((&runtime), SCF_VERBOSE_EVENTS);
		}
		break;
	case SCSC_API_EXPANSION:
		if (intval) {
			if (oldintval > -1) {
				if (oldintval) {
					oset_set_flag((&runtime), SCF_API_EXPANSION);
				} else {
					oset_clear_flag((&runtime), SCF_API_EXPANSION);
				}
			}
			newintval = oset_test_flag((&runtime), SCF_API_EXPANSION);
		}
		break;
	case SCSC_THREADED_SYSTEM_EXEC:
		if (intval) {
			if (oldintval > -1) {
				if (oldintval) {
					oset_set_flag((&runtime), SCF_THREADED_SYSTEM_EXEC);
				} else {
					oset_clear_flag((&runtime), SCF_THREADED_SYSTEM_EXEC);
				}
			}
			newintval = oset_test_flag((&runtime), SCF_THREADED_SYSTEM_EXEC);
		}
		break;
	case SCSC_CALIBRATE_CLOCK:
		oset_apr_time_calibrate_clock();
		break;
	case SCSC_FLUSH_DB_HANDLES:
		oset_cache_db_flush_handles();
		break;
	case SCSC_SEND_SIGHUP:
		handle_SIGHUP(1);
		break;
	case SCSC_SYNC_CLOCK:
		oset_apr_time_sync();
		newintval = 0;
		break;
	case SCSC_SYNC_CLOCK_WHEN_IDLE:
		newintval = sset_core_session_sync_clock();
		break;
	case SCSC_SQL:
		if (oldintval) {
			oset_core_sqldb_resume();
		} else {
			oset_core_sqldb_pause();
		}
		break;
	case SCSC_HUPALL:
		//oset_core_session_hupall(OSET_CAUSE_MANAGER_REQUEST);
		break;
	case SCSC_CANCEL_SHUTDOWN:
		oset_clear_flag((&runtime), SCF_SHUTDOWN_REQUESTED);
		break;
	case SCSC_SAVE_HISTORY:
		oset_console_save_history();
		break;
	case SCSC_CRASH:
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Declinatio Mortuus Obfirmo!");
		oset_console_save_history();
		abort();
		break;
	case SCSC_SHUTDOWN_NOW:
		oset_console_save_history();
		exit(0);
		break;
	case SCSC_REINCARNATE_NOW:
		oset_console_save_history();
		exit(OSET_STATUS_RESTART);
		break;
	case SCSC_SHUTDOWN_CHECK:
		newintval = !!oset_test_flag((&runtime), SCF_SHUTDOWN_REQUESTED);
		break;
	case SCSC_SHUTDOWN:

		if (oldintval) {
			oset_set_flag((&runtime), SCF_RESTART);
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Restarting");
		} else {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Shutting down");
		}
		runtime.running = 0;
		break;
	case SCSC_CHECK_RUNNING:
		newintval = runtime.running;
		break;
	case SCSC_LOGLEVEL:
		if (oldintval >= OSET_LOG2_DISABLE) {
			runtime.hard_log_level = oldintval;
		}

		if (runtime.hard_log_level > OSET_LOG2_DEBUG) {
			runtime.hard_log_level = OSET_LOG2_DEBUG;
		}
		newintval = runtime.hard_log_level;
		break;
	case SCSC_DEBUG_LEVEL:
		if (oldintval > -1) {
			if (oldintval > 10)
				oldintval = 10;
			runtime.debug_level = oldintval;
		}
		newintval = runtime.debug_level;
		break;
	case SCSC_MIN_IDLE_CPU:
		{
			double *dval = (double *) val;
			if (dval) {
				*dval = oset_core_min_idle_cpu(*dval);
			}
			intval = NULL;
		}
		break;
	case SCSC_LAST_SPS:
		newintval = runtime.sps_last;
		break;
	case SCSC_SPS_PEAK:
		if (oldintval == -1) {
			runtime.sps_peak = 0;
		}
		newintval = runtime.sps_peak;
		break;
	case SCSC_SPS_PEAK_FIVEMIN:
		newintval = runtime.sps_peak_fivemin;
		break;
	case SCSC_SESSIONS_PEAK:
		newintval = runtime.sessions_peak;
		break;
	case SCSC_SESSIONS_PEAK_FIVEMIN:
		newintval = runtime.sessions_peak_fivemin;
		break;
	case SCSC_SPS:
		oset_apr_mutex_lock(runtime.throttle_mutex);
		if (oldintval > 0) {
			runtime.sps_total = oldintval;
		}
		newintval = runtime.sps_total;
		oset_apr_mutex_unlock(runtime.throttle_mutex);
		break;

	case SCSC_RECLAIM:
		oset_core_memory_reclaim_all();
		newintval = 0;
		break;
	case SCSC_SHUTDOWN_CAUSE:
		runtime.shutdown_cause = oldintval;
		break;
	}

	if (intval) {
		*intval = newintval;
	}


	return 0;
}


OSET_DECLARE(void) sset_core_set_globals(void)
{
#define BUFSIZE 1024
	char base_dir[1024] = OM_PREFIX_DIR;

	if (!OSET_GLOBAL_dirs.mod_dir && (OSET_GLOBAL_dirs.mod_dir = (char *) malloc(BUFSIZE))) {
		if (OSET_GLOBAL_dirs.base_dir)
			oset_apr_snprintf(OSET_GLOBAL_dirs.mod_dir, BUFSIZE, "%s%smod", OSET_GLOBAL_dirs.base_dir, OSET_PATH_SEPARATOR);
		else
#ifdef SSET_MOD_DIR
			oset_apr_snprintf(OSET_GLOBAL_dirs.mod_dir, BUFSIZE, "%s", SSET_MOD_DIR);
#else
			oset_apr_snprintf(OSET_GLOBAL_dirs.mod_dir, BUFSIZE, "%s%smod", base_dir, OSET_PATH_SEPARATOR);
#endif
	}

	if (!OSET_GLOBAL_dirs.lib_dir && (OSET_GLOBAL_dirs.lib_dir = (char *) malloc(BUFSIZE))) {
		if (OSET_GLOBAL_dirs.base_dir)
			oset_apr_snprintf(OSET_GLOBAL_dirs.lib_dir, BUFSIZE, "%s%slib", OSET_GLOBAL_dirs.base_dir, OSET_PATH_SEPARATOR);
		else
#ifdef SSET_LIB_DIR
			oset_apr_snprintf(OSET_GLOBAL_dirs.lib_dir, BUFSIZE, "%s", SSET_LIB_DIR);
#else
			oset_apr_snprintf(OSET_GLOBAL_dirs.lib_dir, BUFSIZE, "%s%slib", base_dir, OSET_PATH_SEPARATOR);
#endif
	}

	if (!OSET_GLOBAL_dirs.conf_dir && (OSET_GLOBAL_dirs.conf_dir = (char *) malloc(BUFSIZE))) {
		if (OSET_GLOBAL_dirs.base_dir)
			oset_apr_snprintf(OSET_GLOBAL_dirs.conf_dir, BUFSIZE, "%s%sconf", OSET_GLOBAL_dirs.base_dir, OSET_PATH_SEPARATOR);
		else
#ifdef SSET_CONF_DIR
			oset_apr_snprintf(OSET_GLOBAL_dirs.conf_dir, BUFSIZE, "%s", SSET_CONF_DIR);
#else
			oset_apr_snprintf(OSET_GLOBAL_dirs.conf_dir, BUFSIZE, "%s%sconf", base_dir, OSET_PATH_SEPARATOR);
#endif
	}

	if (!OSET_GLOBAL_dirs.log_dir && (OSET_GLOBAL_dirs.log_dir = (char *) malloc(BUFSIZE))) {
		if (OSET_GLOBAL_dirs.base_dir)
			oset_apr_snprintf(OSET_GLOBAL_dirs.log_dir, BUFSIZE, "%s%slog", OSET_GLOBAL_dirs.base_dir, OSET_PATH_SEPARATOR);
		else
#ifdef SSET_LOG_DIR
			oset_apr_snprintf(OSET_GLOBAL_dirs.log_dir, BUFSIZE, "%s", SSET_LOG_DIR);
#else
			oset_apr_snprintf(OSET_GLOBAL_dirs.log_dir, BUFSIZE, "%s%slog", base_dir, OSET_PATH_SEPARATOR);
#endif
	}

	if (!OSET_GLOBAL_dirs.run_dir && (OSET_GLOBAL_dirs.run_dir = (char *) malloc(BUFSIZE))) {
		if (OSET_GLOBAL_dirs.base_dir)
			oset_apr_snprintf(OSET_GLOBAL_dirs.run_dir, BUFSIZE, "%s%srun", OSET_GLOBAL_dirs.base_dir, OSET_PATH_SEPARATOR);
		else
#ifdef SSET_RUN_DIR
			oset_apr_snprintf(OSET_GLOBAL_dirs.run_dir, BUFSIZE, "%s", SSET_RUN_DIR);
#else
			oset_apr_snprintf(OSET_GLOBAL_dirs.run_dir, BUFSIZE, "%s%srun", base_dir, OSET_PATH_SEPARATOR);
#endif
	}


	if (!OSET_GLOBAL_dirs.db_dir && (OSET_GLOBAL_dirs.db_dir = (char *) malloc(BUFSIZE))) {
		if (OSET_GLOBAL_dirs.base_dir)
			oset_apr_snprintf(OSET_GLOBAL_dirs.db_dir, BUFSIZE, "%s%sdb", OSET_GLOBAL_dirs.base_dir, OSET_PATH_SEPARATOR);
		else
#ifdef SSET_DB_DIR
			oset_apr_snprintf(OSET_GLOBAL_dirs.db_dir, BUFSIZE, "%s", SSET_DB_DIR);
#else
			oset_apr_snprintf(OSET_GLOBAL_dirs.db_dir, BUFSIZE, "%s%sdb", base_dir, OSET_PATH_SEPARATOR);
#endif
	}

	if (!OSET_GLOBAL_dirs.temp_dir && (OSET_GLOBAL_dirs.temp_dir = (char *) malloc(BUFSIZE))) {
		if (OSET_GLOBAL_dirs.base_dir)
			oset_apr_snprintf(OSET_GLOBAL_dirs.temp_dir, BUFSIZE, "%s%stmp", OSET_GLOBAL_dirs.base_dir, OSET_PATH_SEPARATOR);
		else
#ifdef SSET_TEMP_DIR
			oset_apr_snprintf(OSET_GLOBAL_dirs.temp_dir, BUFSIZE, "%s", SSET_TEMP_DIR);
#else
			oset_apr_snprintf(OSET_GLOBAL_dirs.temp_dir, BUFSIZE, "%s%stmp", base_dir, OSET_PATH_SEPARATOR);
#endif

	}

	if (!OSET_GLOBAL_filenames.conf_name && (OSET_GLOBAL_filenames.conf_name = (char *) malloc(BUFSIZE))) {
		oset_apr_snprintf(OSET_GLOBAL_filenames.conf_name, BUFSIZE, "%s", "sset.xml");
	}

	/* Do this last because it being empty is part of the above logic */
	if (!OSET_GLOBAL_dirs.base_dir && (OSET_GLOBAL_dirs.base_dir = (char *) malloc(BUFSIZE))) {
		oset_apr_snprintf(OSET_GLOBAL_dirs.base_dir, BUFSIZE, "%s", base_dir);
	}

	oset_sys_assert(OSET_GLOBAL_dirs.base_dir);
	oset_sys_assert(OSET_GLOBAL_dirs.mod_dir);
	oset_sys_assert(OSET_GLOBAL_dirs.lib_dir);
	oset_sys_assert(OSET_GLOBAL_dirs.conf_dir);
	oset_sys_assert(OSET_GLOBAL_dirs.log_dir);
	oset_sys_assert(OSET_GLOBAL_dirs.run_dir);
	oset_sys_assert(OSET_GLOBAL_dirs.db_dir);
	oset_sys_assert(OSET_GLOBAL_dirs.temp_dir);

	oset_sys_assert(OSET_GLOBAL_filenames.conf_name);
}


int main(int argc, char * argv[])
{
    char pid_path[256] = "";	        /* full path to the pid file */
	char pid_buffer[32] = "";	        /* pid string */
	char old_pid_buffer[32] = { 0 };	/* pid string */
	oset_size_t pid_len, old_pid_len;
	const char *err = NULL; 	        /* error value for return from sset initialization */
	oset_bool_t nc = OSET_FALSE;	    /* TRUE if we are running in noconsole mode */
	oset_bool_t do_wait = OSET_FALSE;
	const char *runas_user = NULL;
	const char *runas_group = NULL;
	int fds[2] = { 0, 0 };
	pid_t pid = 0;
	int i, x;
	int ret = 0;
	const char *local_argv[1024] = { 0 };
	int alt_dirs = 0, alt_base = 0, log_set = 0, run_set = 0, do_kill = 0;
	int priority = 0;
	oset_core_flag_t flags = SCF_USE_SQL | SCF_CALIBRATE_CLOCK | SCF_USE_CLOCK_RT;
	oset_apr_memory_pool_t *pool = NULL;
	oset_apr_file_t *fd;
	oset_status_t destroy_status;

	for (x = 0; x < argc; x++) {
		local_argv[i++] = argv[x];
	}

	for (x = 1; x < i; x++) {
		if (oset_strlen_zero(local_argv[x]))
			continue;

		if (!strcmp(local_argv[x], "-help") || !strcmp(local_argv[x], "-h")) {
			printf("%s\n", helpmap);
			exit(EXIT_SUCCESS);
		}

		else if (!strcmp(local_argv[x], "-version")) {
			show_version();
			exit(EXIT_SUCCESS);
		}

		else if (!strcmp(local_argv[x], "-u")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "Option '%s' requires an argument!\n", local_argv[x - 1]);
				exit(EXIT_FAILURE);
			}
			runas_user = local_argv[x];
		}

		else if (!strcmp(local_argv[x], "-g")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "Option '%s' requires an argument!\n", local_argv[x - 1]);
				exit(EXIT_FAILURE);
			}
			runas_group = local_argv[x];
		}
		
#ifdef HAVE_SETRLIMIT
		else if (!strcmp(local_argv[x], "-core")) {
			struct rlimit rlp;
			memset(&rlp, 0, sizeof(rlp));
			rlp.rlim_cur = RLIM_INFINITY;
			rlp.rlim_max = RLIM_INFINITY;
			setrlimit(RLIMIT_CORE, &rlp);
		}	
#endif

		else if (!strcmp(local_argv[x], "-hp") || !strcmp(local_argv[x], "-rp")) {
			priority = 2;
		}

		else if (!strcmp(local_argv[x], "-lp")) {
			priority = -1;
		}

		else if (!strcmp(local_argv[x], "-np")) {
			priority = 1;
		}
		else if (!strcmp(local_argv[x], "-nosql")) {
			flags &= ~SCF_USE_SQL;
		}

		else if (!strcmp(local_argv[x], "-nort")) {
			flags &= ~SCF_USE_CLOCK_RT;
		}

		else if (!strcmp(local_argv[x], "-nocal")) {
			flags &= ~SCF_CALIBRATE_CLOCK;
		}

		else if (!strcmp(local_argv[x], "-stop")) {
			do_kill = OSET_TRUE;
		}

		else if (!strcmp(local_argv[x], "-nc")) {
			nc = OSET_TRUE;
		}

		else if (!strcmp(local_argv[x], "-ncwait")) {
			oset_env_set("OM_BG_TIMEOUT","10");
			nc = OSET_TRUE;
			do_wait = OSET_TRUE;
		}

		else if (!strcmp(local_argv[x], "-c")) {
			nc = OSET_FALSE;
		}

		else if (!strcmp(local_argv[x], "-conf")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "When using -conf you must specify a config directory\n");
				return 255;
			}

			OSET_GLOBAL_dirs.conf_dir = (char *) malloc(strlen(local_argv[x]) + 1);
			if (!OSET_GLOBAL_dirs.conf_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
			strcpy(OSET_GLOBAL_dirs.conf_dir, local_argv[x]);
			alt_dirs++;
		}

		else if (!strcmp(local_argv[x], "-mod")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "When using -mod you must specify a module directory\n");
				return 255;
			}

			OSET_GLOBAL_dirs.mod_dir = (char *) malloc(strlen(local_argv[x]) + 1);
			if (!OSET_GLOBAL_dirs.mod_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
			strcpy(OSET_GLOBAL_dirs.mod_dir, local_argv[x]);
		}

		else if (!strcmp(local_argv[x], "-log")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "When using -log you must specify a log directory\n");
				return 255;
			}

			OSET_GLOBAL_dirs.log_dir = (char *) malloc(strlen(local_argv[x]) + 1);
			if (!OSET_GLOBAL_dirs.log_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
			strcpy(OSET_GLOBAL_dirs.log_dir, local_argv[x]);
			alt_dirs++;
			log_set = OSET_TRUE;
		}

		else if (!strcmp(local_argv[x], "-run")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "When using -run you must specify a pid directory\n");
				return 255;
			}

			OSET_GLOBAL_dirs.run_dir = (char *) malloc(strlen(local_argv[x]) + 1);
			if (!OSET_GLOBAL_dirs.run_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
			strcpy(OSET_GLOBAL_dirs.run_dir, local_argv[x]);
			run_set = OSET_TRUE;
		}

		else if (!strcmp(local_argv[x], "-db")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "When using -db you must specify a db directory\n");
				return 255;
			}

			OSET_GLOBAL_dirs.db_dir = (char *) malloc(strlen(local_argv[x]) + 1);
			if (!OSET_GLOBAL_dirs.db_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
			strcpy(OSET_GLOBAL_dirs.db_dir, local_argv[x]);
			alt_dirs++;
		}

		else if (!strcmp(local_argv[x], "-base")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "When using -base you must specify a base directory\n");
				return 255;
			}

			OSET_GLOBAL_dirs.base_dir = (char *) malloc(strlen(local_argv[x]) + 1);
			if (!OSET_GLOBAL_dirs.base_dir) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
			strcpy(OSET_GLOBAL_dirs.base_dir, local_argv[x]);
			alt_base = 1;
		}

		else if (!strcmp(local_argv[x], "-cfgname")) {
			x++;
			if (oset_strlen_zero(local_argv[x]) || is_option(local_argv[x])) {
				fprintf(stderr, "When using -cfgname you must specify a filename\n");
				return 255;
			}

			OSET_GLOBAL_filenames.conf_name = (char *) malloc(strlen(local_argv[x]) + 1);
			if (!OSET_GLOBAL_filenames.conf_name) {
				fprintf(stderr, "Allocation error\n");
				return 255;
			}
			strcpy(OSET_GLOBAL_filenames.conf_name, local_argv[x]);
		}

	}

	if (log_set && !run_set) {
		OSET_GLOBAL_dirs.run_dir = (char *) malloc(strlen(OSET_GLOBAL_dirs.log_dir) + 1);
		if (!OSET_GLOBAL_dirs.run_dir) {
			fprintf(stderr, "Allocation error\n");
			return 255;
		}
		strcpy(OSET_GLOBAL_dirs.run_dir, OSET_GLOBAL_dirs.log_dir);
	}
	
	if (do_kill) {
		return oset_kill_background();
	}

    //oset_pkbuf_init();
	oset_core_initialize();

	if (oset_core_apr_initialize() != OSET_TRUE) {
		fprintf(stderr, "FATAL ERROR! Could not initialize APR\n");
		return 255;
	}

	if (alt_dirs && alt_dirs != 3 && !alt_base) {
		fprintf(stderr, "You must specify all or none of -conf, -log, and -db\n");
		return 255;
	}

#if defined(HAVE_SETRLIMIT)
	struct rlimit rlp;

	memset(&rlp, 0, sizeof(rlp));
	getrlimit(RLIMIT_STACK, &rlp);

	if (rlp.rlim_cur != OSET_THREAD_STACKSIZE) {
		char buf[1024] = "";
		int i = 0;

		memset(&rlp, 0, sizeof(rlp));
		rlp.rlim_cur = OSET_THREAD_STACKSIZE;
		rlp.rlim_max = OSET_SYSTEM_THREAD_STACKSIZE;
		setrlimit(RLIMIT_STACK, &rlp);

		oset_core_apr_terminate();
		oset_core_terminate();
		//oset_pkbuf_final();

		if (argv) ret = (int) execv(argv[0], argv);

		for (i = 0; i < argc; i++) {
			oset_apr_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s ", argv[i]);
		}

		return system(buf);
	}
#endif

	signal(SIGILL,  handle_SIG);
	signal(SIGTSTP, handle_SIG);
    signal(SIGTERM, handle_SIG);
	signal(SIGHUP,  handle_SIG);

	if (do_wait) {
		if (pipe(fds)) {
			fprintf(stderr, "System Error!\n");
			exit(-1);
		}
	}

	if (nc) {
		daemonize(do_wait ? fds : NULL);
	}

	if (oset_core_set_process_privileges() < 0) {
		return 255;
	}

	switch (priority) {
	case 2:
		set_realtime_priority();
		break;
	case 1:
		set_normal_priority();
		break;
	case -1:
		set_low_priority();
		break;
	default:
		set_auto_priority();
		break;
	}

	oset_core_setrlimits();

	if (runas_user || runas_group) {
		if (change_user_group(runas_user, runas_group) < 0) {
			fprintf(stderr, "Failed to om user [%s] / group [%s]\n",
				oset_strlen_zero(runas_user)  ? "-" : runas_user,
				oset_strlen_zero(runas_group) ? "-" : runas_group);
			return 255;
		}
	}

	sset_core_set_globals();

	pid = getpid();

	memset(pid_buffer, 0, sizeof(pid_buffer));
	oset_apr_snprintf(pid_path, sizeof(pid_path), "%s%s%s", OSET_GLOBAL_dirs.run_dir, OSET_PATH_SEPARATOR, pfile);
	oset_apr_snprintf(pid_buffer, sizeof(pid_buffer), "%d", pid);
	pid_len = strlen(pid_buffer);

	apr_pool_create(&pool, NULL);

	oset_apr_dir_make_recursive(OSET_GLOBAL_dirs.run_dir, OSET_DEFAULT_DIR_PERMS, pool);

	if (oset_apr_file_open(&fd, pid_path, OSET_FOPEN_READ, OSET_FPROT_UREAD | OSET_FPROT_UWRITE, pool) == OSET_STATUS_SUCCESS) {

		old_pid_len = sizeof(old_pid_buffer) -1;
		oset_apr_file_read(fd, old_pid_buffer, &old_pid_len);
		oset_apr_file_close(fd);
	}

	if (oset_apr_file_open(&fd,
						 pid_path,
						 OSET_FOPEN_WRITE | OSET_FOPEN_CREATE | OSET_FOPEN_TRUNCATE,
						 OSET_FPROT_UREAD | OSET_FPROT_UWRITE, pool) != OSET_STATUS_SUCCESS) {
		fprintf(stderr, "Cannot open pid file %s.\n", pid_path);
		return 255;
	}

	if (oset_apr_file_lock(fd, OSET_FLOCK_EXCLUSIVE | OSET_FLOCK_NONBLOCK) != OSET_STATUS_SUCCESS) {
		fprintf(stderr, "Cannot lock pid file %s.\n", pid_path);
		old_pid_len = strlen(old_pid_buffer);
		if (strlen(old_pid_buffer)) {
			oset_apr_file_write(fd, old_pid_buffer, &old_pid_len);
		}
		return 255;
	}

	oset_apr_file_write(fd, pid_buffer, &pid_len);

	if (sset_core_init_and_modload(flags, nc ? OSET_FALSE : OSET_TRUE, &err) != OSET_STATUS_SUCCESS) {
		fprintf(stderr, "Cannot Initialize [%s]\n", err);
		return 255;
	}

	if (do_wait) {
		if (fds[1] > -1) {
			int i, v = 1;

			if ((i = write(fds[1], &v, sizeof(v))) < 0) {
				fprintf(stderr, "System Error [%s]\n", strerror(errno));
			} else {
				(void)read(fds[1], &v, sizeof(v));
			}

			shutdown(fds[1], 2);
			close(fds[1]);
			fds[1] = -1;
		}
	}

	sset_core_runtime_loop(nc);

	destroy_status = sset_core_destroy();

	oset_apr_file_close(fd);
	apr_pool_destroy(pool);
	oset_core_apr_terminate();

	if (unlink(pid_path) != 0) {
		fprintf(stderr, "Failed to delete pid file [%s]\n", pid_path);
	}

	if (destroy_status == OSET_STATUS_RESTART) {
		char buf[1024] = { 0 };
		int j = 0;

		oset_sleep(1000000);
		if (!argv || !argv[0] || execv(argv[0], argv) == -1) {
			fprintf(stderr, "Restart Failed [%s] resorting to plan b\n", strerror(errno));
			for (j = 0; j < argc; j++) {
				oset_apr_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s ", argv[j]);
			}
			ret = system(buf);
		}
	}

	return ret;


}


