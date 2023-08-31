/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "lib/rlc/rlc_lib.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlclib"

static int rlc_lcid_compare(const void *a, const void *b)
{
	const int *x = a;
	const int *y = b;
	//升序
	return *x - *y;
}

static void rlc_lib_valid_lcids_insert(rlc_lib_t *rlc, uint32_t lcid)
{
	uint32_t *val = NULL;
	cvector_for_each_in(val, rlc->valid_lcids_cached){
		if(lcid == *val) return;
	}
	cvector_push_back(rlc->valid_lcids_cached, lcid);
	//qsort(rlc->valid_lcids_cached, cvector_size(rlc->valid_lcids_cached), sizeof(uint32_t), rlc_lcid_compare);
}

static void rlc_lib_valid_lcids_delete(rlc_lib_t *rlc, uint32_t lcid)
{
	for (int i = 0; i < cvector_size(rlc->valid_lcids_cached); i++){
		if(rlc->valid_lcids_cached[i] = lcid){
			cvector_erase(rlc->valid_lcids_cached, i);
		}
	}
}

static void rlc_lib_write_ul_pdu_suspended(rlc_common *rlc_entity, uint8_t* payload, uint32_t nof_bytes)
{
  if (rlc_entity->suspended) {//suspended
	rlc_common_queue_rx_pdu(rlc_entity, payload, nof_bytes);
  } else {
	rlc_entity->func._write_ul_pdu(rlc_entity, payload, nof_bytes);
  }
}

rlc_common *rlc_array_find_by_lcid(rlc_lib_t *rlc, uint32_t lcid)
{
    return (rlc_common *)oset_hash_get(rlc->rlc_array, &lcid, sizeof(lcid));
}

rlc_common * rlc_array_valid_lcid(rlc_lib_t *rlc, uint32_t lcid)
{
  // ???SRSRAN_N_RADIO_BEARERS
  if (lcid >= SRSRAN_N_RADIO_BEARERS) {
    oset_error("Radio bearer id must be in [0:%d] - %d", SRSRAN_N_RADIO_BEARERS, lcid);
    return NULL;
  }

  return rlc_array_find_by_lcid(rlc, lcid);
}

static void get_buffer_state(rlc_lib_t *rlc, uint32_t lcid, uint32_t *tx_queue, uint32_t *prio_tx_queue)
{
	//oset_apr_thread_rwlock_rdlock(rlc->rwlock);
	rlc_common  *rlc_entity = rlc_array_valid_lcid(rlc, lcid);
	if (rlc_entity) {
		if (is_suspended(rlc_entity)) {
			*tx_queue      = 0;
			*prio_tx_queue = 0;
		} else {
			rlc_entity->func._get_buffer_state(rlc_entity, tx_queue, prio_tx_queue);
		}
	}
	//oset_apr_thread_rwlock_unlock(rlc->rwlock);
}


static void rlc_lib_update_bsr(rlc_lib_t *rlc, uint32_t lcid)
{
	if (rlc->bsr_callback) {
		uint32_t tx_queue = 0, prio_tx_queue = 0;
		get_buffer_state(lcid, &tx_queue, &prio_tx_queue);
	}
}


void rlc_lib_reset_metrics(rlc_lib_t *rlc)
{
	oset_hash_index_t *hi = NULL;
	for (hi = oset_hash_first(rlc->rlc_array); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
	    it->func._reset_metrics(it);
	}

	rlc->metrics_tp = oset_epoch_time_now();//oset_time_now();
}

void rlc_lib_get_metrics(rlc_lib_t *rlc, rlc_ue_metrics_t *ue_m, uint32_t nof_tti)
{
	time_t secs = oset_epoch_time_now() - rlc->metrics_tp;

	oset_hash_index_t *hi = NULL;
	for (hi = oset_hash_first(rlc->rlc_array); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
		rlc_bearer_metrics_t metrics = it->func._get_metrics(it);

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

	rlc_lib_reset_metrics(rlc);
}


// Methods modifying the RLC array need to acquire the write-lock
int rlc_lib_add_bearer(rlc_lib_t *rlc, uint32_t lcid, rlc_config_t *cnfg)
{
  //oset_apr_thread_rwlock_rdlock(rlc->rwlock);
  //oset_apr_thread_rwlock_unlock(rlc->rwlock);

  if (NULL != rlc_array_valid_lcid(rlc, lcid)) {
	oset_warn("LCID %d already exists", lcid);
	return OSET_ERROR;
  }

  rlc_common  *rlc_entity = NULL;

  // 初始默认 lcid=0(ccch), tm模式
  switch (cnfg->rlc_mode) {
    case (rlc_mode_t)tm:
      // 在 eNodeB 或 UE 侧，一个 TM 实体只能接收或发送数据，而不能同时收发数据，即 TM 实体只提供单向的数据传输服务
      rlc_entity = (rlc_common *)(rlc_tm_init(lcid, rlc->rnti, rlc->usepool));
      break;
    case (rlc_mode_t)am:
      switch (cnfg->rat) {
        case (srsran_rat_t)nr:
          // 一个 UM 实体只能接收或发送数据，而不能同时收发数据。UM 实体只提供单向的数据传输服务
          rlc_entity = (rlc_common *)(new rlc_am(cnfg->rat, logger, lcid, pdcp, rrc, timers));
          break;
        default:
          oset_error("AM not supported for this RAT");
          return OSET_ERROR;
      }
      break;
    case (rlc_mode_t)um:
      switch (cnfg->rat) {
        case (srsran_rat_t)nr:
          rlc_entity = (rlc_common *)(new rlc_um_nr_init(lcid, rlc->rnti));
          break;
        default:
          oset_error("UM not supported for this RAT");
          return OSET_ERROR;
      }
      break;
    default:
      oset_error("Cannot add RLC entity - invalid mode");
      return OSET_ERROR;
  }

  // make sure entity has been created
  if (rlc_entity == NULL) {
    oset_error("Couldn't allocate new RLC entity");
    return OSET_ERROR;
  }

  // configure entity
  if (cnfg->rlc_mode != (rlc_mode_t)tm) {
    if (!rlc_entity->func._configure(rlc_entity, cnfg)) {
      oset_error("Error configuring RLC entity.");
      return OSET_ERROR;
    }
  }

  rlc_entity->func._set_bsr_callback(rlc_entity, rlc->bsr_callback);

  oset_hash_set(rlc->rlc_array, &rlc_entity->lcid, sizeof(rlc_entity->lcid), NULL);
  oset_hash_set(rlc->rlc_array, &rlc_entity->lcid, sizeof(rlc_entity->lcid), rlc_entity);

  if (NULL == rlc_array_find_by_lcid(rlc, lcid)) {
    oset_error("Error inserting RLC entity in to array.");
    return OSET_ERROR;
  }

  rlc_lib_valid_lcids_insert(rlc, lcid);

  oset_info("Added %s radio bearer with LCID %d in %s", rat_to_string(cnfg->rat), lcid, rlc_mode_to_string(cnfg->rlc_mode, true));

  return OSET_ERROR;
}

void rlc_lib_del_bearer(rlc_lib_t *rlc, uint32_t lcid)
{
	rlc_lib_valid_lcids_delete(rlc, lcid);

	//oset_apr_thread_rwlock_rdlock(rlc->rwlock);
	if (rlc_array_valid_lcid(rlc, lcid)) {
		rlc_common *it = rlc_array_find_by_lcid(rlc, lcid);
		oset_hash_set(rlc->rlc_array, &it->lcid, sizeof(it->lcid), NULL);
		it->func._stop(it);
		oset_info("Deleted RLC bearer with LCID %d", lcid);
	} else {
		oset_error("Can't delete bearer with LCID %d. Bearer doesn't exist.", lcid);
	}
	//oset_apr_thread_rwlock_unlock(rlc->rwlock);
}


static void rlc_lib_init2(rlc_lib_t *rlc, uint32_t lcid_)
{
	rlc->default_lcid = lcid_;

	rlc_lib_reset_metrics(rlc);

	rlc_config_t default_rlc_cfg = default_rlc_config();

	//oset_apr_thread_rwlock_wrlock(rlc->rwlock);
	// create default RLC_TM bearer for SRB0
	rlc_lib_add_bearer(rlc, rlc->default_lcid, &default_rlc_cfg);
	//oset_apr_thread_rwlock_unlock(rlc->rwlock);

}

// BCCH、PCCH 和CCCH 只采用 TM 模式,DCCH 只可采用 AM 模式,而 DTCH 既可以采用 UM模式又可以采用 AM 模式,具体由高层的 RRC 配置
void rlc_lib_init(rlc_lib_t *rlc, uint32_t lcid_, bsr_callback_t bsr_callback_)
{
	//oset_apr_thread_rwlock_create(&rlc->rwlock, rlc->usepool);
	//oset_pool_init(&rlc->pool, static_pool_size);
	rlc->rlc_array = oset_hash_make();

	rlc->bsr_callback = bsr_callback_;
	rlc->metrics_tp = 0;
	rlc_lib_init2(rlc, lcid_);
}

void rlc_lib_stop(rlc_lib_t *rlc)
{
	//oset_apr_thread_rwlock_wrlock(rlc->rwlock);
	uint32_t *val = NULL;
	cvector_for_each_in(val, rlc->valid_lcids_cached){
		rlc_lib_del_bearer(rlc, *val);
	}
	cvector_free(rlc->valid_lcids_cached);

	rlc->bsr_callback = NULL;
	oset_hash_destroy(rlc->rlc_array);
    //oset_pool_final(&rlc->pool);
	//oset_apr_thread_rwlock_unlock(rlc->rwlock);
	//oset_apr_thread_rwlock_destroy(rlc->rwlock);
}


// Write PDU methods are called from Stack thread context, no need to acquire the lock
void rlc_lib_write_ul_pdu(rlc_lib_t *rlc, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes)
{
	rlc_common	*rlc_entity = rlc_array_valid_lcid(rlc, lcid);

	if (NULL != rlc_entity) {
		rlc_lib_write_ul_pdu_suspended(rlc_entity, payload, nof_bytes);
		rlc_lib_update_bsr(rlc, lcid);
	} else {
		oset_warn("LCID %d doesn't exist. Dropping PDU", lcid);
	}
}

uint32_t rlc_lib_read_dl_pdu(rlc_lib_t *rlc, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes)
{
  uint32_t ret = 0;

  //oset_apr_thread_rwlock_rdlock(rlc->rwlock);
  rlc_common  *rlc_entity = rlc_array_valid_lcid(rlc, lcid);
  //oset_apr_thread_rwlock_unlock(rlc->rwlock);

  if (NULL != rlc_entity) {
	ret = rlc_entity->func._read_dl_pdu(rlc_entity, payload, nof_bytes);
	rlc_lib_update_bsr(rlc, lcid);
  } else {
    oset_warn("LCID %d doesn't exist", lcid);
  }

  ASSERT_IF_NOT(ret <= nof_bytes, "Created too big RLC PDU (%d > %d)", ret, nof_bytes);

  return ret;
}

/*******************************************************************************
  PDCP/rrc interface (called from Stack thread and therefore no lock required)
*******************************************************************************/
void rlc_lib_write_dl_sdu(rlc_lib_t *rlc, uint32_t lcid, byte_buffer_t *sdu)
{
  // TODO: rework build PDU logic to allow large SDUs (without concatenation)
  if (sdu->N_bytes > RLC_MAX_SDU_SIZE) {
    oset_warn("Dropping too long SDU of size %d B (Max. size %d B).", sdu->N_bytes, RLC_MAX_SDU_SIZE);
    return;
  }
  rlc_common  *rlc_entity = rlc_array_valid_lcid(rlc, lcid);
  
  if (NULL != rlc_entity) {
  	rlc_entity->func._write_dl_sdu(rlc_entity, sdu);
    rlc_lib_update_bsr(rlc, lcid);
  } else {
    oset_warn("RLC LCID %d doesn't exist. Deallocating SDU", lcid);
  }
}

bool rlc_lib_rb_is_um(rlc_lib_t *rlc, uint32_t lcid)
{
  bool ret = false;
  rlc_common *rlc_entity = rlc_array_valid_lcid(rlc, lcid);

  if (NULL != rlc_entity) {
	ret = (rlc_entity->func._get_mode() == (rlc_mode_t)um);
  } else {
	oset_warn("LCID %d doesn't exist.", lcid);
  }
  return ret;
}

bool rlc_lib_sdu_queue_is_full(rlc_lib_t *rlc, uint32_t lcid)
{
  rlc_common *rlc_entity = rlc_array_valid_lcid(rlc, lcid);

  if (NULL != rlc_entity) {
    return rlc_entity->func._sdu_queue_is_full();
  }
  oset_warn("RLC LCID %d doesn't exist. Ignoring queue check", lcid);
  return false;
}

void rlc_lib_discard_sdu(rlc_lib_t *rlc, uint32_t lcid, uint32_t discard_sn)
{
  rlc_common *rlc_entity = rlc_array_valid_lcid(rlc, lcid);

  if (NULL != rlc_entity) {
    rlc_entity->func._discard_sdu(rlc_entity, discard_sn);
    rlc_lib_update_bsr(rlc, lcid);
  } else {
    oset_warn("RLC LCID %d doesn't exist. Ignoring discard SDU", lcid);
  }
}

