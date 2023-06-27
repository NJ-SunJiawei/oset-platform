/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "gnb_common.h"
#include "mac/mac.h"
#include "rlc/rlc.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlc"

static rlc_manager_t rlc_manager = {0};

rlc_manager_t *rlc_manager_self(void)
{
	return &rlc_manager;
}

static void rlc_manager_init(void)
{
	rlc_manager.app_pool = gnb_manager_self()->app_pool;
	oset_apr_mutex_init(&rlc_manager.mutex, OSET_MUTEX_NESTED, rlc_manager.app_pool);
	oset_apr_thread_cond_create(&rlc_manager.cond, rlc_manager.app_pool);
	oset_apr_thread_rwlock_create(&rlc_manager.rwlock, rlc_manager.app_pool);
}

static void rlc_manager_destory(void)
{
	oset_apr_mutex_destroy(rlc_manager.mutex);
	oset_apr_thread_cond_destroy(rlc_manager.cond);
	oset_apr_thread_rwlock_destroy(rlc_manager.rwlock);
	rlc_manager.app_pool = NULL; /*app_pool release by openset process*/
}

int rlc_init(void)
{
	rlc_manager_init();
	oset_pool_init(&rlc_manager.ue_pool, SRSENB_MAX_UES);
	oset_list_init(&rlc_manager.rlc_ue_list);
	rlc_manager.users = oset_hash_make();
	return OSET_OK;
}

int rlc_destory(void)
{
	oset_list_empty(&rlc_manager.rlc_ue_list);
	oset_hash_destroy(rlc_manager.users);
	oset_pool_final(&rlc_manager.ue_pool);
	rlc_manager_destory();
	return OSET_OK;
}

void rlc_user_interface_set_rnti(uint16_t rnti, rlc_user_interface *user)
{
    oset_assert(user);
	user->rnti = rnti;
    oset_hash_set(rlc_manager.users, &rnti, sizeof(rnti), NULL);
    oset_hash_set(rlc_manager.users, &rnti, sizeof(rnti), user);
}

rlc_user_interface *rlc_user_interface_find_by_rnti(uint16_t rnti)
{
    return (rlc_user_interface *)oset_hash_get(rlc_manager.users, &rnti, sizeof(rnti));
}

// In the eNodeB, there is no polling for buffer state from the scheduler.
// This function is called by UE RLC instance every time the tx/retx buffers are updated
void rlc_update_bsr(uint32_t rnti, uint32_t lcid, uint32_t tx_queue, uint32_t prio_tx_queue)
{
	oset_debug("Buffer state: rnti=0x%x, lcid=%d, tx_queue=%d, prio_tx_queue=%d", rnti, lcid, tx_queue, prio_tx_queue);
	API_mac_rlc_buffer_state(rnti, lcid, tx_queue, prio_tx_queue);
}

void rlc_add_user(uint16_t rnti)
{
	//oset_apr_thread_rwlock_wrlock(rlc_manager.rwlock);
	if (NULL == rlc_user_interface_find_by_rnti(rnti)) {
		rlc_user_interface *user = NULL;
		oset_pool_alloc(&rlc_manager.ue_pool, &user);
		ASSERT_IF_NOT(user, "Could not allocate rlc user %d context from pool", rnti);
		memset(user, 0, sizeof(rlc_user_interface));

		rlc_init(&user->rlc, srb_to_lcid((nr_srb)srb0), rlc_update_bsr);
		user->rnti = rnti;
	}
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
}

void API_rlc_rrc_add_user(uint16_t rnti)
{
	rlc_add_user(rnti);
}

int API_rlc_mac_read_pdu(uint16_t rnti, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes)
{
	int ret = OSET_ERROR;
	rlc_user_interface *user = rlc_user_interface_find_by_rnti(rnti);

	//oset_apr_thread_rwlock_rdlock(rlc_manager.rwlock);
	if (user) {
		if (rnti != SRSRAN_MRNTI) {
			ret = rlc_manager.users[rnti].rlc->read_pdu(lcid, payload, nof_bytes);
		} else {
			ret = rlc_manager.users[rnti].rlc->read_pdu_mch(lcid, payload, nof_bytes);//mcch 多播控制信道
		}
	} else {
		ret = OSET_ERROR;
	}
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
  return ret;
}

