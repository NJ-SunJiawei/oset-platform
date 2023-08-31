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

static int pdcp_lcid_compare(const void *a, const void *b)
{
	const int *x = a;
	const int *y = b;
	//升序
	return *x - *y;
}

void pdcp_lib_valid_lcids_insert(pdcp_lib_t *pdcp, uint32_t lcid)
{
	uint32_t *val = NULL;
	cvector_for_each_in(val, pdcp->valid_lcids_cached){
		if(lcid == *val) return;
	}
	cvector_push_back(pdcp->valid_lcids_cached, lcid);
	qsort(pdcp->valid_lcids_cached, cvector_size(pdcp->valid_lcids_cached), sizeof(uint32_t), pdcp_lcid_compare);
}

void pdcp_lib_valid_lcids_delete(pdcp_lib_t *pdcp, uint32_t lcid)
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

pdcp_entity_nr* pdcp_array_valid_lcid(pdcp_lib_t *pdcp, uint32_t lcid)
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
	oset_apr_mutex_init(&pdcp->cache_mutex, pdcp->usepool);
	pdcp->metrics_tp = 0;
}


void pdcp_lib_stop(pdcp_lib_t *pdcp)
{
	uint32_t *val = NULL;
	cvector_for_each_in(val, pdcp->valid_lcids_cached){
		pdcp_lib_del_bearer(pdcp, *val);
	}	
	cvector_free(pdcp->valid_lcids_cached);

	oset_hash_destroy(pdcp->pdcp_array);
	oset_apr_mutex_destroy(pdcp->cache_mutex);
}



int pdcp_lib_add_bearer(pdcp_lib_t *pdcp, uint32_t lcid, pdcp_config_t *cfg)
{
	pdcp_entity_nr  *entity = pdcp_array_valid_lcid(pdcp, lcid);

	if (NULL != entity) {
		return pdcp_entity_nr_configure(entity, cfg) ? OSET_OK : OSET_ERROR;
	}

	// For now we create an pdcp entity lte for nr due to it's maturity
	if (cfg->rat == (srsran_rat_t)nr) {
		entity = (pdcp_entity_base *)(pdcp_entity_nr_init(lcid), pdcp->rnti);
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
	pdcp_lib_valid_lcids_insert(pdcp, lcid);
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
	pdcp_lib_valid_lcids_delete(pdcp, lcid);
	//oset_apr_mutex_unlock(pdcp->cache_mutex);

	pdcp_entity_nr *entity = pdcp_array_valid_lcid(pdcp, lcid);
	if (entity) {
		oset_info("Deleted PDCP bearer %s", entity->base.rb_name);
		oset_hash_set(pdcp->pdcp_array, &entity->base.lcid, sizeof(entity->base.lcid), NULL);
		pdcp_entity_nr_stop(entity);
	} else {
		oset_warn("Can't delete bearer with LCID=%s. Cause: bearer doesn't exist", lcid);
	}
}

void pdcp_lib_config_security(pdcp_lib_t *pdcp, uint32_t lcid, struct as_security_config_t *sec_cfg)
{
	pdcp_entity_nr *entity = pdcp_array_valid_lcid(pdcp, lcid);
	if (entity) {
		pdcp_entity_base_config_security(&entity->base, sec_cfg);
	}
}

void pdcp_lib_enable_integrity(pdcp_lib_t *pdcp, uint32_t lcid, srsran_direction_t direction)
{
	pdcp_entity_nr *entity = pdcp_array_valid_lcid(pdcp, lcid);
	if (entity) {
		pdcp_entity_base_enable_integrity(&entity->base, direction);
	}
}

void pdcp_lib_enable_encryption(pdcp_lib_t *pdcp, uint32_t lcid, srsran_direction_t direction)
{
	pdcp_entity_nr *entity = pdcp_array_valid_lcid(pdcp, lcid);
	if (entity) {
		pdcp_entity_base_enable_encryption(&entity->base, direction);
	}
}

void pdcp_lib_write_ul_pdu(pdcp_lib_t *pdcp, uint32_t lcid, byte_buffer_t *pdu)
{
	pdcp_entity_nr *entity = pdcp_array_valid_lcid(pdcp, lcid);
	if (entity) {
		pdcp_entity_nr_write_ul_pdu(entity, pdu);
	} else {
		oset_warn("Dropping PDU, lcid=%d doesnt exists", lcid);
	}
}

void pdcp_lib_write_dl_sdu(pdcp_lib_t *pdcp, uint32_t lcid, byte_buffer_t *sdu, int sn)
{
	pdcp_entity_nr *entity = pdcp_array_valid_lcid(pdcp, lcid);
	if (entity) {
		pdcp_entity_nr_write_dl_sdu(entity, sdu, sn);
	} else {
		oset_warn("LCID %d doesn't exist. Deallocating SDU", lcid);
	}
}

void pdcp_lib_reset_metrics(pdcp_lib_t *pdcp)
{
	oset_hash_index_t *hi = NULL;
	for (hi = oset_hash_first(pdcp->pdcp_array); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		pdcp_entity_nr  *it = oset_hash_this_val(hi);
	    pdcp_entity_nr_reset_metrics(it);
	}

	pdcp->metrics_tp = oset_epoch_time_now();//oset_time_now();
}

void pdcp_lib_get_metrics(pdcp_lib_t *pdcp, pdcp_ue_metrics_t *ue_m, uint32_t nof_tti)
{
	time_t secs = oset_epoch_time_now() - pdcp->metrics_tp;

	oset_hash_index_t *hi = NULL;
	for (hi = oset_hash_first(pdcp->pdcp_array); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		pdcp_entity_nr  *it = oset_hash_this_val(hi);
		pdcp_bearer_metrics_t metrics = pdcp_entity_nr_get_metrics(it);

		// Rx/Tx rate based on real time
		double rx_rate_mbps_real_time = (metrics.num_rx_pdu_bytes * 8 / (double)1e6) / secs;
		double tx_rate_mbps_real_time = (metrics.num_tx_pdu_bytes * 8 / (double)1e6) / secs;

		// Rx/Tx rate based on number of TTIs (tti ~ ms)
		double rx_rate_mbps = (nof_tti > 0) ? ((metrics.num_rx_pdu_bytes * 8 / (double)1e6) / (nof_tti / 1000.0)) : 0.0;
		double tx_rate_mbps = (nof_tti > 0) ? ((metrics.num_tx_pdu_bytes * 8 / (double)1e6) / (nof_tti / 1000.0)) : 0.0;

		oset_debug("lcid=%d, rx_rate_mbps=%4.2f (real=%4.2f), tx_rate_mbps=%4.2f (real=%4.2f)",
		         lcid,
		         rx_rate_mbps,
		         rx_rate_mbps_real_time,
		         tx_rate_mbps,
		         tx_rate_mbps_real_time);
		ue_m->bearer[lcid] = metrics;
	}

	pdcp_lib_reset_metrics(pdcp);
}

