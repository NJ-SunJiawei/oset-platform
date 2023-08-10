/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "lib/pdcp/pdcp_lib.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-libpdcp"

pdcp_entity_base* pdcp_valid_lcid(pdcp_lib_t *pdcp, uint32_t lcid)
{
	if (lcid >= SRSRAN_N_RADIO_BEARERS) {
		oset_error("Radio bearer id must be in [0:%d] - %d", SRSRAN_N_RADIO_BEARERS, lcid);
		return NULL;
	}

	return pdcp_array_find_by_lcid(pdcp, lcid);
}

//void pdcp_array_set_lcid(pdcp_lib_t *pdcp, uint32_t lcid, pdcp_entity_base *pdcp_base_entity)
//{
//    oset_assert(pdcp_entity_base);
//	oset_hash_set(pdcp->pdcp_array, &lcid, sizeof(lcid), NULL);
//	oset_hash_set(pdcp->pdcp_array, &lcid, sizeof(lcid), pdcp_entity_base);
//}

pdcp_entity_base *pdcp_array_find_by_lcid(pdcp_lib_t *pdcp, uint32_t lcid)
{
    return (pdcp_entity_base *)oset_hash_get(pdcp->pdcp_array, &lcid, sizeof(lcid));
}


void pdcp_lib_init(pdcp_lib_t *pdcp)
{
	pdcp->pdcp_array = oset_hash_make();
	//pdcp->pdcp_array_mrb = oset_hash_make();
	oset_apr_mutex_init(&pdcp->cache_mutex, pdcp->usepool);
	oset_stl_array_init(pdcp->valid_lcids_cached);
	pdcp->metrics_tp = 0;
}


void pdcp_lib_stop(pdcp_lib_t *pdcp)
{
	oset_hash_destroy(pdcp->pdcp_array);
	//oset_hash_destroy(pdcp->pdcp_array_mrb);
	oset_apr_mutex_destroy(pdcp->cache_mutex);
	oset_stl_array_term(pdcp->valid_lcids_cached);
}



int pdcp_lib_add_bearer(pdcp_lib_t *pdcp, uint32_t lcid, pdcp_config_t *cfg)
{
  if (valid_lcid(lcid)) {
    return pdcp_array[lcid]->configure(cfg) ? SRSRAN_SUCCESS : SRSRAN_ERROR;
  }

  std::unique_ptr<pdcp_entity_base> entity;

  // For now we create an pdcp entity lte for nr due to it's maturity
  if (cfg.rat == srsran::srsran_rat_t::lte) {
    entity.reset(new pdcp_entity_lte{rlc, rrc, gw, task_sched, logger, lcid});
  } else if (cfg.rat == srsran::srsran_rat_t::nr) {
    entity.reset(new pdcp_entity_nr{rlc, rrc, gw, task_sched, logger, lcid});
  }

  if (not entity->configure(cfg)) {
    logger.error("Can not configure PDCP entity");
    return SRSRAN_ERROR;
  }

  if (not pdcp_array.insert(std::make_pair(lcid, std::move(entity))).second) {
    logger.error("Error inserting PDCP entity in to array.");
    return SRSRAN_ERROR;
  }

  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    valid_lcids_cached.insert(lcid);
  }

  logger.info("Add %s%d (lcid=%d, sn_len=%dbits)",
              cfg.rb_type == PDCP_RB_IS_DRB ? "DRB" : "SRB",
              cfg.bearer_id,
              lcid,
              cfg.sn_len);

  return SRSRAN_SUCCESS;
}

