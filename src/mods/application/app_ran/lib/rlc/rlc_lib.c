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
#define OSET_LOG2_DOMAIN   "app-gnb-librlc"


static void rlc_lib_write_pdu_suspended(rlc_common *rlc_entity, uint8_t* payload, uint32_t nof_bytes)
{
  if (rlc_entity->suspended) {//suspended
	rlc_common_queue_rx_pdu(rlc_entity, payload, nof_bytes);
  } else {
	rlc_entity->func->_write_pdu(rlc_entity, payload, nof_bytes);
  }
}


static void rlc_lib_configure(rlc_common *rlc_entity, rlc_config_t *cnfg)
{
	if(rlc_entity->mode == (rlc_mode_t)tm){
		// nothing
	}else if(rlc_entity->mode == (rlc_mode_t)am){
		//todo
	}else if(rlc_entity->mode == (rlc_mode_t)um){
		//todo
	}
}


rlc_common * rlc_valid_lcid(rlc_t *rlc, uint32_t lcid)
{
  if (lcid >= SRSRAN_N_RADIO_BEARERS) {
    oset_error("Radio bearer id must be in [0:%d] - %d", SRSRAN_N_RADIO_BEARERS, lcid);
    return NULL;
  }

  return rlc_array_find_by_lcid(rlc, lcid);
}


void rlc_reset_metrics(rlc_t *rlc)
{
	oset_hash_index_t *hi = NULL;
	for (hi = oset_hash_first(rlc->rlc_array); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
		if(it->mode == (rlc_mode_t)tm){
			rlc_tm_reset_metrics((rlc_tm *)it);
		}else if(it->mode == (rlc_mode_t)am){
			//todo
		}else if(it->mode == (rlc_mode_t)um){
			//todo
		}
	}

	/*for (hi = oset_hash_first(rlc->rlc_array_mrb); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
		if(it->mode == (rlc_mode_t)tm){
			rlc_tm_reset_metrics((rlc_tm *)it);
		}else if(it->mode == (rlc_mode_t)am){
			//todo
		}else if(it->mode == (rlc_mode_t)um){
			//todo
		}

	}*/

	rlc->metrics_tp = oset_micro_time_now();//oset_time_now()
}

void rlc_array_set_lcid(rlc_t *rlc, uint32_t lcid, rlc_common *rlc_entity)
{
    oset_assert(rlc_entity);
	oset_hash_set(rlc->rlc_array, &lcid, sizeof(lcid), NULL);
	oset_hash_set(rlc->rlc_array, &lcid, sizeof(lcid), rlc_entity);
}

rlc_common *rlc_array_find_by_lcid(rlc_t *rlc, uint32_t lcid)
{
    return (rlc_common *)oset_hash_get(rlc->rlc_array, &lcid, sizeof(lcid));
}

// Methods modifying the RLC array need to acquire the write-lock
int rlc_add_bearer(rlc_t *rlc, uint32_t lcid, rlc_config_t *cnfg)
{
  //oset_apr_thread_rwlock_wrlock(rlc->rwlock);
  //oset_apr_thread_rwlock_unlock(rlc->rwlock);
  if (NULL != rlc_valid_lcid(rlc, lcid)) {
    oset_warn("LCID %d already exists", lcid);
    return OSET_ERROR;
  }

  rlc_common  *rlc_entity = NULL;

  // 初始默认 lcid=0(ccch), tm模式
  switch (cnfg->rlc_mode) {
    case (rlc_mode_t)tm:
      rlc_entity = (rlc_common *)(rlc_tm_init(lcid, rlc->usepool));
      break;
    case (rlc_mode_t)am:
      switch (cnfg->rat) {
        case (srsran_rat_t)lte:
          rlc_entity = (rlc_common *)(new rlc_am(cnfg->rat, logger, lcid, pdcp, rrc, timers));
          break;
        case (srsran_rat_t)nr:
          rlc_entity = (rlc_common *)(new rlc_am(cnfg->rat, logger, lcid, pdcp, rrc, timers));
          break;
        default:
          oset_error("AM not supported for this RAT");
          return OSET_ERROR;
      }
      break;
    case (rlc_mode_t)um:
      switch (cnfg->rat) {
        case (srsran_rat_t)lte:
          rlc_entity = (rlc_common *)(new rlc_um_lte(logger, lcid, pdcp, rrc, timers));
          break;
        case (srsran_rat_t)nr:
          rlc_entity = (rlc_common *)(new rlc_um_nr(logger, lcid, pdcp, rrc, timers));
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
    if (!rlc_entity->func->_configure(rlc_entity, cnfg)) {
      oset_error("Error configuring RLC entity.");
      return OSET_ERROR;
    }
  }

  rlc_entity->func->_set_bsr_callback(rlc_entity, rlc->bsr_callback);

  rlc_array_set_lcid(rlc, lcid, rlc_entity);


  if (NULL == rlc_array_find_by_lcid(rlc, lcid)) {
    oset_error("Error inserting RLC entity in to array.");
    return OSET_ERROR;
  }

  oset_info("Added %s radio bearer with LCID %d in %s", rat_to_string(cnfg->rat), lcid, rlc_mode_to_string(cnfg->rlc_mode, true));

  return OSET_ERROR;
}

static void rlc_lib_init2(rlc_t *rlc, uint32_t lcid_)
{
	rlc->default_lcid = lcid_;

	rlc_reset_metrics(rlc);

	rlc_config_t default_rlc_cfg = default_rlc_config();

	// create default RLC_TM bearer for SRB0
	rlc_add_bearer(rlc, rlc->default_lcid, &default_rlc_cfg);
}


void rlc_lib_init(rlc_t *rlc, uint32_t lcid_, bsr_callback_t bsr_callback_)
{
	//oset_apr_thread_rwlock_create(&rlc->rwlock, rlc->usepool);
	oset_pool_init(&rlc->pool, static_pool_size);
	rlc->rlc_array = oset_hash_make();
	rlc->rlc_array_mrb = oset_hash_make();

	rlc->bsr_callback = bsr_callback_;
	rlc_lib_init2(rlc, lcid_);
}

void rlc_lib_stop(rlc_t *rlc)
{
	oset_hash_index_t *hi = NULL;
	for (hi = oset_hash_first(rlc->rlc_array); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
		if(it->mode == (rlc_mode_t)tm){
			rlc_tm_stop((rlc_tm *)it);
		}else if(it->mode == (rlc_mode_t)am){
			//todo
		}else if(it->mode == (rlc_mode_t)um){
			//todo
		}
	}

	/*for (hi = oset_hash_first(rlc->rlc_array_mrb); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
		if(it->mode == (rlc_mode_t)tm){
			rlc_tm_stop((rlc_tm *)it);
		}else if(it->mode == (rlc_mode_t)am){
			//todo
		}else if(it->mode == (rlc_mode_t)um){
			//todo
		}
	}*/

	rlc->bsr_callback = NULL;
	oset_hash_destroy(rlc->rlc_array);
	oset_hash_destroy(rlc->rlc_array_mrb);
    oset_pool_final(&rlc->pool);
	//oset_apr_thread_rwlock_destroy(rlc->rwlock);
}


// Write PDU methods are called from Stack thread context, no need to acquire the lock
void rlc_lib_write_pdu(rlc_t *rlc, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes)
{
	rlc_common	*rlc_entity = rlc_valid_lcid(rlc, lcid);

	if (NULL != rlc_entity) {
		rlc_lib_write_pdu_suspended(rlc_entity, payload, nof_bytes);
		rlc_lib_update_bsr(lcid);
	} else {
		oset_warn("LCID %d doesn't exist. Dropping PDU", lcid);
	}
}


