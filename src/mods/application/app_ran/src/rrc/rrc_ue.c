/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "rrc/rrc_ue.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc-ue"

void log_rx_pdu_fail(uint16_t           rnti,
                         uint32_t          lcid,
                         byte_buffer_t     *pdu,
                         const char        *cause_str)
{
	oset_error("Rx %s PDU, rnti=0x%x - Discarding. Cause: %s", get_rb_name(lcid), rnti, cause_str);
	oset_hex_print(OSET_LOG2_ERROR, pdu->msg, pdu->N_bytes)
}

void log_rrc_message(const char			   *source,
						const direction_t		dir,
						byte_buffer_t			*pdu,
						const char				*msg_type)

{
	 oset_info("%s - %s %s (%d Byte)", source, (dir == Rx) ? "Rx" : "Tx", msg_type, pdu->N_bytes);
	 oset_hex_print(OSET_LOG2_ERROR, pdu->msg, pdu->N_bytes);
}


static void log_rrc_ue_message(uint16_t rnti,
									 nr_srb              srb,
	                                 const direction_t   dir,
	                                 byte_buffer_t       *pdu,
	                                 const char          *msg_type)
{
	char strbuf[64] = {0};
	sprintf(strbuf, "rnti=0x%x, %s", rnti, get_srb_name(srb));
	log_rrc_message(strbuf, Tx, pdu, msg_type);
}


//todo
void activity_timer_expired(rrc_nr_ue *ue)
{
	oset_assert(ue);
	oset_info("Activity timer for rnti=0x%x expired", ue->rnti);

	switch (ue->type) {
		case MSG5_RX_TIMEOUT:
		case UE_INACTIVITY_TIMEOUT: {
		  ue->state = RRC_INACTIVE;
		  // Start NGAP Release UE context
		  parent->ngap->user_release_request(ue->rnti, asn1::ngap::cause_radio_network_opts::user_inactivity);

		  break;
		}
		case MSG3_RX_TIMEOUT: {
		  // MSG3 timeout, no need to notify NGAP or LTE stack. Just remove UE
		  ue->state = RRC_IDLE;
		  rrc_rem_user_info(ue->rnti);
		  break;
		}
		default:
		  // Unhandled activity timeout, just remove UE and log an error
		  rrc_rem_user_info(ue->rnti);
		  oset_error(
		      "Unhandled reason for activity timer expiration. rnti=0x%x, cause %d", ue->rnti, ue->type);
	}
}


static void set_activity_timer(rrc_nr_ue *ue)
{
	oset_assert(ue);
	uint32_t deadline_ms = 0;

	switch (ue->type) {
		case MSG3_RX_TIMEOUT:
		  // TODO: Retrieve the parameters from somewhere(RRC?) - Currently hardcoded to 100ms
		  deadline_ms = 100;
		  break;
		case MSG5_RX_TIMEOUT:
		  // TODO: Retrieve the parameters from somewhere(RRC?) - Currently hardcoded to 1s
		  deadline_ms = 5000;
		  break;
		case UE_INACTIVITY_TIMEOUT:
		  deadline_ms = rrc_manager_self()->cfg.inactivity_timeout_ms;
		  break;
		default:
		  oset_error("Unknown timeout type %d", ue->type);
		  return;
	}

	static const char* options[] = {"Msg3 reception", "UE inactivity", "Msg5 reception"};
	oset_debug("Setting timer for %s for rnti=0x%x to %dms", options[ue->type]), ue->rnti, deadline_ms);

	ue->activity_timer_deadline_ms = deadline_ms;

	if(ue->activity_timer){
		gnb_timer_delete(ue->activity_timer);
		ue->activity_timer = NULL;
	}
	ue->activity_timer = gnb_timer_add(gnb_manager_self()->app_timer, activity_timer_expired, ue);

	rrc_nr_ue_set_activity(ue, true);
	oset_debug("Activity registered for rnti=0x%x (timeout_value=%dms)", ue->rnti, deadline_ms);
}

static bool init_pucch(void)
{
	// TODO: Allocate PUCCH resources

	return true;
}

static int send_dl_ccch(rrc_nr_ue *ue, ASN_RRC_DL_CCCH_Message_t *dl_ccch_msg)
{
	// Allocate a new PDU buffer, pack the message and send to PDCP
	byte_buffer_t *pdu = oset_rrc_encode2(&asn_DEF_ASN_RRC_DL_CCCH_Message, dl_ccch_msg, asn_struct_free_context);
	if (pdu == NULL) {
		oset_error("Failed to send DL-CCCH");
		return OSET_ERROR;
	}
	char fmtbuf[64] = {0};
	sprintf(fmtbuf, "DL-CCCH.%s", #ASN_RRC_DL_CCCH_MessageType__c1_PR_rrcReject);
	log_rrc_ue_message(ue->rnti, srb0, Tx, pdu, fmtbuf);
	API_rlc_rrc_write_dl_sdu(ue->rnti, srb_to_lcid(srb0), pdu);

	oset_free(pdu);
	return OSET_OK;
}

static int update_rlc_bearers(rrc_nr_ue *ue, struct cell_group_cfg_s *cell_group_diff)
{
	// Release RLC radio bearers
	uint8_t *lcid = NULL;
	cvector_for_each_in(lcid, cell_group_diff->rlc_bearer_to_release_list){
		API_rlc_rrc_del_bearer(ue->rnti, *lcid);
	}

	// Add/Mod RLC radio bearers
	struct rlc_bearer_cfg_s *rb = NULL;
	cvector_for_each_in(rb, cell_group_diff->rlc_bearer_to_add_mod_list){
		rlc_config_t         rlc_cfg = {0};
		uint8_t              rb_id = 0;
		if (rb->served_radio_bearer.type_.value == (served_radio_bearer_types)srb_id) {
		  rb_id = rb->served_radio_bearer.c;
		  if (! rb->rlc_cfg_present) {
		  	uint32_t default_sn_size = 12;
		    rlc_cfg = default_rlc_am_nr_config(default_sn_size);
		  } else {
		    if (make_rlc_config_t(rb->rlc_cfg, rb_id, &rlc_cfg) != OSET_OK) {
		      oset_error("Failed to build RLC config");
		      // TODO: HANDLE
		      return OSET_ERROR;
		    }
		  }
		} else {
		  rb_id = rb->served_radio_bearer.c;
		  if (! rb->rlc_cfg_present) {
		    oset_error("No RLC config for DRB");
		    // TODO: HANDLE
		    return OSET_ERROR;
		  }
		  if (make_rlc_config_t(rb->rlc_cfg, rb_id, &rlc_cfg) != OSET_OK) {
		    oset_error("Failed to build RLC config");
		    // TODO: HANDLE
		    return OSET_ERROR;
		  }
		}
		API_rlc_rrc_add_bearer(ue->rnti, rb->lc_ch_id, rlc_cfg);
	}

  return OSET_OK;
}

static const int32_t prioritised_bit_rate_options[] = {0, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, -1};
static const uint16_t bucket_size_dur_options[] = {5, 10, 20, 50, 100, 150, 300, 500, 1000};

static int update_mac(rrc_nr_ue *ue, cell_group_cfg_s *cell_group_config, bool is_config_complete)
{
	if (!is_config_complete) {
		// Release bearers
		uint8_t *lcid = NULL;
		cvector_for_each_in(lcid, cell_group_config->rlc_bearer_to_release_list){
			cvector_push_back(ue->uecfg.lc_ch_to_rem, *lcid);
		}

		struct rlc_bearer_cfg_s *bearer = NULL;
		cvector_for_each_in(bearer, cell_group_config->rlc_bearer_to_add_mod_list){
			sched_nr_ue_lc_ch_cfg_t lc_ch_cfg = {0};
			lc_ch_cfg.lcid = bearer->lc_ch_id;
			mac_lc_ch_cfg_t *lch = &lc_ch_cfg.cfg;
			lch->direction       = (direction_t)BOTH;
			if (bearer->mac_lc_ch_cfg.ul_specific_params_present) {
				lch->priority = bearer->mac_lc_ch_cfg.ul_specific_params.prio;
				lch->pbr      = prioritised_bit_rate_options[bearer->mac_lc_ch_cfg.ul_specific_params.prioritised_bit_rate];
				lch->bsd      = bucket_size_dur_options[bearer->mac_lc_ch_cfg.ul_specific_params.bucket_size_dur];
				lch->group    = bearer->mac_lc_ch_cfg.ul_specific_params.lc_ch_group;
				// TODO: remaining fields
			}
			cvector_push_back(ue->uecfg.lc_ch_to_add, lc_ch_cfg);
		}

		if (cell_group_config->sp_cell_cfg_present &&\
			cell_group_config->sp_cell_cfg.sp_cell_cfg_ded_present &&\
		    cell_group_config->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg_present &&\
		    cell_group_config->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg.init_ul_bwp_present &&\
		    cell_group_config->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg.init_ul_bwp.pucch_cfg_present) {
		  struct pdcch_cfg_s* pucch_cfg = &cell_group_config->sp_cell_cfg.sp_cell_cfg_ded.ul_cfg.init_ul_bwp.pucch_cfg.c;
		  fill_phy_pucch_cfg(pucch_cfg, &ue->uecfg.phy_cfg.pucch);
		}
	} else {
		struct pdcch_cfg_s* pdcch = &cell_group_config->sp_cell_cfg.sp_cell_cfg_ded.init_dl_bwp.pdcch_cfg.c;
		for (auto& ss : pdcch->search_spaces_to_add_mod_list) {
			uecfg.phy_cfg.pdcch.search_space_present[ss.search_space_id] = true;
			srsran::make_phy_search_space_cfg(ss, &uecfg.phy_cfg.pdcch.search_space[ss.search_space_id]);
		}
		for (auto& cs : pdcch.ctrl_res_set_to_add_mod_list) {
			uecfg.phy_cfg.pdcch.coreset_present[cs.ctrl_res_set_id] = true;
			srsran::make_phy_coreset_cfg(cs, &uecfg.phy_cfg.pdcch.coreset[cs.ctrl_res_set_id]);
		}
	}

	uecfg.sp_cell_cfg.reset(new sp_cell_cfg_s{cell_group_cfg.sp_cell_cfg});
	uecfg.mac_cell_group_cfg.reset(new mac_cell_group_cfg_s{cell_group_cfg.mac_cell_group_cfg});
	uecfg.phy_cell_group_cfg.reset(new phys_cell_group_cfg_s{cell_group_cfg.phys_cell_group_cfg});
	srsran::make_csi_cfg_from_serv_cell(cell_group_config.sp_cell_cfg.sp_cell_cfg_ded, &uecfg.phy_cfg.csi);
	parent->mac->ue_cfg(rnti, uecfg);

	return OSET_OK;
}

void rrc_rem_user_info(uint16_t rnti)
{
	msg_def_t *msg_ptr = task_alloc_msg(TASK_RRC, RRC_SELF_REM_USER_INFO);
	oset_assert(msg_ptr);

	RQUE_MSG_TTI(msg_ptr) = 0;
	RRC_SELF_REM_USER_INFO(msg_ptr).rnti = rnti;
	task_send_msg(TASK_RRC, msg_ptr);
}

// default true
void rrc_nr_ue_set_activity(rrc_nr_ue *ue, bool enabled)
{
	if (!enabled) {
		if (ue->activity_timer.running) {
			oset_debug("Inactivity timer interrupted for rnti=0x%x", ue->rnti);
		}
		gnb_timer_stop(ue->activity_timer);
		return;
	}

	// re-start activity timer with current timeout value
	gnb_timer_start(ue->activity_timer, ue->activity_timer_deadline_ms);
	oset_info("Activity registered for rnti=0x%x (timeout_value=%dms)", ue->rnti, ue->activity_timer_deadline_ms);
}

/**
 * @brief Deactivate all Bearers (MAC logical channel) for this specific RNTI
 *
 * The function iterates over the bearers or MAC logical channels and deactivates them by setting each one to IDLE
 */
void rrc_nr_ue_deactivate_bearers(rrc_nr_ue *ue)
{
	// Iterate over the bearers (MAC LC CH) and set each of them to IDLE
	for (uint32_t lcid = 1; lcid < SCHED_NR_MAX_LCID; ++lcid) {
		cvector_push_back(ue->uecfg.lc_ch_to_rem, lcid)
	}

	// No need to check the returned value, as the function ue_cfg will return SRSRAN_SUCCESS (it asserts if it fails)
	ue->uecfg.phy_cell_group_cfg = NULL;
	ue->uecfg.mac_cell_group_cfg = NULL;
	ue->uecfg.sp_cell_cfg        = NULL;
	API_mac_rrc_api_ue_cfg(ue->rnti, &ue->uecfg);
}

void rrc_nr_ue_remove(rrc_nr_ue *ue)
{
    oset_assert(ue);

	gnb_timer_delete(ue->activity_timer);
	cvector_free(ue->uecfg.carriers);
	cvector_free(ue->uecfg.lc_ch_to_add);
	cvector_free(ue->uecfg.lc_ch_to_rem);

	cvector_free(ue->radio_bearer_cfg.srb_to_add_mod_list);
	cvector_free(ue->radio_bearer_cfg.drb_to_add_mod_list);
	cvector_free(ue->radio_bearer_cfg.drb_to_release_list);

	free_master_cell_cfg_vector(ue->cell_group_cfg);

    oset_list_remove(&rrc_manager_self()->rrc_ue_list, ue);
    oset_hash_set(rrc_manager_self()->users, &ue->rnti, sizeof(ue->rnti), NULL);
	oset_core_destroy_memory_pool(&ue->usepool);

    oset_info("[Removed] Number of RRC-UEs is now %d", oset_list_count(&rrc_manager_self()->rrc_ue_list));
}

void rrc_nr_ue_set_rnti(uint16_t rnti, rrc_nr_ue *ue)
{
    oset_assert(ue);
	ue->rnti = rnti;
    oset_hash_set(rrc_manager_self()->users, &rnti, sizeof(rnti), NULL);
    oset_hash_set(rrc_manager_self()->users, &rnti, sizeof(rnti), ue);
}

rrc_nr_ue *rrc_nr_ue_find_by_rnti(uint16_t rnti)
{
    return (rrc_nr_ue *)oset_hash_get(
            rrc_manager_self()->users, &rnti, sizeof(rnti));
}


void rrc_nr_ue_add(uint16_t rnti_, uint32_t pcell_cc_idx, bool start_msg3_timer)
{
	rrc_nr_ue *ue = NULL;
	oset_apr_memory_pool_t *usepool = NULL;
	oset_core_new_memory_pool(&usepool);

	ue = oset_core_alloc(usepool, sizeof(*ue));
	ASSERT_IF_NOT(ue, "Could not allocate sched ue %d context from pool", rnti_);
	memset(ue, 0, sizeof(rrc_nr_ue));

	ue->usepool = usepool;
	ue->rnti = rnti_;
	// Set default MAC UE config
	cvector_reserve(ue->uecfg.carriers, 1);
	sched_nr_ue_cc_cfg_t cell = {0};
	cell.active = true;
	cell.cc     = pcell_cc_idx;
	cvector_push_back(ue->uecfg.carriers, cell);

	ue->uecfg.phy_cfg  = rrc_manager_self()->cell_ctxt->default_phy_ue_cfg_nr;

	if (!rrc_manager_self()->cfg.is_standalone) {
		// Add the final PDCCH config in case of NSA
		fill_phy_pdcch_cfg(rrc_manager_self()->cfg.cell_list[pcell_cc_idx], &ue->uecfg.phy_cfg.pdcch);
	} else {
		//ue->cell_group_cfg      = rrc_manager_self()->cell_ctxt->master_cell_group;
		fill_master_cell_cfg_from_enb_cfg_inner(rrc_manager_self()->cfg, pcell_cc_idx, &ue->cell_group_cfg);
	}

	ue->type = start_msg3_timer ? MSG3_RX_TIMEOUT : MSG5_RX_TIMEOUT;

	// Set timer for MSG3_RX_TIMEOUT or UE_INACTIVITY_TIMEOUT
	set_activity_timer(ue);

	rrc_nr_ue_set_rnti(rnti_, ue);
    oset_list_add(&rrc_manager_self()->rrc_ue_list, ue);
    oset_info("[Added] Number of RRC-UEs is now %d", oset_list_count(&rrc_manager_self()->rrc_ue_list));
}

///////////////////////////////////////////////////////////////////////////////////////////
void rrc_nr_ue_get_metrics(rrc_ue_metrics_t *metrics)
{
	/*TODO fill RRC metrics*/
}

/*******************************************************************************
send DL interface
*******************************************************************************/
/// TS 38.331, RRCReject message
void send_rrc_reject(rrc_nr_ue *ue, uint8_t reject_wait_time_secs)
{
	ASN_RRC_DL_CCCH_Message_t msg = {0};

	memset(&msg, 0, sizeof (ASN_RRC_DL_CCCH_Message_t));
	msg.message.present = ASN_RRC_DL_CCCH_MessageType_PR_c1;
	asn1cCalloc(msg.message.choice.c1, c1);
	c1->present = ASN_RRC_DL_CCCH_MessageType__c1_PR_rrcReject;

	asn1cCalloc(msg.message.choice.c1->choice.rrcReject, reject);
	reject->criticalExtensions.present = ASN_RRC_RRCReject__criticalExtensions_PR_rrcReject;
	asn1cCalloc(reject->criticalExtensions.choice.rrcReject, rrcReject);
	rrcReject->waitTime = CALLOC(1, sizeof(ASN_RRC_RejectWaitTime_t));
	rrcReject->waitTime = &reject_wait_time_secs;

	// See TS 38.331, RejectWaitTime
	if (reject_wait_time_secs > 0) {
		asn1cCallocOne(rrcReject->waitTime, &reject_wait_time_secs);
	}

	if (send_dl_ccch(ue, &msg) != OSET_OK) {
		// TODO: Handle
		oset_error("rnti 0x%x:send_dl_ccch() rrc_reject fail", ue->rnti);
	}

	// TODO: remove user
}

/// TS 38.331, RRCSetup
void send_rrc_setup(rrc_nr_ue *ue)
{
	const uint8_t max_wait_time_secs = 16;

	// Add SRB1 to UE context
	// Note: See 5.3.5.6.3 - SRB addition/modification
	struct srb_to_add_mod_s srb1 = {0};
	srb1.srb_id            = 1;
	cvector_push_back(ue->radio_bearer_cfg.srb_to_add_mod_list, srb1)

	// - Setup masterCellGroup
	// - Derive master cell group config bearers
	if (fill_cellgroup_with_radio_bearer_cfg_inner(rrc_manager_self()->cfg,
													ue->rnti,
													&rrc_manager_self()->bearer_mapper,
													ue->radio_bearer_cfg,
													ue->cell_group_cfg) != OSET_OK) {
		oset_error("Couldn't fill cellGroupCfg during RRC Setup");
		send_rrc_reject(ue, max_wait_time_secs);
		return;
	}

	////////////////////////////////////////////////////////////////////////////

	// Generate RRC setup message
	ASN_RRC_DL_CCCH_Message_t msg = {0};
	msg.message.present = ASN_RRC_DL_CCCH_MessageType_PR_c1;
	asn1cCalloc(msg.message.choice.c1, c1);
	c1->present = ASN_RRC_DL_CCCH_MessageType__c1_PR_rrcSetup;

	asn1cCalloc(msg.message.choice.c1->choice.rrcSetup, rrcsetup);
	rrcsetup->rrc_TransactionIdentifier   = (uint8_t)((ue->transaction_id++) % 4);
	rrcsetup->criticalExtensions.present  = ASN_RRC_RRCSetup__criticalExtensions_PR_rrcSetup;
	asn1cCalloc(rrcsetup->criticalExtensions.choice.rrcSetup, rrcsetup_ies);

	// Fill RRC Setup
	// - Setup SRB1
	rrcsetup_ies->radioBearerConfig.srb_ToAddModList = CALLOC(1,sizeof(struct ASN_RRC_SRB_ToAddModList));
	asn1cSequenceAdd(rrcsetup_ies->radioBearerConfig.srb_ToAddModList->list, struct ASN_RRC_SRB_ToAddMod, SRB1_config);	
	SRB1_config->srb_Identity = ue->next_radio_bearer_cfg.srb_to_add_mod_list[0].srb_id;

	// masterCellGroup
	ASN_RRC_CellGroupConfig masterCellGroup = {0};
	fill_master_cell_cfg(&ue->cell_group_cfg, &masterCellGroup);
	byte_buffer_t *pdu = oset_rrc_encode2(&asn_DEF_ASN_RRC_CellGroupConfig, &masterCellGroup, asn_struct_free_context);
	ASSERT_IF_NOT(pdu, "masterCellGroup encode failed!");

    rrcsetup_ies->masterCellGroup.buf = CALLOC(pdu->N_bytes);
    memcpy(rrcsetup_ies->masterCellGroup.buf, pdu->msg, pdu->N_bytes);
    rrcsetup_ies->masterCellGroup.size = pdu->N_bytes;

	// add RLC bearers
	update_rlc_bearers(ue, ue->cell_group_cfg);

	// add PDCP bearers
	// this is done after updating the RLC bearers,
	// so the PDCP can query the RLC mode
	update_pdcp_bearers(ue->radio_bearer_cfg, ue->cell_group_cfg);

	// add MAC bearers添加MAC承载
	update_mac(ue->cell_group_cfg, false);

	// Send RRC Setup message to UE
	if (send_dl_ccch(msg) != OSET_OK) {
		send_rrc_reject(ue, max_wait_time_secs);
	}

	oset_free(pdu);
}

/*******************************************************************************
handle UL interface
*******************************************************************************/
void handle_rrc_setup_request(rrc_nr_ue *ue, ASN_RRC_RRCSetupRequest_t *msg)
{
	const uint8_t max_wait_time_secs = 16;
	if (! parent->ngap->is_amf_connected()) {
		oset_error("MME isn't connected. Sending Connection Reject");
		send_rrc_reject(ue, max_wait_time_secs);
		return;
	}

	// Allocate PUCCH resources and reject if not available
	if (! init_pucch()) {
		oset_warn("Could not allocate PUCCH resources for rnti=0x%x. Sending Connection Reject", rnti);
		send_rrc_reject(ue, max_wait_time_secs);
		return;
	}

	ASN_RRC_RRCSetupRequest_IEs_t *rrcSetupRequest = msg->rrcSetupRequest;

	switch (rrcSetupRequest->ue_Identity.present) {
	  case ASN_RRC_InitialUE_Identity_PR_ng_5G_S_TMSI_Part1:
	  	oset_asn_BIT_STRING_to_uint64(rrcSetupRequest->ue_Identity.choice.ng_5G_S_TMSI_Part1, &ue->ctxt.setup_ue_id);
	    break;
	  case ASN_RRC_InitialUE_Identity_PR_randomValue:
	  	oset_asn_BIT_STRING_to_uint64(rrcSetupRequest.ue_Identity.choice.randomValue, &ue->ctxt.setup_ue_id);
	    // TODO: communicate with NGAP
	    break;
	  default:
	    oset_error("Unsupported RRCSetupRequest");
	    send_rrc_reject(ue, max_wait_time_secs);
	    return;
	}
	ue->ctxt.connection_cause = rrcSetupRequest->establishmentCause;

	send_rrc_setup(ue);
	ue->type = UE_INACTIVITY_TIMEOUT;
	set_activity_timer(ue);
}

