/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "gnb_common.h"
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

void rlc_add_user(uint16_t rnti)
{
	//oset_apr_thread_rwlock_wrlock(rlc_manager.rwlock);
	if (users.count(rnti) == 0) {
	auto obj = make_rnti_obj<srsran::rlc>(rnti, logger.id().c_str());
	obj->init(&users[rnti],
	          &users[rnti],
	          timers,
	          srb_to_lcid(lte_srb::srb0),
	          [rnti, this](uint32_t lcid, uint32_t tx_queue, uint32_t retx_queue) {
	            update_bsr(rnti, lcid, tx_queue, retx_queue);
	          });
	users[rnti].rnti   = rnti;
	users[rnti].pdcp   = pdcp;
	users[rnti].rrc    = rrc;
	users[rnti].rlc    = std::move(obj);
	users[rnti].parent = this;
	}
	//oset_apr_thread_rwlock_unlock(rlc_manager.rwlock);
}

int rlc_read_pdu_api(uint16_t rnti, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes)
{
	int ret = OSET_ERROR;

	//oset_apr_thread_rwlock_rdlock(rlc_manager.rwlock);
	if (rlc_manager.users.count(rnti)) {
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

