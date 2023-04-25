/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_ue.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-ue"

//////////////////////////////////////////pdu_builder////////////////////////////////////////////

static void pdu_builder_init(pdu_builder        *pdu_builders, uint32_t cc_, ue_buffer_manager *parent_)
{
	pdu_builders->cc = cc_;
	pdu_builders->parent = parent_;
}


//////////////////////////////////////////ue_carrier////////////////////////////////////////////
static void ue_carrier_destory(ue_carrier *carrier)
{
	cvector_free(carrier->bwp_cfg.cce_positions_list);
	harq_entity_destory(&carrier->harq_ent);
	oset_free(carrier);
}


static ue_carrier* ue_carrier_init(sched_nr_ue       *u, uint32_t cc)
{
	osset_assert(u);
	ue_cfg_manager		*uecfg_ = &u->ue_cfg;
	cell_config_manager *cell_params_ = &u->sched_cfg->cells[cc];

	ue_carrier *carrier = oset_malloc(sizeof(ue_carrier));
	osset_assert(carrier);
	carrier->rnti = u->rnti;
	carrier->cc = cc;
	ue_carrier_params_init(&carrier->bwp_cfg, u->rnti, &cell_params_->bwps[0], uecfg_);
	carrier->cell_params = &u->sched_cfg->cells[cc];
	pdu_builder_init(&carrier->pdu_builders, cc, &u->buffers);
	carrier->common_ctxt = &u->common_ctxt;
	harq_entity_init(&carrier->harq_ent, u->rnti, &u->sched_cfg->cells[cc].carrier.nof_prb, SCHED_NR_MAX_HARQ);
	return carrier;
}


//////////////////////////////////////////sched_nr_ue////////////////////////////////////////////
static void free_rach_ue_cfg_helper(sched_nr_ue_cfg_t *uecfg)
{
	cvector_free(uecfg->carriers);
	cvector_free(uecfg->lc_ch_to_add);
	cvector_free(uecfg->lc_ch_to_rem);
	oset_free(uecfg);
}

static sched_nr_ue_cfg_t* get_rach_ue_cfg_helper(uint32_t cc, sched_params_t *sched_params)
{
	sched_nr_ue_cfg_t *uecfg = oset_malloc(sizeof(sched_nr_ue_cfg_t));
	uecfg->maxharq_tx = 4;
	//cvector_reserve(uecfg->carriers, 1);
	sched_nr_ue_cc_cfg_t carrier = {0};
	carrier.active = true;
	carrier.cc     = cc;
	cvector_push_back(uecfg->carriers, carrier)
	uecfg->phy_cfg = sched_params->cells[cc].default_ue_phy_cfg;
	return uecfg;
}

/////////////////////////////sched_nr_ue//////////////////////////////////////////////////////////////
static void sched_nr_ue_set_cfg(sched_nr_ue *u, sched_nr_ue_cfg_t *cfg)
{
	ue_cfg_manager_apply_config_request(&u->ue_cfg, cfg);

	sched_nr_ue_cc_cfg_t *ue_cc_cfg = NULL;
	cvector_for_each_in(ue_cc_cfg, cfg->carriers){
		if (ue_cc_cfg->active) {
		  if (u->carriers[ue_cc_cfg->cc] == nullptr) {
		  	u->carriers[ue_cc_cfg->cc] = ue_carrier_init(u, ue_cc_cfg->cc);
		  } else {
			  //rrc传递下来明确uss coreset和searchspace资源，非coreset0和searchspcae0
		    ue_carrier  *carrier = u->carriers[ue_cc_cfg->cc];
			ue_carrier_params_init(&carrier->bwp_cfg, carrier->rnti, &carrier->cell_params->bwps[0], &u->ue_cfg);
		  }
		}
	}

	//没使用令牌桶，只是初始化相关逻辑信道
	base_ue_buffer_manager_config_lcids(&u->buffers.base_ue ,u->ue_cfg.ue_bearers);
}

void sched_nr_ue_remove(sched_nr_ue *u)
{
	oset_assert(u);

	//todo
    oset_list_remove(&mac_manager_self()->sched.sched_ue_list, u);
    oset_hash_set(&mac_manager_self()->sched.ue_db, &u->rnti, sizeof(u->rnti), NULL);

	oset_info("SCHED: Removed sched user rnti=0x%x", u->rnti);

	sched_nr_ue_cc_cfg_t *ue_cc_cfg = NULL;
	cvector_for_each_in(ue_cc_cfg, u->ue_cfg.carriers){
		ue_carrier_destory(u->carriers[ue_cc_cfg->cc]);
	}
	cvector_free(u->ue_cfg.carriers);

	oset_pool_free(&mac_manager_self()->sched.sched_ue_list, u);
    oset_info("[Removed] Number of SCHED-UEs is now %d", oset_list_count(&mac_manager_self()->sched.sched_ue_list));
}

void sched_nr_ue_remove_all(void)
{
	sched_nr_ue *ue = NULL, *next_ue = NULL;
	oset_list_for_each_safe(&mac_manager_self()->sched.sched_ue_list, next_ue, ue)
		sched_nr_ue_remove(ue);
}

sched_nr_ue *sched_ue_nr_find_by_rnti(uint16_t rnti)
{
    return (sched_nr_ue *)oset_hash_get(
            &mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti));
}

void sched_nr_add_ue_impl(uint16_t rnti, sched_nr_ue *u, uint32_t cc)
{
	oset_assert(u);
	oset_info("SCHED: New sched user rnti=0x%x, cc=%d", rnti, cc);
	oset_hash_set(&mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti), NULL);
	oset_hash_set(&mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti), u);
}

void sched_ue_nr_set_by_rnti(uint16_t rnti, sched_nr_ue *u)
{
	oset_assert(u);
    oset_hash_set(&mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti), NULL);
    oset_hash_set(&mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti), u);
}


sched_nr_ue *sched_nr_ue_add(uint16_t rnti_, uint32_t cc, sched_params_t *sched_cfg_)
{
	//首次主要为了获得phy配置，rrc层会通过mac_ue_cfg二次更新高层信息。
	sched_nr_ue_cfg_t *uecfg = get_rach_ue_cfg_helper(cc, sched_cfg_);

	sched_nr_ue *u = sched_nr_ue_add_inner(rnti_, uecfg, sched_cfg_);

	free_rach_ue_cfg_helper(uecfg);
	return u;
}

sched_nr_ue *sched_nr_ue_add_inner(uint16_t rnti_, sched_nr_ue_cfg_t *uecfg, sched_params_t *sched_cfg_)
{
	sched_nr_ue *u = NULL; 

	// create user object outside of sched main thread
	oset_pool_alloc(&mac_manager_self()->sched.ue_pool, &u);
	ASSERT_IF_NOT(u, "Could not allocate sched ue %d context from pool", rnti_);

	u->rnti = rnti_;
	u->sched_cfg = sched_cfg_;
	base_ue_buffer_manager_init(&u->buffers.base_ue, rnti_);
	ue_cfg_manager_init(&u->ue_cfg, uecfg->carriers[0].cc);//todo cc
	sched_nr_ue_set_cfg(u, uecfg);//create carriers[cc]

    oset_list_add(&mac_manager_self()->sched.sched_ue_list, u);
    oset_info("[Added] Number of SCHED-UEs is now %d", oset_list_count(&mac_manager_self()->sched.sched_ue_list));
	return u;
}

bool sched_nr_ue_has_ca(sched_nr_ue *u)
{
	int active = 0;
	sched_nr_ue_cc_cfg_t *cc = NULL;
	cvector_for_each_in(cc, u->ue_cfg.carriers){
		if (cc->active) active++;
	}
	//有两个以上小区激活态
	return ((cvector_size(u->ue_cfg.carriers) > 1) && (active > 1));
}

void sched_nr_ue_new_slot(sched_nr_ue *ue, slot_point pdcch_slot)
{
	ue->last_tx_slot = pdcch_slot;

	for(int i = 0; i < SCHED_NR_MAX_CARRIERS; ++i){
		if (NULL != ue->carriers[i]) {
			harq_entity_new_slot(&ue->carriers[i].harq_ent ,pdcch_slot - TX_ENB_DELAY);
		}
	}

	for (std::unique_ptr<ue_carrier>& cc : ue->carriers) {
		if (cc != nullptr) {
		  cc->harq_ent.new_slot(pdcch_slot - TX_ENB_DELAY); //void harq_entity::new_slot(slot_point slot_rx_)
		  //清除重传失败达最大上限的harq
		}
	}

	// Compute pending DL/UL bytes for {rnti, pdcch_slot}
	if (sched_cfg.sched_cfg.auto_refill_buffer) {
	common_ctxt.pending_dl_bytes = 1000000;
	common_ctxt.pending_ul_bytes = 1000000;
	} else {
	common_ctxt.pending_dl_bytes = buffers.get_dl_tx_total();//获取当前等待下行的数据总大小
	common_ctxt.pending_ul_bytes = buffers.get_bsr();//终端上报bsr缓存状态报告int base_ue_buffer_manager<isNR>::get_bsr() const
	for (auto& ue_cc_cfg : ue_cfg.carriers) {
	  auto& cc = carriers[ue_cc_cfg.cc];
	  if (cc != nullptr) {
	    // Discount UL HARQ pending bytes to BSR   将UL HARQ挂起字节折扣到BSR
	    for (uint32_t pid = 0; pid < cc->harq_ent.nof_ul_harqs(); ++pid) {
	      if (not cc->harq_ent.ul_harq(pid).empty()) {//tb.active
	        common_ctxt.pending_ul_bytes -= std::min(cc->harq_ent.ul_harq(pid).tbs() / 8, common_ctxt.pending_ul_bytes);
	        if (last_sr_slot.valid() and cc->harq_ent.ul_harq(pid).harq_slot_tx() > last_sr_slot) {//若上行harq已经处理完毕，就清空该sr
	          last_sr_slot.clear();
	        }
	      }
	    }
	  }
	}
	if (common_ctxt.pending_ul_bytes == 0 and last_sr_slot.valid()) {
	  // If unanswered SR is pending如果未应答SR待定
	  common_ctxt.pending_ul_bytes = 512;//sr之后尚未bsr，先预设512
	}
	}
}

