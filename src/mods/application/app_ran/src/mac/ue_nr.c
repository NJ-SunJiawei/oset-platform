/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac/ue_nr.h"
#include "mac/mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-ue_nr"

ue_nr *ue_nr_add(uint16_t rnti)
{
    ue_nr *ue_nr_ct = NULL;
    oset_pool_alloc(&mac_manager_self()->ue_nr_mac_pool, &ue_nr_ct);
    if (ue_nr_ct == NULL) {
        oset_error("Could not allocate ue_nr_ct context from pool");
        return NULL;
    }

    memset(ue_nr_ct, 0, sizeof(ue_nr));
	ue_nr_ct->ue_rlc_buffer = oset_malloc(sizeof(byte_buffer_t));

	ue_nr_set_rnti(ue_nr_ct, rnti);

    oset_info("[Added] Number of MAC-UEs is now %d", oset_hash_count(&mac_manager_self()->ue_db));

	return ue_nr_ct;
}

void ue_nr_remove(ue_nr *ue_nr_ct)
{
    oset_assert(ue_nr_ct);

    oset_free(ue_nr_ct->ue_rlc_buffer);
    oset_hash_set(&mac_manager_self()->ue_db, &ue_nr_ct->rnti, sizeof(ue_nr_ct->rnti), NULL);
    oset_pool_free(&mac_manager_self()->ue_nr_mac_pool, ue_nr_ct);

    oset_info("[Removed] Number of MAC-UEs is now %d", oset_hash_count(&mac_manager_self()->ue_db));
}

int ue_nr_set_rnti(ue_nr *ue_nr_ct, uint16_t rnti)
{
    oset_assert(ue_nr_ct);
	ue_nr_ct->rnti = rnti;
    oset_hash_set(&mac_manager_self()->ue_db, &rnti, sizeof(rnti), NULL);
    oset_hash_set(&mac_manager_self()->ue_db, &rnti, sizeof(rnti), ue_nr_ct);
    return OSET_OK;
}

ue_nr *ue_nr_find_by_rnti(uint16_t rnti)
{
    return (ue_nr *)oset_hash_get(
            &mac_manager_self()->ue_db, &rnti, sizeof(rnti));
}


