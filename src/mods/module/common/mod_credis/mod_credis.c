/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.04
************************************************************************/

#include "oset-core.h"
#include "credis.h"

OSET_MODULE_LOAD_FUNCTION(mod_redis_load);
OSET_MODULE_SHUTDOWN_FUNCTION(mod_redis_shutdown);
OSET_MODULE_DEFINITION(libmod_credis, mod_redis_load, mod_redis_shutdown, NULL);

static struct{
	char *host;
	int port;
	int timeout;
	oset_bool_t ignore_connect_fail;
} globals;

static oset_xml_config_item_t instructions[] = {
	/* parameter name        type                 reloadable   pointer                         default value     options structure */
	OSET_CONFIG_ITEM_STRING_STRDUP("host", CONFIG_RELOAD, &globals.host, NULL, "localhost", "Hostname for redis server"),
	OSET_CONFIG_ITEM("port", OSET_CONFIG_INT, CONFIG_RELOADABLE, &globals.port, (void *) 6379, NULL,NULL, NULL),
	OSET_CONFIG_ITEM("timeout", OSET_CONFIG_INT, CONFIG_RELOADABLE, &globals.timeout, (void *) 10000, NULL,NULL, NULL),
	OSET_CONFIG_ITEM("ignore_connect_fail", OSET_CONFIG_BOOL, CONFIG_RELOADABLE, &globals.ignore_connect_fail, OSET_FALSE, NULL, "true|false", "Set to true in order to continue when redis is not contactable"),
	OSET_CONFIG_ITEM_END()
};

/* HASH STUFF */
typedef struct {
	oset_hashtable_t *hash;
	oset_apr_mutex_t *mutex;
} limit_redis_private_t;

static oset_status_t redis_factory(REDIS *redis)
{
	if (!((*redis) = credis_connect(globals.host, globals.port, globals.timeout))) {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Couldn't connect to redis server at %s:%d timeout:%d", globals.host, globals.port, globals.timeout);
		return OSET_STATUS_FALSE;
	}
	return OSET_STATUS_SUCCESS;
}

/* \brief Enforces limit_redis restrictions
 * \param session current session
 * \param realm limit realm
 * \param id limit id
 * \param max maximum count
 * \param interval interval for rate limiting
 * \return OSET_TRUE if the access is allowed, OSET_FALSE if it isnt
 */
OSET_LIMIT_INCR(limit_incr_redis)
{
	limit_redis_private_t *pvt = NULL;
	int val,uuid_val;
	char *rediskey = NULL;
	char *uuid_rediskey = NULL;
	uint8_t increment = 1;
	oset_status_t status = OSET_STATUS_SUCCESS;
	REDIS redis;

	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "mod_redis is deprecated and will be removed in FS 1.8. Check out mod_hiredis.");

	if (redis_factory(&redis) != OSET_STATUS_SUCCESS) {
		if ( globals.ignore_connect_fail ) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "ignore_connect_fail=true, so ignoring the fact that redis was not contactabl and continuing with the call" );
			return OSET_STATUS_SUCCESS;
		} else {
			return OSET_STATUS_FALSE;
		}
	}

	/* Get the keys for redis server */
	uuid_rediskey = oset_core_session_sprintf(session,"%s_%s_%s", oset_core_get_osetname(), realm, resource);
	rediskey = oset_core_session_sprintf(session, "%s_%s", realm, resource);

	if ((pvt = oset_session_get_private(session, "limit_redis"))) {
		increment = !oset_core_hash_find_locked(pvt->hash, rediskey, pvt->mutex);
	} else {
		/* This is the first limit check on this channel, create a hashtable, set our prviate data and add a state handler */
		pvt = (limit_redis_private_t *) oset_core_session_alloc(session, sizeof(limit_redis_private_t));
		oset_core_hash_init(&pvt->hash);
		oset_apr_mutex_init(&pvt->mutex, OSET_MUTEX_NESTED, oset_core_session_get_pool(session));
		oset_session_set_private(session, "limit_redis", pvt);
	}

	if (!(oset_core_hash_find_locked(pvt->hash, rediskey, pvt->mutex))) {
		oset_core_hash_insert_locked(pvt->hash, rediskey, rediskey, pvt->mutex);
	}

   	if (increment) {
		if (credis_incr(redis, rediskey, &val) != 0) {
			oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Couldn't increment value corresponding to %s", rediskey);
			oset_goto_status(OSET_STATUS_FALSE, end);
		}

		if (max > 0) {
			if (val > max){
				if (credis_decr(redis, rediskey, &val) != 0) {
               		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Couldn't decrement value corresponding to %s", rediskey);
					oset_goto_status(OSET_STATUS_GENERR, end);
				} else {
           			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_INFO, "Usage for %s exceeds maximum rate of %d",
						rediskey, max);
					oset_goto_status(OSET_STATUS_FALSE, end);
				}
			} else {
				if (credis_incr(redis, uuid_rediskey, &uuid_val) != 0) {
       				oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Couldn't increment value corresponding to %s", uuid_rediskey);
       				oset_goto_status(OSET_STATUS_FALSE, end);
				}
			}
		} else  {
			if (credis_incr(redis, uuid_rediskey, &uuid_val) != 0) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Couldn't increment value corresponding to %s", uuid_rediskey);
			oset_goto_status(OSET_STATUS_FALSE, end);
			}
		}
    }
/*
	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG10, "Limit incr redis : rediskey : %s val : %d max : %d", rediskey, val, max);
	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG10, "Limit incr redis : uuid_rediskey : %s uuid_val : %d max : %d", uuid_rediskey,uuid_val,max);
*/
end:
	if (redis) {
		credis_close(redis);
	}
	return status;
}

/* !\brief Releases usage of a limit_redis-controlled ressource  */
OSET_LIMIT_RELEASE(limit_release_redis)
{
	limit_redis_private_t *pvt = oset_session_get_private(session, "limit_redis");
	int val, uuid_val;
	char *rediskey = NULL;
	char *uuid_rediskey = NULL;
	int status = OSET_STATUS_SUCCESS;
	REDIS redis;

	if (!pvt || !pvt->hash) {
        oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "No hashtable for session %s", oset_core_session_get_uuid(session));
		return OSET_STATUS_SUCCESS;
	}

	if (redis_factory(&redis) != OSET_STATUS_SUCCESS) {
                if ( globals.ignore_connect_fail ) {
                        oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "ignore_connect_fail=true, so ignoring the fact that redis was not contactabl and continuing with the call" );
                        return OSET_STATUS_SUCCESS;
                } else {
                        return OSET_STATUS_FALSE;
                }

	}

	oset_apr_mutex_lock(pvt->mutex);

	/* clear for uuid */
	if (realm == NULL && resource == NULL) {
		oset_hashtable_index_t *hi = NULL;
		/* Loop through the channel's hashtable which contains mapping to all the limit_redis_item_t referenced by that channel */
		while ((hi = oset_core_hash_first_iter(pvt->hash, hi))) {
			void *p_val = NULL;
			const void *p_key;
			char *p_uuid_key = NULL;
			oset_ssize_t keylen;

			oset_core_hash_this(hi, &p_key, &keylen, &p_val);

			if (credis_decr(redis, (const char*)p_key, &val) != 0) {
				oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Couldn't decrement value corresponding to %s", (char *)p_key);
				free(hi);
				oset_goto_status(OSET_STATUS_FALSE, end);
			}
	   		p_uuid_key = oset_core_session_sprintf(session, "%s_%s", oset_core_get_osetname(), (char *)p_key);
			if (credis_decr(redis,p_uuid_key,&uuid_val) != 0) {
				oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Couldn't decrement value corresponding to %s", p_uuid_key);
				free(hi);
				oset_goto_status(OSET_STATUS_FALSE, end);
			}
			oset_core_hash_delete(pvt->hash, (const char *) p_key);
			/*
        	oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG10, "Limit release redis : rediskey : %s val : %d", (char *)p_val,val);
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_DEBUG10, "Limit incr redis : uuid_rediskey : %s uuid_val : %d",
					 p_uuid_key, uuid_val);*/
		}

	} else {
	   	rediskey = oset_core_session_sprintf(session, "%s_%s", realm, resource);
		uuid_rediskey = oset_core_session_sprintf(session, "%s_%s_%s", oset_core_get_osetname(), realm, resource);
		oset_core_hash_delete(pvt->hash, (const char *) rediskey);

		if (credis_decr(redis, rediskey, &val) != 0) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Couldn't decrement value corresponding to %s", rediskey);
			oset_goto_status(OSET_STATUS_FALSE, end);
		}
		if (credis_decr(redis, uuid_rediskey, &uuid_val) != 0) {
			oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_ERROR, "Couldn't decrement value corresponding to %s", uuid_rediskey);
			oset_goto_status(OSET_STATUS_FALSE, end);
		}

/*
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_INFO, "Limit release redis : rediskey : %s val : %d", rediskey,val);
		oset_log2_printf(OSET_CHANNEL_SESSION_LOG(session), OSET_LOG2_INFO, "Limit incr redis : uuid_rediskey : %s uuid_val : %d", uuid_rediskey,uuid_val);
*/
	}
end:
	oset_apr_mutex_unlock(pvt->mutex);
	if (redis) {
		credis_close(redis);
	}
	return status;
}

OSET_LIMIT_USAGE(limit_usage_redis)
{
	char *redis_key;
	char *str;
	REDIS redis;
	int usage;

	if (redis_factory(&redis) != OSET_STATUS_SUCCESS) {
		return 0;
	}

	redis_key = oset_mprintf("%s_%s", realm, resource);

	if (credis_get(redis, redis_key, &str) != 0){
		usage = 0;
	} else {
		usage = atoi(str);
	}

	if (redis) {
		credis_close(redis);
	}

	oset_safe_free(redis_key);
	return usage;
}

OSET_LIMIT_RESET(limit_reset_redis)
{
	REDIS redis;
	if (redis_factory(&redis) == OSET_STATUS_SUCCESS) {
		char *rediskey = oset_mprintf("%s_*", oset_core_get_osetname());
		int dec = 0, val = 0, keyc;
		char *uuids[2000];

		if ((keyc = credis_keys(redis, rediskey, uuids, oset_arraylen(uuids))) > 0) {
			int i = 0;
			int hostnamelen = (int)strlen(oset_core_get_osetname())+1;

			for (i = 0; i < keyc && uuids[i]; i++){
				const char *key = uuids[i] + hostnamelen;
				char *value;

				if ((int)strlen(uuids[i]) <= hostnamelen) {
					continue; /* Sanity check */
				}

				credis_get(redis, key, &value);
				dec = atoi(value);
				credis_decrby(redis, key, dec, &val);

				oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_DEBUG, "DECR %s by %d. value is now %d", key, dec, val);
			}
		}
		oset_safe_free(rediskey);
		credis_close(redis);
		return OSET_STATUS_SUCCESS;
	} else {
		oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_WARNING, "Couldn't check/clear old redis entries");
		return OSET_STATUS_FALSE;
	}
}

OSET_LIMIT_STATUS(limit_status_redis)
{
	char *ret = oset_mprintf("This function is not yet available for Redis DB");
	return ret;
}

OSET_MODULE_LOAD_FUNCTION(mod_redis_load)
{
	oset_limit_interface_t *limit_interface = NULL;

	*module_interface = oset_loadable_module_create_module_interface(pool, modname);

	if (oset_xml_config_parse_module_settings("credis.conf", OSET_FALSE, instructions) != OSET_STATUS_SUCCESS) {
		return OSET_STATUS_FALSE;
	}

	//oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_CRIT, "mod_credis is deprecated and will be removed. Check out mod_hiredis.");

	/* If core was restarted and we still have active calls, decrement them so our global count stays valid */
	limit_reset_redis();

	OSET_ADD_LIMIT(limit_interface, "credis", limit_incr_redis, limit_release_redis, limit_usage_redis, limit_reset_redis, limit_status_redis, NULL);

	return OSET_STATUS_SUCCESS;
}

OSET_MODULE_SHUTDOWN_FUNCTION(mod_redis_shutdown)
{

	oset_xml_config_cleanup(instructions);

	return OSET_STATUS_SUCCESS;
}

