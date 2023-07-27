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
	//oset_apr_thread_rwlock_create(&rlc_manager.rwlock, rlc_manager.app_pool);
}

static void rlc_manager_destory(void)
{
	oset_apr_mutex_destroy(rlc_manager.mutex);
	oset_apr_thread_cond_destroy(rlc_manager.cond);
	//oset_apr_thread_rwlock_destroy(rlc_manager.rwlock);
	rlc_manager.app_pool = NULL; /*app_pool release by openset process*/
}

int rlc_init(void)
{
	rlc_manager_init();
	oset_list_init(&rlc_manager.rlc_ue_list);
	rlc_manager.users = oset_hash_make();
	return OSET_OK;
}

int rlc_destory(void)
{
	rlc_rem_user_all();
	oset_list_empty(&rlc_manager.rlc_ue_list);
	oset_hash_destroy(rlc_manager.users);
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

static void rlc_add_user(uint16_t rnti)
{
	//oset_apr_thread_rwlock_wrlock(rlc_manager.rwlock);
	if (NULL == rlc_user_interface_find_by_rnti(rnti)) {
		rlc_user_interface *user = NULL;
		oset_apr_memory_pool_t	*usepool = NULL;
		oset_core_new_memory_pool(&usepool);

		user = oset_core_alloc(usepool, sizeof(*user));
		ASSERT_IF_NOT(user, "Could not allocate rlc user %d context from pool", rnti);
		memset(user, 0, sizeof(rlc_user_interface));

		user->rlc.usepool = usepool;
		user->rnti = rnti;
		user->rlc.rnti = rnti;

		// SRB0没有加密和完整性保护。SRB0上承载的信令有：RRCConnectionRequest、RRCConnectionReject、RRCConnectionSetup和RRCConnectionReestablishmentRequest、RRCConnectionReestablishment、RRCConnectionReestablishmentReject。
		// SRB0是用于传输RRC连接请求和RRC连接建立消息的承载，它不经过PDCP层，直接通过CCCH逻辑信道传输。
		// SRB1是用于传输其他RRC消息的承载，它经过PDCP层，并且在安全激活后受到完整性保护和加密，它通过DCCH逻辑信道传输。
		// SRB2是用于传输包含NAS消息的RRC消息的承载，它也经过PDCP层，并且在安全激活后受到完整性保护和加密，它通过DCCH逻辑信道传输。SRB2的优先级低于SRB1 SRB0。

		// 将SRB1定为与基站联系的信令通道，把SRB2定为与MME联系的信令通道
		// SRB2上传送的RRC信令非常少，只有两 种：UL information transfer以及DL information transfer消息，基站看到这两条信令，就知道不用处理，直接转发
		rlc_lib_init(&user->rlc, srb_to_lcid((nr_srb)srb0), rlc_update_bsr);
		rlc_user_interface_set_rnti(rnti, user);
    	oset_list_add(&rlc_manager.rlc_ue_list, user);
	}
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
}

static void rlc_rem_user(uint16_t rnti)
{
	//oset_apr_thread_rwlock_wrlock(rlc_manager.rwlock);
	rlc_user_interface* user = rlc_user_interface_find_by_rnti(rnti);
	if (NULL == user) {
		oset_error("Removing rnti=0x%x. Already removed", rnti);
	}else{
		rlc_lib_stop(&user->rlc);
		oset_list_remove(&rlc_manager.rlc_ue_list, user);
		oset_hash_set(rlc_manager.users, &rnti, sizeof(rnti), NULL);
		oset_core_destroy_memory_pool(&user->rlc.usepool);
		user = NULL;
	}
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
}

void rlc_rem_user_all(void)
{
	//oset_apr_thread_rwlock_wrlock(rlc_manager.rwlock);
	rlc_user_interface *user = NULL, *next_user = NULL;
	oset_list_for_each_safe(rlc_manager.rlc_ue_list, next_user, user)
	  	rlc_rem_user(user->rnti);
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
}

/*******************************************************************************
RRC interface
*******************************************************************************/

void API_rlc_rrc_add_user(uint16_t rnti)
{
	rlc_add_user(rnti);
}

void API_rlc_rrc_rem_user(uint16_t rnti)
{
	rlc_rem_user(rnti);
}

void API_rlc_rrc_write_dl_sdu(uint16_t rnti, uint32_t lcid, byte_buffer_t *sdu)
{
	//oset_apr_thread_rwlock_rdlock(rlc_manager.rwlock);
	rlc_user_interface *user = rlc_user_interface_find_by_rnti(rnti);
	if (user) {
		if (rnti != SRSRAN_MRNTI) {
			users[rnti].rlc->write_sdu(lcid, std::move(sdu));
		} else {
			users[rnti].rlc->write_sdu_mch(lcid, std::move(sdu));
		}
	}
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
}


/*******************************************************************************
MAC interface
*******************************************************************************/
int API_rlc_mac_read_pdu(uint16_t rnti, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes)
{
	int ret = OSET_ERROR;

	//oset_apr_thread_rwlock_rdlock(rlc_manager.rwlock);
	rlc_user_interface *user = rlc_user_interface_find_by_rnti(rnti);
	if (user) {
		if (rnti != SRSRAN_MRNTI) {
			ret = users[rnti].rlc->read_pdu(lcid, payload, nof_bytes);
		} else {
			ret = users[rnti].rlc->read_pdu_mch(lcid, payload, nof_bytes);//mcch 多播控制信道
		}
	} else {
		ret = OSET_ERROR;
	}
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
  return ret;
}



// gnb mac====》rlc uplink
//--------------sdu-----------(服务数据)
//--------------handle+-------
//--------------pdu-----------(协议数据)
void API_rlc_mac_write_ul_pdu(uint16_t rnti, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes)
{
	//oset_apr_thread_rwlock_rdlock(rlc_manager.rwlock);
	rlc_user_interface *user = rlc_user_interface_find_by_rnti(rnti);
	if (user) {
		rlc_lib_write_ul_pdu(&user->rlc, lcid, payload, nof_bytes);
	}else{
		oset_error("rnti=0x%x. API_rlc_mac_write_ul_pdu() fail", rnti);
	}
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
}

