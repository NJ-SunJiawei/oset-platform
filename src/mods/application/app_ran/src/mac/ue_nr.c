/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac/ue_nr.h"
#include "mac/mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-ue_nr"

ue_nr *ue_nr_add(uint16_t rnti)
{
    ue_nr *ue = NULL;
    oset_pool_alloc(&mac_manager_self()->ue_pool, &ue);
	ASSERT_IF_NOT(ue, "Could not allocate sched ue %d context from pool", rnti);

    memset(ue, 0, sizeof(ue_nr));

	ue->ue_rlc_buffer = oset_malloc(sizeof(byte_buffer_t));

	ue_nr_set_rnti(rnti, ue);
    oset_list_add(&mac_manager_self()->mac_ue_list, ue);
	oset_apr_mutex_init(&ue->metrics_mutex, OSET_MUTEX_NESTED, mac_manager_self()->app_pool);

    oset_info("[Added] Number of MAC-UEs is now %d", oset_list_count(&mac_manager_self()->mac_ue_list));

	return ue;
}

void ue_nr_remove(ue_nr *ue)
{
    oset_assert(ue);

    oset_free(ue->ue_rlc_buffer);

    oset_list_remove(&mac_manager_self()->mac_ue_list, ue);
    oset_hash_set(mac_manager_self()->ue_db, &ue->rnti, sizeof(ue->rnti), NULL);
    oset_pool_free(&mac_manager_self()->ue_pool, ue);
	oset_apr_mutex_destroy(ue->metrics_mutex);

    oset_info("[Removed] Number of MAC-UEs is now %d", oset_list_count(&mac_manager_self()->mac_ue_list));
}

void ue_nr_set_rnti(uint16_t rnti, ue_nr *ue)
{
    oset_assert(ue);
	ue->rnti = rnti;
    oset_hash_set(mac_manager_self()->ue_db, &rnti, sizeof(rnti), NULL);
    oset_hash_set(mac_manager_self()->ue_db, &rnti, sizeof(rnti), ue);
}

ue_nr *ue_nr_find_by_rnti(uint16_t rnti)
{
    return (ue_nr *)oset_hash_get(
            mac_manager_self()->ue_db, &rnti, sizeof(rnti));
}

/******* METRICS interface ***************/
void ue_nr_metrics_read(ue_nr *ue, mac_ue_metrics_t* metrics_)
{
	uint32_t ul_buffer = 0;
	uint32_t dl_buffer = 0;

	sched_nr_ue *sched_ue = sched_nr_ue_find_by_rnti(ue->rnti);
	if(sched_ue != NULL){
		dl_buffer = get_dl_tx_total(&sched_ue->buffers)
		ul_buffer = get_ul_bsr_total(&sched_ue->buffers)
	}

	metrics_->rnti      = ue->rnti;
	metrics_->ul_buffer = ul_buffer;
	metrics_->dl_buffer = dl_buffer;

	// set PCell sector id
	// TODO: use ue_cfg when multiple NR carriers are supported
	metrics_.cc_idx = 0;

	oset_apr_mutex_lock(ue->metrics_mutex);
	ue->phr_counter          = 0;
	ue->dl_cqi_valid_counter = 0;
	ue->pucch_sinr_counter   = 0;
	ue->pusch_sinr_counter   = 0;
	oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_dl_cqi(ue_nr *ue, srsran_uci_cfg_nr_t *cfg_, uint32_t dl_cqi)
{
	// Process CQI
	for (uint32_t i = 0; i < cfg_->nof_csi; i++) {
		// Skip if invalid or not supported CSI report
		if (cfg_->csi[i].cfg.quantity != SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI ||
		    cfg_->csi[i].cfg.freq_cfg != SRSRAN_CSI_REPORT_FREQ_WIDEBAND) {
			continue;
		}
		// Add statistics
		ue->ue_metrics.dl_cqi = SRSRAN_VEC_SAFE_CMA(dl_cqi, ue->ue_metrics.dl_cqi, ue->dl_cqi_valid_counter);
		oset_apr_mutex_lock(ue->metrics_mutex);
		ue->dl_cqi_valid_counter++;
		oset_apr_mutex_unlock(ue->metrics_mutex);
	}
}

void ue_nr_metrics_rx(ue_nr *ue, bool crc, uint32_t tbs)
{
	//std::lock_guard<std::mutex> lock(metrics_mutex);
	if (crc) {
		ue->ue_metrics.rx_brate += tbs * 8;
	} else {
		ue->ue_metrics.rx_errors++;
	}
	ue->ue_metrics.rx_pkts++;
}

void ue_nr_metrics_tx(ue_nr *ue, bool crc, uint32_t tbs)
{
	//std::lock_guard<std::mutex> lock(metrics_mutex);
	if (crc) {
		ue->ue_metrics.tx_brate += tbs * 8;
	} else {
		ue->ue_metrics.tx_errors++;
	}
	ue->ue_metrics.tx_pkts++;
}

void ue_nr_metrics_dl_mcs(ue_nr *ue, uint32_t mcs)
{
  //std::lock_guard<std::mutex> lock(metrics_mutex);
  ue->ue_metrics.dl_mcs = SRSRAN_VEC_CMA((float)mcs, ue->ue_metrics.dl_mcs, ue->ue_metrics.dl_mcs_samples);
  ue->ue_metrics.dl_mcs_samples++;
}

void ue_nr_metrics_ul_mcs(ue_nr *ue, uint32_t mcs)
{
  //std::lock_guard<std::mutex> lock(metrics_mutex);
  ue->ue_metrics.ul_mcs = SRSRAN_VEC_CMA((float)mcs, ue_metrics.ul_mcs, ue_metrics.ul_mcs_samples);
  ue->ue_metrics.ul_mcs_samples++;
}

void ue_nr_metrics_cnt(ue_nr *ue)
{
	oset_apr_mutex_lock(ue->metrics_mutex);
	ue->ue_metrics.nof_tti++;
	oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_pucch_sinr(ue_nr *ue, float sinr)
{
	// discard nan or inf values for average SINR
	if (!isinf(sinr) && !isnan(sinr)) {
		ue->ue_metrics.pucch_sinr = SRSRAN_VEC_SAFE_CMA((float)sinr, ue->ue_metrics.pucch_sinr, ue->pucch_sinr_counter);
		oset_apr_mutex_lock(ue->metrics_mutex);
		ue->pucch_sinr_counter++;
		oset_apr_mutex_unlock(ue->metrics_mutex);
	}
}

void ue_nr_metrics_pusch_sinr(ue_nr *ue, float sinr)
{
	// discard nan or inf values for average SINR
	if (!isinf(sinr) && !isnan(sinr)) {
		ue->ue_metrics.pusch_sinr = SRSRAN_VEC_SAFE_CMA((float)sinr, ue->ue_metrics.pusch_sinr, ue->pusch_sinr_counter);
		oset_apr_mutex_lock(ue->metrics_mutex);
		ue->pusch_sinr_counter++;
		oset_apr_mutex_unlock(ue->metrics_mutex);
	}
}

