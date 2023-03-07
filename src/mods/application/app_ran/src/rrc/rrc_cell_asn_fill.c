/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "rrc/rrc_cell_asn_fill.h"
		
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc"

void fill_pdsch_cfg_common(rrc_cell_cfg_nr_t *cell_cfg, struct ASN_RRC_PDSCH_ConfigCommon *out)
{
	out->pdsch_TimeDomainAllocationList = CALLOC(1,sizeof(struct ASN_RRC_PDSCH_TimeDomainResourceAllocationList));
	asn1cSequenceAdd(out->pdsch_TimeDomainAllocationList->list, struct ASN_RRC_PDSCH_TimeDomainResourceAllocation, pdscha1);	
	pdscha1->mappingType = ASN_RRC_PDSCH_TimeDomainResourceAllocation__mappingType_typeA;
	pdscha1->startSymbolAndLength = 40;
}


// Called for SA and NSA
void fill_pdcch_cfg_common(rrc_cell_cfg_nr_t *cell_cfg, struct ASN_RRC_PDCCH_ConfigCommon *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	if(cell_cfg->pdcch_cfg_common.common_ctrl_res_set_present){
        //is_sa no enter
	}

	out->commonSearchSpaceList = CALLOC(1,sizeof(struct ASN_RRC_PDCCH_ConfigCommon__commonSearchSpaceList));
	//just 1 common_search_space_list
	struct search_space_s *ss = cell_cfg->pdcch_cfg_common.common_search_space_list->array[0];

	asn1cSequenceAdd(out->commonSearchSpaceList->list, struct ASN_RRC_SearchSpace, ss1);
	ss1->searchSpaceId = ss->search_space_id;
	asn1cCallocOne(ss1->controlResourceSetId, ss->ctrl_res_set_id);
	ss1->monitoringSlotPeriodicityAndOffset = CALLOC(1,sizeof(*ss1->monitoringSlotPeriodicityAndOffset));
	ss1->monitoringSlotPeriodicityAndOffset->present = ASN_RRC_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
	ss1->monitoringSymbolsWithinSlot = CALLOC(1,sizeof(*ss1->monitoringSymbolsWithinSlot));
	oset_asn_buffer_to_BIT_STRING(ss->monitoring_symbols_within_slot, 2, 2, ss1->monitoringSymbolsWithinSlot);
	ss1->nrofCandidates = CALLOC(1,sizeof(*ss1->nrofCandidates));
	ss1->nrofCandidates->aggregationLevel1 = ss->nrof_candidates.aggregation_level1;
	ss1->nrofCandidates->aggregationLevel2 = ss->nrof_candidates.aggregation_level2;
	ss1->nrofCandidates->aggregationLevel4 = ss->nrof_candidates.aggregation_level4;
	ss1->nrofCandidates->aggregationLevel8 = ss->nrof_candidates.aggregation_level8;
	ss1->nrofCandidates->aggregationLevel16 = ss->nrof_candidates.aggregation_level16;
	ss1->searchSpaceType = CALLOC(1,sizeof(*ss1->searchSpaceType));
	ss1->searchSpaceType->present = ASN_RRC_SearchSpace__searchSpaceType_PR_common;
	ss1->searchSpaceType->choice.common=CALLOC(1,sizeof(*ss1->searchSpaceType->choice.common));
	ss1->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0 = CALLOC(1,sizeof(*ss1->searchSpaceType->choice.common->dci_Format0_0_AndFormat1_0));

	asn1cCallocOne(out->searchSpaceSIB1,  0);
	asn1cCallocOne(out->searchSpaceOtherSystemInformation, 1);
	asn1cCallocOne(out->pagingSearchSpace, 1);
	asn1cCallocOne(out->ra_SearchSpace, 1);
}


// Called for SA
void fill_init_dl_bwp(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BWP_DownlinkCommon_t *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	out->genericParameters.locationAndBandwidth    = 14025;
	out->genericParameters.subcarrierSpacing = cell_cfg.phy_cell.carrier.scs;

    out->pdcch_ConfigCommon = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PDCCH_ConfigCommon));
	out->pdcch_ConfigCommon.present = ASN_RRC_SetupRelease_PDCCH_ConfigCommon_PR_setup;
    out->pdcch_ConfigCommon.choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PDCCH_ConfigCommon));
	fill_pdcch_cfg_common(cell_cfg, out->pdcch_ConfigCommon.choice.setup);

    out->pdsch_ConfigCommon = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PDSCH_ConfigCommon));
	out->pdsch_ConfigCommon.present = ASN_RRC_SetupRelease_PDSCH_ConfigCommon_PR_setup;
    out->pdsch_ConfigCommon.choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PDSCH_ConfigCommon));
	fill_pdsch_cfg_common(cell_cfg, out->pdsch_ConfigCommon.choice.setup);
}


void fill_dl_cfg_common_sib(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_DownlinkConfigCommonSIB_t *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;
	band_helper_t *band_helper = gnb_manager_self()->band_helper;

	uint32_t scs_hz = SRSRAN_SUBC_SPACING_NR(cell_cfg->phy_cell.carrier.scs);
	uint32_t prb_bw = scs_hz * SRSRAN_NRE;

	for(int i = 0; i< 1; i++) {
		asn1cSequenceAdd(out->frequencyInfoDL.frequencyBandList.list, struct ASN_RRC_NR_MultiBandInfo, nrMultiBandInfo);
		asn1cCallocOne(nrMultiBandInfo->freqBandIndicatorNR, cell_cfg->band);
	}

	double	 ssb_freq_start 	 = cell_cfg->ssb_freq_hz - SRSRAN_SSB_BW_SUBC * scs_hz / 2;
	double	 offset_point_a_hz   = ssb_freq_start - nr_arfcn_to_freq_2c(band_helper, cell_cfg->dl_absolute_freq_point_a);
	uint32_t offset_point_a_prbs = offset_point_a_hz / prb_bw;
	out->frequencyInfoDL.offsetToPointA = offset_point_a_prbs;

	for(int i = 0; i< 1; i++) {
		asn1cSequenceAdd(out->frequencyInfoDL.scs_SpecificCarrierList.list,
			   struct ASN_RRC_SCS_SpecificCarrier, scs_SpecificCarrierInfo);
		scs_SpecificCarrierInfo->offsetToCarrier = cell_cfg->phy_cell.carrier.offset_to_carrier;
		scs_SpecificCarrierInfo->subcarrierSpacing = cell_cfg->phy_cell.carrier.scs;//ASN_RRC_SubcarrierSpacing_kHz15
		scs_SpecificCarrierInfo->carrierBandwidth = cell_cfg->phy_cell.carrier.nof_prb;
	}

	fill_init_dl_bwp(cell_cfg, &out->initialDownlinkBWP);

	out->bcch_Config.modificationPeriodCoeff = ASN_RRC_BCCH_Config__modificationPeriodCoeff_n4;
	out->pcch_Config.defaultPagingCycle = ASN_RRC_PagingCycle_rf128;
	out->pcch_Config.nAndPagingFrameOffset.present = ASN_RRC_PCCH_Config__nAndPagingFrameOffset_PR_oneT;
	out->pcch_Config.ns = ASN_RRC_PCCH_Config__ns_one;

}

void fill_ul_cfg_common_sib(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_UplinkConfigCommonSIB *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;
	band_helper_t *band_helper = gnb_manager_self()->band_helper;

	for(int i = 0; i< 1; i++) {
		asn1cCalloc(out->frequencyInfoUL.frequencyBandList,  frequencyBandList);
		asn1cSequenceAdd(frequencyBandList->list, struct ASN_RRC_NR_MultiBandInfo, nrMultiBandInfo);
		asn1cCallocOne(nrMultiBandInfo->freqBandIndicatorNR, cell_cfg->band);
	}
	asn1cCallocOne(out->frequencyInfoUL.absoluteFrequencyPointA,
	               get_abs_freq_point_a_arfcn_2c(cell_cfg->phy_cell.carrier.nof_prb, cell_cfg->ul_arfcn));

	for(int i = 0; i< 1; i++) {
		asn1cSequenceAdd(out->frequencyInfoUL.scs_SpecificCarrierList.list, struct ASN_RRC_SCS_SpecificCarrier, SCS_SpecificCarrier);
		SCS_SpecificCarrier->offsetToCarrier = cell_cfg->phy_cell.carrier.offset_to_carrier;
		SCS_SpecificCarrier->subcarrierSpacing = cell_cfg->phy_cell.carrier.scs;
		SCS_SpecificCarrier->carrierBandwidth = cell_cfg->phy_cell.carrier.nof_prb;
	}
	asn1cCallocOne(out->frequencyInfoUL.p_Max, 10);

	out->initialUplinkBWP.genericParameters.locationAndBandwidth = 14025;
	out->initialUplinkBWP.genericParameters.subcarrierSpacing = cell_cfg->phy_cell.carrier.scs;

	//out->initialUplinkBWP.rach_ConfigCommon


	out.init_ul_bwp.rach_cfg_common_present = true;
	fill_rach_cfg_common(cfg, cc, out.init_ul_bwp.rach_cfg_common.set_setup());

	out.init_ul_bwp.pusch_cfg_common_present = true;
	pusch_cfg_common_s& pusch                = out.init_ul_bwp.pusch_cfg_common.set_setup();
	pusch.pusch_time_domain_alloc_list.resize(1);
	pusch.pusch_time_domain_alloc_list[0].k2_present           = true;
	pusch.pusch_time_domain_alloc_list[0].k2                   = 4;
	pusch.pusch_time_domain_alloc_list[0].map_type.value       = pusch_time_domain_res_alloc_s::map_type_opts::type_a;
	pusch.pusch_time_domain_alloc_list[0].start_symbol_and_len = 27;
	pusch.p0_nominal_with_grant_present                        = true;
	pusch.p0_nominal_with_grant                                = -76;

	out.init_ul_bwp.pucch_cfg_common_present = true;
	pucch_cfg_common_s& pucch                = out.init_ul_bwp.pucch_cfg_common.set_setup();
	pucch.pucch_res_common_present           = true;
	pucch.pucch_res_common                   = 11;
	pucch.pucch_group_hop.value              = pucch_cfg_common_s::pucch_group_hop_opts::neither;
	pucch.p0_nominal_present                 = true;
	pucch.p0_nominal                         = -90;

	out.time_align_timer_common.value = time_align_timer_opts::infinity;
}


int fill_serv_cell_cfg_common_sib(rrc_cell_cfg_nr_t *cell_cfg, struct ASN_RRC_ServingCellConfigCommonSIB *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;


	fill_dl_cfg_common_sib(cell_cfg, &out->downlinkConfigCommon);

	out->uplinkConfigCommon = CALLOC(1, sizeof(struct ASN_RRC_UplinkConfigCommonSIB));
	fill_ul_cfg_common_sib(cell_cfg, out->uplinkConfigCommon);

	out.ssb_positions_in_burst.in_one_group.from_number(0x80);

	out.ssb_periodicity_serving_cell.value = serving_cell_cfg_common_sib_s::ssb_periodicity_serving_cell_opts::ms10;

	// The time advance offset is not supported by the current PHY
	out.n_timing_advance_offset_present = true;
	out.n_timing_advance_offset         = serving_cell_cfg_common_sib_s::n_timing_advance_offset_opts::n0;

	// TDD UL-DL config
	if (cell_cfg.duplex_mode == SRSRAN_DUPLEX_MODE_TDD) {
	out.tdd_ul_dl_cfg_common_present = true;
	fill_tdd_ul_dl_config_common(cell_cfg, out.tdd_ul_dl_cfg_common);
	}

	out.ss_pbch_block_pwr = cell_cfg.pdsch_rs_power;

  return OSET_OK;
}

int fill_mib_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BCCH_BCH_Message_t *mib_pdu)
{
	mib_pdu =  CALLOC(1,sizeof(struct ASN_RRC_BCCH_BCH_Message_t));
	mib_pdu->message.present = ASN_RRC_BCCH_BCH_MessageType_PR_mib;
	mib_pdu->message.choice.mib = CALLOC(1,sizeof(struct ASN_RRC_MIB_t));

	ASN_RRC_MIB_t *mib = mib_pdu->message.choice.mib;

	oset_asn_uint8_to_BIT_STRING(0, 2, &mib->systemFrameNumber);
	switch (cell_cfg.phy_cell.carrier.scs) {
	case srsran_subcarrier_spacing_15kHz:
	case srsran_subcarrier_spacing_60kHz:
	  mib->subCarrierSpacingCommon = ASN_RRC_MIB__subCarrierSpacingCommon_scs15or60;
	  break;
	case srsran_subcarrier_spacing_30kHz:
	case srsran_subcarrier_spacing_120kHz:
	  mib->subCarrierSpacingCommon = ASN_RRC_MIB__subCarrierSpacingCommon_scs30or120;
	  break;
	default:
	  oset_error("Invalid carrier SCS=%d Hz", SRSRAN_SUBC_SPACING_NR(cell_cfg.phy_cell.carrier.scs));
	}
	mib->ssb_SubcarrierOffset       = cell_cfg.ssb_offset;
	mib->dmrs_TypeA_Position        = ASN_RRC_MIB__dmrs_TypeA_Position_pos2;
	mib->pdcch_ConfigSIB1.controlResourceSetZero = 0;
	mib->pdcch_ConfigSIB1.searchSpaceZero = cell_cfg.coreset0_idx;
	mib->cellBarred                 = ASN_RRC_MIB__cellBarred_notBarred;
	mib->intraFreqReselection       = ASN_RRC_MIB__intraFreqReselection_allowed;
	oset_asn_uint8_to_BIT_STRING(0, 7, &mib->spare);// This makes a spare of 1 bits

	return OSET_OK;
}

int fill_sib1_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BCCH_DL_SCH_Message_t *sib1_pdu)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	sib1_pdu = CALLOC(1,sizeof(ASN_RRC_BCCH_DL_SCH_Message_t));
	sib1_pdu->message.present = ASN_RRC_BCCH_DL_SCH_MessageType_PR_c1;
	sib1_pdu->message.choice.c1 = CALLOC(1,sizeof(struct ASN_RRC_BCCH_DL_SCH_MessageType__c1));
	sib1_pdu->message.choice.c1->present = ASN_RRC_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1;
	sib1_pdu->message.choice.c1->choice.systemInformationBlockType1 = CALLOC(1,sizeof(struct ASN_RRC_SIB1_t));
	
	struct ASN_RRC_SIB1_t *sib1 = sib1_pdu->message.choice.c1->choice.systemInformationBlockType1;

	// cellSelectionInfo
	sib1->cellSelectionInfo = CALLOC(1,sizeof(struct ASN_RRC_SIB1__cellSelectionInfo));
	sib1->cellSelectionInfo->q_RxLevMin = -70;
	sib1->cellSelectionInfo->q_QualMin = -20;

	// cellAccessRelatedInfo
	asn1cSequenceAdd(sib1->cellAccessRelatedInfo.plmn_IdentityList.list, struct ASN_RRC_PLMN_IdentityInfo, nr_plmn_info);
	for (int i = 0; i < 1; ++i) {
		plmn_id_t plmn = {0};	  
		asn1cSequenceAdd(nr_plmn_info->plmn_IdentityList.list, struct ASN_RRC_PLMN_Identity, nr_plmn);
		asn1cCalloc(nr_plmn->mcc,  mcc);
		mcc_to_bytes(cfg->mcc, &plmn.mcc[0]);
		asn1cSequenceAdd(mcc->list, ASN_RRC_MCC_MNC_Digit_t, mcc0);
		*mcc0=plmn.mcc[0];//(cfg->mcc/100)%10
		asn1cSequenceAdd(mcc->list, ASN_RRC_MCC_MNC_Digit_t, mcc1);
		*mcc1=plmn.mcc[1];//(cfg->mcc/10)%10
		asn1cSequenceAdd(mcc->list, ASN_RRC_MCC_MNC_Digit_t, mcc2);
		*mcc2=plmn.mcc[2];//(cfg->mcc)%10

		mnc_to_bytes(cfg->mnc, &plmn.mnc[0], &plmn.nof_mnc_digits);
		if(plmn->nof_mnc_digits == 3) {
		asn1cSequenceAdd(nr_plmn->mnc.list, ASN_RRC_MCC_MNC_Digit_t, mnc0);
		*mnc0=(cfg->mnc/100)%10;
		}
		asn1cSequenceAdd(nr_plmn->mnc.list, ASN_RRC_MCC_MNC_Digit_t, mnc1);
		*mnc1=(cfg->mnc/10)%10;
		asn1cSequenceAdd(nr_plmn->mnc.list, ASN_RRC_MCC_MNC_Digit_t, mnc2);
		*mnc2=(cfg->mnc)%10;
	}//end plmn loop
	oset_asn_uint64_to_BIT_STRING(((cfg->enb_id << 8U) + cell_cfg->phy_cell.cell_id), 36, &nr_plmn_info->cellIdentity);
	oset_asn_uint32_to_BIT_STRING(cell_cfg->tac, 24, &nr_plmn_info->trackingAreaCode);
	nr_plmn_info->cellReservedForOperatorUse = ASN_RRC_PLMN_IdentityInfo__cellReservedForOperatorUse_notReserved;

	asn1cCalloc(sib1->connEstFailureControl, cnEstFailCtrl);
	cnEstFailCtrl->connEstFailCount = ASN_RRC_ConnEstFailureControl__connEstFailCount_n1;
	cnEstFailCtrl->connEstFailOffsetValidity = ASN_RRC_ConnEstFailureControl__connEstFailOffsetValidity_s30;
	cnEstFailCtrl->connEstFailOffset = CALLOC(1,sizeof(long));
	cnEstFailCtrl->connEstFailOffset = 1;

	//si-SchedulingInfo TODU
	
	// servingCellConfigCommon
	asn1cCalloc(sib1->servingCellConfigCommon,	ServCellCom);
	HANDLE_ERROR(fill_serv_cell_cfg_common_sib(cfg, cell_cfg->cell_idx, sib1->servingCellConfigCommon));

    asn1cCalloc(sib1->ue_TimersAndConstants, ue_timers);
	ue_timers->t300 = ASN_RRC_UE_TimersAndConstants__t300_ms1000;
	ue_timers->t301 = ASN_RRC_UE_TimersAndConstants__t301_ms1000;
	ue_timers->t310 = ASN_RRC_UE_TimersAndConstants__t310_ms1000;
	ue_timers->n310 = ASN_RRC_UE_TimersAndConstants__n310_n1;
	ue_timers->t311 = ASN_RRC_UE_TimersAndConstants__t311_ms30000;
	ue_timers->n311 = ASN_RRC_UE_TimersAndConstants__n311_n1;
	ue_timers->t319 = ASN_RRC_UE_TimersAndConstants__t319_ms1000;

	return OSET_OK;
}

