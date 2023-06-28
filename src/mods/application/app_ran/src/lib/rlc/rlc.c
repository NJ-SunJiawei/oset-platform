/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "lib/rlc/rlc.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librlc"

rlc_common *rlc_common_find_by_lcid(rlc_t *rlc, uint16_t lcid)
{
    return (rlc_common *)oset_hash_get(rlc->rlc_array, &lcid, sizeof(lcid));
}


bool rlc_valid_lcid(rlc_t *rlc, uint32_t lcid)
{
  if (lcid >= SRSRAN_N_RADIO_BEARERS) {
    oset_error("Radio bearer id must be in [0:%d] - %d", SRSRAN_N_RADIO_BEARERS, lcid);
    return false;
  }

  if (NULL == rlc_common_find_by_lcid(rlc, lcid)) {
    return false;
  }

  return true;
}


void rlc_reset_metrics(rlc_t *rlc)
{
	oset_hash_index_t *hi = NULL;
	for (hi = oset_hash_first(rlc->rlc_array); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
	}

	for (hi = oset_hash_first(rlc->rlc_array_mrb); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
	}

	for (rlc_map_t::iterator it = rlc_array.begin(); it != rlc_array.end(); ++it) {
	it->second->reset_metrics();//void rlc_tm::reset_metrics() //am//um
	}

	for (rlc_map_t::iterator it = rlc_array_mrb.begin(); it != rlc_array_mrb.end(); ++it) {
	it->second->reset_metrics();
	}

	rlc->metrics_tp = oset_micro_time_now();//oset_time_now()
}

// Methods modifying the RLC array need to acquire the write-lock
// 修改RLC数组的方法需要获取写锁
int rlc_add_bearer(rlc_t *rlc, uint32_t lcid, rlc_config_t *cnfg)
{
  //oset_apr_thread_rwlock_wrlock(rlc->rwlock);
  //oset_apr_thread_rwlock_unlock(rlc->rwlock);

  if (rlc_valid_lcid(lcid)) {
    oset_warn("LCID %d already exists", lcid);
    return OSET_ERROR;
  }

  rlc_common  *rlc_entity = NULL;

  // 初始默认 lcid=0(ccch), tm模式
  switch (cnfg->rlc_mode) {
    case (rlc_mode_t)tm:
      rlc_entity = std::unique_ptr<rlc_common>(new rlc_tm(logger, lcid, pdcp, rrc));
      break;
    case (rlc_mode_t)am:
      switch (cnfg->rat) {
        case (srsran_rat_t)lte:
          rlc_entity = std::unique_ptr<rlc_common>(new rlc_am(cnfg.rat, logger, lcid, pdcp, rrc, timers));
          break;
        case (srsran_rat_t)nr:
          rlc_entity = std::unique_ptr<rlc_common>(new rlc_am(cnfg.rat, logger, lcid, pdcp, rrc, timers));
          break;
        default:
          oset_error("AM not supported for this RAT");
          return OSET_ERROR;
      }
      break;
    case (rlc_mode_t)um:
      switch (cnfg.rat) {
        case (srsran_rat_t)lte:
          rlc_entity = std::unique_ptr<rlc_common>(new rlc_um_lte(logger, lcid, pdcp, rrc, timers));
          break;
        case (srsran_rat_t)nr:
          rlc_entity = std::unique_ptr<rlc_common>(new rlc_um_nr(logger, lcid, pdcp, rrc, timers));
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
  if (cnfg.rlc_mode != (rlc_mode_t)tm) {
  	//bool rlc_um_nr::configure(const rlc_config_t& cnfg_)
  	//bool rlc_am_nr_tx::configure(const rlc_config_t& cfg_)
    if (not rlc_entity->configure(cnfg)) {
      oset_error("Error configuring RLC entity.");
      return OSET_ERROR;
    }
  }

  rlc_entity->set_bsr_callback(bsr_callback);
  //void rlc_am::set_bsr_callback(bsr_callback_t callback)
  //void rlc_um_base::set_bsr_callback(bsr_callback_t callback)

  if (!rlc->rlc_array.insert(rlc_map_pair_t(lcid, std::move(rlc_entity))).second) {
    oset_error("Error inserting RLC entity in to array.");
    return OSET_ERROR;
  }

  oset_info("Added %s radio bearer with LCID %d in %s", rat_to_string(cnfg->rat), lcid, rlc_mode_to_string(cnfg->rlc_mode, true));

  return OSET_ERROR;
}

static void rlc_lib_init(rlc_t *rlc, uint32_t lcid_)
{
	rlc->default_lcid = lcid_;

	rlc_reset_metrics(rlc);

	rlc_config_t default_rlc_cfg = default_rlc_config();

	// create default RLC_TM bearer for SRB0
	rlc_add_bearer(rlc, rlc->default_lcid, &default_rlc_cfg);
}


void rlc_init(rlc_t *rlc, uint32_t lcid_, bsr_callback_t bsr_callback_)
{
	oset_apr_thread_rwlock_create(&rlc->rwlock, rlc->usepool);
	rlc->bsr_callback = bsr_callback_;
	rlc_lib_init(rlc, lcid_);
}

void rlc_stop(rlc_t *rlc)
{
	oset_hash_index_t *hi = NULL;
	for (hi = oset_hash_first(rlc->rlc_array); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
	}

	for (hi = oset_hash_first(rlc->rlc_array_mrb); hi; hi = oset_hash_next(hi)) {
		uint16_t lcid = *(uint16_t *)oset_hash_this_key(hi);
		rlc_common  *it = oset_hash_this_val(hi);
	}

	for (rlc_map_t::iterator it = rlc_array.begin(); it != rlc_array.end(); ++it) {
	it->second->stop();
	}
	for (rlc_map_t::iterator it = rlc_array_mrb.begin(); it != rlc_array_mrb.end(); ++it) {
	it->second->stop();
	}
}

