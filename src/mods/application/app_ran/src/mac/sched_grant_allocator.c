/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_grant_allocator.h"
		
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-grant"
#define SLOT_IDX(tti) (count_idx(&tti)%TTIMOD_SZ)

static void bwp_slot_grid_init(bwp_slot_grid *slot, bwp_params_t *bwp_cfg_, uint32_t slot_idx_)
{
	int i = 0;
	//dl init
	cvector_reserve(slot->dl.phy.ssb, MAX_SSB);
	cvector_reserve(slot->dl.phy.pdcch_dl, MAX_GRANTS);
	cvector_reserve(slot->dl.phy.pdcch_ul, MAX_GRANTS);
	cvector_reserve(slot->dl.phy.pdsch, MAX_GRANTS);
	cvector_reserve(slot->dl.phy.nzp_csi_rs, MAX_NZP_CSI_RS);
	cvector_reserve(slot->dl.data, MAX_GRANTS);
	for(i = 0; i < MAX_GRANTS; ++i){
		cvector_reserve(slot->dl.data[i].subpdus, MAX_SUBPDUS);
	}

	cvector_reserve(slot->dl.rar, MAX_GRANTS);
	for(i = 0; i < MAX_GRANTS; ++i){
		cvector_reserve(slot->dl.rar[i].grants, MAX_GRANTS);
	}

	cvector_reserve(slot->dl.sib_idxs, MAX_GRANTS);
	//ul init
	cvector_reserve(slot->ul.pucch, MAX_GRANTS);
	cvector_reserve(slot->ul.pusch, MAX_GRANTS);
	//ack init
	cvector_reserve(slot->pending_acks, MAX_GRANTS);

	slot->slot_idx = slot_idx_;
	slot->cfg = bwp_cfg_;
	bwp_pdcch_allocator_init(&slot->pdcchs, bwp_cfg_, slot_idx_, slot->dl.phy.pdcch_dl, slot->dl.phy.pdcch_ul);
	pdsch_allocator_init(&slot->pdschs, bwp_cfg_, slot_idx_, slot->dl.phy.pdsch);
	pusch_allocator_init(&slot->puschs, bwp_cfg_, slot_idx_, slot->ul.pusch);
	slot->rar_softbuffer = harq_softbuffer_pool_get_tx(harq_buffer_pool_self(bwp_cfg_->cc), bwp_cfg_->cfg.rb_width);//bwp_cfg_->nof_prb
}

void bwp_slot_grid_destory(bwp_slot_grid *slot)
{
	//pdcchs
	bwp_pdcch_allocator_destory(&slot->pdcchs);
	//pdsch
	pdsch_allocator_destory(&slot->pdschs);
	//pusch
	pusch_allocator_destory(&slot->puschs);

	//dl
	cvector_free(slot->dl.phy.ssb);
	cvector_free(slot->dl.phy.nzp_csi_rs);

	dl_pdu_t  *dl_pdu_node = NULL;
	cvector_for_each_in(dl_pdu_node, slot->dl.data){
		cvector_free(dl_pdu_node->subpdus);
	}
	cvector_free(slot->dl.data);

	rar_t  *rar_node = NULL;
	cvector_for_each_in(rar_node, slot->dl.rar){
		cvector_free(rar_node->grants);
	}
	cvector_free(slot->dl.rar);

	cvector_free(slot->dl.sib_idxs);

	pucch_t  *pucch_node = NULL;
	cvector_for_each_in(pucch_node, slot->ul.pucch){
		cvector_free(pucch_node->candidates);
	}
	cvector_free(slot->ul.pucch);

	//ack
	cvector_free(slot->pending_acks);
}

void bwp_slot_grid_reset(bwp_slot_grid *slot)
{
	bwp_pdcch_allocator_reset(&slot->pdcchs);
	pdsch_allocator_reset(&slot->pdschs);
	pusch_allocator_reset(&slot->puschs);
	cvector_free(slot->dl.phy.ssb);
	cvector_free(slot->dl.phy.nzp_csi_rs);

	dl_pdu_t  *dl_pdu_node = NULL;
	cvector_for_each_in(dl_pdu_node, slot->dl.data){
		cvector_clear(dl_pdu_node->subpdus);
	}
	cvector_clear(slot->dl.data);

	rar_t  *rar_node = NULL;
	cvector_for_each_in(rar_node, slot->dl.rar){
		cvector_clear(rar_node->grants);
	}
	cvector_clear(slot->dl.rar);

	cvector_clear(slot->dl.sib_idxs);

	pucch_t  *pucch_node = NULL;
	cvector_for_each_in(pucch_node, slot->ul.pucch){
		cvector_free(pucch_node->candidates);
	}
	cvector_clear(slot->ul.pucch);

	cvector_clear(slot->pending_acks);
}


void bwp_slot_grid_reserve_pdsch(bwp_slot_grid *slot, prb_grant *grant)
{
	pdsch_allocator_reserve_prbs(&slot->pdschs, grant);
}

///////////////////////////////////////////////////////////////////////////

pdsch_allocator* bwp_res_grid_get_pdschs(bwp_res_grid *res, slot_point tti)
{
	return &res->slots[SLOT_IDX(tti)].pdschs;
}

bwp_slot_grid* bwp_res_grid_get_slot(bwp_res_grid *res, slot_point tti)
{
	return &res->slots[SLOT_IDX(tti)];
}

void bwp_res_grid_destory(bwp_res_grid *res)
{
	bwp_slot_grid *slot = NULL;
	cvector_for_each_in(slot, res->slots){
		bwp_slot_grid_destory(slot);
	}

	cvector_free(res->slots);
}


void bwp_res_grid_init(bwp_res_grid *res, bwp_params_t *bwp_cfg_)
{
	res->cfg = bwp_cfg_;
	//TTIMOD_SZ todo???
	cvector_reserve(res->slots, TTIMOD_SZ);
	// 1 frame 20 slot(30khz)
	// 1 frame 10 slot(15khz)
	for (uint32_t sl = 0; sl < TTIMOD_SZ; ++sl){
		bwp_slot_grid  slot_grid = {0};
		bwp_slot_grid_init(&slot_grid, bwp_cfg_, sl % ((uint32_t)SRSRAN_NSLOTS_PER_FRAME_NR(bwp_cfg_->cell_cfg.carrier.scs)));
		cvector_push_back(res->slots, slot_grid);
	}
}
/////////////////////////////////////////////////////////////////////////
slot_point get_pdcch_tti(bwp_slot_allocator *bwp_alloc)
{
	return bwp_alloc->pdcch_slot;
}
slot_point get_rx_tti(bwp_slot_allocator *bwp_alloc)
{
	return slot_point_sub_jump(bwp_alloc->pdcch_slot, TX_ENB_DELAY);
}

bwp_slot_grid *get_tx_slot_grid(bwp_slot_allocator *bwp_alloc)
{
	return &bwp_alloc->bwp_grid->slots[SLOT_IDX(get_pdcch_tti(bwp_alloc))];
}

bwp_slot_grid *get_slot_grid(bwp_slot_allocator *bwp_alloc, slot_point slot)
{
	return &bwp_alloc->bwp_grid->slots[SLOT_IDX(slot)];
}

static prb_bitmap* occupied_dl_prbs(bwp_slot_allocator *bwp_alloc, slot_point sl_tx, uint32_t ss_id, srsran_dci_format_nr_t dci_fmt)
{
	return pdsch_allocator_occupied_prbs(bwp_alloc->bwp_grid->slots[SLOT_IDX(sl_tx)].pdschs, ss_id, dci_fmt);
}

prb_grant find_optimal_dl_grant(bwp_slot_allocator *bwp_alloc, slot_ue *ue, uint32_t ss_id)
{
	static const srsran_dci_format_nr_t dci_fmt = srsran_dci_format_nr_1_0; // TODO: Support more DCI formats

	// 获取当前prb_bitmap
	prb_bitmap *used_prb_mask = occupied_dl_prbs(bwp_alloc, ue->pdsch_slot, ss_id, dci_fmt);

	// prb_bitmap中查找出最大的一块空闲
	prb_interval prb_interv = find_empty_interval_of_length(used_prb_mask, bit_size(used_prb_mask), 0);

	return prb_interv;
}


bwp_slot_allocator* bwp_slot_allocator_init(bwp_res_grid *bwp_grid_, slot_point pdcch_slot_)
{
	bwp_slot_allocator *bwp_alloc = oset_malloc(sizeof(bwp_slot_allocator));
	oset_assert(bwp_alloc);

	bwp_alloc->cfg = bwp_grid_->cfg;
	bwp_alloc->bwp_grid = bwp_grid_;
	bwp_alloc->pdcch_slot = pdcch_slot_;

	return bwp_alloc;
}

alloc_result bwp_slot_allocator_alloc_si(bwp_slot_allocator *bwp_alloc,
													uint32_t            aggr_idx,
													uint32_t            si_idx,
													uint32_t            si_ntx,
													prb_interval        *interv,
													tx_harq_softbuffer  *softbuffer)
{
	static const uint32_t               ss_id   = 0;//sib ss_id = 0
	static const srsran_dci_format_nr_t dci_fmt = srsran_dci_format_nr_1_0;

	bwp_slot_grid *bwp_pdcch_slot = get_tx_slot_grid(bwp_alloc);

	// Verify there is space in PDSCH
	// 鉴定授权grap的合法性
	prb_grant grap = prb_grant_interval_init(interv);
	alloc_result ret = pdsch_allocator_is_si_grant_valid(&bwp_pdcch_slot->pdschs, ss_id, &grap);
	if (ret != (alloc_result)success) {
	  return ret;
	}

	// Allocate PDCCH
	pdcch_dl_alloc_result pdcch_result = bwp_pdcch_allocator_alloc_si_pdcch(&bwp_pdcch_slot->pdcchs, ss_id, aggr_idx);
	if (!pdcch_result.has_val) {
		oset_error("[%5u] SCHED: Cannot allocate SIB due to lack of PDCCH space", GET_RSLOT_ID(bwp_alloc->pdcch_slot));
		return pdcch_result.res.unexpected;
	}
	pdcch_dl_t *pdcch = pdcch_result.res.val;

	// Allocate PDSCH (no need to verify again if there is space in PDSCH)
	// 申请pdsch
	pdsch_t *pdsch = pdsch_allocator_alloc_si_pdsch_unchecked(&bwp_pdcch_slot->pdschs, ss_id, &grap, &pdcch->dci);

	// Generate DCI for SIB
	pdcch->dci_cfg.coreset0_bw = srsran_coreset_get_bw(&bwp_alloc->cfg->cfg.pdcch.coreset[0]);
	pdcch->dci.mcs             = 5;//mcs_index
	pdcch->dci.rv              = 0;
	pdcch->dci.sii             = si_idx == 0 ? 0 : 1;

	// Generate PDSCH
	srsran_slot_cfg_t slot_cfg  = {0};
	slot_cfg.idx = count_idx(&bwp_alloc->pdcch_slot);
	// Resource allocation
	int code     = srsran_ra_dl_dci_to_grant_nr(&bwp_alloc->cfg->cell_cfg->carrier,
												&slot_cfg,
												&bwp_alloc->cfg->cfg.pdsch,
												&pdcch->dci,
												&pdsch->sch,
												&pdsch->sch.grant);
	if (code != SRSRAN_SUCCESS) {
		oset_error("[%5u] Error generating SIB PDSCH grant", GET_RSLOT_ID(bwp_alloc->pdcch_slot));
		bwp_pdcch_allocator_cancel_last_pdcch(&bwp_pdcch_slot->pdcchs);
		cvector_pop_back(bwp_pdcch_slot->pdschs);
		oset_free(pdsch);// free back()
		return (alloc_result)other_cause;
	}

	//si没有ack,没有harq
	pdsch->sch.grant.tb[0].softbuffer.tx = &softbuffer->buffer;

	// Store SI msg index
	cvector_push_back(bwp_pdcch_slot->dl.sib_idxs, si_idx);

	return (alloc_result)success;
}

alloc_result bwp_slot_allocator_alloc_rar_and_msg3(bwp_slot_allocator *bwp_alloc,
																uint16_t	   ra_rnti,
																uint32_t	   aggr_idx,
																prb_interval   *interv,
																span_t(dl_sched_rar_info_t) *pending_rachs)
{
	int i = 0;
	srsran_slot_cfg_t slot_cfg = {0};
	static const uint32_t 	msg3_nof_prbs = 3;//msg3 prb
	static const uint32_t 	m = 0;//ra pusch time alloc list idx
	static const srsran_dci_format_nr_t dci_fmt = srsran_dci_format_nr_1_0;

	//msg1(rach)和msg2(rar)没有ack
	bwp_slot_grid *bwp_pdcch_slot = get_tx_slot_grid(bwp_alloc);
	slot_point	   msg3_slot      = slot_point_add_jump(bwp_alloc->pdcch_slot, bwp_alloc->cfg.pusch_ra_list[m].msg3_delay);
	bwp_slot_grid *bwp_msg3_slot  = get_slot_grid(bwp_alloc, msg3_slot);

	// Verify there is space in PDSCH for RAR
	prb_grant grap_rar = prb_grant_interval_init(interv);
	alloc_result ret = pdsch_allocator_is_rar_grant_valid(bwp_pdcch_slot->pdschs, &grap_rar);
	if (ret != (alloc_result)success) {
		return ret;
	}

	for (i = 0; i < pending_rachs->len; i++) { 
		dl_sched_rar_info_t *rach = pending_rachs[i];
		slot_ue* ue_it = slot_ue_find_by_rnti(rach->temp_crnti, rach->cc);
		if (NULL == ue_it) {
			oset_error("[%5u]SCHED: Postponing rnti=0x%x RAR allocation. Cause: The ue object not yet fully created", GET_RSLOT_ID(bwp_alloc->pdcch_slot), rach->temp_crnti);
			return (alloc_result)no_rnti_opportunity;
		}
	}

	// 当前bwp_pdcch_slot最多处理MAX_GRANTS个msg2消息
	ASSERT_IF_NOT(!(MAX_GRANTS == cvector_size(bwp_pdcch_slot->dl.rar)), "[%5u]The #RARs should be below #PDSCHs", GET_RSLOT_ID(bwp_alloc->pdcch_slot));

	if (!cvector_empty(bwp_pdcch_slot->dl.phy.ssb)) {
		// TODO: support concurrent PDSCH and SSB
		// 尚不支持并发PDSCH和SSB
		oset_debug("[%5u]SCHED: skipping RAR allocation. Cause: concurrent PDSCH and SSB not yet supported", GET_RSLOT_ID(bwp_alloc->pdcch_slot));
		return (alloc_result)no_sch_space;
	}

	// Verify there is space in PUSCH for Msg3s
	// 当前bwp_msg3_slot最多预授权MAX_GRANTS个msg3消息
	ret = pusch_allocator_has_grant_space(&bwp_msg3_slot->puschs, pending_rachs->len, true);
	if (ret != (alloc_result)success) {
		return ret;
	}

	// Check Msg3 RB collision
	uint32_t	 total_msg3_nof_prbs = msg3_nof_prbs * pending_rachs->len;
	prb_interval all_msg3_rbs = find_empty_interval_of_length(pusch_allocator_occupied_prbs(&bwp_msg3_slot->puschs), total_msg3_nof_prbs, 0);
	if (prb_interval_length(&all_msg3_rbs) < total_msg3_nof_prbs) {
		oset_warn("[%5u]SCHED: No space in PUSCH for Msg3", GET_RSLOT_ID(bwp_alloc->pdcch_slot));
		return (alloc_result)sch_collision;
	}

	// Allocate PDCCH position for RAR
	pdcch_dl_alloc_result pdcch_result = bwp_pdcch_allocator_alloc_rar_pdcch(&bwp_pdcch_slot->pdcchs, ra_rnti, aggr_idx);
	if (!pdcch_result.has_val) {
		// Could not find space in PDCCH
		return pdcch_result.res.unexpected;
	}

	pdcch_dl_t *pdcch = pdcch_result.res.val;

	slot_ue* slot_u = slot_ue_find_by_rnti(pending_rachs[0].temp_crnti, bwp_alloc->cfg->cc);
	pdcch->dci_cfg 	= ue_carrier_params_get_dci_cfg(&slot_u->ue->bwp_cfg);
	pdcch->dci.mcs 	= 5;

	// Allocate RAR PDSCH
	pdsch_t *pdsch = pdsch_allocator_alloc_rar_pdsch_unchecked(&bwp_pdcch_slot->pdschs, &grap_rar, &pdcch->dci);

	// Fill RAR PDSCH content
	// TODO: Properly fill Msg3 grants
	slot_cfg = {0};
	slot_cfg.idx = count_idx(&bwp_alloc->pdcch_slot);
	int code	 = srsran_ra_dl_dci_to_grant_nr(&bwp_alloc->cfg->cell_cfg->carrier,
													&slot_cfg,
													&bwp_alloc->cfg->cfg.pdsch,
													&pdcch->dci,
													&pdsch->sch,
													&pdsch->sch.grant);
	ASSERT_IF_NOT(code == SRSRAN_SUCCESS, "[%5u]Error converting DCI to grant", GET_RSLOT_ID(bwp_alloc->pdcch_slot));
	pdsch.sch.grant.tb[0].softbuffer.tx = &bwp_pdcch_slot->rar_softbuffer->buffer;

	//mac rar pdu???
	/////////////////////////////////////////////////////////////////////////////////
	// Generate Msg3 grants in PUSCH
	slot_cfg = {0};
	uint32_t  last_msg3 = all_msg3_rbs.start_;
	const int mcs = 0, max_harq_msg3_retx = 4;
	slot_cfg.idx = count_idx(&msg3_slot);

	rar_t rar_out = {0}; //对应msg2 rar_out[i] ==> rar_out[i].grants[0~len] 对应msg3集合
	for (i = 0; i < pending_rachs->len; i++) { 
		dl_sched_rar_info_t *grant = pending_rachs[i];
		slot_ue *slot_u = slot_ue_find_by_rnti(grant->temp_crnti, grant->cc);

		// Generate RAR grant
		msg3_grant_t rar_grant = {0};
		rar_grant.data	= grant;
		fill_dci_ul_from_cfg(bwp_alloc->cfg, &rar_grant.msg3_dci);
		// Fill Msg3 DCI context(pdcch_ul)
		// msg3上行授权调度， ul_grant包含在mac rar中(27 bit)
		rar_grant.msg3_dci.ctx.coreset_id = pdcch->dci.ctx.coreset_id;
		rar_grant.msg3_dci.ctx.rnti_type  = srsran_rnti_type_tc;
		rar_grant.msg3_dci.ctx.rnti 	  = slot_u->ue->rnti;
		rar_grant.msg3_dci.ctx.ss_type	  = srsran_search_space_type_rar;
		rar_grant.msg3_dci.ctx.format	  = srsran_dci_format_nr_rar;

		// Allocate Msg3 PUSCH allocation
		prb_interval msg3_interv = {0};
		prb_interval_init(&msg3_interv, last_msg3, last_msg3 + msg3_nof_prbs);
		prb_grant grap_msg3 = prb_grant_interval_init(msg3_interv);
	
		last_msg3 += msg3_nof_prbs;
		pusch_t *pusch = pusch_allocator_alloc_pusch_unchecked(&bwp_msg3_slot->puschs, &grap_msg3, &rar_grant.msg3_dci);

		// Allocate UL HARQ
		slot_u->h_ul = find_empty_ul_harq(&slot_u->ue->harq_ent);
		ASSERT_IF_NOT(slot_u.h_ul != NULL, "[%5u]Failed to allocate Msg3", GET_RSLOT_ID(bwp_alloc->pdcch_slot));
		bool success = ul_harq_proc_new_tx(slot_u.h_ul, msg3_slot, msg3_interv, mcs, max_harq_msg3_retx, rar_grant.msg3_dci);
		ASSERT_IF_NOT(success, "[%5u]Failed to allocate Msg3", GET_RSLOT_ID(bwp_alloc->pdcch_slot));

		// Generate PUSCH content(pusch out)
		success = get_pusch_cfg(ue_carrier_params_phy(&slot_u->ue->bwp_cfg), &slot_cfg, &rar_grant.msg3_dci, &pusch->sch);
		ASSERT_IF_NOT(success, "[%5u]Error converting DCI to PUSCH grant", GET_RSLOT_ID(bwp_alloc->pdcch_slot));
		pusch->sch.grant.tb[0].softbuffer.rx = &slot_u->h_ul->softbuffer->buffer;
		ul_harq_proc_set_tbs(slot_u->h_ul, pusch->sch.grant.tb[0].tbs);//set harq tbs,并且清空soft buffer

		cvector_push_back(rar_out.grants, rar_grant);

	}

	cvector_push_back(bwp_pdcch_slot->dl.rar, rar_out);

	return (alloc_result)success;
}


static alloc_result verify_uci_space(const bwp_slot_grid *uci_grid)
{
	if (MAX_GRANTS == cvector_size(uci_grid->pending_acks)) {
		oset_warn("SCHED: No space for ACK.");
		return (alloc_result)no_grant_space;
	}
	return (alloc_result)success;
}


// ue is the UE (1 only) that will be allocated
// func computes the grant allocation for this UE
alloc_result bwp_slot_allocator_alloc_pdsch(bwp_slot_allocator *bwp_alloc,
														slot_ue *slot_u,
														uint32_t ss_id,
														prb_grant *dl_grant)
{
	static const uint32_t 			  aggr_idx	= 2;
	static const srsran_dci_format_nr_t dci_fmt	= srsran_dci_format_nr_1_0;
	static const srsran_rnti_type_t	  rnti_type = srsran_rnti_type_c; //c-rnti

	bwp_slot_grid *bwp_pdcch_slot = get_slot_grid(bwp_alloc, slot_u->pdcch_slot);
	bwp_slot_grid *bwp_pdsch_slot = get_slot_grid(bwp_alloc, slot_u->pdsch_slot);
	bwp_slot_grid *bwp_uci_slot   = get_slot_grid(bwp_alloc, slot_u->uci_slot);//k1 ack
	// UCI(Uplink Control Information)是由PUCCH承载的上行控制信息；与DCI不同,UCI可以根据情况由PUCCH或PUSCH承载，而DCI只能由PDCCH承载。

	// Verify there is space in PDSCH//确认是否有空间
	alloc_result ret = pdsch_allocator_is_ue_grant_valid(bwp_alloc, &slot_u->ue->bwp_cfg, ss_id, dci_fmt, dl_grant);
	if (ret != (alloc_result)success) {
		return ret;
	}

	//uci传递ack信息，预先将ack 上行资源占用
	alloc_result result = verify_uci_space(bwp_uci_slot);
	if (result != (alloc_result)success) {
		return result;
	}

	if (slot_u->h_dl == NULL) {
		oset_warn("[%5u]SCHED: Trying to allocate rnti=0x%x with no available DL HARQs", GET_RSLOT_ID(bwp_alloc->pdcch_slot), slot_u->ue->rnti);
		return (alloc_result)no_rnti_opportunity;
	}

	if (!cvector_empty(bwp_pdcch_slot->dl.phy.ssb)) {
		// TODO: support concurrent PDSCH and SSB
		// 尚不支持并发PDSCH和SSB
		oset_debug("[%5u]SCHED: skipping RAR allocation. Cause: concurrent PDSCH and SSB not yet supported", GET_RSLOT_ID(bwp_alloc->pdcch_slot));
		return (alloc_result)no_sch_space;
	}

	// Check space in PUCCH/PUSCH for UCI
	// TODO

	// Find space and allocate PDCCH
	pdcch_dl_alloc_result pdcch_result = bwp_pdcch_allocator_alloc_dl_pdcch(&bwp_pdcch_slot->pdcchs, rnti_type, ss_id, aggr_idx, &slot_u->ue->bwp_cfg);
	if (!pdcch_result.has_val) {
		// Could not find space in PDCCH
		return pdcch_result.res.unexpected;
	}

	//dai针对tdd1~6模式(对于fdd和tdd0上行一个子帧对应一个下行子帧ACK,不需要)
	pdcch_dl_t *pdcch 	   = pdcch_result.res.val;
	pdcch->dci_cfg 		   = ue_carrier_params_get_dci_cfg(&slot_u->ue->bwp_cfg);
	pdcch->dci.pucch_resource = 0;//PUCCH resource indicator //??? todo不同的ue分配的应该不一样，时频区分
	// 参考oran： reserve_next_harq_res_available函数
	// 公共PUCCH：PUCCH resource indicator计算rPUCCH, 用来计算uci频域信息
	// 专用PUCCH：通过Ouci确定resourceSET,PUCCH resource indicator选择SET中资源resource

	harq_ack_t *p = NULL;
	cvector_for_each_in(p, bwp_uci_slot->pending_acks){
		if(p->res.rnti == slot_u->ue->rnti){
			pdcch->dci.dai++;
		}
	}
	pdcch->dci.dai % = 4;// MAX_GRANTS

	// Allocate PDSCH
	pdsch_t *pdsch = pdsch_allocator_alloc_ue_pdsch_unchecked(&bwp_pdcch_slot->pdschs, ss_id, dci_fmt, dl_grant, &slot_u->ue->bwp_cfg, &pdcch->dci);

	// Select MCS and Allocate HARQ
	int mcs = ue_carrier_params_fixed_pdsch_mcs(&slot_u->ue->bwp_cfg);//cnf默认配置mcs值
	const static int min_MCS_ccch = 4;
	if (empty(slot_u->h_dl->proc.tb)) {
		if (mcs < 0) {
			//AMC(mcs_table暂时好像使用默认值64qam)
			mcs = srsran_ra_nr_cqi_to_mcs(/* cqi */ dl_cqi(slot_u),
										/* cqi_table_idx */ slot_u->ue->bwp_cfg.cfg_.phy_cfg.csi.reports[0].cqi_table,
										/* mcs_table */ pdsch->sch.sch_cfg.mcs_table,
										/* dci_format */ pdcch->dci.ctx.format,
										/* search_space_type*/ pdcch->dci.ctx.ss_type,
										/* rnti_type */ rnti_type);
			if (mcs < 0) {
				oset_warn("[%5u]SCHED: UE rnti=0x%x reported CQI=0 - Using lowest MCS=0", GET_RSLOT_ID(bwp_alloc->pdcch_slot), slot_u->ue->rnti);
				mcs = 0;
			}
		}
		// Overwrite MCS if there are pending bytes for LCID. The optimal way would be to verify that there are pending
		// bytes and that the MAC SDU for CCCH gets segmented. But since the event of segmentation happens at most a couple
		// of times (e.g., to send msg4/RRCSetup), we opt for the less optimal but simpler approach.
		if (get_pending_bytes(slot_u, (nr_lcid_sch_t)CCCH) && mcs < min_MCS_ccch) {
			mcs = min_MCS_ccch;
			oset_info("[%5u]SCHED: MCS increased to min value %d to allocate SRB0/CCCH for rnti=0x%x", GET_RSLOT_ID(bwp_alloc->pdcch_slot), min_MCS_ccch, slot_u->ue->rnti);
		}
		bool success = dl_harq_proc_new_tx(slot_u->h_dl, slot_u->pdsch_slot, slot_u->uci_slot, dl_grant, mcs, slot_u->ue->bwp_cfg.cfg_->maxharq_tx, &pdcch->dci);
		ASSERT_IF_NOT(success, "Failed to allocate DL HARQ");
	} else {
		bool success = dl_harq_proc_new_retx(slot_u->h_dl, slot_u->pdsch_slot, slot_u->uci_slot, dl_grant, pdcch.dci);
		mcs 		 = MCS(slot_u->h_dl->proc.tb);//采用之前的mcs,重传采用非自适应
		ASSERT_IF_NOT(success, "Failed to allocate DL HARQ retx");
	}

	srsran_slot_cfg_t slot_cfg = {0};
	slot_cfg.idx = count_idx(&slot_u->pdsch_slot);
	// Value 0.95 is from TS 38.214 v15.14.00, Section 5.1.3, page 17
	// 码率算上crc,为真实码率
	const static float max_R = 0.95;
	double R_prime = 0;
	// The purpose of the internal loop is to decrease the MCS if the effective coderate is too high. This loop
	// only affects the high MCS values
	// 如果有效码率太高,内部环路的目的是减小MCS,这个循环仅影响高MCS值
	while (true) {
		// Generate PDSCH
		bool success = get_pdsch_cfg(ue_carrier_params_phy(&slot_u->ue->bwp_cfg), &slot_cfg, &pdcch->dci, &pdsch->sch);
		ASSERT_IF_NOT(success, "Error converting DCI to grant");
		if (nof_retx(&slot_u->h_dl->proc) != 0) {
			//重传tbs不变
			ASSERT_IF_NOT(pdsch->sch.grant.tb[0].tbs == (int)TBS(slot_u->h_dl->proc.tb), "The TBS did not remain constant in retx");
		}
		R_prime = pdsch->sch.grant.tb[0].R_prime;

		// 重传码率不变
		// CCCH码率保障
		if (nof_retx(&slot_u->h_dl->proc) > 0 || R_prime < max_R || mcs <= 0 || (get_pending_bytes(slot_u, (nr_lcid_sch_t)CCCH) && mcs < min_MCS_ccch)) {
			break;
		}
		// Decrease MCS if first tx and rate is too high 如果第一次tx和速率过高，则降低MCS
		mcs--;
		pdcch->dci.mcs = mcs;
	}
	if (R_prime >= max_R && mcs == 0) {
		oset_warn("Couldn't find mcs that leads to R<0.95");
	}

	harq_proc_set_mcs(&slot_u->h_dl->proc, mcs);
	// set HARQ TBS,不会清空soft buffer
	harq_proc_set_tbs(&slot_u->h_dl->proc, pdsch->sch.grant.tb[0].tbs);
	// 软合并buffer地址，在物理层中使用
	pdsch->sch.grant.tb[0].softbuffer.tx = &slot_u->h_dl->softbuffer->buffer;
	// buffer地址，赋值需要下行传输的TB数据源 (从RLC获取,新传harq中会清空), 重传数据是上一次的
	pdsch->data[0] = slot_u->h_dl->pdu;

	// Select scheduled LCIDs and update UE buffer state
	dl_pdu_t dl_pdu_data = {0};
	// NOTE: ue.h_dl->tbs() has to be converted from bits to bytes
	bool segmented_ccch_pdu = build_pdu(slot_u, TBS(slot_u->h_dl->proc.tb)/8, &dl_pdu_data);
	if (false == segmented_ccch_pdu) {
		oset_error("[%5u]SCHED: Insufficient resources to allocate SRB0/CCCH for rnti=0x%x", GET_RSLOT_ID(bwp_alloc->pdcch_slot), min_MCS_ccch, ue->rnti);
	}
	cvector_push_back(bwp_pdsch_slot->dl.data, dl_pdu_data);

	// Generate PUCCH
	harq_ack_t pending_ack = {0};
	pending_ack.phy_cfg = ue_carrier_params_phy(&slot_u->ue->bwp_cfg);
	//配置n+K1 slot中的ack资源
	bool success = get_pdsch_ack_resource(pending_ack.phy_cfg, &pdcch->dci, &pending_ack.res);
	ASSERT_IF_NOT(success, "Error getting ack resource");
	cvector_push_back(bwp_uci_slot->pending_acks, pending_ack);
	
	return (alloc_result)success;
}

static bool is_common_ss_allowed(srsran_rnti_type_t rnti_type, srsran_search_space_type_t ss_type)
{
	switch (rnti_type) {
		case srsran_rnti_type_c:
			return ss_type == srsran_search_space_type_common_1 || ss_type == srsran_search_space_type_common_3;
		case srsran_rnti_type_tc:
		case srsran_rnti_type_ra:
			// TODO: Fix UE config to not use common3
			return ss_type == srsran_search_space_type_common_1 || ss_type == srsran_search_space_type_common_3;
		case srsran_rnti_type_si:
			return ss_type == srsran_search_space_type_common_0;
		default:
			// TODO: Remaining cases
			break;
	}
	return false;
};

static bool contains_dci_fmt(uint32_t aggr_idx, srsran_dci_format_nr_t prio_dcis[2], srsran_search_space_t *ss) {
	if (ss->nof_candidates[aggr_idx] > 0 && ss->nof_formats > 0) {
		for (uint32_t i = 0; i < 2; ++i) {
			for (uint32_t j = 0; j < ss->nof_formats; ++j) {
				if (ss->formats[j] == prio_dcis[i]) {
					return true;
				}
			}
		}
	}
	return false;
};

static candidate_ss_list_t find_ss(srsran_pdcch_cfg_nr_t         *pdcch,
									uint32_t					aggr_idx,
									srsran_rnti_type_t			rnti_type,
									srsran_dci_format_nr_t      prio_dcis[2])
{
	candidate_ss_list_t ret = NULL;
	int i = 0;

	if (rnti_type == srsran_rnti_type_c) {
		for (i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; ++i) {
			if(pdcch->search_space_present[i]){
				srsran_search_space_t *ss = pdcch->search_space[i];
				if (ss->type == srsran_search_space_type_ue && contains_dci_fmt(aggr_idx, prio_dcis, ss)) {
					cvector_push_back(ret, ss);
				}
			}
		}
	}

	for (i = 0; i < SRSRAN_UE_DL_NR_MAX_NOF_SEARCH_SPACE; ++i) {
		if(pdcch->search_space_present[i]){
			srsran_search_space_t *ss = pdcch->search_space[i];
			if (is_common_ss_allowed(rnti_type, ss->type) && contains_dci_fmt(aggr_idx, prio_dcis, ss)) {
				cvector_push_back(ret, ss);
			}
		}
	}
	return ret;
}


alloc_result bwp_slot_allocator_alloc_pusch(bwp_slot_allocator *bwp_alloc,
														slot_ue   *slot_u, 
														prb_grant *ul_grant)
{
	static const uint32_t 		        aggr_idx = 2;
	static const srsran_dci_format_nr_t dci_fmt_list[2] = {srsran_dci_format_nr_0_1, srsran_dci_format_nr_0_0};
	static const srsran_rnti_type_t		rnti_type = srsran_rnti_type_c;

	bwp_slot_grid *bwp_pdcch_slot = get_slot_grid(bwp_alloc, slot_u->pdcch_slot);//eg dci0_X 上行调度命令
	bwp_slot_grid *bwp_pusch_slot = get_slot_grid(bwp_alloc, slot_u->pusch_slot);//k2 //eg msg4/bsr/上行数据grant预申请

	//对于上行数据发送，如果需要重传，基站不向UE发送ACK/NACK信息，而是直接调度UE进行数据重传(PDCCH调度,通过NDI判断新传、重传)

	if (slot_u->h_ul == NULL) {
		oset_warn("[%5u]SCHED: Trying to allocate rnti=0x%x with no available UL HARQs", GET_RSLOT_ID(bwp_alloc->pdcch_slot), slot_u->ue->rnti);
		return (alloc_result)no_rnti_opportunity;
	}

	// Choose SearchSpace + DCI format
	phy_cfg_nr_t *phy_cfg = ue_carrier_params_phy(&slot_u->ue->bwp_cfg);
	candidate_ss_list_t ss_candidates = find_ss(&phy_cfg->pdcch, aggr_idx, rnti_type, dci_fmt_list);
	if (cvector_empty(ss_candidates)) {
		// Could not find space in PDCCH
		oset_warn("[%5u]SCHED: No PDCCH candidates for any of the rnti=0x%x search spaces", GET_RSLOT_ID(bwp_alloc->pdcch_slot), slot_u->ue->rnti);
		return (alloc_result)no_cch_space;
	}
	const srsran_search_space_t *ss = *ss_candidates[0];
	cvector_free(ss_candidates);

	// Verify if PUSCH allocation is valid
	alloc_result ret = pusch_allocator_is_grant_valid(&bwp_pusch_slot->puschs, ss->type, ul_grant, true);
	if (ret != (alloc_result)success) {
		return ret;
	}

	pdcch_ul_alloc_result pdcch_result = bwp_pdcch_allocator_alloc_ul_pdcch(&bwp_pdcch_slot->pdcchs, ss->id, aggr_idx, &slot_u->ue->bwp_cfg);
	if (!pdcch_result.has_val) {
		// Could not find space in PDCCH
		return pdcch_result.res.unexpected;
	}

	// Allocation Successful
	pdcch_ul_t *pdcch = pdcch_result.res.val;
	pdcch->dci_cfg 	  = ue_carrier_params_get_dci_cfg(&slot_u->ue->bwp_cfg);

	// Allocate PUSCH
	pusch_t *pusch = pusch_allocator_alloc_pusch_unchecked(&bwp_pusch_slot->puschs, ul_grant, &pdcch->dci);

	if (empty(slot_u->h_ul->proc.tb)) {
		int  mcs	 = ue_carrier_params_fixed_pusch_mcs(&slot_u->ue->bwp_cfg);//配置上行mcs，配置参数中获取
		bool success = ul_harq_proc_new_tx(slot_u->h_ul, slot_u->pusch_slot, ul_grant, mcs, slot_u->ue->bwp_cfg.cfg_->maxharq_tx, &pdcch->dci);//bool ul_harq_proc::new_tx
		ASSERT_IF_NOT(success, "Failed to allocate UL HARQ");
	} else {
		// 重传时重新下发dci0-X,分配上行资源
		bool success = dl_harq_proc_new_retx(slot_u->h_dl, slot_u->pusch_slot, ul_grant, &pdcch->dci);//UE通过每次DCI0-0中NDI判断上行ack是否成功,  PUSCH为上行分配好预留的mcs和资源
		ASSERT_IF_NOT(success, "Failed to allocate UL HARQ retx");
	}

	// Generate PUSCH content
	srsran_slot_cfg_t slot_cfg = {0};
	slot_cfg.idx = count_idx(&slot_u->pusch_slot);
	pusch->pid    = slot_u->h_ul->proc.pid;
	bool success = get_pusch_cfg(ue_carrier_params_phy(&slot_u->ue->bwp_cfg), &slot_cfg, &pdcch->dci, &pusch->sch);//申请上行pusch资源和相关配置，计算tbs相关
	ASSERT_IF_NOT(success, "Error converting DCI to PUSCH grant");
	pusch->sch.grant.tb[0].softbuffer.rx = &slot_u->h_ul->softbuffer->buffer;
	if (nof_retx(&slot_u->h_ul->proc) == 0) {
		// set HARQ TBS,不会清空soft buffer
		harq_proc_set_tbs(&slot_u->h_ul->proc, pusch->sch.grant.tb[0].tbs);
	} else {
		ASSERT_IF_NOT(pusch->sch.grant.tb[0].tbs == (int)TBS(slot_u->h_ul->proc.tb), "The TBS did not remain constant in retx");
	}

	return (alloc_result)success;
}

