/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
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
	du_cell_config  *du_cell = byn_array_get_data(rrc_manager.du_cfg.cells, cc);
	oset_assert(du_cell);

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
	byn_array_add(&rrc_manager.cell_ctxt->sib_buffer, packed_sib2);

	return OSET_OK;
}

void rrc_config_phy(uint32_t cc)
{
	common_cfg_t *common_cfg = phy_manager_self()->common_cfg;
	oset_assert(common_cfg);
	rrc_cell_cfg_nr_t *cell_cfg = oset_list2_find(rrc_manager.cfg.cell_list, cc)->data;
	oset_assert(cell_cfg);
	du_cell_config  *du_cell = byn_array_get_data(rrc_manager.du_cfg.cells, cc);
	oset_assert(du_cell);
	//fill carrier
	common_cfg->carrier = cell_cfg->phy_cell.carrier;
	//fill pucch
	fill_phy_pdcch_cfg_common(du_cell, cell_cfg, &common_cfg->pdcch);

	bool ret = fill_phy_pdcch_cfg(cell_ctxt->master_cell_group->sp_cell_cfg.sp_cell_cfg_ded.init_dl_bwp.pdcch_cfg.setup(), &common_cfg.pdcch);
	srsran_assert(ret, "Failed to generate Dedicated PDCCH config");
	srsran::make_phy_rach_cfg(du_cfg->cell(0).serv_cell_cfg_common().ul_cfg_common.init_ul_bwp.rach_cfg_common.setup(),
	                        cfg.cell_list[0].duplex_mode,
	                        &common_cfg.prach);
	common_cfg.duplex_mode = cfg.cell_list[0].duplex_mode;
	ret                    = srsran::fill_phy_ssb_cfg(
	  cfg.cell_list[0].phy_cell.carrier, du_cfg->cell(0).serv_cell_cfg_common(), &common_cfg.ssb);
	srsran_assert(ret, "Failed to generate PHY config");
	if (phy->set_common_cfg(common_cfg) < SRSRAN_SUCCESS) {
		logger.error("Couldn't set common PHY config");
		return;
	}
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
	config_mac();

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
	//free mib sib1
	for(i = 0; i < byn_array_get_count(rrc_manager.du_cfg.cells), i++){
		du_cell_config *cell = byn_array_get_data(rrc_manager.du_cfg.cells, i);
		oset_free(cell->packed_mib);
	}

	//free sibs
	for(i = 0; i < byn_array_get_count(rrc_manager.cell_ctxt->sib_buffer), i++){
		oset_free(byn_array_get_data(rrc_manager.cell_ctxt->sib_buffer, i));
	}

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


