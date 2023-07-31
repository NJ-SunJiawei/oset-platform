/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef BEARER_MANAGER_H_
#define BEARER_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "oset-core.h"
#include "lib/common/common.h"

typedef struct {
	srsran_rat_t           rat;
	uint32_t			   lcid;
	uint8_t			       eps_bearer_id;
	uint32_t			   five_qi;// = 0
}radio_bearer_t;

typedef struct {
	uint16_t     rnti;
	oset_hash_t  *bearers;//std::map<uint32_t, radio_bearer_t>
	oset_hash_t  *lcid_to_eps_bearer_id;//std::map<uint32_t, uint32_t> 
}ue_bearer_manager_impl;

typedef struct {
	oset_apr_thread_rwlock_t *rwlock; /// RW lock to protect access from RRC/GW threads
	ue_bearer_manager_impl impl;
}ue_bearer_manager;


typedef struct {
	oset_hash_t *users_map;//std::unordered_map<uint16_t, ue_bearer_manager_impl>
}enb_bearer_manager;


bool is_valid(radio_bearer_t *rb);
void eps_bearer_id_set_lcid(ue_bearer_manager_impl *user, uint32_t lcid, uint8_t *eps_bearer_id);
uint8_t *eps_bearer_id_find_by_lcid(ue_bearer_manager_impl *user, uint32_t lcid);
void radio_bearer_set_rnti(ue_bearer_manager_impl *user, uint8_t eps_bearer_id, radio_bearer_t *rb);
radio_bearer_t *radio_bearer_find_by_rnti(ue_bearer_manager_impl *user, uint8_t eps_bearer_id);
radio_bearer_t* ue_bearer_manager_impl_get_eps_bearer_id_for_lcid(ue_bearer_manager_impl *user, uint32_t lcid);
bool ue_bearer_manager_impl_add_eps_bearer(ue_bearer_manager_impl *user, uint8_t eps_bearer_id, srsran_rat_t rat, uint32_t lcid);
ue_bearer_manager_impl *ue_bearer_manager_impl_init(uint16_t rnti);
void ue_bearer_manager_set_rnti(enb_bearer_manager *bearer_mapper, uint16_t rnti, ue_bearer_manager_impl *user);
ue_bearer_manager_impl *ue_bearer_manager_find_by_rnti(enb_bearer_manager *bearer_mapper, uint16_t rnti);
void add_eps_bearer(enb_bearer_manager *bearer_mapper, uint16_t rnti, uint8_t eps_bearer_id, srsran_rat_t rat, uint32_t lcid);
void remove_eps_bearer(enb_bearer_manager *bearer_mapper, uint16_t rnti, uint8_t eps_bearer_id);
void rem_user(enb_bearer_manager *bearer_mapper, uint16_t rnti);
radio_bearer_t* get_lcid_bearer(enb_bearer_manager *bearer_mapper, uint16_t rnti, uint32_t lcid);
radio_bearer_t *get_radio_bearer(enb_bearer_manager *bearer_mapper, uint16_t rnti, uint32_t eps_bearer_id);


#ifdef __cplusplus
}
#endif

#endif
