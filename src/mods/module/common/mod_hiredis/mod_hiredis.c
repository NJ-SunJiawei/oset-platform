/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#include "mod_hiredis.h"

mod_hiredis_global_t mod_hiredis_globals;

OSET_MODULE_SHUTDOWN_FUNCTION(mod_hiredis_shutdown);
OSET_MODULE_LOAD_FUNCTION(mod_hiredis_load);
OSET_MODULE_DEFINITION(libmod_hiredis, mod_hiredis_load, mod_hiredis_shutdown, NULL);

#define DECR_DEL_SCRIPT "local v=redis.call(\"decr\",KEYS[1]);if v <= 0 then redis.call(\"del\",KEYS[1]) end;return v;"

/**
 * Get exclusive access to limit_pvt, if it exists
 */
static hiredis_limit_pvt_t *get_limit_pvt(oset_core_session_t *session)
{
	hiredis_limit_pvt_t *limit_pvt = oset_session_get_private(session, "hiredis_limit_pvt");
	if (limit_pvt) {
		/* pvt already exists, return it */
		oset_apr_mutex_lock(limit_pvt->mutex);
		return limit_pvt;
	}
	return NULL;
}

/**
 * Add limit_pvt and get exclusive access to it
 */
static hiredis_limit_pvt_t *add_limit_pvt(oset_core_session_t *session)
{
	hiredis_limit_pvt_t *limit_pvt = oset_session_get_private(session, "hiredis_limit_pvt");
	if (limit_pvt) {
		/* pvt already exists, return it */
		oset_apr_mutex_lock(limit_pvt->mutex);
		return limit_pvt;
	}

	/* not created yet, add it - NOTE a channel mutex would be better here if we had access to it */
	oset_apr_mutex_lock(mod_hiredis_globals.limit_pvt_mutex);
	limit_pvt = oset_session_get_private(session, "hiredis_limit_pvt");
	if (limit_pvt) {
		/* was just added by another thread */
		oset_apr_mutex_unlock(mod_hiredis_globals.limit_pvt_mutex);
		oset_apr_mutex_lock(limit_pvt->mutex);
		return limit_pvt;
	}

	/* still not created yet, add it */
	limit_pvt = oset_core_session_alloc(session, sizeof(*limit_pvt));
	oset_apr_mutex_init(&limit_pvt->mutex, OSET_MUTEX_NESTED, oset_core_session_get_pool(session));
	limit_pvt->first = NULL;
	oset_session_set_private(session, "hiredis_limit_pvt", limit_pvt);
	oset_apr_mutex_unlock(mod_hiredis_globals.limit_pvt_mutex);
	oset_apr_mutex_lock(limit_pvt->mutex);
	return limit_pvt;
}

/**
 * Release exclusive acess to limit_pvt
 */
static void release_limit_pvt(hiredis_limit_pvt_t *limit_pvt)
{
	if (limit_pvt) {
		oset_apr_mutex_unlock(limit_pvt->mutex);
	}
}

OSET_STANDARD_APP(raw_app)
{
	char *response = NULL, *profile_name = NULL, *cmd = NULL;
	hiredis_profile_t *profile = NULL;

	if ( !zstr(data) ) {
		profile_name = strdup(data);
	} else {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: invalid data! Use the format 'default set keyname value'");
		goto done;
	}

	if ( (cmd = strchr(profile_name, ' '))) {
		*cmd = '\0';
		cmd++;
	} else {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: invalid data! Use the format 'default set keyname value'");
		goto done;
	}

	profile = oset_core_hash_find(mod_hiredis_globals.profiles, profile_name);

	if ( !profile ) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: Unable to locate profile[%s]", profile_name);
		return;
	}

	if ( hiredis_profile_execute_sync(profile, session, &response, cmd) != OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: profile[%s] error executing [%s] because [%s]", profile_name, cmd, response ? response : "");
	}

	oset_session_set_variable(session, "hiredis_raw_response", response ? response : "");

 done:
	oset_safe_free(profile_name);
	oset_safe_free(response);
	return;
}

OSET_STANDARD_API(raw_api)
{
	hiredis_profile_t *profile = NULL;
	char *data = NULL, *input = NULL, *response = NULL;
	oset_status_t status = OSET_STATUS_SUCCESS;

	if ( !zstr(cmd) ) {
		input = strdup(cmd);
	} else {
		oset_goto_status(OSET_STATUS_GENERR, done);
	}

	if ( (data = strchr(input, ' '))) {
		*data = '\0';
		data++;
	}

	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "hiredis: debug: profile[%s] for command [%s]", input, data);

	profile = oset_core_hash_find(mod_hiredis_globals.profiles, input);

	if ( !profile ) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: Unable to locate profile[%s]", input);
		oset_goto_status(OSET_STATUS_GENERR, done);
	}

	if ( hiredis_profile_execute_sync(profile, session, &response, data) != OSET_STATUS_SUCCESS) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: profile[%s] error executing [%s] reason:[%s]", input, data, response ? response : "");
		oset_goto_status(OSET_STATUS_GENERR, done);
	}

	if (response) {
		stream->write_function(stream, response);
	}
 done:
	oset_safe_free(input);
	oset_safe_free(response);
	return status;
}

/*
OSET_LIMIT_INCR(name) static oset_status_t name (oset_core_session_t *session, const char *realm, const char *resource,
                                                     const int max, const int interval)
*/
OSET_LIMIT_INCR(hiredis_limit_incr)
{
	hiredis_profile_t *profile = NULL;
	char *response = NULL, *limit_key = NULL;
	int64_t count = 0; /* Redis defines the incr action as to be performed on a 64 bit signed integer */
	time_t now = oset_epoch_time_now(NULL);
	oset_status_t status = OSET_STATUS_SUCCESS;
	hiredis_limit_pvt_t *limit_pvt = NULL;
	hiredis_limit_pvt_node_t *limit_pvt_node = NULL;
	oset_apr_memory_pool_t *session_pool = oset_core_session_get_pool(session);

	if ( zstr(realm) ) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: realm must be defined");
		oset_goto_status(OSET_STATUS_GENERR, done);
	}

	if ( interval < 0 ) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: interval must be >= 0");
		oset_goto_status(OSET_STATUS_GENERR, done);
	}

	profile = oset_core_hash_find(mod_hiredis_globals.profiles, realm);

	if ( !profile ) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: Unable to locate profile[%s]", realm);
		oset_goto_status(OSET_STATUS_GENERR, done);
	}

	if ( interval ) {
		limit_key = oset_core_session_sprintf(session, "%s_%d", resource, now / interval);
	} else {
		limit_key = oset_core_session_sprintf(session, "%s", resource);
	}

	if ( (status = hiredis_profile_execute_pipeline_printf(profile, session, &response, "incr %s", limit_key) ) != OSET_STATUS_SUCCESS ) {
		if ( status == OSET_STATUS_SOCKERR && profile->ignore_connect_fail) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_INFO, "hiredis: ignoring profile[%s] connection error incrementing [%s]", realm, limit_key);
			oset_goto_status(OSET_STATUS_SUCCESS, done);
		} else if ( profile->ignore_error ) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_INFO, "hiredis: ignoring profile[%s] general error incrementing [%s]", realm, limit_key);
			oset_goto_status(OSET_STATUS_SUCCESS, done);
		}
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: profile[%s] error incrementing [%s] because [%s]", realm, limit_key, response ? response : "");
		oset_session_set_variable(session, "hiredis_raw_response", response ? response : "");
		oset_goto_status(OSET_STATUS_GENERR, done);
	}

	/* set expiration for interval on first increment */
	if ( interval && !strcmp("1", response ? response : "") ) {
		hiredis_profile_execute_pipeline_printf(profile, session, NULL, "expire %s %d", limit_key, interval);
	}

	oset_session_set_variable(session, "hiredis_raw_response", response ? response : "");

	count = atoll(response ? response : "");

	if ( oset_is_number(response ? response : "") && count <= 0 ) {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_WARNING, "limit not positive after increment, resource = %s, val = %s", limit_key, response ? response : "");
	} else {
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG, "resource = %s, response = %s", limit_key, response ? response : "");
	}

	if ( !oset_is_number(response ? response : "") && !profile->ignore_error ) {
		/* got response error */
		oset_goto_status(OSET_STATUS_GENERR, done);
	} else if ( max > 0 && count > 0 && count > max ) {
		oset_session_set_variable(session, "hiredis_limit_exceeded", "true");
		if ( !interval ) { /* don't need to decrement intervals if limit exceeded since the interval keys are named w/ timestamp */
			if ( profile->delete_when_zero ) {
				hiredis_profile_eval_pipeline(profile, session, NULL, DECR_DEL_SCRIPT, 1, limit_key);
			} else {
				hiredis_profile_execute_pipeline_printf(profile, session, NULL, "decr %s", limit_key);
			}
		}
		oset_goto_status(OSET_STATUS_GENERR, done);
	}

	if ( !interval && count > 0 ) {
		/* only non-interval limits need to be released on session destroy */
		limit_pvt_node = oset_core_alloc(session_pool, sizeof(*limit_pvt_node));
		limit_pvt_node->realm = oset_core_strdup(session_pool, realm);
		limit_pvt_node->resource = oset_core_strdup(session_pool, resource);
		limit_pvt_node->limit_key = limit_key;
		limit_pvt_node->inc = 1;
		limit_pvt_node->interval = interval;
		limit_pvt = add_limit_pvt(session);
		limit_pvt_node->next = limit_pvt->first;
		limit_pvt->first = limit_pvt_node;
		release_limit_pvt(limit_pvt);
	}

 done:
	oset_safe_free(response);
	return status;
}

/*
  OSET_LIMIT_RELEASE(name) static oset_status_t name (oset_core_session_t *session, const char *realm, const char *resource)
*/
OSET_LIMIT_RELEASE(hiredis_limit_release)
{
	hiredis_profile_t *profile = NULL;
	char *response = NULL;
	oset_status_t status = OSET_STATUS_SUCCESS;
	hiredis_limit_pvt_t *limit_pvt = get_limit_pvt(session);

	if (!limit_pvt) {
		/* nothing to release */
		return OSET_STATUS_SUCCESS;
	}

	/* If realm and resource are NULL, then clear all of the limits */
	if ( zstr(realm) && zstr(resource) ) {
		hiredis_limit_pvt_node_t *cur = NULL;

		for ( cur = limit_pvt->first; cur; cur = cur->next ) {
			/* Rate limited resources are not auto-decremented, they will expire. */
			if ( !cur->interval && cur->inc ) {
				oset_status_t result;
				cur->inc = 0; /* mark as released */
				profile = oset_core_hash_find(mod_hiredis_globals.profiles, cur->realm);
				if ( profile->delete_when_zero ) {
					result = hiredis_profile_eval_pipeline(profile, session, &response, DECR_DEL_SCRIPT, 1, cur->limit_key);
				} else {
					result = hiredis_profile_execute_pipeline_printf(profile, session, &response, "decr %s", cur->limit_key);
				}
				if ( result != OSET_STATUS_SUCCESS ) {
					oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: profile[%s] error decrementing [%s] because [%s]",
									  cur->realm, cur->limit_key, response ? response : "");
				}
				oset_safe_free(response);
				response = NULL;
			}
		}
	} else if (!zstr(resource) ) {
		/* clear single non-interval resource */
		hiredis_limit_pvt_node_t *cur = NULL;
		for (cur = limit_pvt->first; cur; cur = cur->next ) {
			if ( !cur->interval && cur->inc && !strcmp(cur->resource, resource) && (zstr(realm) || !strcmp(cur->realm, realm)) ) {
				/* found the resource to clear */
				cur->inc = 0; /* mark as released */
				profile = oset_core_hash_find(mod_hiredis_globals.profiles, cur->realm);
				if (profile) {
					if ( profile->delete_when_zero ) {
						status = hiredis_profile_eval_pipeline(profile, session, &response, DECR_DEL_SCRIPT, 1, cur->limit_key);
					} else {
						status = hiredis_profile_execute_pipeline_printf(profile, session, &response, "decr %s", cur->limit_key);
					}
					if ( status != OSET_STATUS_SUCCESS ) {
						if ( status == OSET_STATUS_SOCKERR && profile->ignore_connect_fail ) {
							oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_INFO, "hiredis: ignoring profile[%s] connection error decrementing [%s]", cur->realm, cur->limit_key);
							oset_goto_status(OSET_STATUS_SUCCESS, done);
						} else if ( profile->ignore_error ) {
							oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_INFO, "hiredis: ignoring profile[%s] general error decrementing [%s]", realm, cur->limit_key);
							oset_goto_status(OSET_STATUS_SUCCESS, done);
						}
						oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "hiredis: profile[%s] error decrementing [%s] because [%s]", realm, cur->limit_key, response ? response : "");
						oset_session_set_variable(session, "hiredis_raw_response", response ? response : "");
						oset_goto_status(OSET_STATUS_GENERR, done);
					}

					oset_session_set_variable(session, "hiredis_raw_response", response ? response : "");
				}
				break;
			}
		}
	}

 done:
	release_limit_pvt(limit_pvt);
	oset_safe_free(response);
	return status;
}

/*
OSET_LIMIT_USAGE(name) static int name (const char *realm, const char *resource, uint32_t *rcount)
 */
OSET_LIMIT_USAGE(hiredis_limit_usage)
{
	hiredis_profile_t *profile = oset_core_hash_find(mod_hiredis_globals.profiles, realm);
	int64_t count = 0; /* Redis defines the incr action as to be performed on a 64 bit signed integer */
	char *response = NULL;

	if ( zstr(realm) ) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "hiredis: realm must be defined");
		goto err;
	}

	if ( !profile ) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "hiredis: Unable to locate profile[%s]", realm);
		goto err;
	}

	if ( hiredis_profile_execute_pipeline_printf(profile, NULL, &response, "get %s", resource) != OSET_STATUS_SUCCESS ) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "hiredis: profile[%s] error querying [%s] because [%s]", realm, resource, response ? response : "");
		goto err;
	}

	count = atoll(response ? response : "");

	oset_safe_free(response);
	return count;

 err:
	oset_safe_free(response);
	return -1;
}

/*
OSET_LIMIT_RESET(name) static oset_status_t name (void)
 */
OSET_LIMIT_RESET(hiredis_limit_reset)
{
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "hiredis: unable to globally reset hiredis limit resources. Use 'hiredis_raw set resource_name 0'");
	return OSET_STATUS_NOTIMPL;
}

/*
  OSET_LIMIT_INTERVAL_RESET(name) static oset_status_t name (const char *realm, const char *resource)
*/
OSET_LIMIT_INTERVAL_RESET(hiredis_limit_interval_reset)
{
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "hiredis: unable to reset hiredis interval limit resources.");
	return OSET_STATUS_NOTIMPL;
}

/*
OSET_LIMIT_STATUS(name) static char * name (void)
 */
OSET_LIMIT_STATUS(hiredis_limit_status)
{
	return strdup("-ERR not supported");
}

OSET_MODULE_LOAD_FUNCTION(mod_hiredis_load)
{
	oset_application_interface_t *app_interface;
	oset_api_interface_t *api_interface;
	oset_limit_interface_t *limit_interface;

	memset(&mod_hiredis_globals, 0, sizeof(mod_hiredis_globals));
	*module_interface = oset_loadable_module_create_module_interface(pool, modname);
	mod_hiredis_globals.pool = pool;
	oset_apr_mutex_init(&mod_hiredis_globals.limit_pvt_mutex, OSET_MUTEX_NESTED, pool);

	oset_core_hash_init(&(mod_hiredis_globals.profiles));

	if ( mod_hiredis_do_config() != OSET_STATUS_SUCCESS ) {
		return OSET_STATUS_GENERR;
	}

	OSET_ADD_LIMIT(limit_interface, "hiredis", hiredis_limit_incr, hiredis_limit_release, hiredis_limit_usage,
					 hiredis_limit_reset, hiredis_limit_status, hiredis_limit_interval_reset);
	OSET_ADD_APP(app_interface, "hiredis_raw", "hiredis_raw", "hiredis_raw", raw_app, "", SAF_ZOMBIE_EXEC);
	OSET_ADD_API(api_interface, "hiredis_api", "hiredis_api", raw_api, "");

	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_hiredis_shutdown)
{
	oset_hashtable_index_t *hi;
	hiredis_profile_t *profile = NULL;
	/* loop through profiles, and destroy them */

	while ((hi = oset_core_hash_first(mod_hiredis_globals.profiles))) {
		oset_core_hash_this(hi, NULL, NULL, (void **)&profile);
		hiredis_profile_destroy(&profile);
		oset_safe_free(hi);
	}

	oset_core_hash_destroy(&(mod_hiredis_globals.profiles));

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
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4
 */
