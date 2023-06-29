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
#include "rlc/rlc.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-ue_nr"

ue_nr *ue_nr_add(uint16_t rnti)
{
    ue_nr *ue = NULL;
	oset_apr_memory_pool_t *usepool = NULL;
	oset_core_new_memory_pool(&usepool);

	ue = oset_core_alloc(usepool, sizeof(*ue));
	ASSERT_IF_NOT(ue, "Could not allocate sched ue %d context from pool", rnti);
    memset(ue, 0, sizeof(ue_nr));

	ue->usepool = usepool;
	ue->ue_rlc_buffer = byte_buffer_init();
	//ue->last_msg3 = byte_buffer_init();

	ue_nr_set_rnti(rnti, ue);
    oset_list_add(&mac_manager_self()->mac_ue_list, ue);
	oset_apr_mutex_init(&ue->metrics_mutex, OSET_MUTEX_NESTED, usepool);

    oset_info("[Added] Number of MAC-UEs is now %d", oset_list_count(&mac_manager_self()->mac_ue_list));

	return ue;
}

void ue_nr_remove(ue_nr *ue)
{
    oset_assert(ue);

    oset_free(ue->ue_rlc_buffer);
    //oset_free(ue->last_msg3);
	cvector_free(ue->mac_pdu_dl.subpdus);
	cvector_free(ue->mac_pdu_ul.subpdus);

    oset_list_remove(&mac_manager_self()->mac_ue_list, ue);
    oset_hash_set(mac_manager_self()->ue_db, &ue->rnti, sizeof(ue->rnti), NULL);
	oset_apr_mutex_destroy(ue->metrics_mutex);
	oset_core_destroy_memory_pool(&ue->usepool);

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

int ue_nr_generate_pdu(ue_nr *ue, byte_buffer_t *pdu, uint32_t tbs_len, cvector_vector_t(uint32_t) subpdu_lcids)
{
  //std::lock_guard<std::mutex> lock(mutex);
  if (mac_sch_pdu_nr_init_tx(&ue->mac_pdu_dl, pdu, tbs_len, false) != OSET_OK) {
    oset_error("Couldn't initialize MAC PDU buffer");
    return OSET_ERROR;
  }

  bool drb_activity = false; // inform RRC about user activity if true

  int32_t remaining_len = ue->mac_pdu_dl.remaining_len;

  oset_debug("0x%x Generating MAC PDU (%d B)", ue->rnti, remaining_len);

  // First, add CEs as indicated by scheduler
  uint32_t *lcid = NULL;
  cvector_for_each_in(lcid, subpdu_lcids){
    oset_debug("adding lcid=%d", *lcid);
	// 根据msg3组装 msg4中的Contention Resolution Identity MAC CE
    if (*lcid == (mac_sch_subpdu_nr)CON_RES_ID) {
      if (ue->last_msg3 != NULL) {//get from mac ul
        ue_con_res_id_t id = {0};
        memcpy(id, ue->last_msg3->msg, sizeof(id));
        if (mac_sch_pdu_nr_add_ue_con_res_id_ce(&ue->mac_pdu_dl, id) != OSET_OK) {
          oset_error("0x%x Failed to add ConRes CE", ue->rnti);
        }
        ue->last_msg3 = NULL; // don't use this Msg3 again
      } else {
        oset_warn("0x%x Can't add ConRes CE. No Msg3 stored", ue->rnti);
      }
    } else {
      // add SDUs for given LCID
      while (remaining_len >= MIN_RLC_PDU_LEN) {
        // clear read buffer
        byte_buffer_clear(ue->ue_rlc_buffer);

        // Determine space for RLC
        // 2^8为256 <8bit表示长度最大值为256> ,去掉mac subheader头
        remaining_len -= (remaining_len >= MAC_SUBHEADER_LEN_THRESHOLD ? 3 : 2);

        // read RLC PDU API
        int pdu_len = API_rlc_mac_read_pdu(ue->rnti, *lcid, ue->ue_rlc_buffer->msg, remaining_len);

        if (pdu_len > remaining_len) {
          oset_error("Can't add SDU of %d B. Available space %d B", pdu_len, remaining_len);
          break;
        } else {
          // Add SDU if RLC has something to tx
          if (pdu_len > 0) {
            ue->ue_rlc_buffer->N_bytes = pdu_len;
            oset_debug("Read %d B from RLC", ue->ue_rlc_buffer->N_bytes);

            // add to MAC PDU and pack
            if (mac_sch_pdu_nr_add_sdu(&ue->mac_pdu_dl, *lcid, ue->ue_rlc_buffer->msg, ue->ue_rlc_buffer->N_bytes) != OSET_OK) {
              oset_error("Error packing MAC PDU");
              break;
            }

            // set DRB activity flag but only notify RRC once
            if (*lcid > 3) {//lcid>3（srb0 srb1 srb2）的逻辑信道为drb类型信道
              drb_activity = true;
            }
          } else {
            break;
          }

          remaining_len -= pdu_len;
          oset_debug("%d B remaining PDU", remaining_len);
        }
      }
    }
  }

  mac_sch_pdu_nr_pack(&ue->mac_pdu_dl);

  if (drb_activity) {
    // Indicate DRB activity in DL to RRC
    API_rrc_mac_set_activity_user(ue->rnti);
    oset_debug("DL activity rnti=0x%x", ue->rnti);
  }

  mac_sch_pdu_nr_to_string(&ue->mac_pdu_dl, ue->rnti);

  return OSET_OK;
}


/******* METRICS interface ***************/
void ue_nr_metrics_read(ue_nr *ue, mac_ue_metrics_t* metrics_)
{
	uint32_t ul_buffer = 0;
	uint32_t dl_buffer = 0;

	sched_nr_ue *sched_ue = sched_nr_ue_find_by_rnti(ue->rnti);
	if(sched_ue != NULL){
		dl_buffer = sched_ue->common_ctxt.pending_dl_bytes;//get_dl_tx_total(&sched_ue->buffers)
		ul_buffer = sched_ue->common_ctxt.pending_ul_bytes;//get_ul_bsr_total(&sched_ue->buffers)
	}

	metrics_->rnti      = ue->rnti;
	metrics_->ul_buffer = ul_buffer;
	metrics_->dl_buffer = dl_buffer;

	// set PCell sector id
	// TODO: use ue_cfg when multiple NR carriers are supported
	metrics_.cc_idx = 0;

	//oset_apr_mutex_lock(ue->metrics_mutex);
	ue->phr_counter          = 0;
	ue->dl_cqi_valid_counter = 0;
	ue->pucch_sinr_counter   = 0;
	ue->pusch_sinr_counter   = 0;
	//oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_dl_cqi(ue_nr *ue, srsran_uci_cfg_nr_t *cfg_, uint32_t dl_cqi)
{
	//oset_apr_mutex_lock(ue->metrics_mutex);
	// Process CQI
	for (uint32_t i = 0; i < cfg_->nof_csi; i++) {
		// Skip if invalid or not supported CSI report
		if (cfg_->csi[i].cfg.quantity != SRSRAN_CSI_REPORT_QUANTITY_CRI_RI_PMI_CQI ||
		    cfg_->csi[i].cfg.freq_cfg != SRSRAN_CSI_REPORT_FREQ_WIDEBAND) {
			continue;
		}
		// Add statistics
		ue->ue_metrics.dl_cqi = SRSRAN_VEC_SAFE_CMA(dl_cqi, ue->ue_metrics.dl_cqi, ue->dl_cqi_valid_counter);
		ue->dl_cqi_valid_counter++;
	}
	//oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_rx(ue_nr *ue, bool crc, uint32_t tbs)
{
	//oset_apr_mutex_lock(ue->metrics_mutex);
	if (crc) {
		ue->ue_metrics.rx_brate += tbs * 8;
	} else {
		ue->ue_metrics.rx_errors++;
	}
	ue->ue_metrics.rx_pkts++;
	//oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_tx(ue_nr *ue, bool crc, uint32_t tbs)
{
	//oset_apr_mutex_lock(ue->metrics_mutex);
	if (crc) {
		ue->ue_metrics.tx_brate += tbs * 8;
	} else {
		ue->ue_metrics.tx_errors++;
	}
	ue->ue_metrics.tx_pkts++;
	//oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_dl_mcs(ue_nr *ue, uint32_t mcs)
{
  //oset_apr_mutex_lock(ue->metrics_mutex);
  ue->ue_metrics.dl_mcs = SRSRAN_VEC_CMA((float)mcs, ue->ue_metrics.dl_mcs, ue->ue_metrics.dl_mcs_samples);
  ue->ue_metrics.dl_mcs_samples++;
  //oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_ul_mcs(ue_nr *ue, uint32_t mcs)
{
  //std::lock_guard<std::mutex> lock(metrics_mutex);
  //oset_apr_mutex_lock(ue->metrics_mutex);
  ue->ue_metrics.ul_mcs = SRSRAN_VEC_CMA((float)mcs, ue->ue_metrics.ul_mcs, ue->ue_metrics.ul_mcs_samples);
  ue->ue_metrics.ul_mcs_samples++;
  //oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_cnt(ue_nr *ue)
{
	//oset_apr_mutex_lock(ue->metrics_mutex);
	ue->ue_metrics.nof_tti++;
	//oset_apr_mutex_unlock(ue->metrics_mutex);
}

void ue_nr_metrics_pucch_sinr(ue_nr *ue, float sinr)
{
	// discard nan or inf values for average SINR
	if (!isinf(sinr) && !isnan(sinr)) {
		//oset_apr_mutex_lock(ue->metrics_mutex);
		ue->ue_metrics.pucch_sinr = SRSRAN_VEC_SAFE_CMA((float)sinr, ue->ue_metrics.pucch_sinr, ue->pucch_sinr_counter);
		ue->pucch_sinr_counter++;
		//oset_apr_mutex_unlock(ue->metrics_mutex);
	}
}

void ue_nr_metrics_pusch_sinr(ue_nr *ue, float sinr)
{
	// discard nan or inf values for average SINR
	if (!isinf(sinr) && !isnan(sinr)) {
		//oset_apr_mutex_lock(ue->metrics_mutex);
		ue->ue_metrics.pusch_sinr = SRSRAN_VEC_SAFE_CMA((float)sinr, ue->ue_metrics.pusch_sinr, ue->pusch_sinr_counter);
		ue->pusch_sinr_counter++;
		//oset_apr_mutex_unlock(ue->metrics_mutex);
	}
}

