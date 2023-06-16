/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/mac.h"
#include "mac/sched_nr_ue.h"
#include "lib/mac/mac_sch_pdu_nr.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-ue"

//////////////////////////////////////////ue_buffer_manager////////////////////////////////////////////

static int get_dl_tx_total(ue_buffer_manager *buffers)
{
	//RLC消息
	int total_bytes = base_ue_buffer_manager_get_dl_tx_total(&buffers->base_ue);

	//MAC Control Element消息
	ce_t *ce = NULL;
	oset_stl_queue_foreach(&buffers->pending_ces, ce){
		total_bytes += mac_sch_subpdu_nr_sizeof_ce(ce->lcid, false);
	}
	return total_bytes;
}

static int get_ul_bsr_total(ue_buffer_manager *buffers)
{
	int total_bytes = base_ue_buffer_manager_get_bsr(&buffers->base_ue);
	return total_bytes;
}


//////////////////////////////////////////pdu_builder////////////////////////////////////////////
static void pdu_builder_init(pdu_builder        *pdu_builders, uint32_t cc_, ue_buffer_manager *parent_)
{
	pdu_builders->cc = cc_;
	pdu_builders->parent = parent_;
}

static uint32_t pdu_builder_pending_bytes(pdu_builder           *pdu_builders, uint32_t lcid)  
{
	return get_dl_tx_inner(&pdu_builders->parent->base_ue ,lcid);
}

/**
 * @brief Allocates LCIDs and update US buffer states depending on available resources and checks if there is SRB0/CCCH
 MAC PDU segmentation

 * @param rem_bytes TBS to be filled with MAC CEs and MAC SDUs [in bytes]
 * @param reset_buf_states If true, when there is SRB0/CCCH MAC PDU segmentation, restore the UE buffers and scheduled
 LCIDs as before running this function
 * @return true if there is no SRB0/CCCH MAC PDU segmentation, false otherwise
 */
bool pdu_builder_alloc_subpdus(pdu_builder           *pdu_builders, uint32_t rem_bytes, dl_pdu_t *pdu)
{
	// 调度的是单个ue（rnti）的lcid资源
	// rem_bytes tb传输块允许大小
	// First step: allocate MAC CEs until resources allow
	ce_t *ce = NULL;
	oset_stl_queue_foreach(&pdu_builders->parent->pending_ces, ce){
		if (ce->cc == pdu_builders->cc) {
			// Note: This check also avoids thread collisions across UE carriers
			uint32_t size_ce = mac_sch_subpdu_nr_sizeof_ce(ce->lcid, false);
			if (size_ce > rem_bytes) {
				break;
			}
			rem_bytes -= size_ce;
			cvector_push_back(pdu->subpdus, ce->lcid);
			oset_stl_queue_del_first(&pdu_builders->parent->pending_ces);
		}
	}

	// todo ???? 为啥不记录长度 ???? 应该用令牌桶
	// Second step: allocate the remaining LCIDs (LCIDs for MAC CEs are addressed above)
	for (uint32_t lcid = 0; rem_bytes > 0 && is_lcid_valid(lcid); ++lcid) {
		uint32_t pending_lcid_bytes = get_dl_tx_total_inner(&pdu_builders->parent->base_ue, lcid);
		// Return false if the TBS is too small to store the entire CCCH buffer without segmentation
		if (lcid == (nr_lcid_sch_t)CCCH && pending_lcid_bytes > rem_bytes) {
			cvector_push_back(pdu->subpdus, lcid);
			return false;
		}
		if (pending_lcid_bytes > 0) {
			rem_bytes -= MIN(rem_bytes, pending_lcid_bytes);
			cvector_push_back(pdu->subpdus, lcid);
		}
	}

	return true;
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
	harq_entity_init(&carrier->harq_ent, cc, u->rnti, &cell_params_->carrier.nof_prb, SCHED_NR_MAX_HARQ);
	return carrier;
}

int ue_carrier_dl_ack_info(ue_carrier *carrier, uint32_t pid, uint32_t tb_idx, bool ack)
{
	int tbs = dl_ack_info(&carrier->harq_ent, pid, tb_idx, ack);
	if (tbs < 0) {
		oset_warn("SCHED: rnti=0x%x,cc=%d received DL HARQ-ACK for empty pid=%d", carrier->rnti, carrier->cc, pid);
		return tbs;
	}

	//和save_metrics同处一个线程，无需加锁
	if (ack) {
		carrier->metrics.tx_brate += tbs;
	} else {
		carrier->metrics.tx_errors++;
	}
	carrier->metrics.tx_pkts++;
	return tbs;
}

int ue_carrier_ul_crc_info(ue_carrier *carrier, uint32_t pid, bool crc)
{
	int ret = ul_crc_info(&carrier->harq_ent, pid, crc);
	if (ret < 0) {
		oset_warn("SCHED: rnti=0x%x,cc=%d received CRC for empty pid=%d", carrier->rnti, carrier->cc, pid);
	}
	return ret;
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
			// rrc传递下来明确uss coreset和searchspace资源，非coreset0和searchspcae0
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
    oset_hash_set(mac_manager_self()->sched.ue_db, &u->rnti, sizeof(u->rnti), NULL);

	oset_info("SCHED: Removed sched user rnti=0x%x", u->rnti);

	oset_stl_queue_term(&u->buffers.pending_ces);

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

sched_nr_ue *sched_nr_ue_find_by_rnti(uint16_t rnti)
{
    return (sched_nr_ue *)oset_hash_get(
            mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti));
}

void sched_nr_add_ue_impl(uint16_t rnti, sched_nr_ue *u, uint32_t cc)
{
	oset_assert(u);
	oset_info("SCHED: New sched user rnti=0x%x, cc=%d", rnti, cc);
	oset_hash_set(mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti), NULL);
	oset_hash_set(mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti), u);
}

void sched_nr_ue_set_by_rnti(uint16_t rnti, sched_nr_ue *u)
{
	oset_assert(u);
    oset_hash_set(mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti), NULL);
    oset_hash_set(mac_manager_self()->sched.ue_db, &rnti, sizeof(rnti), u);
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

	memset(u, 0, sizeof(sched_nr_ue));

	u->rnti = rnti_;
	u->sched_cfg = sched_cfg_;
	base_ue_buffer_manager_init(&u->buffers.base_ue, rnti_);
	oset_stl_queue_init(&u->buffers.pending_ces);
	ue_cfg_manager_init(&u->ue_cfg, uecfg->carriers[0].cc);//todo cc
	sched_nr_ue_set_cfg(u, uecfg);//create carriers[cc]

	slot_point_init(&u->last_sr_slot);
	slot_point_init(&u->last_tx_slot);
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
			//清除(rx_slot=已接收到的slot)重传失败达最大上限的harq
			harq_entity_new_slot(&ue->carriers[i].harq_ent, slot_point_sub_jump(pdcch_slot, TX_ENB_DELAY));
		}
	}

	// Compute pending DL/UL bytes for {rnti, pdcch_slot}
	if (ue->sched_cfg->sched_cfg->auto_refill_buffer) {
		ue->common_ctxt.pending_dl_bytes = 1000000;
		ue->common_ctxt.pending_ul_bytes = 1000000;
	} else {
		ue->common_ctxt.pending_dl_bytes = get_dl_tx_total(&ue->buffers);// 获取当前等待下行的数据总大小
		ue->common_ctxt.pending_ul_bytes = get_ul_bsr_total(&ue->buffers);// 终端上报bsr缓存状态报告

		sched_nr_ue_cc_cfg_t *ue_cc_cfg = NULL;
		cvector_for_each_in(ue_cc_cfg, ue->ue_cfg.carriers){	
		  ue_carrier *cc = ue->carriers[ue_cc_cfg.cc];
		  if (NULL != cc) {
		    // Discount UL HARQ pending bytes to BSR
		    // 获得bsr后需要将ul harq重传的资源扣除，剩余的才能用来给ue其他上行数据
		    for (uint32_t pid = 0; pid < nof_ul_harqs(&cc->harq_ent); ++pid) {
			  // harq重传
		      if (!empty(cc->harq_ent.ul_harqs[pid].proc.tb)) {
		        ue->common_ctxt.pending_ul_bytes -= MIN(TBS(cc->harq_ent.ul_harqs[pid].proc.tb) / 8, ue->common_ctxt.pending_ul_bytes);
				// 若上行harq slot>last_sr_slot(已经拿到上行数据授权，不需要在发sr)，且该slot已处理，就清空该sr
				if (slot_valid(&ue->last_sr_slot) && harq_slot_tx(&cc->harq_ent.ul_harqs[pid].proc) > ue->last_sr_slot) {
					slot_clear(&ue->last_sr_slot);
		        }
		      }
		    }
		  }
		}
		if (ue->common_ctxt.pending_ul_bytes == 0 && slot_valid(&ue->last_sr_slot)) {
		  // If unanswered SR is pending
		  // sr之后尚未收到bsr，先预设512
		  ue->common_ctxt.pending_ul_bytes = 512;
		}
	}
}

void sched_nr_ue_ul_sr_info(sched_nr_ue *ue)
{
	ue->last_sr_slot = slot_point_sub_jump(ue->last_tx_slot, TX_ENB_DELAY);
}

void sched_nr_ue_add_dl_mac_ce(sched_nr_ue *ue, uint32_t ce_lcid, uint32_t nof_cmds)
{
	for (uint32_t i = 0; i < nof_cmds; ++i) {
		// If not specified otherwise, the CE is transmitted in PCell
		ce_t ce = {0};
		ce.lcid = ce_lcid;
		//todo
		ce.cc = ue->ue_cfg.carriers[0].cc;
		oset_stl_queue_add_last(&ue->buffers.pending_ces, ce)
	}
}

void sched_nr_ue_ul_bsr(sched_nr_ue *ue, uint32_t lcg, uint32_t bsr_val)
{
	base_ue_buffer_manager_ul_bsr(&ue->buffers.base_ue, lcg, bsr_val);
}


void sched_nr_ue_rlc_buffer_state(sched_nr_ue *ue, uint32_t lcid, uint32_t newtx, uint32_t priotx)
{
	base_ue_buffer_manager_dl_buffer_state(&ue->buffers.base_ue, lcid, newtx, priotx);
}

static slot_ue* slot_ue_init(ue_carrier *ue_, slot_point slot_tx_, uint32_t cc)
{
	slot_ue *slot_u = NULL; 
	oset_pool_alloc(&mac_manager_self()->sched.cc_workers[cc].slot_ue_pool, &slot_u);
	oset_assert(slot_u);

	memset(slot_u, 0, sizeof(slot_ue));

	slot_point_init(&slot_u->pdcch_slot);
	slot_point_init(&slot_u->pdsch_slot);
	slot_point_init(&slot_u->pusch_slot);
	slot_point_init(&slot_u->uci_slot);

	slot_u->ue = ue_;
	slot_u->pdcch_slot = slot_tx_;

	const uint32_t k0 = 0;
	slot_u->pdsch_slot  = slot_point_add_jump(slot_u->pdcch_slot, k0);             //dci 1_X  dci0_X
	uint32_t k1         = ue_carrier_params_get_k1(&ue_->bwp_cfg, slot_u->pdsch_slot);
	slot_u->uci_slot    = slot_point_add_jump(slot_u->pdsch_slot, k1);             //dl ack/sr/csi
	uint32_t k2         = ue_->bwp_cfg.bwp_cfg->pusch_ra_list[0].K;
	slot_u->pusch_slot  = slot_point_add_jump( slot_u->pdcch_slot, k2);            //dci0_1触发得到的msg4/bsr/ul data

	slot_u->dl_active = ue_->cell_params.bwps[0].slots[slot_idx(&slot_u->pdsch_slot)].is_dl;
	if (slot_u->dl_active) {
		slot_u->dl_bytes = ue_->common_ctxt.pending_dl_bytes;
		slot_u->h_dl     = find_pending_dl_retx(&ue_->harq_ent);//重传优先
		if (NULL == slot_u->h_dl) {
			slot_u->h_dl = find_empty_dl_harq(&ue_->harq_ent);
		}
	}

	slot_u->ul_active = ue_->cell_params.bwps[0].slots[slot_idx(&slot_u->pusch_slot)].is_ul;
	if (slot_u->ul_active) {
		slot_u->ul_bytes = ue_->common_ctxt.pending_ul_bytes;//已经减去要重传bytes
		slot_u->h_ul     = find_pending_ul_retx(&ue_->harq_ent);
		if (NULL == slot_u->h_ul) {
			slot_u->h_ul = find_empty_ul_harq(&ue_->harq_ent);
		}
	}

	return slot_u;
}

static void slot_ue_set_by_rnti(uint16_t rnti, slot_ue *u, uint32_t cc)
{
	oset_assert(u);
    oset_hash_set(mac_manager_self()->sched.cc_workers[cc].slot_ues, &rnti, sizeof(rnti), NULL);
    oset_hash_set(mac_manager_self()->sched.cc_workers[cc].slot_ues, &rnti, sizeof(rnti), u);
}

void slot_ue_alloc(sched_nr_ue *ue, slot_point pdcch_slot, uint32_t cc)
{
  ASSERT_IF_NOT(ue->carriers[cc] != NULL, "make_slot_ue() called for unknown rnti=0x%x,cc=%d", ue->rnti, cc);
  slot_ue* slot_u = slot_ue_init(ue->carriers[cc], pdcch_slot, cc);
  oset_assert(slot_u);
  slot_ue_set_by_rnti(ue->rnti, slot_u, cc);
  cvector_push_back(mac_manager_self()->sched.cc_workers[cc].slot_ue_list, slot_u);
}

void slot_ue_clear(uint32_t cc)
{
    oset_hash_index_t *hi = NULL;
    for (hi = oset_hash_first(mac_manager_self()->sched.cc_workers[cc].slot_ues); hi; hi = oset_hash_next(hi)) {
        slot_ue *slot_u = oset_hash_this_val(hi);
		oset_pool_free(&mac_manager_self()->sched.cc_workers[cc].slot_ue_pool, slot_u);
	}
	oset_hash_clear(mac_manager_self()->sched.cc_workers[cc].slot_ues);
	cvector_clear(mac_manager_self()->sched.cc_workers[cc].slot_ue_list);
}

void slot_ue_destory(uint32_t cc)
{
	cvector_free(mac_manager_self()->sched.cc_workers[cc].slot_ue_list);
	oset_hash_destroy(mac_manager_self()->sched.cc_workers[cc].slot_ues);
	oset_pool_final(&mac_manager_self()->sched.cc_workers[cc].slot_ue_pool);
}

slot_ue *slot_ue_find_by_rnti(uint16_t rnti, uint32_t cc)
{
    return (slot_ue *)oset_hash_get(
            mac_manager_self()->sched.cc_workers[cc].slot_ues, &rnti, sizeof(rnti));
}

///////////////////////////////////////////////////////////////////////////////////////////

/// Channel Information Getters
uint32_t dl_cqi(slot_ue *slot_u)
{
	return slot_u->ue->dl_cqi;
}

uint32_t ul_cqi(slot_ue *slot_u)
{ 
	return slot_u->ue->ul_cqi;
}

bool get_pending_bytes(slot_ue *slot_u, uint32_t lcid)
{
	return pdu_builder_pending_bytes(&slot_u->ue->pdu_builders, lcid);
}

/// Build PDU with MAC CEs and MAC SDUs
bool build_pdu(slot_ue *slot_u, uint32_t rem_bytes, dl_pdu_t *pdu)
{
	return pdu_builder_alloc_subpdus(&slot_u->ue->pdu_builders, rem_bytes, pdu);
}



