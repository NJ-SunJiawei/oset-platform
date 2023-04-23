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
    ue_nr *ue = NULL;
    oset_pool_alloc(&mac_manager_self()->ue_pool, &ue);
	ASSERT_IF_NOT(ue, "Could not allocate sched ue %d context from pool", rnti);

    memset(ue, 0, sizeof(ue_nr));
	ue->ue_rlc_buffer = oset_malloc(sizeof(byte_buffer_t));

	ue_nr_set_rnti(rnti, ue);
    oset_list_add(&mac_manager_self()->mac_ue_list, ue);

    oset_info("[Added] Number of MAC-UEs is now %d", oset_list_count(&mac_manager_self()->mac_ue_list));

	return ue;
}

void ue_nr_remove(ue_nr *ue)
{
    oset_assert(ue);

    oset_free(ue->ue_rlc_buffer);

    oset_list_remove(&mac_manager_self()->mac_ue_list, ue);
    oset_hash_set(&mac_manager_self()->ue_db, &ue->rnti, sizeof(ue->rnti), NULL);
    oset_pool_free(&mac_manager_self()->ue_pool, ue);

    oset_info("[Removed] Number of MAC-UEs is now %d", oset_list_count(&mac_manager_self()->mac_ue_list));
}

void ue_nr_set_rnti(uint16_t rnti, ue_nr *ue)
{
    oset_assert(ue);
	ue->rnti = rnti;
    oset_hash_set(&mac_manager_self()->ue_db, &rnti, sizeof(rnti), NULL);
    oset_hash_set(&mac_manager_self()->ue_db, &rnti, sizeof(rnti), ue);
}

ue_nr *ue_nr_find_by_rnti(uint16_t rnti)
{
    return (ue_nr *)oset_hash_get(
            &mac_manager_self()->ue_db, &rnti, sizeof(rnti));
}


