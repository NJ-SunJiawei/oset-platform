/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/

#ifndef RRC_H_
#define RRC_H_

#include "rlc/rlc_lib.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
  uint32_t lcid;
  uint32_t plmn;
  uint16_t mtch_stop;
  uint8_t *payload;
} mch_service_t;

typedef struct
{
    oset_lnode_t     lnode;
	uint16_t	     rnti;
	rlc_lib_t        rlc;
}rlc_user_interface;

typedef struct rlc_manager_s{
	oset_apr_memory_pool_t   *app_pool;
	oset_apr_mutex_t         *mutex;
	oset_apr_thread_cond_t   *cond;
	//oset_apr_thread_rwlock_t *rwlock;

	oset_hash_t            *users;//std::map<uint32_t, user_interface> users
	oset_list_t 		   rlc_ue_list;

	cvector_vector_t(mch_service_t)	mch_services;
}rlc_manager_t;

rlc_manager_t *rlc_manager_self(void);
int rlc_init(void);
int rlc_destory(void);
void rlc_user_interface_set_rnti(uint16_t rnti, rlc_user_interface *user);
rlc_user_interface *rlc_user_interface_find_by_rnti(uint16_t rnti);
void rlc_rem_user_all(void);
////////////////////////////////////////////////////////////////////////////////////////////
/**********************mac api****************************************/
void API_rlc_mac_write_ul_pdu(uint16_t rnti, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes);
int API_rlc_mac_read_dl_pdu(uint16_t rnti, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes);
/**********************rrc api****************************************/
void API_rlc_rrc_add_user(uint16_t rnti);
void API_rlc_rrc_rem_user(uint16_t rnti);
void API_rlc_rrc_add_bearer(uint16_t rnti, uint32_t lcid, rlc_config_t *cnfg);
void API_rlc_rrc_del_bearer(uint16_t rnti, uint32_t lcid);
void API_rlc_rrc_write_dl_sdu(uint16_t rnti, uint32_t lcid, byte_buffer_t *sdu);
/**********************pdcp api****************************************/

bool API_rlc_pdcp_rb_is_um(uint16_t rnti, uint32_t lcid);


#ifdef __cplusplus
}
#endif

#endif
