/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/

#ifndef PDCP_LIB_H_
#define PDCP_LIB_H_

#include "pdcp/pdcp_entity_nr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	oset_apr_memory_pool_t	 *usepool;
	uint16_t	             rnti;
	// task_sched_handle  task_sched;
	oset_hash_t              *pdcp_array; // std::map<uint16_t, std::unique_ptr<pdcp_entity_nr>>;
	//oset_hash_t              *pdcp_array_mrb;
	// cache valid lcids to be checked from separate thread
	oset_apr_mutex_t		 *cache_mutex;
	cvector_vector_t(uint32_t) valid_lcids_cached;
	// oset_rbtree_t      valid_lcids_cached;

	// Timer needed for metrics calculation
	oset_time_t              metrics_tp;
}pdcp_lib_t;

void pdcp_valid_lcids_insert(pdcp_lib_t *pdcp, uint32_t lcid);
void pdcp_valid_lcids_delete(pdcp_lib_t *pdcp, uint32_t lcid);

pdcp_entity_nr* pdcp_valid_lcid(pdcp_lib_t *pdcp, uint32_t lcid);
pdcp_entity_nr *pdcp_array_find_by_lcid(pdcp_lib_t *pdcp, uint32_t lcid);

void pdcp_lib_init(pdcp_lib_t *pdcp);
void pdcp_lib_stop(pdcp_lib_t *pdcp);
int pdcp_lib_add_bearer(pdcp_lib_t *pdcp, uint32_t lcid, pdcp_config_t *cfg);
void pdcp_lib_del_bearer(pdcp_lib_t *pdcp, uint32_t lcid);
void pdcp_lib_config_security(pdcp_lib_t *pdcp, uint32_t lcid, struct as_security_config_t *sec_cfg);

#ifdef __cplusplus
}
#endif

#endif
