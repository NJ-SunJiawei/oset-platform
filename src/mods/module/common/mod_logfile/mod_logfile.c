/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#include "oset-core.h"

OSET_MODULE_LOAD_FUNCTION(mod_logfile_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_logfile_shutdown);
OSET_MODULE_DEFINITION(libmod_logfile, mod_logfile_load, mod_logfile_shutdown, NULL);

#define DEFAULT_LIMIT	 0xA00000	/* About 10 MB */
#define WARM_FUZZY_OFFSET 256
#define MAX_ROT 4096			/* why not */

static oset_apr_memory_pool_t *module_pool = NULL;
static oset_hashtable_t *profile_hash = NULL;

static struct {
	int rotate;
	oset_apr_mutex_t *mutex;
	oset_event_node_t *node;
} globals;

struct logfile_profile {
	char *name;
	oset_size_t log_size;		/* keep the log size in check for rotation */
	oset_size_t roll_size;	/* the size that we want to rotate the file at */
	oset_size_t max_rot;		/* number of log files to keep within the rotation */
	char *logfile;
	oset_apr_file_t *log_afd;
	oset_hashtable_t *log_hash;
	uint32_t all_level;
	uint32_t suffix;			/* suffix of the highest logfile name */
	oset_bool_t log_uuid;
};

typedef struct logfile_profile logfile_profile_t;

static oset_status_t load_profile(oset_xml_t xml);

#if 0
static void del_mapping(char *var, logfile_profile_t *profile)
{
	oset_core_hash_insert(profile->log_hash, var, NULL);
}
#endif

static void add_mapping(logfile_profile_t *profile, char *var, char *val)
{
	if (!strcasecmp(var, "all")) {
		profile->all_level |= (uint32_t) oset_log2_str2mask(val);
		return;
	}

	oset_core_hash_insert(profile->log_hash, var, (void *) (intptr_t) oset_log2_str2mask(val));
}

static oset_status_t mod_logfile_rotate(logfile_profile_t *profile);

static oset_status_t mod_logfile_openlogfile(logfile_profile_t *profile, oset_bool_t check)
{
	unsigned int flags = 0;
	oset_apr_file_t *afd;
	oset_status_t stat;

	flags |= OSET_FOPEN_CREATE;
	flags |= OSET_FOPEN_READ;
	flags |= OSET_FOPEN_WRITE;
	flags |= OSET_FOPEN_APPEND;

	stat = oset_apr_file_open(&afd, profile->logfile, flags, OSET_FPROT_OS_DEFAULT, module_pool);
	if (stat != OSET_STATUS_SUCCESS) {
		return OSET_STATUS_FALSE;
	}

	profile->log_afd = afd;

	profile->log_size = oset_apr_file_get_size(profile->log_afd);

	if (check && profile->roll_size && profile->log_size >= profile->roll_size) {
		mod_logfile_rotate(profile);
	}

	return OSET_STATUS_SUCCESS;
}

/* rotate the log file */
static oset_status_t mod_logfile_rotate(logfile_profile_t *profile)
{
	unsigned int i = 0;
	char *filename = NULL;
	oset_status_t stat = 0;
	int64_t offset = 0;
	oset_apr_memory_pool_t *pool = NULL;
	oset_apr_time_exp_t tm;
	char date[80] = "";
	oset_size_t retsize;
	oset_status_t status = OSET_STATUS_SUCCESS;

	oset_apr_mutex_lock(globals.mutex);

	oset_apr_time_exp_lt(&tm, oset_micro_time_now());
	oset_apr_strftime_nocheck(date, &retsize, sizeof(date), "%Y-%m-%d-%H-%M-%S", &tm);

	profile->log_size = 0;

	stat = oset_apr_file_seek(profile->log_afd, OSET_SEEK_SET, &offset);

	if (stat != OSET_STATUS_SUCCESS) {
		status = OSET_STATUS_FALSE;
		goto end;
	}

	oset_core_new_memory_pool(&pool);
	filename = oset_core_alloc(pool, strlen(profile->logfile) + WARM_FUZZY_OFFSET);

	if (profile->max_rot) {
		char *from_filename = NULL;
		char *to_filename = NULL;

		from_filename = oset_core_alloc(pool, strlen(profile->logfile) + WARM_FUZZY_OFFSET);
		to_filename = oset_core_alloc(pool, strlen(profile->logfile) + WARM_FUZZY_OFFSET);

		for (i=profile->suffix; i>1; i--) {
			sprintf((char *) to_filename, "%s.%i", profile->logfile, i);
			sprintf((char *) from_filename, "%s.%i", profile->logfile, i-1);

			if (oset_apr_file_exists(to_filename, pool) == OSET_STATUS_SUCCESS) {
				if ((status = oset_apr_file_remove(to_filename, pool)) != OSET_STATUS_SUCCESS) {
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Error removing log %s",to_filename);
					goto end;
				}
			}

			if ((status = oset_apr_file_rename(from_filename, to_filename, pool)) != OSET_STATUS_SUCCESS) {
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Error renaming log from %s to %s [%s]",
								  from_filename, to_filename, strerror(errno));
				goto end;
			}
		}

		sprintf((char *) to_filename, "%s.%i", profile->logfile, i);

		if (oset_apr_file_exists(to_filename, pool) == OSET_STATUS_SUCCESS) {
			if ((status = oset_apr_file_remove(to_filename, pool)) != OSET_STATUS_SUCCESS) {
				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Error removing log %s [%s]", to_filename, strerror(errno));
				goto end;
			}
		}

		oset_apr_file_close(profile->log_afd);
		if ((status = oset_apr_file_rename(profile->logfile, to_filename, pool)) != OSET_STATUS_SUCCESS) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Error renaming log from %s to %s [%s]", profile->logfile, to_filename, strerror(errno));
			goto end;
		}

		if ((status = mod_logfile_openlogfile(profile, OSET_FALSE)) != OSET_STATUS_SUCCESS) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Error reopening log %s", profile->logfile);
		}
		if (profile->suffix < profile->max_rot) {
			profile->suffix++;
		}

		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "New log started: %s", profile->logfile);

		goto end;
	}

	/* XXX This have no real value EXCEPT making sure if we rotate within the same second, the end index will increase */
	for (i = 1; i < MAX_ROT; i++) {
		sprintf((char *) filename, "%s.%s.%i", profile->logfile, date, i);
		if (oset_apr_file_exists(filename, pool) == OSET_STATUS_SUCCESS) {
			continue;
		}

		oset_apr_file_close(profile->log_afd);
		oset_apr_file_rename(profile->logfile, filename, pool);
		if ((status = mod_logfile_openlogfile(profile, OSET_FALSE)) != OSET_STATUS_SUCCESS) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Error Rotating Log!");
			goto end;
		}
		break;
	}

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "New log started.");

  end:

	if (pool) {
		oset_core_destroy_memory_pool(&pool);
	}

	oset_apr_mutex_unlock(globals.mutex);

	return status;
}

/* write to the actual logfile */
static oset_status_t mod_logfile_raw_write(logfile_profile_t *profile, char *log_data)
{
	oset_size_t len;
	oset_status_t status = OSET_STATUS_SUCCESS;
	len = strlen(log_data);

	if (len <= 0 || !profile->log_afd) {
		return OSET_STATUS_FALSE;
	}

	oset_apr_mutex_lock(globals.mutex);

	if (oset_apr_file_write(profile->log_afd, log_data, &len) != OSET_STATUS_SUCCESS) {
		oset_apr_file_close(profile->log_afd);
		if ((status = mod_logfile_openlogfile(profile, OSET_TRUE)) == OSET_STATUS_SUCCESS) {
			len = strlen(log_data);
			oset_apr_file_write(profile->log_afd, log_data, &len);
		}
	}

	oset_apr_mutex_unlock(globals.mutex);

	if (status == OSET_STATUS_SUCCESS) {
		profile->log_size += len;

		if (profile->roll_size && profile->log_size >= profile->roll_size) {
			mod_logfile_rotate(profile);
		}
	}

	return status;
}

static oset_status_t process_node(const oset_log2_node_t *node, oset_log2_level_t level)
{
	oset_hashtable_index_t *hi;
	void *val;
	const void *var;
	logfile_profile_t *profile;

	for (hi = oset_core_hash_first(profile_hash); hi; hi = oset_core_hash_next(&hi)) {
		size_t mask = 0;
		size_t ok = 0;

		oset_core_hash_this(hi, &var, NULL, &val);
		profile = val;

		ok = oset_log2_check_mask(profile->all_level, level);

		if (!ok) {
			mask = (size_t) oset_core_hash_find(profile->log_hash, node->file);
			ok = oset_log2_check_mask(mask, level);
		}

		if (!ok) {
			mask = (size_t) oset_core_hash_find(profile->log_hash, node->func);
			ok = oset_log2_check_mask(mask, level);
		}

		if (!ok) {
			char tmp[256] = "";
			oset_snprintf(tmp, sizeof(tmp), "%s:%s", node->file, node->func);
			mask = (size_t) oset_core_hash_find(profile->log_hash, tmp);
			ok = oset_log2_check_mask(mask, level);
		}

		if (ok) {
			if (profile->log_uuid && !zstr(node->userdata)) {
				char buf[2048];
				char *dup = strdup(node->data);
				char *lines[100];
				int argc, i;

				argc = oset_split(dup, '\n', lines);
				for (i = 0; i < argc; i++) {
					oset_snprintf(buf, sizeof(buf), "%s %s\n", node->userdata, lines[i]);
					mod_logfile_raw_write(profile, buf);
				}

				free(dup);

			} else {
				mod_logfile_raw_write(profile, node->data);
			}
		}

	}

	return OSET_STATUS_SUCCESS;
}

static oset_status_t mod_logfile_logger(const oset_log2_node_t *node, oset_log2_level_t level)
{
	return process_node(node, level);
}

static void cleanup_profile(void *ptr)
{
	logfile_profile_t *profile = (logfile_profile_t *) ptr;

	oset_core_hash_destroy(&profile->log_hash);
	oset_apr_file_close(profile->log_afd);
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_INFO, "Closing %s", profile->logfile);
	oset_safe_free(profile->logfile);

}

static oset_status_t load_profile(oset_xml_t xml)
{
	oset_xml_t param, settings;
	char *name = (char *) oset_xml_attr_soft(xml, "name");
	logfile_profile_t *new_profile;

	new_profile = oset_core_alloc(module_pool, sizeof(*new_profile));
	memset(new_profile, 0, sizeof(*new_profile));
	oset_core_hash_init(&(new_profile->log_hash));
	new_profile->name = oset_core_strdup(module_pool, oset_str_nil(name));

	new_profile->suffix = 1;
	new_profile->log_uuid = OSET_TRUE;

	if ((settings = oset_xml_child(xml, "settings"))) {
		for (param = oset_xml_child(settings, "param"); param; param = param->next) {
			char *var = (char *) oset_xml_attr_soft(param, "name");
			char *val = (char *) oset_xml_attr_soft(param, "value");
			if (!strcmp(var, "logfile")) {
				new_profile->logfile = strdup(val);
			} else if (!strcmp(var, "rollover")) {
				new_profile->roll_size = oset_atoui(val);
			} else if (!strcmp(var, "maximum-rotate")) {
				new_profile->max_rot = oset_atoui(val);
				if (new_profile->max_rot == 0) {
					new_profile->max_rot = MAX_ROT;
				}
			} else if (!strcmp(var, "uuid")) {
				new_profile->log_uuid = oset_true(val);
			}
		}
	}

	if ((settings = oset_xml_child(xml, "mappings"))) {
		for (param = oset_xml_child(settings, "map"); param; param = param->next) {
			char *var = (char *) oset_xml_attr_soft(param, "name");
			char *val = (char *) oset_xml_attr_soft(param, "value");

			add_mapping(new_profile, var, val);
		}
	}

	if (zstr(new_profile->logfile)) {
		char logfile[512];
		oset_snprintf(logfile, sizeof(logfile), "%s%s%s", OSET_GLOBAL_dirs.log_dir, OSET_PATH_SEPARATOR, "sset.log");
		new_profile->logfile = strdup(logfile);
	}

	if (mod_logfile_openlogfile(new_profile, OSET_TRUE) != OSET_STATUS_SUCCESS) {
		return OSET_STATUS_GENERR;
	}

	oset_core_hash_insert_destructor(profile_hash, new_profile->name, (void *) new_profile, cleanup_profile);
	return OSET_STATUS_SUCCESS;
}


static void event_handler(oset_event_t *event)
{
	const char *sig = oset_event_get_header(event, "Trapped-Signal");
	oset_hashtable_index_t *hi;
	void *val;
	const void *var;
	logfile_profile_t *profile;

	if (sig && !strcmp(sig, "HUP")) {
		if (globals.rotate) {
			for (hi = oset_core_hash_first(profile_hash); hi; hi = oset_core_hash_next(&hi)) {
				oset_core_hash_this(hi, &var, NULL, &val);
				profile = val;
				mod_logfile_rotate(profile);
			}
		} else {
			oset_apr_mutex_lock(globals.mutex);
			for (hi = oset_core_hash_first(profile_hash); hi; hi = oset_core_hash_next(&hi)) {
				oset_core_hash_this(hi, &var, NULL, &val);
				profile = val;
				oset_apr_file_close(profile->log_afd);
				if (mod_logfile_openlogfile(profile, OSET_TRUE) != OSET_STATUS_SUCCESS) {
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "Error Re-opening Log!");
				}
			}
			oset_apr_mutex_unlock(globals.mutex);
		}
	}
}

OSET_MODULE_LOAD_FUNCTION(mod_logfile_load)
{
	char *cf = "logfile.conf";
	oset_xml_t cfg, xml, settings, param, profiles, xprofile;

	module_pool = pool;

	memset(&globals, 0, sizeof(globals));
	oset_apr_mutex_init(&globals.mutex, OSET_MUTEX_NESTED, module_pool);

	if (profile_hash) {
		oset_core_hash_destroy(&profile_hash);
	}
	oset_core_hash_init(&profile_hash);

	if (oset_event_bind_removable(modname, OSET_EVENT_TRAP, OSET_EVENT_SUBCLASS_ANY, event_handler, NULL, &globals.node) != OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Couldn't bind!");
		return OSET_STATUS_GENERR;
	}

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

	if (!(xml = oset_xml_open_cfg(cf, &cfg, NULL))) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Open of %s failed", cf);
	} else {
		if ((settings = oset_xml_child(cfg, "settings"))) {
			for (param = oset_xml_child(settings, "param"); param; param = param->next) {
				char *var = (char *) oset_xml_attr_soft(param, "name");
				char *val = (char *) oset_xml_attr_soft(param, "value");
				if (!strcmp(var, "rotate-on-hup")) {
					globals.rotate = oset_true(val);
				}
			}
		}

		if ((profiles = oset_xml_child(cfg, "profiles"))) {
			for (xprofile = oset_xml_child(profiles, "profile"); xprofile; xprofile = xprofile->next) {
				if (load_profile(xprofile) != OSET_STATUS_SUCCESS) {
					oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "error loading profile.");
				}
			}
		}

		oset_xml_free(xml);
	}

	oset_log2_bind_logger(mod_logfile_logger, OSET_LOG2_DEBUG, OSET_FALSE);

	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_logfile_shutdown)
{
	oset_log2_unbind_logger(mod_logfile_logger);
	oset_event_unbind(&globals.node);
	oset_core_hash_destroy(&profile_hash);
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
