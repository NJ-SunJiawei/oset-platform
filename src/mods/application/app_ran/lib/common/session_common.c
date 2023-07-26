/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "gnb_common.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-session"

void gnb_session_common_create(oset_core_session_t           *common, oset_apr_memory_pool_t *usepool)
{
	oset_uuid_t uuid;

	//common = oset_core_alloc(usepool, sizeof(oset_core_session_t));
	common->pool = usepool;

	oset_apr_uuid_get(&uuid);
    oset_apr_uuid_format(common->uuid_str, &uuid);

	//oset_session_set_variable(common, "uuid", common->uuid_str);

	oset_event_create_plain(&common->variables, OSET_EVENT_CHANNEL_DATA);
	oset_core_hash_init(&common->private_hash);
	oset_apr_mutex_init(&common->profile_mutex, OSET_MUTEX_NESTED, usepool);
	oset_apr_thread_rwlock_create(&common->rwlock, usepool);
}


void gnb_session_common_destory(oset_core_session_t *common)
{

	if (common->private_hash) oset_core_hash_destroy(&common->private_hash);
	oset_apr_mutex_lock(common->profile_mutex);
	oset_event_destroy(&common->variables);
	oset_apr_mutex_unlock(common->profile_mutex);
	
	oset_apr_thread_rwlock_destroy(common->rwlock);

	common->pool = NULL;

}

