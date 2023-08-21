/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "pdcp/pdcp_lib.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-pdcplib"

static int lcid_compare(const void *a, const void *b)
{
	const int *x = a;
	const int *y = b;
	//升序
	return *x - *y;
}

void pdcp_valid_lcids_insert(pdcp_lib_t *pdcp, uint32_t lcid)
{
	uint32_t *val = NULL;
	cvector_for_each_in(val, pdcp->valid_lcids_cached){
		if(lcid == *val) return;
	}
	cvector_push_back(pdcp->valid_lcids_cached, lcid);
	qsort(pdcp->valid_lcids_cached, cvector_size(pdcp->valid_lcids_cached), sizeof(uint32_t), lcid_compare);
}

void pdcp_valid_lcids_delete(pdcp_lib_t *pdcp, uint32_t lcid)
{
	for (int i = 0; i < cvector_size(pdcp->valid_lcids_cached); i++){
		if(pdcp->valid_lcids_cached[i] = lcid){
			cvector_erase(pdcp->valid_lcids_cached, i);
		}
	}
}


pdcp_entity_nr *pdcp_array_find_by_lcid(pdcp_lib_t *pdcp, uint32_t lcid)
{
    return (pdcp_entity_nr *)oset_hash_get(pdcp->pdcp_array, &lcid, sizeof(lcid));
}

pdcp_entity_nr* pdcp_valid_lcid(pdcp_lib_t *pdcp, uint32_t lcid)
{
	if (lcid >= SRSRAN_N_RADIO_BEARERS) {
		oset_error("Radio bearer id must be in [0:%d] - %d", SRSRAN_N_RADIO_BEARERS, lcid);
		return NULL;
	}

	return pdcp_array_find_by_lcid(pdcp, lcid);
}

void pdcp_lib_init(pdcp_lib_t *pdcp)
{
	pdcp->pdcp_array = oset_hash_make();
	//pdcp->pdcp_array_mrb = oset_hash_make();
	oset_apr_mutex_init(&pdcp->cache_mutex, pdcp->usepool);
	pdcp->metrics_tp = 0;
}


void pdcp_lib_stop(pdcp_lib_t *pdcp)
{
	oset_hash_destroy(pdcp->pdcp_array);
	//oset_hash_destroy(pdcp->pdcp_array_mrb);
	oset_apr_mutex_destroy(pdcp->cache_mutex);

	uint32_t *val = NULL;
	cvector_for_each_in(val, pdcp->valid_lcids_cached){
		pdcp_lib_del_bearer(pdcp, *val);
	}	
	cvector_free(pdcp->valid_lcids_cached);
}



int pdcp_lib_add_bearer(pdcp_lib_t *pdcp, uint32_t lcid, pdcp_config_t *cfg)
{
	pdcp_entity_nr  *entity = pdcp_valid_lcid(pdcp, lcid);

	if (NULL != entity) {
		return pdcp_entity_nr_configure(entity, cfg) ? OSET_OK : OSET_ERROR;
	}

	// For now we create an pdcp entity lte for nr due to it's maturity
	if (cfg->rat == (srsran_rat_t)nr) {
		entity = (pdcp_entity_base *)(pdcp_entity_nr_init(lcid), pdcp->rnti, pdcp->usepool);
	}else{
		oset_error("Can not support other type PDCP entity");
		return OSET_ERROR;
	}

	if (!pdcp_entity_nr_configure(entity, cfg)) {
		oset_error("Can not configure PDCP entity");
		return OSET_ERROR;
	}

	oset_hash_set(pdcp->pdcp_array, &entity->base.lcid, sizeof(entity->base.lcid), NULL);
	oset_hash_set(pdcp->pdcp_array, &entity->base.lcid, sizeof(entity->base.lcid), entity);

	if (NULL == pdcp_array_find_by_lcid(pdcp, lcid)) {
		oset_error("Error inserting PDCP entity in to array.");
		return OSET_ERROR;
	}

	//oset_apr_mutex_lock(pdcp->cache_mutex);
	pdcp_valid_lcids_insert(pdcp, lcid);
	//oset_apr_mutex_unlock(pdcp->cache_mutex);

	oset_info("Add %s%d (lcid=%d, sn_len=%dbits)",
	          cfg->rb_type == PDCP_RB_IS_DRB ? "DRB" : "SRB",
	          cfg->bearer_id,
	          lcid,
	          cfg->sn_len);

	return OSET_OK;
}

void pdcp_lib_del_bearer(pdcp_lib_t *pdcp, uint32_t lcid)
{
	//oset_apr_mutex_lock(pdcp->cache_mutex);
	pdcp_valid_lcids_delete(pdcp, lcid);
	//oset_apr_mutex_unlock(pdcp->cache_mutex);

	pdcp_entity_nr *entity = pdcp_valid_lcid(pdcp, lcid);
	if (entity) {
		oset_info("Deleted PDCP bearer %s", entity->base.rb_name);
		pdcp_entity_nr_stop(entity);
		oset_hash_set(pdcp->pdcp_array, &entity->base.lcid, sizeof(entity->base.lcid), NULL);
	} else {
		oset_warn("Can't delete bearer with LCID=%s. Cause: bearer doesn't exist", lcid);
	}
}

void pdcp_lib_config_security(pdcp_lib_t *pdcp, uint32_t lcid, struct as_security_config_t *sec_cfg)
{
	pdcp_entity_nr *entity = pdcp_valid_lcid(pdcp, lcid);
	if (entity) {
		pdcp_entity_base_config_security(entity, sec_cfg);
	}
}


