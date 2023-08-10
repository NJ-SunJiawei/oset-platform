/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "lib/common/bearer_manager.h"
		
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-libbearer"

bool is_valid(radio_bearer_t *rb) 
{ 
	return rb->rat != NULL; 
}

void eps_bearer_id_set_lcid(ue_bearer_manager_impl *user, uint32_t *lcid, uint8_t *eps_bearer_id)
{
    oset_assert(user);
    oset_assert(eps_bearer_id);
    oset_hash_set(user->lcid_to_eps_bearer_id, lcid, sizeof(*lcid), NULL);
    oset_hash_set(user->lcid_to_eps_bearer_id, lcid, sizeof(*lcid), eps_bearer_id);
}

uint8_t *eps_bearer_id_find_by_lcid(ue_bearer_manager_impl *user, uint32_t lcid)
{
    return (uint8_t *)oset_hash_get(user->lcid_to_eps_bearer_id, &lcid, sizeof(lcid));
}


void radio_bearer_set_eps_bearer_id(ue_bearer_manager_impl *user, uint8_t *eps_bearer_id, radio_bearer_t *rb)
{
    oset_assert(user);
    oset_assert(rb);
    oset_hash_set(user->bearers, eps_bearer_id, sizeof(*eps_bearer_id), NULL);
    oset_hash_set(user->bearers, eps_bearer_id, sizeof(*eps_bearer_id), rb);
}

radio_bearer_t *radio_bearer_find_by_eps_bearer_id(ue_bearer_manager_impl *user, uint8_t eps_bearer_id)
{
    return (radio_bearer_t *)oset_hash_get(user->bearers, &eps_bearer_id, sizeof(eps_bearer_id));
}

radio_bearer_t* ue_bearer_manager_impl_get_eps_bearer_id_for_lcid(ue_bearer_manager_impl *user, uint32_t lcid)
{
	uint8_t *lcid_it = eps_bearer_id_find_by_lcid(user, lcid);
	return lcid_it != NULL ?radio_bearer_find_by_eps_bearer_id(user, *lcid_it) : NULL;
}

radio_bearer_t* ue_bearer_manager_impl_get_radio_bearer(ue_bearer_manager_impl *user, uint8_t eps_bearer_id)
{
	radio_bearer_t * it = radio_bearer_find_by_eps_bearer_id(user, eps_bearer_id);
	return it;
}

bool ue_bearer_manager_impl_add_eps_bearer(ue_bearer_manager_impl *user, uint8_t eps_bearer_id, srsran_rat_t rat, uint32_t lcid)
{
	radio_bearer_t *bearer_it = radio_bearer_find_by_eps_bearer_id(user, eps_bearer_id);
	if (bearer_it != NULL) {
		return false;
	}
	radio_bearer_t *rb = oset_malloc(radio_bearer_t);
	rb->rat = rat;
	rb->lcid = lcid;
	rb->eps_bearer_id = eps_bearer_id;
	rb->five_qi = 0;

	radio_bearer_set_eps_bearer_id(user, &rb->eps_bearer_id, rb);
	eps_bearer_id_set_lcid(user, &rb->lcid, &rb->eps_bearer_id);//oset_memdup(&eps_bearer_id, sizeof(uint8_t)
	return true;
}

bool ue_bearer_manager_impl_remove_eps_bearer(ue_bearer_manager_impl *user, uint8_t eps_bearer_id)
{
	radio_bearer_t *bearer_it = radio_bearer_find_by_eps_bearer_id(user, eps_bearer_id);
	if (bearer_it == NULL) {
		return false;
	}

	oset_hash_set(user->bearers, &bearer_it->eps_bearer_id, sizeof(bearer_it->eps_bearer_id), NULL);
	oset_hash_set(user->lcid_to_eps_bearer_id, &bearer_it->lcid, sizeof(bearer_it->lcid), NULL);
	oset_free(bearer_it);
	return true;
}

ue_bearer_manager_impl *ue_bearer_manager_impl_init(uint16_t rnti)
{
	ue_bearer_manager_impl *user = oset_malloc(sizeof(*user));
	ASSERT_IF_NOT(user, "Could not allocate sched ue %d context from pool");
	memset(user, 0, sizeof(ue_bearer_manager_impl));	
	user->rnti = rnti;
	user->bearers = oset_hash_make();
	user->lcid_to_eps_bearer_id = oset_hash_make();
	return user;
}


ue_bearer_manager_impl *ue_bearer_manager_find_by_rnti(enb_bearer_manager *bearer_mapper, uint16_t rnti)
{
    return (ue_bearer_manager_impl *)oset_hash_get(bearer_mapper->users_map, &rnti, sizeof(rnti));
}

void add_eps_bearer(enb_bearer_manager *bearer_mapper, uint16_t rnti, uint8_t eps_bearer_id, srsran_rat_t rat, uint32_t lcid)
{
	ue_bearer_manager_impl *user_it = ue_bearer_manager_find_by_rnti(bearer_mapper, rnti);
	if (user_it == NULL) {
		// add empty bearer map
		ue_bearer_manager_impl *user = ue_bearer_manager_impl_init(rnti);
		oset_assert(user);
		oset_hash_set(bearer_mapper->users_map, &user->rnti, sizeof(user->rnti), NULL);
		oset_hash_set(bearer_mapper->users_map, &user->rnti, sizeof(user->rnti), user);
	}

	if (ue_bearer_manager_impl_add_eps_bearer(eps_bearer_id, rat, lcid)) {
		oset_info("Bearers: Registered eps-BearerID=%d for rnti=0x%x, lcid=%d over %s-PDCP",
		            eps_bearer_id,
		            rnti,
		            lcid,
		            rat_to_string(rat));
	} else {
		oset_warn("Bearers: EPS bearer ID %d for rnti=0x%x already registered", eps_bearer_id, rnti);
	}
}

void remove_eps_bearer(enb_bearer_manager *bearer_mapper, uint16_t rnti, uint8_t eps_bearer_id)
{
	ue_bearer_manager_impl *user_it = ue_bearer_manager_find_by_rnti(bearer_mapper, rnti);;
	if (user_it == NULL) {
		oset_info("Bearers: No EPS bearer registered for rnti=0x%x", rnti);
		return;
	}

	if (ue_bearer_manager_impl_remove_eps_bearer(user_it, eps_bearer_id)) {
		oset_info("Bearers: Removed mapping for EPS bearer ID %d for rnti=0x%x", eps_bearer_id, rnti);
	} else {
		oset_info("Bearers: Can't remove EPS bearer ID %d, rnti=0x%x", eps_bearer_id, rnti);
	}
}


void rem_user(enb_bearer_manager *bearer_mapper, uint16_t rnti)
{
	ue_bearer_manager_impl *user_it = ue_bearer_manager_find_by_rnti(bearer_mapper, rnti);
	if (user_it == NULL) {
		oset_info("Bearers: No EPS bearer registered for rnti=0x%x", rnti);
		return;
	}

 	oset_info("Bearers: Removed rnti=0x%x from EPS bearer manager", rnti);
	oset_hash_set(bearer_mapper->users_map, &user_it->rnti, sizeof(user_it->rnti), NULL);
	oset_hash_destroy(user_it->bearers);
	oset_hash_destroy(user_it->lcid_to_eps_bearer_id);
	oset_free(user_it);
}

radio_bearer_t* get_lcid_bearer(enb_bearer_manager *bearer_mapper, uint16_t rnti, uint32_t lcid)
{
	ue_bearer_manager_impl *user_it = ue_bearer_manager_find_by_rnti(bearer_mapper, rnti);
	if (user_it == NULL) {
    	return NULL;
  	}
  	return ue_bearer_manager_impl_get_eps_bearer_id_for_lcid(user_it, lcid);
}

radio_bearer_t *get_radio_bearer(enb_bearer_manager *bearer_mapper, uint16_t rnti, uint32_t eps_bearer_id)
{
	ue_bearer_manager_impl *user_it = ue_bearer_manager_find_by_rnti(bearer_mapper, rnti);;
	if (user_it == NULL) {
    	return NULL;
  	}

  	return ue_bearer_manager_impl_get_radio_bearer(user_it, eps_bearer_id);
}

