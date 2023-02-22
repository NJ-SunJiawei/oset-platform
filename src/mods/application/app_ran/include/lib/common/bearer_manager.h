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
  srsran_rat_t         rat;
  uint32_t			   lcid;
  uint32_t			   eps_bearer_id;
  uint32_t			   five_qi;// = 0
}radio_bearer_t;

typedef struct {
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


#ifdef __cplusplus
}
#endif

#endif
