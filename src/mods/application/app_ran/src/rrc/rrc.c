/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "lib/common/phy_cfg_nr_default.h"
#include "rrc/rrc_cell_asn_fill.h"
#include "rrc/rrc_cell_asn_fill_inner.h"
#include "rrc/rrc.h"


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc"


static rrc_manager_t rrc_manager = {0};

rrc_manager_t *rrc_manager_self(void)
{
	return &rrc_manager;
}

static void rrc_manager_init(void)
{
	rrc_manager.app_pool = gnb_manager_self()->app_pool;
	oset_apr_mutex_init(&rrc_manager.mutex, OSET_MUTEX_NESTED, rrc_manager.app_pool);
	oset_apr_thread_cond_create(&rrc_manager.cond, rrc_manager.app_pool);

}

static void rrc_manager_destory(void)
{
	oset_apr_mutex_destroy(rrc_manager.mutex);
	oset_apr_thread_cond_destroy(rrc_manager.cond);
	rrc_manager.app_pool = NULL; /*app_pool release by openset process*/
}

int32_t rrc_generate_sibs(uint32_t cc)
{
	if (!rrc_manager.cfg.is_standalone) {
		return OSET_OK;
	}

	// SIB1 packing
	du_cell_config  *du_cell = byn_array_get_data(&rrc_manager.du_cfg.cells, cc);
	oset_assert(du_cell);

	//byn_array_add(&rrc_manager.cell_ctxt->sibs, du_cell->sib1);
	byn_array_add(&rrc_manager.cell_ctxt->sib_buffer, du_cell->packed_sib1);

	//struct ASN_RRC_SIB1_t *sib1 = du_cell->sib1.message.choice.c1->choice.systemInformationBlockType1;
	//const uint32_t nof_messages = sib1->si_SchedulingInfo->schedulingInfoList.list.count ? sib1->si_SchedulingInfo->schedulingInfoList.list.count : 0;
	
	// SI messages packing Todo parse_sib2()
	ASN_RRC_BCCH_DL_SCH_Message_t *sib_pdu = CALLOC(1,sizeof(ASN_RRC_BCCH_DL_SCH_Message_t));
	sib_pdu->message.present = ASN_RRC_BCCH_DL_SCH_MessageType_PR_c1;
	sib_pdu->message.choice.c1 = CALLOC(1,sizeof(*sib2_pdu->message.choice.c1));
	sib_pdu->message.choice.c1->present = ASN_RRC_BCCH_DL_SCH_MessageType__c1_PR_systemInformation;
	sib_pdu->message.choice.c1->choice.systemInformation = CALLOC(1,sizeof(ASN_RRC_SystemInformation_t));
	asn1cCalloc(sib_pdu->message.choice.c1->choice.systemInformation->criticalExtensions.choice, sibs);
	//sib2
	asn1cSequenceAdd(sibs->sib_TypeAndInfo.list, SystemInformation_IEs__sib_TypeAndInfo__Member, sib_msg);
	sib_msg->present = ASN_RRC_SystemInformation_IEs__sib_TypeAndInfo__Member_PR_sib2;
	sib_msg->choice.sib2 = CALLOC(1,sizeof(struct ASN_RRC_SIB2));
	sib_msg->choice.sib2->cellReselectionInfoCommon.q_Hyst = ASN_RRC_SIB2__cellReselectionInfoCommon__q_Hyst_dB5;

	oset_pkbuf_t *packed_sib2 = oset_rrc_encode(&asn_DEF_ASN_RRC_BCCH_DL_SCH_Message, sib_pdu, asn_struct_free_all);
	//oset_free(cell->packed_sib2);
	//byn_array_add(&rrc_manager.cell_ctxt->sibs, sib_pdu);
	byn_array_add(&rrc_manager.cell_ctxt->sib_buffer, packed_sib2);
	return OSET_OK;
}

void rrc_config_phy(uint32_t cc)
{
	common_cfg_t *common_cfg = phy_manager_self()->common_cfg;
	oset_assert(common_cfg);
	rrc_cell_cfg_nr_t *rrc_cell_cfg = oset_list2_find(rrc_manager.cfg.cell_list, cc)->data;
	oset_assert(rrc_cell_cfg);
	du_cell_config  *du_cell = byn_array_get_data(&rrc_manager.du_cfg.cells, cc);
	oset_assert(du_cell);
	//fill carrier
	common_cfg->carrier = rrc_cell_cfg->phy_cell.carrier;
	//fill pucch
	fill_phy_pdcch_cfg_common(du_cell, rrc_cell_cfg, &common_cfg->pdcch);
	bool ret = fill_phy_pdcch_cfg(rrc_cell_cfg, &common_cfg->pdcch);
	ASSERT_IF_NOT(ret, "Failed to generate Dedicated PDCCH config");
	make_phy_rach_cfg(rrc_cell_cfg, &common_cfg->prach);
	common_cfg->duplex_mode = rrc_cell_cfg->duplex_mode;
	ret	= fill_phy_ssb_cfg(rrc_cell_cfg, &common_cfg->ssb);
	ASSERT_IF_NOT(ret, "Failed to generate PHY config");
}

static void get_default_cell_cfg(phy_cfg_nr_t *phy_cfg, sched_nr_cell_cfg_t *cell_cfg)
{
	cell_cfg->pci                    = phy_cfg->carrier.pci;
	cell_cfg->dl_center_frequency_hz = phy_cfg->carrier.dl_center_frequency_hz;
	cell_cfg->ul_center_frequency_hz = phy_cfg->carrier.ul_center_frequency_hz;
	cell_cfg->ssb_center_freq_hz     = phy_cfg->carrier.ssb_center_freq_hz;
	byn_array_set_bounded(&cell_cfg->dl_cfg_common.freq_info_dl.scs_specific_carrier_list, 1);
	struct scs_specific_carrier_s *ss_carrier = oset_core_alloc(rrc_manager.app_pool, sizeof(struct scs_specific_carrier_s));
	ss_carrier->subcarrier_spacing = (enum subcarrier_spacing_e)phy_cfg->carrier.scs;
	ss_carrier->offset_to_carrier = phy_cfg->carrier.offset_to_carrier;
	byn_array_add(&cell_cfg->dl_cfg_common.freq_info_dl.scs_specific_carrier_list, ss_carrier);

	cell_cfg->dl_cfg_common.init_dl_bwp.generic_params.subcarrier_spacing = (enum subcarrier_spacing_e)phy_cfg->carrier.scs;
	cell_cfg->ul_cfg_common.init_ul_bwp.rach_cfg_common_present = true;
	cell_cfg->ul_cfg_common.init_ul_bwp.rach_cfg_common.type_ = setup;
	fill_rach_cfg_common_inner(&phy_cfg->prach, &cell_cfg->ul_cfg_common.init_ul_bwp.rach_cfg_common.c);//fill_rach_cfg_common(const srsran_prach_cfg_t& prach_cfg, asn1::rrc_nr::rach_cfg_common_s& asn1_type)
	cell_cfg->dl_cell_nof_prb    = phy_cfg->carrier.nof_prb;
	cell_cfg->nof_layers         = phy_cfg->carrier.max_mimo_layers;
	cell_cfg->ssb_periodicity_ms = phy_cfg->ssb.periodicity_ms;
	for (uint32_t i = 0; i < 8; ++i) {//8bit
		bit_set(&cell_cfg->ssb_positions_in_burst.in_one_group, i, phy_cfg->ssb.position_in_burst[i]);
	}
	// TODO: phy_cfg.ssb_positions_in_burst.group_presence_present
	cell_cfg->dmrs_type_a_position       = (enum dmrs_type_a_position_e_)pos2;
	cell_cfg->ssb_scs                    = (enum subcarrier_spacing_e )phy_cfg->ssb.scs;
	cell_cfg->pdcch_cfg_sib1.ctrl_res_set_zero = 0;
	cell_cfg->pdcch_cfg_sib1.search_space_zero = 0;

	cell_cfg->bwps[4] = {0};
	cell_cfg->bwps[0].pdcch    = phy_cfg->pdcch;
	cell_cfg->bwps[0].pdsch    = phy_cfg->pdsch;
	cell_cfg->bwps[0].pusch    = phy_cfg->pusch;
	cell_cfg->bwps[0].pucch    = phy_cfg->pucch;
	cell_cfg->bwps[0].harq_ack = phy_cfg->harq_ack;
	cell_cfg->bwps[0].rb_width = phy_cfg->carrier.nof_prb;

	if (phy_cfg->duplex.mode == SRSRAN_DUPLEX_MODE_TDD) {
		cell_cfg->tdd_ul_dl_cfg_common = {0};
		ASSERT_IF_NOT(make_phy_tdd_cfg(phy_cfg->duplex, srsran_subcarrier_spacing_15kHz, &cell_cfg->tdd_ul_dl_cfg_common), "Failed to generate TDD config");
	}
}

void rrc_config_mac(uint32_t cc)
{
	rrc_cell_cfg_nr_t *rrc_cell_cfg = oset_list2_find(rrc_manager.cfg.cell_list, cc)->data;
	oset_assert(rrc_cell_cfg);
	du_cell_config  *du_cell = byn_array_get_data(&rrc_manager.du_cfg.cells, cc);
	oset_assert(du_cell);

	// Fill MAC scheduler configuration for SIBs
	// TODO: use parsed cell NR cfg configuration
	//default
	reference_cfg_t ref_args = {.carrier = R_CARRIER_CUSTOM_10MHZ,
								.duplex  = R_DUPLEX_TDD_CUSTOM_6_4,
								.pdcch   = R_PDCCH_CUSTOM_COMMON_SS,
								.pdsch   = R_PDSCH_DEFAULT,
								.pusch   = R_PUSCH_DEFAULT,
								.pucch   = R_PUCCH_CUSTOM_ONE,
								.harq    = R_HARQ_AUTO,
								.prach   = R_PRACH_DEFAULT_LTE,};
	ref_args.duplex = rrc_cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_TDD
													? R_DUPLEX_TDD_CUSTOM_6_4
													: R_DUPLEX_FDD;

	//default phy cfg
	phy_cfg_nr_t  phy_cfg = {0};
	phy_cfg_nr_default_init(&ref_args, &phy_cfg);
	//A_DYN_ARRAY_OF(sched_nr_cell_cfg_t) sched_cells_cfg = {0};
	//byn_array_add(&sched_cells_cfg, get_default_cell_cfg(&phy_cfg));
	//sched_nr_cell_cfg_t  *cell = sched_cells_cfg[cc];
	sched_nr_cell_cfg_t  *cell = oset_core_alloc(rrc_manager.app_pool, sizeof(sched_nr_cell_cfg_t));
	get_default_cell_cfg(&phy_cfg, cell);
	
	// Derive cell config from rrc_nr_cfg_t
	fill_phy_pdcch_cfg_common(du_cell, rrc_cell_cfg, &cell->bwps[0].pdcch);
	bool ret = fill_phy_pdcch_cfg(rrc_cell_cfg, &cell->bwps[0].pdcch);
	ASSERT_IF_NOT(ret, "Failed to generate Dedicated PDCCH config");
	cell->pci                    = rrc_cell_cfg->phy_cell.carrier.pci;
	cell->nof_layers             = rrc_cell_cfg->phy_cell.carrier.max_mimo_layers;
	cell->dl_cell_nof_prb        = rrc_cell_cfg->phy_cell.carrier.nof_prb;
	cell->ul_cell_nof_prb        = rrc_cell_cfg->phy_cell.carrier.nof_prb;
	cell->dl_center_frequency_hz = rrc_cell_cfg->phy_cell.carrier.dl_center_frequency_hz;
	cell->ul_center_frequency_hz = rrc_cell_cfg->phy_cell.carrier.ul_center_frequency_hz;
	cell->ssb_center_freq_hz     = rrc_cell_cfg->phy_cell.carrier.ssb_center_freq_hz;
	cell->dmrs_type_a_position   = (enum dmrs_type_a_position_e_)pos2;//ASN_RRC_MIB__dmrs_TypeA_Position_pos2
	cell->pdcch_cfg_sib1         = du_cell->mib.pdcch_cfg_sib1;
	if (du_cfg->cell(cc).serv_cell_cfg_common().tdd_ul_dl_cfg_common_present) {
	cell.tdd_ul_dl_cfg_common.emplace(du_cfg->cell(cc).serv_cell_cfg_common().tdd_ul_dl_cfg_common);
	}
	cell.dl_cfg_common       = du_cfg->cell(cc).serv_cell_cfg_common().dl_cfg_common;
	cell.ul_cfg_common       = du_cfg->cell(cc).serv_cell_cfg_common().ul_cfg_common;
	cell.ss_pbch_block_power = du_cfg->cell(cc).serv_cell_cfg_common().ss_pbch_block_pwr;
	bool valid_cfg = make_pdsch_cfg_from_serv_cell(cell_ctxt->master_cell_group->sp_cell_cfg.sp_cell_cfg_ded,
	                                                     &cell->bwps[0].pdsch);
	srsran_assert(valid_cfg, "Invalid NR cell configuration.");
	cell.ssb_positions_in_burst = du_cfg->cell(cc).serv_cell_cfg_common().ssb_positions_in_burst;
	cell.ssb_periodicity_ms     = du_cfg->cell(cc).serv_cell_cfg_common().ssb_periodicity_serving_cell.to_number();
	cell.ssb_scs                = rrc_cell_cfg->phy_cell.carrier.scs;
	cell.ssb_offset             = du_cell->mib.ssb_subcarrier_offset;
	if (not cfg.is_standalone) {
	const serving_cell_cfg_common_s& serv_cell =
	    cell_ctxt->master_cell_group->sp_cell_cfg.recfg_with_sync.sp_cell_cfg_common;
	// Derive cell config from ASN1
	cell.ssb_scs = serv_cell.ssb_subcarrier_spacing;
	}

	// Set SIB1 and SI messages
	cell.sibs.resize(cell_ctxt->sib_buffer.size());
	for (uint32_t i = 0; i < cell_ctxt->sib_buffer.size(); i++) {
		cell.sibs[i].len = cell_ctxt->sib_buffer[i]->N_bytes;
		if (i == 0) {
		  cell.sibs[i].period_rf       = 16; // SIB1 is always 16 rf
		  cell.sibs[i].si_window_slots = 160;
		} else {
		  cell.sibs[i].period_rf = du_cfg->cell(0).sib1.si_sched_info.sched_info_list[i - 1].si_periodicity.to_number();
		  cell.sibs[i].si_window_slots = du_cfg->cell(0).sib1.si_sched_info.si_win_len.to_number();
		}
	}

	// Configure MAC/scheduler
	mac->cell_cfg(sched_cells_cfg);//int mac_nr::cell_cfg(const std::vector<srsenb::sched_nr_cell_cfg_t>& nr_cells)

	// Make default UE PHY config object
	rrc_manager.cell_ctxt->default_phy_ue_cfg_nr = get_common_ue_phy_cfg(cell);
	
	byn_array_empty(&cell->dl_cfg_common.freq_info_dl.scs_specific_carrier_list);
}


static int rrc_init(void)
{
    oset_lnode2_t *lnode = NULL;
	int ret = OSET_ERROR;

	rrc_manager_init();
	//todo
	rrc_manager.cfg = &gnb_manager_self()->rrc_nr_cfg;

	// log cell configs
	oset_list2_for_each(rrc_manager.cfg.cell_list, lnode){
	    rrc_cell_cfg_nr_t *cell_tt =  (rrc_cell_cfg_nr_t *)lnode->data;
		oset_notice("Cell idx=%d, pci=%d, nr_dl_arfcn=%d, nr_ul_arfcn=%d, band=%d, duplex=%s, n_rb_dl=%d, ssb_arfcn=%d",
					cell_tt->cell_idx,
					cell_tt->phy_cell.carrier.pci,
					cell_tt->dl_arfcn,
					cell_tt->ul_arfcn,
					cell_tt->band,
					cell_tt->duplex_mode == SRSRAN_DUPLEX_MODE_FDD ? "FDD" : "TDD",
					cell_tt->phy_cell.carrier.nof_prb,
					cell_tt->ssb_absolute_freq_point);
	}

	oset_list2_for_each(rrc_manager.cfg.cell_list, lnode){
	    rrc_cell_cfg_nr_t *cell =  (rrc_cell_cfg_nr_t *)lnode->data;
		ret = du_config_manager_add_cell(cell);
		ASSERT_IF_NOT(ret == OSET_OK, "Failed to configure NR cell %d", cell->cell_idx);
	}

	rrc_manager.cell_ctxt = oset_core_alloc(rrc_manager.app_pool, sizeof(cell_ctxt_t));
	ret = fill_master_cell_cfg_from_enb_cfg(rrc_manager.cfg, 0, rrc_manager.cell_ctxt->master_cell_group);
	ASSERT_IF_NOT(ret == OSET_OK, "Failed to configure MasterCellGroup");

	// derived
	rrc_manager.slot_dur_ms = 1;

	if (rrc_generate_sibs(0) != OSET_OK) {
		oset_error("Couldn't generate SIB messages.");
		return OSET_ERROR;
	}

	rrc_config_phy(0); // if PHY is not yet initialized, config will be stored and applied on initialization
	rrc_config_mac(0);

	oset_info("Number of 5QI %d", rrc_manager.cfg.five_qi_cfg->count);
	/*for (const std::pair<const uint32_t, rrc_nr_cfg_five_qi_t>& five_qi_cfg : cfg.five_qi_cfg) {
	  logger.info("5QI configuration. 5QI=%d", five_qi_cfg.first);
	  if (logger.info.enabled()) {
		asn1::json_writer js{};
		five_qi_cfg.second.pdcp_cfg.to_json(js);
		logger.info("PDCP NR configuration: %s", js.to_string().c_str());
		js = {};
		five_qi_cfg.second.rlc_cfg.to_json(js);
		logger.info("RLC NR configuration: %s", js.to_string().c_str());
	  }
	}*/

	oset_info("NIA preference list: NIA%d, NIA%d, NIA%d",
				rrc_manager.cfg.nia_preference_list[0],
				rrc_manager.cfg.nia_preference_list[1],
				rrc_manager.cfg.nia_preference_list[2]);
	oset_info("NEA preference list: NEA%d, NEA%d, NEA%d",
				rrc_manager.cfg.nea_preference_list[0],
				rrc_manager.cfg.nea_preference_list[1],
				rrc_manager.cfg.nea_preference_list[2]);
	rrc_manager.running = true;

	return OSET_OK;
}

static int rrc_destory(void)
{	
	int i = 0;
	//free mib
	for(i = 0; i < byn_array_get_count(&rrc_manager.du_cfg.cells), i++){
		du_cell_config *cell = byn_array_get_data(&rrc_manager.du_cfg.cells, i);
		oset_free(cell->packed_mib);
		//oset_asn_free_all(cell->mib);
	}

	//free sibs
	for(i = 0; i < byn_array_get_count(&rrc_manager.cell_ctxt->sib_buffer), i++){
		oset_free(byn_array_get_data(&rrc_manager.cell_ctxt->sib_buffer, i));
	}
	//for(i = 0; i < byn_array_get_count(&rrc_manager.cell_ctxt->sibs), i++){
	//	oset_asn_free_all(byn_array_get_data(&rrc_manager.cell_ctxt->sibs, i));
	//}

	byn_array_empty(&rrc_manager.du_cfg.cells);
	byn_array_empty(&rrc_manager.cell_ctxt->sib_buffer);
	byn_array_empty(&rrc_manager.cell_ctxt->sibs);

	//todo
	rrc_manager_destory();
	return OSET_OK;
}

static void gnb_rrc_task_handle(msg_def_t *msg_p, uint32_t msg_l)
{
	oset_assert(msg_p);
	oset_assert(msg_l > 0);
	switch (RQUE_MSG_ID(msg_p))
	{	
		case 1111:
			/*info handle*/
			break;

		default:
			oset_error("Received unhandled message: %d:%s",  RQUE_MSG_ID(msg_p), RQUE_MSG_NAME(msg_p));
			break;
	}
}

void *gnb_rrc_task(oset_threadplus_t *thread, void *data)
{
	msg_def_t *received_msg = NULL;
	uint32_t length = 0;
	task_map_t *task = task_map_self(TASK_RRC);
	int rv = 0;

	rrc_init();
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Starting RRC layer thread");
	for ( ;; ){
		rv = oset_ring_queue_try_get(task->msg_queue, &received_msg, &length);
		if(rv != OSET_OK)
		{
			if (rv == OSET_DONE)
				break;

			if (rv == OSET_RETRY){
				continue;
			}
		}
		gnb_rrc_task_handle(received_msg, length);
		task_free_msg(RQUE_MSG_ORIGIN_ID(received_msg), received_msg);
		received_msg = NULL;
		length = 0;
	}
	rrc_destory();
}


