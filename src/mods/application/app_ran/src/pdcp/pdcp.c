/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "gnb_common.h"
#include "pdcp/pdcp.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-pdcp"

static pdcp_manager_t pdcp_manager = {0};

pdcp_manager_t *pdcp_manager_self(void)
{
	return &pdcp_manager;
}

static void pdcp_manager_init(void)
{
	pdcp_manager.app_pool = gnb_manager_self()->app_pool;
	oset_apr_mutex_init(&pdcp_manager.mutex, OSET_MUTEX_NESTED, pdcp_manager.app_pool);
	oset_apr_thread_cond_create(&pdcp_manager.cond, pdcp_manager.app_pool);
}

static void pdcp_manager_destory(void)
{
	oset_apr_mutex_destroy(pdcp_manager.mutex);
	oset_apr_thread_cond_destroy(pdcp_manager.cond);
	pdcp_manager.app_pool = NULL; /*app_pool release by openset process*/
}

int pdcp_init(void)
{
	pdcp_manager_init();
	oset_list_init(&pdcp_manager.pdcp_ue_list);
	pdcp_manager.users = oset_hash_make();
	return OSET_OK;
}

int pdcp_destory(void)
{
	pdcp_rem_user_all();
	oset_list_empty(&pdcp_manager.pdcp_ue_list);
	oset_hash_destroy(pdcp_manager.users);
	pdcp_manager_destory();
	return OSET_OK;
}

void pdcp_user_interface_set_rnti(uint16_t rnti, pdcp_user_interface *user)
{
    oset_assert(user);
	user->rnti = rnti;
    oset_hash_set(pdcp_manager.users, &user->rnti, sizeof(user->rnti), NULL);
    oset_hash_set(pdcp_manager.users, &user->rnti, sizeof(user->rnti), user);
}

pdcp_user_interface *pdcp_user_interface_find_by_rnti(uint16_t rnti)
{
    return (pdcp_user_interface *)oset_hash_get(pdcp_manager.users, &rnti, sizeof(rnti));
}

static void pdcp_add_bearer(uint16_t rnti, uint32_t lcid, pdcp_config_t *cfg)
{
	pdcp_user_interface *users = pdcp_user_interface_find_by_rnti(rnti);
	if (users) {
		if (rnti != SRSRAN_MRNTI) {
			pdcp_lib_add_bearer(&users->pdcp, lcid, cfg);
		} else {
			// todo
			//pdcp_lib_add_bearer_mrb(&users->pdcp, lcid, cfg);
		}
	}else{
		oset_warn("can't find pdcp interface by rnti=0x%x", rnti);
	}
}

static void pdcp_del_bearer(uint16_t rnti, uint32_t lcid)
{
	pdcp_user_interface *users = pdcp_user_interface_find_by_rnti(rnti);
	if (users) {
		pdcp_lib_del_bearer(&users->pdcp, lcid);
	}
}

static void pdcp_add_user(uint16_t rnti)
{
  if (NULL == pdcp_user_interface_find_by_rnti(rnti)) {
	  pdcp_user_interface *user = NULL;
	  oset_apr_memory_pool_t  *usepool = NULL;
	  oset_core_new_memory_pool(&usepool);
	  
	  user = oset_core_alloc(usepool, sizeof(*user));
	  ASSERT_IF_NOT(user, "Could not allocate pdcp user %d context from pool", rnti);
	  memset(user, 0, sizeof(rlc_user_interface));

	  user->pdcp.usepool = usepool;
	  user->rnti = rnti;
	  user->pdcp.rnti = rnti;
	  
	  pdcp_lib_init(&user->pdcp);
	  pdcp_user_interface_set_rnti(rnti, user);
	  oset_list_add(&pdcp_manager.pdcp_ue_list, user);
  }
}

static void pdcp_rem_user(uint16_t rnti)
{
	pdcp_user_interface* user = pdcp_user_interface_find_by_rnti(rnti);
	if (NULL == user) {
		oset_error("Removing rnti=0x%x. Already removed", rnti);
	}else{
		pdcp_lib_stop(&user->pdcp);
		oset_list_remove(&pdcp_manager.pdcp_ue_list, user);
		oset_hash_set(pdcp_manager.users, &user->rnti, sizeof(user->rnti), NULL);
		oset_core_destroy_memory_pool(&user->pdcp.usepool);
		user = NULL;
	}
}

void pdcp_rem_user_all(void)
{
	pdcp_user_interface *user = NULL, *next_user = NULL;
	oset_list_for_each_safe(pdcp_manager.pdcp_ue_list, next_user, user)
	  	pdcp_rem_user(user->rnti);
}

static void pdcp_config_security(uint16_t rnti, uint32_t lcid, struct as_security_config_t *sec_cfg)
{
	pdcp_user_interface *users = pdcp_user_interface_find_by_rnti(rnti);
	if (users) {
		pdcp_lib_config_security(&users->pdcp, lcid, sec_cfg);
	}
}

static void pdcp_enable_integrity(uint16_t rnti, uint32_t lcid)
{
	pdcp_user_interface *users = pdcp_user_interface_find_by_rnti(rnti);
	if (users) {
		pdcp_lib_enable_integrity(&users->pdcp, lcid, DIRECTION_TXRX);
	}
}

static void pdcp_enable_encryption(uint16_t rnti, uint32_t lcid)
{
	pdcp_user_interface *users = pdcp_user_interface_find_by_rnti(rnti);
	if (users) {
		pdcp_lib_enable_encryption(&users->pdcp, lcid, DIRECTION_TXRX);
	}
}


static void pdcp_write_ul_pdu_interface(uint16_t rnti, uint32_t lcid, byte_buffer_t *pdu)
{
  pdcp_user_interface *user = pdcp_user_interface_find_by_rnti(rnti);
  if (user) {
	  pdcp_lib_write_pdu(&user->pdcp, lcid, pdu);
  }
}

/*******************************************************************************
RRC interface
*******************************************************************************/
void API_pdcp_rrc_add_user(uint16_t rnti)
{
	pdcp_add_user(rnti);
}

void API_pdcp_rrc_rem_user(uint16_t rnti)
{
	pdcp_rem_user(rnti);
}

void API_pdcp_rrc_add_bearer(uint16_t rnti, uint32_t lcid, pdcp_config_t *cfg)
{
	pdcp_add_bearer(rnti, lcid, cfg);
}

void API_pdcp_rrc_del_bearer(uint16_t rnti, uint32_t lcid)
{
	pdcp_del_bearer(rnti, lcid);
}

void API_pdcp_rrc_config_security(uint16_t rnti, uint32_t lcid, struct as_security_config_t *sec_cfg)
{
	pdcp_config_security(rnti, lcid, sec_cfg);
}

void API_pdcp_rrc_enable_integrity(uint16_t rnti, uint32_t lcid)
{
	pdcp_enable_integrity(rnti, lcid);
}

void API_pdcp_rrc_enable_encryption(uint16_t rnti, uint32_t lcid)
{
	pdcp_enable_encryption(rnti, lcid);
}


/*******************************************************************************
RLC interface
*******************************************************************************/
void API_pdcp_rlc_write_ul_pdu(uint16_t rnti, uint32_t lcid, byte_buffer_t *pdu)
{
	pdcp_write_ul_pdu_interface(rnti, lcid, pdu);
}

