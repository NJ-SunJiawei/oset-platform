/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "rrc/rrc_nr_ue.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc-ue"

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

	if (! rrc_manager_self()->cfg.is_standalone) {
	// Add the final PDCCH config in case of NSA
		fill_phy_pdcch_cfg(rrc_manager_self()->cfg.cell_list[pcell_cc_idx], &ue->uecfg.phy_cfg.pdcch);
	} else {
		ue->cell_group_cfg      = rrc_manager_self()->cell_ctxt->master_cell_group;
		ue->next_cell_group_cfg = ue->cell_group_cfg;
	}

	ue->type = start_msg3_timer ? MSG3_RX_TIMEOUT : MSG5_RX_TIMEOUT;

	// Set timer for MSG3_RX_TIMEOUT or UE_INACTIVITY_TIMEOUT
	set_activity_timer(ue);

	rrc_nr_ue_set_rnti(rnti_, ue);
    oset_list_add(&rrc_manager_self()->rrc_ue_list, ue);
    oset_info("[Added] Number of RRC-UEs is now %d", oset_list_count(&rrc_manager_self()->rrc_ue_list));
}

void rrc_nr_ue_get_metrics(rrc_ue_metrics_t *metrics)
{
	/*TODO fill RRC metrics*/
}

