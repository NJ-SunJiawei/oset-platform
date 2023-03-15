/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "rrc/rrc.h"
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
        // see set_derived_nr_cell_params()
		/*
		controlResourceSetZero  searchSpaceZero 分别对应CORESET 0 和SearchSpace 0的配置。
		具体含义和MIB 中的pdcch-ConfigSIB1 类似（NSA 下需要在此配置，SA下仅在MIB 中配置即可）
		可以配置额外的CORESET （1-11），不配置时，默认为CORESET 0（SSB 关联）
		可以配置1-4个搜索空间：searchSpaceSIB1/searchSpaceOtherSystemInformation/pagingSearchSpace/ra-SearchSpace ,都是可选配置，当不配置时，都默认使用SearchSpace 0。
		*/
	}

	out->commonSearchSpaceList = CALLOC(1,sizeof(*out->commonSearchSpaceList));
	//just 1 common_search_space_list
	struct search_space_s *ss = byn_array_get_data(&cell_cfg->pdcch_cfg_common.common_search_space_list, 0);
	//????searchspace #0
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
	out->pdcch_ConfigCommon->present = ASN_RRC_SetupRelease_PDCCH_ConfigCommon_PR_setup;
    out->pdcch_ConfigCommon->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PDCCH_ConfigCommon));
	fill_pdcch_cfg_common(cell_cfg, out->pdcch_ConfigCommon->choice.setup);

    out->pdsch_ConfigCommon = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PDSCH_ConfigCommon));
	out->pdsch_ConfigCommon->present = ASN_RRC_SetupRelease_PDSCH_ConfigCommon_PR_setup;
    out->pdsch_ConfigCommon->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PDSCH_ConfigCommon));
	fill_pdsch_cfg_common(cell_cfg, out->pdsch_ConfigCommon->choice.setup);
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
		asn1cSequenceAdd(out->frequencyInfoDL.scs_SpecificCarrierList.list, struct ASN_RRC_SCS_SpecificCarrier, scs_SpecificCarrierInfo);
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

int fill_rach_cfg_common(rrc_cell_cfg_nr_t *cell_cfg, struct ASN_RRC_RACH_ConfigCommon *rach)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	// rach-ConfigGeneric
	rach->rach_ConfigGeneric.prach_ConfigurationIndex = 0;
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_TDD) {
		// Note: Give more time margin to fit RAR
		rach->rach_ConfigGeneric.prach_ConfigurationIndex = 8;
	}
	rach->rach_ConfigGeneric.msg1_FDM = ASN_RRC_RACH_ConfigGeneric__msg1_FDM_one;
	rach->rach_ConfigGeneric.msg1_FrequencyStart = 1;// zero not supported with current PRACH implementation
	rach->rach_ConfigGeneric.zeroCorrelationZoneConfig = 0;
	rach->rach_ConfigGeneric.preambleReceivedTargetPower = -110;
	rach->rach_ConfigGeneric.preambleTransMax = ASN_RRC_RACH_ConfigGeneric__preambleTransMax_n7;
	rach->rach_ConfigGeneric.powerRampingStep = ASN_RRC_RACH_ConfigGeneric__powerRampingStep_dB4;
	rach->rach_ConfigGeneric.ra_ResponseWindow = ASN_RRC_RACH_ConfigGeneric__ra_ResponseWindow_sl10;

	// totalNumberOfRA-Preambles
	if (cell_cfg->num_ra_preambles != 64) {
		asn1cCallocOne(rach->totalNumberOfRA_Preambles, cell_cfg->num_ra_preambles);
	}

	rach->ssb_perRACH_OccasionAndCB_PreamblesPerSSB = CALLOC(1, sizeof(*rach->ssb_perRACH_OccasionAndCB_PreamblesPerSSB));
	rach->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present = ASN_RRC_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one;
	rach->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.one = (cell_cfg->num_ra_preambles/4 - 1);

    rach->ra_ContentionResolutionTimer = ASN_RRC_RACH_ConfigCommon__ra_ContentionResolutionTimer_sf64;
    rach->prach_RootSequenceIndex.present = ASN_RRC_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l839;
	rach->prach_RootSequenceIndex.choice.l839 = cell_cfg->prach_root_seq_idx;
	rach->restrictedSetConfig = ASN_RRC_RACH_ConfigCommon__restrictedSetConfig_unrestrictedSet;
	return OSET_OK;
}


void fill_ul_cfg_common_sib(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_UplinkConfigCommonSIB *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;
	band_helper_t *band_helper = gnb_manager_self()->band_helper;

	asn1cCalloc(out->frequencyInfoUL.frequencyBandList,  frequencyBandList);
	for(int i = 0; i< 1; i++) {
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

	out->initialUplinkBWP.rach_ConfigCommon = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_RACH_ConfigCommon));
	out->initialUplinkBWP.rach_ConfigCommon->present = ASN_RRC_SetupRelease_RACH_ConfigCommon_PR_setup;
	out->initialUplinkBWP.rach_ConfigCommon->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_RACH_ConfigCommon));

	fill_rach_cfg_common(cell_cfg, out->initialUplinkBWP.rach_ConfigCommon->choice.setup);

	out->initialUplinkBWP.pusch_ConfigCommon = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PUSCH_ConfigCommon));
	out->initialUplinkBWP.pusch_ConfigCommon->present = ASN_RRC_SetupRelease_PUSCH_ConfigCommon_PR_setup;
	out->initialUplinkBWP.pusch_ConfigCommon->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PUSCH_ConfigCommon));
	asn1cCalloc(out->initialUplinkBWP.pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList, pusch_TimeDomainAllocList);
	for(int i = 0; i< 1; i++) {
		asn1cSequenceAdd(pusch_TimeDomainAllocList->list, struct ASN_RRC_PUSCH_TimeDomainResourceAllocation, PUSCH_TimeDomainResourceAllocation);
	    asn1cCallocOne(PUSCH_TimeDomainResourceAllocation->k2, 4);
		PUSCH_TimeDomainResourceAllocation->mappingType = ASN_RRC_PUSCH_TimeDomainResourceAllocation__mappingType_typeA;
		PUSCH_TimeDomainResourceAllocation->startSymbolAndLength = 27;
	}
	asn1cCallocOne(out->initialUplinkBWP.pusch_ConfigCommon->choice.setup->p0_NominalWithGrant, -76);

	out->initialUplinkBWP.pucch_ConfigCommon = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PUCCH_ConfigCommon));
	out->initialUplinkBWP.pucch_ConfigCommon->present = ASN_RRC_SetupRelease_PUCCH_ConfigCommon_PR_setup;
	out->initialUplinkBWP.pucch_ConfigCommon->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PUCCH_ConfigCommon));
	asn1cCallocOne(out->initialUplinkBWP.pucch_ConfigCommon->choice.setup->pucch_ResourceCommon, 11);
	out->initialUplinkBWP.pucch_ConfigCommon->choice.setup->pucch_GroupHopping = ASN_RRC_PUCCH_ConfigCommon__pucch_GroupHopping_neither;
	asn1cCallocOne(out->initialUplinkBWP.pucch_ConfigCommon->choice.setup->p0_nominal, -90);

	out->timeAlignmentTimerCommon = ASN_RRC_TimeAlignmentTimer_infinity;
}

void fill_tdd_ul_dl_config_common(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_TDD_UL_DL_ConfigCommon_t *tdd)
{
	oset_assert(cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_TDD, "This function should only be called for TDD configs");
	// TDD UL-DL config
	tdd->referenceSubcarrierSpacing = cell_cfg->phy_cell.carrier.scs;
	tdd->pattern1.dl_UL_TransmissionPeriodicity = ASN_RRC_TDD_UL_DL_Pattern__dl_UL_TransmissionPeriodicity_ms10;
	tdd->pattern1.nrofDownlinkSlots = 6;
    tdd->pattern1.nrofDownlinkSymbols = 0;
	tdd->pattern1.nrofUplinkSlots = 4;
	tdd->pattern1.nrofUplinkSymbols = 0;
}

int fill_serv_cell_cfg_common_sib(rrc_cell_cfg_nr_t *cell_cfg, struct ASN_RRC_ServingCellConfigCommonSIB *out)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;


	fill_dl_cfg_common_sib(cell_cfg, &out->downlinkConfigCommon);

	out->uplinkConfigCommon = CALLOC(1, sizeof(struct ASN_RRC_UplinkConfigCommonSIB));
	fill_ul_cfg_common_sib(cell_cfg, out->uplinkConfigCommon);

	oset_asn_uint8_to_BIT_STRING(0x80, 0, &out->ssb_PositionsInBurst.inOneGroup);
	out->ssb_PeriodicityServingCell = ASN_RRC_ServingCellConfigCommonSIB__ssb_PeriodicityServingCell_ms10;

	// The time advance offset is not supported by the current PHY
	asn1cCallocOne(out->n_TimingAdvanceOffset, ASN_RRC_ServingCellConfigCommonSIB__n_TimingAdvanceOffset_n0);
	

	// TDD UL-DL config
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_TDD) {
		out->tdd_UL_DL_ConfigurationCommon = CALLOC(1, sizeof(struct ASN_RRC_TDD_UL_DL_ConfigCommon));
		fill_tdd_ul_dl_config_common(cell_cfg, out->tdd_UL_DL_ConfigurationCommon);
	}

	out->ss_PBCH_BlockPower = cell_cfg->pdsch_rs_power;

	return OSET_OK;
}

int fill_sib1_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BCCH_DL_SCH_Message_t *sib1_pdu)
{
	rrc_nr_cfg_t *cfg = rrc_manager_self()->cfg;

	sib1_pdu = CALLOC(1,sizeof(ASN_RRC_BCCH_DL_SCH_Message_t));
	sib1_pdu->message.present = ASN_RRC_BCCH_DL_SCH_MessageType_PR_c1;
	sib1_pdu->message.choice.c1 = CALLOC(1,sizeof(*sib1_pdu->message.choice.c1));
	sib1_pdu->message.choice.c1->present = ASN_RRC_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1;
	sib1_pdu->message.choice.c1->choice.systemInformationBlockType1 = CALLOC(1,sizeof(ASN_RRC_SIB1_t));
	
	struct ASN_RRC_SIB1_t *sib1 = sib1_pdu->message.choice.c1->choice.systemInformationBlockType1;

	// cellSelectionInfo
	sib1->cellSelectionInfo = CALLOC(1,sizeof(*sib1->cellSelectionInfo));
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

	//si-SchedulingInfo Todo parse_sib1()
	sib1->si_SchedulingInfo = CALLOC(1, sizeof(struct ASN_RRC_SI_SchedulingInfo));

	sib1->si_SchedulingInfo->si_RequestConfig = CALLOC(1, sizeof(struct ASN_RRC_SI_RequestConfig));
	sib1->si_SchedulingInfo->si_RequestConfig->rach_OccasionsSI = CALLOC(1, sizeof(struct ASN_RRC_SI_RequestConfig__rach_OccasionsSI));
	sib1->si_SchedulingInfo->si_RequestConfig->rach_OccasionsSI->rach_ConfigSI.ra_ResponseWindow = ASN_RRC_RACH_ConfigGeneric__ra_ResponseWindow_sl8;
	sib1->si_SchedulingInfo->si_WindowLength = ASN_RRC_SI_SchedulingInfo__si_WindowLength_s20;
	for(int i = 0; i < 1; i++){
		asn1cSequenceAdd(sib1->si_SchedulingInfo->schedulingInfoList.list, struct ASN_RRC_SchedulingInfo, si_info);
		si_info->si_BroadcastStatus = ASN_RRC_SchedulingInfo__si_BroadcastStatus_broadcasting;
		si->si_Periodicity = ASN_RRC_SchedulingInfo__si_Periodicity_rf16;
		for(int j = 0; j < 1; j++){
			asn1cSequenceAdd(si->sib_MappingInfo.list, struct ASN_RRC_SIB_TypeInfo, map_info);
			map->type = ASN_RRC_SIB_TypeInfo__type_sibType2;
			asn1cCallocOne(map->valueTag, 0);
		}
	}

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


int fill_mib_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BCCH_BCH_Message_t *mib_pdu)
{
	mib_pdu =  CALLOC(1,sizeof(struct ASN_RRC_BCCH_BCH_Message_t));
	mib_pdu->message.present = ASN_RRC_BCCH_BCH_MessageType_PR_mib;
	mib_pdu->message.choice.mib = CALLOC(1,sizeof(struct ASN_RRC_MIB_t));

	ASN_RRC_MIB_t *mib = mib_pdu->message.choice.mib;

	oset_asn_uint8_to_BIT_STRING(0, (8-2), &mib->systemFrameNumber);
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
	  oset_error("Invalid carrier SCS=%d Hz", SRSRAN_SUBC_SPACING_NR(cell_cfg->phy_cell.carrier.scs));
	}
	mib->ssb_SubcarrierOffset       = cell_cfg->ssb_offset;
	mib->dmrs_TypeA_Position        = ASN_RRC_MIB__dmrs_TypeA_Position_pos2;
	mib->pdcch_ConfigSIB1.controlResourceSetZero = cell_cfg->coreset0_idx;
	mib->pdcch_ConfigSIB1.searchSpaceZero = 0;
	mib->cellBarred                 = ASN_RRC_MIB__cellBarred_notBarred;
	mib->intraFreqReselection       = ASN_RRC_MIB__intraFreqReselection_allowed;
	oset_asn_uint8_to_BIT_STRING(0, (8-7), &mib->spare);// This makes a spare of 1 bits

	return OSET_OK;
}


/// Fill SRB with parameters derived from cfg
void fill_srb(const rrc_nr_cfg_t *cfg, nr_srb srb_id, ASN_RRC_RLC_BearerConfig_t *out)
{
	ASSERT_IF_NOT(srb_id > srb0 && srb_id < count, "Invalid srb_id argument");

	out->logicalChannelIdentity = srb_to_lcid(srb_id);
	
	out->servedRadioBearer = CALLOC(1, sizeof(*out->servedRadioBearer));
	out->servedRadioBearer->present	 = ASN_RRC_RLC_BearerConfig__servedRadioBearer_PR_srb_Identity;
	out->servedRadioBearer->choice.srb_Identity = srb_to_lcid(srb_id);
	
	if (srb_id == srb1) {
		if (cfg->srb1_cfg.present) {
			out->rlc_Config = CALLOC(1, sizeof(struct ASN_RRC_RLC_Config));
			out->rlc_Config->present = cfg->srb1_cfg.rlc_cfg.types;//ASN_RRC_RLC_Config_PR_am
			out->rlc_Config->choice.am  = CALLOC(1, sizeof(*out->rlc_Config->choice.am));

			asn1cCallocOne(out->rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength, cfg->srb1_cfg.rlc_cfg.c.am.dl_am_rlc.sn_field_len);
			out->rlc_Config->choice.am->dl_AM_RLC.t_Reassembly = cfg->srb1_cfg.rlc_cfg.c.am.dl_am_rlc.t_reassembly;
			out->rlc_Config->choice.am->dl_AM_RLC.t_StatusProhibit = cfg->srb1_cfg.rlc_cfg.c.am.dl_am_rlc.t_status_prohibit;

			asn1cCallocOne(out->rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength, cfg->srb1_cfg.rlc_cfg.c.am.ul_am_rlc.sn_field_len);
			out->rlc_Config->choice.am->ul_AM_RLC.t_PollRetransmit = cfg->srb1_cfg.rlc_cfg.c.am.ul_am_rlc.t_poll_retx;
			out->rlc_Config->choice.am->ul_AM_RLC.pollPDU = cfg->srb1_cfg.rlc_cfg.c.am.ul_am_rlc.poll_pdu;
			out->rlc_Config->choice.am->ul_AM_RLC.pollByte = cfg->srb1_cfg.rlc_cfg.c.am.ul_am_rlc.poll_byte;
			out->rlc_Config->choice.am->ul_AM_RLC.maxRetxThreshold = cfg->srb1_cfg.rlc_cfg.c.am.ul_am_rlc.max_retx_thres;
		}
	} else if (srb_id == srb2) {
		if (cfg->srb2_cfg.present) {
			out->rlc_Config = CALLOC(1, sizeof(struct ASN_RRC_RLC_Config));
			out->rlc_Config->present = cfg->srb2_cfg.rlc_cfg.types;//ASN_RRC_RLC_Config_PR_am
			out->rlc_Config->choice.am	= CALLOC(1, sizeof(*out->rlc_Config->choice.am));
		
			asn1cCallocOne(out->rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength, cfg->srb2_cfg.rlc_cfg.c.am.dl_am_rlc.sn_field_len);
			out->rlc_Config->choice.am->dl_AM_RLC.t_Reassembly = cfg->srb2_cfg.rlc_cfg.c.am.dl_am_rlc.t_reassembly;
			out->rlc_Config->choice.am->dl_AM_RLC.t_StatusProhibit = cfg->srb2_cfg.rlc_cfg.c.am.dl_am_rlc.t_status_prohibit;
		
			asn1cCallocOne(out->rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength, cfg->srb2_cfg.rlc_cfg.c.am.ul_am_rlc.sn_field_len);
			out->rlc_Config->choice.am->ul_AM_RLC.t_PollRetransmit = cfg->srb2_cfg.rlc_cfg.c.am.ul_am_rlc.t_poll_retx;
			out->rlc_Config->choice.am->ul_AM_RLC.pollPDU = cfg->srb2_cfg.rlc_cfg.c.am.ul_am_rlc.poll_pdu;
			out->rlc_Config->choice.am->ul_AM_RLC.pollByte = cfg->srb2_cfg.rlc_cfg.c.am.ul_am_rlc.poll_byte;
			out->rlc_Config->choice.am->ul_AM_RLC.maxRetxThreshold = cfg->srb2_cfg.rlc_cfg.c.am.ul_am_rlc.max_retx_thres;
		}
	} else{
			out->rlc_Config = CALLOC(1, sizeof(struct ASN_RRC_RLC_Config));
			out->rlc_Config->present = ASN_RRC_RLC_Config_PR_am
			out->rlc_Config->choice.am	= CALLOC(1, sizeof(*out->rlc_Config->choice.am));
		
			asn1cCallocOne(out->rlc_Config->choice.am->dl_AM_RLC.sn_FieldLength, ASN_RRC_SN_FieldLengthAM_size12);
			out->rlc_Config->choice.am->dl_AM_RLC.t_Reassembly = ASN_RRC_T_Reassembly_ms35;
			out->rlc_Config->choice.am->dl_AM_RLC.t_StatusProhibit = ASN_RRC_T_StatusProhibit_ms0;
		
			asn1cCallocOne(out->rlc_Config->choice.am->ul_AM_RLC.sn_FieldLength, ASN_RRC_SN_FieldLengthAM_size12);
			out->rlc_Config->choice.am->ul_AM_RLC.t_PollRetransmit = ASN_RRC_T_Reassembly_ms45;
			out->rlc_Config->choice.am->ul_AM_RLC.pollPDU = ASN_RRC_PollPDU_infinity;
			out->rlc_Config->choice.am->ul_AM_RLC.pollByte = ASN_RRC_PollByte_infinity;
			out->rlc_Config->choice.am->ul_AM_RLC.maxRetxThreshold = ASN_RRC_UL_AM_RLC__maxRetxThreshold_t8;
	}

	// mac-LogicalChannelConfig -- Cond LCH-Setup
	out->mac_LogicalChannelConfig = CALLOC(1, sizeof(struct ASN_RRC_LogicalChannelConfig));
	out->mac_LogicalChannelConfig->ul_SpecificParameters 					= CALLOC(1, sizeof(*out->mac_LogicalChannelConfig->ul_SpecificParameters));
	out->mac_LogicalChannelConfig->ul_SpecificParameters->priority			= (srb_id == srb1 ? 1 : 3);
	out->mac_LogicalChannelConfig->ul_SpecificParameters->prioritisedBitRate = ASN_RRC_LogicalChannelConfig__ul_SpecificParameters__prioritisedBitRate_infinity;
	out->mac_LogicalChannelConfig->ul_SpecificParameters->bucketSizeDuration = ASN_RRC_LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms5;

	asn1cCallocOne(out->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelGroup, 0);
	asn1cCallocOne(out->mac_LogicalChannelConfig->ul_SpecificParameters->schedulingRequestID, 0);
	out->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelSR_Mask = 0;
	out->mac_LogicalChannelConfig->ul_SpecificParameters->logicalChannelSR_DelayTimerApplied = 0;
}

/// Fill csi-ResoureConfigToAddModList
void fill_csi_resource_cfg_to_add(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_CSI_MeasConfig_t *csi_meas_cfg)
{
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
		csi_meas_cfg->csi_ResourceConfigToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->csi_ResourceConfigToAddModList));
		for(int i = 0 ; i < 1; i++){
			asn1cSequenceAdd(csi_meas_cfg->csi_ResourceConfigToAddModList->list, struct ASN_RRC_CSI_ResourceConfig, csi_res_info1);
			csi_res_info1->csi_ResourceConfigId = 0;
			csi_res_info1->csi_RS_ResourceSetList.present = ASN_RRC_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_nzp_CSI_RS_SSB;
			csi_res_info1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB = CALLOC(1, sizeof(*csi_res_info1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB));
			csi_res_info1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList = CALLOC(1, sizeof(*csi_res_info1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList));
			asn1cSequenceAdd(csi_res_info1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list, struct ASN_RRC_NZP_CSI_RS_ResourceSetId_t, nzp_csi_rs1);
			*nzp_csi_rs1 = 0;
			csi_res_info1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList = CALLOC(1, sizeof(*csi_res_info1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList));
			asn1cSequenceAdd(csi_res_info1->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list, struct ASN_RRC_CSI_SSB_ResourceSetId_t, csi_ssb1);
			*csi_ssb1 = 0;
			csi_res_info1->bwp_Id = 0;
			csi_res_info1->resourceType = ASN_RRC_CSI_ResourceConfig__resourceType_periodic;

			asn1cSequenceAdd(csi_meas_cfg->csi_ResourceConfigToAddModList->list, struct ASN_RRC_CSI_ResourceConfig, csi_res_info2);
			csi_res_info2->csi_ResourceConfigId = 0;
			csi_res_info2->csi_RS_ResourceSetList.present = ASN_RRC_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_csi_IM_ResourceSetList;
			csi_res_info2->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList = CALLOC(1, sizeof(*csi_res_info2->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList));
			asn1cSequenceAdd(csi_res_info2->csi_RS_ResourceSetList.choice.csi_IM_ResourceSetList->list, struct ASN_RRC_CSI_IM_ResourceSetId_t, csi_im);
			*csi_im = 0;
			csi_res_info2->bwp_Id = 0;
			csi_res_info2->resourceType = ASN_RRC_CSI_ResourceConfig__resourceType_periodic;


			asn1cSequenceAdd(csi_meas_cfg->csi_ResourceConfigToAddModList->list, struct ASN_RRC_CSI_ResourceConfig, csi_res_info3);
			csi_res_info3->csi_ResourceConfigId = 2;
			csi_res_info3->csi_RS_ResourceSetList.present = ASN_RRC_CSI_ResourceConfig__csi_RS_ResourceSetList_PR_nzp_CSI_RS_SSB;
			csi_res_info3->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB = CALLOC(1, sizeof(*csi_res_info3->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB));
			csi_res_info3->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList = CALLOC(1, sizeof(*csi_res_info3->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList));
			asn1cSequenceAdd(csi_res_info3->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list, struct ASN_RRC_NZP_CSI_RS_ResourceSetId_t, nzp_csi_rs2);
			*nzp_csi_rs2 = 0;
			csi_res_info3->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList = CALLOC(1, sizeof(*csi_res_info3->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList));
			asn1cSequenceAdd(csi_res_info3->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list, struct ASN_RRC_CSI_SSB_ResourceSetId_t, csi_ssb2);
			*csi_ssb2 = 0;
			csi_res_info3->bwp_Id = 0;
			csi_res_info3->resourceType = ASN_RRC_CSI_ResourceConfig__resourceType_periodic;
		}
	}
}

/// Fill lists of NZP-CSI-RS-Resource and NZP-CSI-RS-ResourceSet with gNB config
void fill_nzp_csi_rs_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_CSI_MeasConfig_t *csi_meas_cfg)
{
	ASSERT_IF_NOT(cfg->is_standalone, "Not support NSA now!")

	if (cfg->is_standalone) {
		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList));
			// item 0
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList->list, struct ASN_RRC_NZP_CSI_RS_Resource, nzp_csi_res0);
			nzp_csi_res0->nzp_CSI_RS_ResourceId = 0;
			nzp_csi_res0->resourceMapping.frequencyDomainAllocation.present = ASN_RRC_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row2;
			oset_asn_uint16_to_BIT_STRING(0x800, 12, &nzp_csi_res0->resourceMapping.frequencyDomainAllocation.choice.row2);
			nzp_csi_res0->resourceMapping.nrofPorts = ASN_RRC_CSI_RS_ResourceMapping__nrofPorts_p1;
			nzp_csi_res0->resourceMapping.firstOFDMSymbolInTimeDomain = 4;
			nzp_csi_res0->resourceMapping.cdm_Type = ASN_RRC_CSI_RS_ResourceMapping__cdm_Type_noCDM;
			nzp_csi_res0->resourceMapping.density.present = ASN_RRC_CSI_RS_ResourceMapping__density_PR_one;
			nzp_csi_res0->resourceMapping.freqBand.startingRB = 0;
			nzp_csi_res0->resourceMapping.freqBand.nrofRBs = cell_cfg->phy_cell.carrier.nof_prb;
			nzp_csi_res0->powerControlOffset = 0;
			asn1cCallocOne(nzp_csi_res0->powerControlOffsetSS, ASN_RRC_NZP_CSI_RS_Resource__powerControlOffsetSS_db0);
			nzp_csi_res0->scramblingID = cell_cfg->phy_cell.cell_id;
			nzp_csi_res0->periodicityAndOffset = CALLOC(1,sizeof(struct ASN_RRC_CSI_ResourcePeriodicityAndOffset));
			nzp_csi_res0->periodicityAndOffset->present = ASN_RRC_CSI_ResourcePeriodicityAndOffset_PR_slots80;
			nzp_csi_res0->periodicityAndOffset->choice.slots80 = 1;
			asn1cCallocOne(nzp_csi_res0->qcl_InfoPeriodicCSI_RS, 0);
			
			// item 1
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList->list, struct ASN_RRC_NZP_CSI_RS_Resource, nzp_csi_res1);
			nzp_csi_res1->nzp_CSI_RS_ResourceId = 1;
			nzp_csi_res1->resourceMapping.frequencyDomainAllocation.present = ASN_RRC_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1;
			oset_asn_uint8_to_BIT_STRING(0x1, (8-4), &nzp_csi_res1->resourceMapping.frequencyDomainAllocation.choice.row1);
			nzp_csi_res1->resourceMapping.nrofPorts = ASN_RRC_CSI_RS_ResourceMapping__nrofPorts_p1;
			nzp_csi_res1->resourceMapping.firstOFDMSymbolInTimeDomain = 4;
			nzp_csi_res1->resourceMapping.cdm_Type = ASN_RRC_CSI_RS_ResourceMapping__cdm_Type_noCDM;
			nzp_csi_res1->resourceMapping.density.present = ASN_RRC_CSI_RS_ResourceMapping__density_PR_three;
			nzp_csi_res1->resourceMapping.freqBand.startingRB = 0;
			nzp_csi_res1->resourceMapping.freqBand.nrofRBs = cell_cfg->phy_cell.carrier.nof_prb;
			nzp_csi_res1->powerControlOffset = 0;
			asn1cCallocOne(nzp_csi_res1->powerControlOffsetSS, ASN_RRC_NZP_CSI_RS_Resource__powerControlOffsetSS_db0);
			nzp_csi_res1->scramblingID = cell_cfg->phy_cell.cell_id;
			nzp_csi_res1->periodicityAndOffset = CALLOC(1,sizeof(struct ASN_RRC_CSI_ResourcePeriodicityAndOffset));
			nzp_csi_res1->periodicityAndOffset->present = ASN_RRC_CSI_ResourcePeriodicityAndOffset_PR_slots40;
			nzp_csi_res1->periodicityAndOffset->choice.slots40 = 11;
			asn1cCallocOne(nzp_csi_res1->qcl_InfoPeriodicCSI_RS, 0);


			// item 2
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList->list, struct ASN_RRC_NZP_CSI_RS_Resource, nzp_csi_res2);
			nzp_csi_res2->nzp_CSI_RS_ResourceId = 2;
			nzp_csi_res2->resourceMapping.frequencyDomainAllocation.present = ASN_RRC_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1;
			oset_asn_uint8_to_BIT_STRING(0x1, (8-4), &nzp_csi_res2->resourceMapping.frequencyDomainAllocation.choice.row1);
			nzp_csi_res2->resourceMapping.nrofPorts = ASN_RRC_CSI_RS_ResourceMapping__nrofPorts_p1;
			nzp_csi_res2->resourceMapping.firstOFDMSymbolInTimeDomain = 8;
			nzp_csi_res2->resourceMapping.cdm_Type = ASN_RRC_CSI_RS_ResourceMapping__cdm_Type_noCDM;
			nzp_csi_res2->resourceMapping.density.present = ASN_RRC_CSI_RS_ResourceMapping__density_PR_three;
			nzp_csi_res2->resourceMapping.freqBand.startingRB = 0;
			nzp_csi_res2->resourceMapping.freqBand.nrofRBs = cell_cfg->phy_cell.carrier.nof_prb;
			nzp_csi_res2->powerControlOffset = 0;
			asn1cCallocOne(nzp_csi_res2->powerControlOffsetSS, ASN_RRC_NZP_CSI_RS_Resource__powerControlOffsetSS_db0);
			nzp_csi_res2->scramblingID = cell_cfg->phy_cell.cell_id;
			nzp_csi_res2->periodicityAndOffset = CALLOC(1,sizeof(struct ASN_RRC_CSI_ResourcePeriodicityAndOffset));
			nzp_csi_res2->periodicityAndOffset->present = ASN_RRC_CSI_ResourcePeriodicityAndOffset_PR_slots40;
			nzp_csi_res2->periodicityAndOffset->choice.slots40 = 11;
			asn1cCallocOne(nzp_csi_res2->qcl_InfoPeriodicCSI_RS, 0);

			// item 3
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList->list, struct ASN_RRC_NZP_CSI_RS_Resource, nzp_csi_res3);
			nzp_csi_res3->nzp_CSI_RS_ResourceId = 3;
			nzp_csi_res3->resourceMapping.frequencyDomainAllocation.present = ASN_RRC_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1;
			oset_asn_uint8_to_BIT_STRING(0x1, (8-4), &nzp_csi_res3->resourceMapping.frequencyDomainAllocation.choice.row1);
			nzp_csi_res3->resourceMapping.nrofPorts = ASN_RRC_CSI_RS_ResourceMapping__nrofPorts_p1;
			nzp_csi_res3->resourceMapping.firstOFDMSymbolInTimeDomain = 4;
			nzp_csi_res3->resourceMapping.cdm_Type = ASN_RRC_CSI_RS_ResourceMapping__cdm_Type_noCDM;
			nzp_csi_res3->resourceMapping.density.present = ASN_RRC_CSI_RS_ResourceMapping__density_PR_three;
			nzp_csi_res3->resourceMapping.freqBand.startingRB = 0;
			nzp_csi_res3->resourceMapping.freqBand.nrofRBs = cell_cfg->phy_cell.carrier.nof_prb;
			nzp_csi_res3->powerControlOffset = 0;
			asn1cCallocOne(nzp_csi_res3->powerControlOffsetSS, ASN_RRC_NZP_CSI_RS_Resource__powerControlOffsetSS_db0);
			nzp_csi_res3->scramblingID = cell_cfg->phy_cell.cell_id;
			nzp_csi_res3->periodicityAndOffset = CALLOC(1,sizeof(struct ASN_RRC_CSI_ResourcePeriodicityAndOffset));
			nzp_csi_res3->periodicityAndOffset->present = ASN_RRC_CSI_ResourcePeriodicityAndOffset_PR_slots40;
			nzp_csi_res3->periodicityAndOffset->choice.slots40 = 12;
			asn1cCallocOne(nzp_csi_res3->qcl_InfoPeriodicCSI_RS, 0);

			// item 4
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList->list, struct ASN_RRC_NZP_CSI_RS_Resource, nzp_csi_res4);
			nzp_csi_res4->nzp_CSI_RS_ResourceId = 4;
			nzp_csi_res4->resourceMapping.frequencyDomainAllocation.present = ASN_RRC_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row1;
			oset_asn_uint8_to_BIT_STRING(0x1, (8-4), &nzp_csi_res4->resourceMapping.frequencyDomainAllocation.choice.row1);
			nzp_csi_res4->resourceMapping.nrofPorts = ASN_RRC_CSI_RS_ResourceMapping__nrofPorts_p1;
			nzp_csi_res4->resourceMapping.firstOFDMSymbolInTimeDomain = 8;
			nzp_csi_res4->resourceMapping.cdm_Type = ASN_RRC_CSI_RS_ResourceMapping__cdm_Type_noCDM;
			nzp_csi_res4->resourceMapping.density.present = ASN_RRC_CSI_RS_ResourceMapping__density_PR_three;
			nzp_csi_res4->resourceMapping.freqBand.startingRB = 0;
			nzp_csi_res4->resourceMapping.freqBand.nrofRBs = cell_cfg->phy_cell.carrier.nof_prb;
			nzp_csi_res4->powerControlOffset = 0;
			asn1cCallocOne(nzp_csi_res4->powerControlOffsetSS, ASN_RRC_NZP_CSI_RS_Resource__powerControlOffsetSS_db0);
			nzp_csi_res4->scramblingID = cell_cfg->phy_cell.cell_id;
			nzp_csi_res4->periodicityAndOffset = CALLOC(1,sizeof(struct ASN_RRC_CSI_ResourcePeriodicityAndOffset));
			nzp_csi_res4->periodicityAndOffset->present = ASN_RRC_CSI_ResourcePeriodicityAndOffset_PR_slots40;
			nzp_csi_res4->periodicityAndOffset->choice.slots40 = 12;
			asn1cCallocOne(nzp_csi_res4->qcl_InfoPeriodicCSI_RS, 0);
		} else {
			csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList));
			// item 0
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceToAddModList->list, struct ASN_RRC_NZP_CSI_RS_Resource, nzp_csi_res0);
			nzp_csi_res0->nzp_CSI_RS_ResourceId = 0;
			nzp_csi_res0->resourceMapping.frequencyDomainAllocation.present = ASN_RRC_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row2;
			oset_asn_uint16_to_BIT_STRING(0b100000000000, 12, &nzp_csi_res0->resourceMapping.frequencyDomainAllocation.choice.row2);
			nzp_csi_res0->resourceMapping.nrofPorts = ASN_RRC_CSI_RS_ResourceMapping__nrofPorts_p1;
			nzp_csi_res0->resourceMapping.firstOFDMSymbolInTimeDomain = 4;
			nzp_csi_res0->resourceMapping.cdm_Type = ASN_RRC_CSI_RS_ResourceMapping__cdm_Type_noCDM;
			nzp_csi_res0->resourceMapping.density.present = ASN_RRC_CSI_RS_ResourceMapping__density_PR_one;
			nzp_csi_res0->resourceMapping.freqBand.startingRB = 0;
			nzp_csi_res0->resourceMapping.freqBand.nrofRBs = cell_cfg->phy_cell.carrier.nof_prb;
			nzp_csi_res0->powerControlOffset = 0;
			nzp_csi_res0->periodicityAndOffset = CALLOC(1,sizeof(struct ASN_RRC_CSI_ResourcePeriodicityAndOffset));
			nzp_csi_res0->periodicityAndOffset->present = ASN_RRC_CSI_ResourcePeriodicityAndOffset_PR_slots80;
			nzp_csi_res0->periodicityAndOffset->choice.slots80 = 0;
			asn1cCallocOne(nzp_csi_res0->qcl_InfoPeriodicCSI_RS, 0);
		}

		// Fill NZP-CSI Resource Sets
		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			csi_meas_cfg->nzp_CSI_RS_ResourceSetToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->nzp_CSI_RS_ResourceSetToAddModList));
			// item 0
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceSetToAddModList->list, struct ASN_RRC_NZP_CSI_RS_ResourceSet, nzp_csi_res_set0);
			nzp_csi_res_set0->nzp_CSI_ResourceSetId = 0;
			asn1cSequenceAdd(nzp_csi_res_set0->nzp_CSI_RS_Resources.list, ASN_RRC_NZP_CSI_RS_ResourceId_t, nzp_csi_rs_res00);
			*nzp_csi_rs_res00 = 0;
			
			// item 1
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceSetToAddModList->list, struct ASN_RRC_NZP_CSI_RS_ResourceSet, nzp_csi_res_set1);
			nzp_csi_res_set1->nzp_CSI_ResourceSetId = 0;
			asn1cSequenceAdd(nzp_csi_res_set1->nzp_CSI_RS_Resources.list, ASN_RRC_NZP_CSI_RS_ResourceId_t, nzp_csi_rs_res10);
			*nzp_csi_rs_res10 = 1;
			asn1cSequenceAdd(nzp_csi_res_set1->nzp_CSI_RS_Resources.list, ASN_RRC_NZP_CSI_RS_ResourceId_t, nzp_csi_rs_res11);
			*nzp_csi_rs_res11 = 2;
			asn1cSequenceAdd(nzp_csi_res_set1->nzp_CSI_RS_Resources.list, ASN_RRC_NZP_CSI_RS_ResourceId_t, nzp_csi_rs_res12);
			*nzp_csi_rs_res12 = 3;
			asn1cSequenceAdd(nzp_csi_res_set1->nzp_CSI_RS_Resources.list, ASN_RRC_NZP_CSI_RS_ResourceId_t, nzp_csi_rs_res13);
			*nzp_csi_rs_res13 = 4;
			//asn1cCallocOne(nzp_csi_res_set1->trs_Info, 0);
			// Skip TRS info
		} else {
			csi_meas_cfg->nzp_CSI_RS_ResourceSetToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->nzp_CSI_RS_ResourceSetToAddModList));
			// item 0
			asn1cSequenceAdd(csi_meas_cfg->nzp_CSI_RS_ResourceSetToAddModList->list, struct ASN_RRC_NZP_CSI_RS_ResourceSet, nzp_csi_res_set0);
			nzp_csi_res_set0->nzp_CSI_ResourceSetId = 0;
			asn1cSequenceAdd(nzp_csi_res_set0->nzp_CSI_RS_Resources.list, ASN_RRC_NZP_CSI_RS_ResourceId_t, nzp_csi_rs_res00);
			*nzp_csi_rs_res00 = 0;
			// Skip TRS info
		}
	} 
}

void fill_csi_im_resource_cfg_to_add(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_CSI_MeasConfig_t *csi_meas_cfg)
{
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD)
	{
		// csi-IM-ResourceToAddModList
		csi_meas_cfg->csi_IM_ResourceToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->csi_IM_ResourceToAddModList));
		asn1cSequenceAdd(csi_meas_cfg->csi_IM_ResourceToAddModList->list, struct ASN_RRC_CSI_IM_Resource, csi_im);
		csi_im->csi_IM_ResourceId = 0;
		// csi-im-resource pattern1
		csi_im->csi_IM_ResourceElementPattern = CALLOC(1, sizeof(*csi_im->csi_IM_ResourceElementPattern));
		csi_im->csi_IM_ResourceElementPattern->present = ASN_RRC_CSI_IM_Resource__csi_IM_ResourceElementPattern_PR_pattern1;
		csi_im->csi_IM_ResourceElementPattern->choice->pattern1 = CALLOC(1, sizeof(*csi_im->csi_IM_ResourceElementPattern->choice->pattern1));
		csi_im->csi_IM_ResourceElementPattern->choice.pattern1->subcarrierLocation_p1 = ASN_RRC_CSI_IM_Resource__csi_IM_ResourceElementPattern__pattern1__subcarrierLocation_p1_s8;
		csi_im->csi_IM_ResourceElementPattern->choice.pattern1->symbolLocation_p1 = 8; //nzpcsi->resourceMapping.firstOFDMSymbolInTimeDomain // same symbol as CSI-RS
		// csi-im-resource freqBand
		csi_im->freqBand = CALLOC(1,sizeof(*csi_im->freqBand));
		csi_im->freqBand->startingRB = 0;
		csi_im->freqBand->nrofRBs = cell_cfg->phy_cell.carrier.nof_prb;
		// csi-im-resource periodicity_and_offset
		csi_im->periodicityAndOffset = CALLOC(1,sizeof(*csi_im->periodicityAndOffset));
		csi_im->periodicityAndOffset->present = ASN_RRC_CSI_ResourcePeriodicityAndOffset_PR_slots80;//nzpcsi->periodicityAndOffset->present// same period and offset of the associated CSI-RS
		csi_im->periodicityAndOffset->choice.slots80 = 1;

		// csi-IM-ResourceSetToAddModList
		csi_meas_cfg->csi_IM_ResourceSetToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->csi_IM_ResourceSetToAddModList));
		asn1cSequenceAdd(csi_meas_cfg->csi_IM_ResourceSetToAddModList->list, struct ASN_RRC_CSI_IM_ResourceSet, csi_im_set);
		csi_im_set->csi_IM_ResourceSetId = 0;
		asn1cSequenceAdd(csi_im_set->csi_IM_Resources.list, struct ASN_RRC_CSI_IM_ResourceId_t, csi_im_rs_id);
		*csi_im_rs_id = 0;
  }
}

/// Fill list of CSI-ReportConfig with gNB config
int fill_csi_report_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_CSI_MeasConfig_t *csi_meas_cfg)
{
	if (cfg->is_standalone) {
		csi_meas_cfg->csi_ReportConfigToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->csi_ReportConfigToAddModList));
		asn1cSequenceAdd(csi_meas_cfg->csi_ReportConfigToAddModList->list, struct ASN_RRC_CSI_ReportConfig, csi_rp_cfg);
		csi_rp_cfg->reportConfigId = 0;
		csi_rp_cfg->resourcesForChannelMeasurement = 0;
		asn1cCallocOne(csi_rp_cfg->csi_IM_ResourcesForInterference, 1);
		csi_rp_cfg->reportConfigType.present = ASN_RRC_CSI_ReportConfig__reportConfigType_PR_periodic;
        csi_rp_cfg->reportConfigType.choice.periodic = CALLOC(1,sizeof(*csi_rp_cfg->reportConfigType.choice.periodic));
        csi_rp_cfg->reportConfigType.choice.periodic->reportSlotConfig.present=ASN_RRC_CSI_ReportPeriodicityAndOffset_PR_slots80;
        //csi_rp_cfg->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80 = 0;
		asn1cSequenceAdd(csi_rp_cfg->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list, struct ASN_RRC_PUCCH_CSI_Resource, pucchcsires);
		pucchcsires->uplinkBandwidthPartId = 0;
		pucchcsires->pucch_Resource = 17;// was 17 in orig PCAP, but code for NSA it was set to 1

		csi_rp_cfg->reportQuantity.present =ASN_RRC_CSI_ReportConfig__reportQuantity_PR_cri_RI_PMI_CQI;
		csi_rp_cfg->reportQuantity.choice.cri_RI_PMI_CQI = (NULL_t)0;

		// Report freq config (optional)
		csi_rp_cfg->reportFreqConfiguration = CALLOC(1,sizeof(*csi_rp_cfg->reportFreqConfiguration));
		asn1cCallocOne(csi_rp_cfg->reportFreqConfiguration->cqi_FormatIndicator, ASN_RRC_CSI_ReportConfig__reportFreqConfiguration__cqi_FormatIndicator_widebandCQI);
		asn1cCallocOne(csi_rp_cfg->reportFreqConfiguration->pmi_FormatIndicator, ASN_RRC_CSI_ReportConfig__reportFreqConfiguration__pmi_FormatIndicator_widebandPMI);

		csi_rp_cfg->timeRestrictionForChannelMeasurements = ASN_RRC_CSI_ReportConfig__timeRestrictionForChannelMeasurements_notConfigured;
		csi_rp_cfg->timeRestrictionForInterferenceMeasurements = ASN_RRC_CSI_ReportConfig__timeRestrictionForInterferenceMeasurements_notConfigured;

		csi_rp_cfg->codebookConfig = CALLOC(1, sizeof(struct ASN_RRC_CodebookConfig));
        csi_rp_cfg->codebookConfig->codebookType.present = ASN_RRC_CodebookConfig__codebookType_PR_type1;
        csi_rp_cfg->codebookConfig->codebookType.choice.type1 = CALLOC(1,sizeof(*csi_rp_cfg->codebookConfig->codebookType.choice.type1));
        csi_rp_cfg->codebookConfig->codebookType.choice.type1->subType.present=ASN_RRC_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel;
		asn1cCalloc(csi_rp_cfg->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel, singlePanelConfig);
        singlePanelConfig->nrOfAntennaPorts.present = ASN_RRC_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_two;
        singlePanelConfig->nrOfAntennaPorts.choice.two = CALLOC(1,sizeof(*singlePanelConfig->nrOfAntennaPorts.choice.two));
		oset_asn_uint8_to_BIT_STRING(0b111111, (8-6), &singlePanelConfig->nrOfAntennaPorts.choice.two->twoTX_CodebookSubsetRestriction);
		oset_asn_uint8_to_BIT_STRING(0x03, (8-8), &singlePanelConfig->typeI_SinglePanel_ri_Restriction);
        csi_rp_cfg->codebookConfig->codebookType.choice.type1->codebookMode=1;

        csi_rp_cfg->groupBasedBeamReporting.present = ASN_RRC_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled;
        csi_rp_cfg->groupBasedBeamReporting.choice.disabled=CALLOC(1,sizeof(*csi_rp_cfg->groupBasedBeamReporting.choice.disabled));

		// Skip CQI table (optional)
		asn1cCallocOne(csi_rp_cfg->cqi_Table, ASN_RRC_CSI_ReportConfig__cqi_Table_table1);
        csi_rp_cfg->subbandSize = ASN_RRC_CSI_ReportConfig__subbandSize_value2;

		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			csi_rp_cfg->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80 = 1;
		} else {
			csi_rp_cfg->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80 = 7;
		}
	} else {
		csi_meas_cfg->csi_ReportConfigToAddModList = CALLOC(1, sizeof(*csi_meas_cfg->csi_ReportConfigToAddModList));
		asn1cSequenceAdd(csi_meas_cfg->csi_ReportConfigToAddModList->list, struct ASN_RRC_CSI_ReportConfig, csi_rp_cfg);
		csi_rp_cfg->reportConfigId = 0;
		csi_rp_cfg->resourcesForChannelMeasurement = 0;
		asn1cCallocOne(csi_rp_cfg->csi_IM_ResourcesForInterference, 1);
		csi_rp_cfg->reportConfigType.present = ASN_RRC_CSI_ReportConfig__reportConfigType_PR_periodic;
        csi_rp_cfg->reportConfigType.choice.periodic = CALLOC(1,sizeof(*csi_rp_cfg->reportConfigType.choice.periodic));
        csi_rp_cfg->reportConfigType.choice.periodic->reportSlotConfig.present=ASN_RRC_CSI_ReportPeriodicityAndOffset_PR_slots80;
        //csi_rp_cfg->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80 = 0;
		asn1cSequenceAdd(csi_rp_cfg->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list, struct ASN_RRC_PUCCH_CSI_Resource, pucchcsires);
		pucchcsires->uplinkBandwidthPartId = 0;
		pucchcsires->pucch_Resource = 17;// was 17 in orig PCAP, but code for NSA it was set to 1

		csi_rp_cfg->reportQuantity.present =ASN_RRC_CSI_ReportConfig__reportQuantity_PR_cri_RI_PMI_CQI;
		csi_rp_cfg->reportQuantity.choice.cri_RI_PMI_CQI = (NULL_t)0;
		
		// Report freq config (optional)
		csi_rp_cfg->reportFreqConfiguration = CALLOC(1,sizeof(*csi_rp_cfg->reportFreqConfiguration));
		asn1cCallocOne(csi_rp_cfg->reportFreqConfiguration->cqi_FormatIndicator, ASN_RRC_CSI_ReportConfig__reportFreqConfiguration__cqi_FormatIndicator_widebandCQI);

		csi_rp_cfg->timeRestrictionForChannelMeasurements = ASN_RRC_CSI_ReportConfig__timeRestrictionForChannelMeasurements_notConfigured;
		csi_rp_cfg->timeRestrictionForInterferenceMeasurements = ASN_RRC_CSI_ReportConfig__timeRestrictionForInterferenceMeasurements_notConfigured;

		csi_rp_cfg->groupBasedBeamReporting.present = ASN_RRC_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled;
        csi_rp_cfg->groupBasedBeamReporting.choice.disabled=CALLOC(1,sizeof(*csi_rp_cfg->groupBasedBeamReporting.choice.disabled));

		// Skip CQI table (optional)
		asn1cCallocOne(csi_rp_cfg->cqi_Table, ASN_RRC_CSI_ReportConfig__cqi_Table_table1);
        csi_rp_cfg->subbandSize = ASN_RRC_CSI_ReportConfig__subbandSize_value2;

		if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
			csi_rp_cfg->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80 = 5;
		} else {
			csi_rp_cfg->reportConfigType.choice.periodic->reportSlotConfig.choice.slots80 = 7;
		}
	}

	return OSET_OK;
}


/// Fill CSI-MeasConfig with gNB config
int fill_csi_meas_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_CSI_MeasConfig_t *csi_meas_cfg)
{
	//  // Fill CSI Report
	//  if (fill_csi_report_from_enb_cfg(cfg, csi_meas_cfg) != OSET_OK) {
	//    oset_error("Failed to configure eNB CSI Report");
	//    return OSET_ERROR;
	//  }

	// Fill CSI resource config
	fill_csi_resource_cfg_to_add(cfg, cell_cfg, csi_meas_cfg);

	// Fill NZP-CSI Resources
	fill_nzp_csi_rs_from_enb_cfg(cfg, cell_cfg, csi_meas_cfg);

	if (cfg->is_standalone) {
		// CSI IM config
		fill_csi_im_resource_cfg_to_add(cfg, cell_cfg, csi_meas_cfg);

		// CSI report config
		fill_csi_report_from_enb_cfg(cfg, cell_cfg, csi_meas_cfg);
	}

	return OSET_OK;
}

void fill_pdsch_cfg_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_PDSCH_Config_t *out)
{
	out->dmrs_DownlinkForPDSCH_MappingTypeA = CALLOC(1,sizeof(struct ASN_RRC_SetupRelease_DMRS_DownlinkConfig));
	out->dmrs_DownlinkForPDSCH_MappingTypeA->present= ASN_RRC_SetupRelease_DMRS_DownlinkConfig_PR_setup;
	out->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup = CALLOC(1,sizeof(struct ASN_RRC_DMRS_DownlinkConfig));
	asn1cCallocOne(out->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition, ASN_RRC_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1);

	out->tci_StatesToAddModList = CALLOC(1, sizeof(*out->tci_StatesToAddModList));
	for(int i = 0; i< 1; i++) {
		asn1cSequenceAdd(out->tci_StatesToAddModList->list, struct ASN_RRC_TCI_State, tcic);
		tcic->tci_StateId=0;
		tcic->qcl_Type1.referenceSignal.present = ASN_RRC_QCL_Info__referenceSignal_PR_ssb;
		tcic->qcl_Type1.referenceSignal.choice.ssb = 0;
		tcic->qcl_Type1.qcl_Type = ASN_RRC_QCL_Info__qcl_Type_typeD;
	}

	out->resourceAllocation = ASN_RRC_PDSCH_Config__resourceAllocation_resourceAllocationType1;
	out->rbg_Size = ASN_RRC_PDSCH_Config__rbg_Size_config1;

	out->prb_BundlingType.present = ASN_RRC_PDSCH_Config__prb_BundlingType_PR_staticBundling;
	out->prb_BundlingType.choice.staticBundling = CALLOC(1, sizeof(*out->prb_BundlingType.choice.staticBundling));
	asn1cCallocOne(out->prb_BundlingType.choice.staticBundling->bundleSize, ASN_RRC_PDSCH_Config__prb_BundlingType__staticBundling__bundleSize_wideband);

	// MCS Table
	// NOTE: For Table 1 or QAM64, set false and comment value
	asn1cCallocOne(out->mcs_Table, ASN_RRC_PDSCH_Config__mcs_Table_qam256);

	// ZP-CSI
	out->zp_CSI_RS_ResourceToAddModList = CALLOC(1, sizeof(*out->zp_CSI_RS_ResourceToAddModList));
	for(int i = 0; i< 1; i++) {
		asn1cSequenceAdd(out->zp_CSI_RS_ResourceToAddModList->list, struct ASN_RRC_ZP_CSI_RS_Resource, zp_csi_rs);
		zp_csi_rs->zp_CSI_RS_ResourceId = 0;
		zp_csi_rs->resourceMapping.frequencyDomainAllocation.present = ASN_RRC_CSI_RS_ResourceMapping__frequencyDomainAllocation_PR_row4;
		oset_asn_uint8_to_BIT_STRING(0b100, (8-3), &zp_csi_rs->resourceMapping.frequencyDomainAllocation.choice.row4);
		zp_csi_rs->resourceMapping.nrofPorts = ASN_RRC_CSI_RS_ResourceMapping__nrofPorts_p4;

		zp_csi_rs->resourceMapping.firstOFDMSymbolInTimeDomain = 8;
		zp_csi_rs->resourceMapping.cdm_Type = ASN_RRC_CSI_RS_ResourceMapping__cdm_Type_fd_CDM2;
		zp_csi_rs->resourceMapping.density.present = ASN_RRC_CSI_RS_ResourceMapping__density_PR_one;
	
		zp_csi_rs->resourceMapping.freqBand.startingRB = 0;
		zp_csi_rs->resourceMapping.freqBand.nrofRBs = cell_cfg->phy_cell.carrier.nof_prb;
		zp_csi_rs->periodicityAndOffset = CALLOC(1,sizeof(struct ASN_RRC_CSI_ResourcePeriodicityAndOffset));
		zp_csi_rs->periodicityAndOffset->present = ASN_RRC_CSI_ResourcePeriodicityAndOffset_PR_slots80;
		zp_csi_rs->periodicityAndOffset->choice.slots80 = 1;
	}

	// TEMP
	/*out->p_ZP_CSI_RS_ResourceSet = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_ZP_CSI_RS_ResourceSet));
	out->p_ZP_CSI_RS_ResourceSet->present = ASN_RRC_SetupRelease_ZP_CSI_RS_ResourceSet_PR_setup;
	out->p_ZP_CSI_RS_ResourceSet->choice.setup =  CALLOC(1, sizeof(*out->p_ZP_CSI_RS_ResourceSet->choice.setup));
	out->p_ZP_CSI_RS_ResourceSet->choice.setup->zp_CSI_RS_ResourceSetId = 0;
	for(int i = 0; i< 1; i++) {
		asn1cSequenceAdd(out->p_ZP_CSI_RS_ResourceSet->choice.setup->zp_CSI_RS_ResourceIdList.list, ASN_RRC_ZP_CSI_RS_ResourceId_t, zp_csi_rs_res);
		*zp_csi_rs_res- = 0;
	}*/
}


/// Fill InitDlBwp with gNB config
int fill_init_dl_bwp_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BWP_DownlinkDedicated_t *init_dl_bwp)
{
	init_dl_bwp->pdcch_Config = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PDCCH_Config));
	init_dl_bwp->pdcch_Config->present = ASN_RRC_SetupRelease_PDCCH_Config_PR_setup;
	init_dl_bwp->pdcch_Config->choice.setup = CALLOC(1, sizeof(*init_dl_bwp->pdcch_Config->choice.setup));
	//coreset
	asn1cCalloc(init_dl_bwp->pdcch_Config->choice.setup->controlResourceSetToAddModList, ctl_rsset_add_list);
	for(int i = 0; i< 1; i++) {
		struct ctrl_res_set_s *coreset2_data = byn_array_get_data(&cell_cfg->pdcch_cfg_ded.ctrl_res_set_to_add_mod_list, i)
		asn1cSequenceAdd(ctl_rsset_add_list->list, struct ASN_RRC_ControlResourceSet, coreset);
		coreset->controlResourceSetId = coreset2_data->ctrl_res_set_id;
		oset_asn_buffer_to_BIT_STRING(coreset2_data->freq_domain_res, 6, 3, &coreset->frequencyDomainResources);
		coreset->duration = coreset2_data->dur;
		coreset->cce_REG_MappingType.present = coreset2_data->cce_reg_map_type.types;//ASN_RRC_ControlResourceSet__cce_REG_MappingType_PR_nonInterleaved
		coreset->precoderGranularity = coreset2_data->precoder_granularity;//ASN_RRC_ControlResourceSet__precoderGranularity_sameAsREG_bundle
	}

	//searchspace
	asn1cCalloc(init_dl_bwp->pdcch_Config->choice.setup->searchSpacesToAddModList, ss_add_list);
	for(int i = 0; i< 1; i++) {
		struct search_space_s *ss2_data = byn_array_get_data(&cell_cfg->pdcch_cfg_ded.search_spaces_to_add_mod_list, i)
		asn1cSequenceAdd(ss_add_list->list, struct ASN_RRC_SearchSpace, ss);
		ss->searchSpaceId = ss2_data->search_space_id;
		asn1cCallocOne(ss->controlResourceSetId, ss2_data->ctrl_res_set_id);
		ss->monitoringSlotPeriodicityAndOffset = CALLOC(1,sizeof(*ss->monitoringSlotPeriodicityAndOffset));
		ss->monitoringSlotPeriodicityAndOffset->present = ASN_RRC_SearchSpace__monitoringSlotPeriodicityAndOffset_PR_sl1;
		ss->monitoringSymbolsWithinSlot = CALLOC(1,sizeof(*ss->monitoringSymbolsWithinSlot));
		oset_asn_buffer_to_BIT_STRING(ss2_data->monitoring_symbols_within_slot, 2, 2, ss->monitoringSymbolsWithinSlot);
		ss->nrofCandidates = CALLOC(1,sizeof(*ss->nrofCandidates));
		ss->nrofCandidates->aggregationLevel1 = ss2_data->nrof_candidates.aggregation_level1;
		ss->nrofCandidates->aggregationLevel2 = ss2_data->nrof_candidates.aggregation_level2;
		ss->nrofCandidates->aggregationLevel4 = ss2_data->nrof_candidates.aggregation_level4;
		ss->nrofCandidates->aggregationLevel8 = ss2_data->nrof_candidates.aggregation_level8;
		ss->nrofCandidates->aggregationLevel16 = ss2_data->nrof_candidates.aggregation_level16;
		ss->searchSpaceType = CALLOC(1,sizeof(*ss->searchSpaceType));
		ss->searchSpaceType->present = ASN_RRC_SearchSpace__searchSpaceType_PR_ue_Specific;
		ss->searchSpaceType->choice.ue_Specific=CALLOC(1,sizeof(*ss->searchSpaceType->choice.ue_Specific));
		ss->searchSpaceType->choice.ue_Specific->dci_Formats = ss2_data->search_space_type.c.ue_spec.dci_formats;
	}

	init_dl_bwp->pdsch_Config = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PDSCH_Config));
	init_dl_bwp->pdsch_Config->present = ASN_RRC_SetupRelease_PDSCH_Config_PR_setup;
	init_dl_bwp->pdsch_Config->choice.setup = CALLOC(1, sizeof(*init_dl_bwp->pdsch_Config->choice.setup));

	fill_pdsch_cfg_from_enb_cfg(cfg, cell_cfg, init_dl_bwp->pdsch_Config->choice.setup);
	// TODO: ADD missing fields

	return OSET_OK;
}

void fill_pucch_cfg_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_PUCCH_Config_t *out)
{
	out->resourceSetToAddModList = CALLOC(1, sizeof(*out->resourceSetToAddModList));
	// Make 2 PUCCH resource sets
	// Make PUCCH resource set for 1-2 bit
	for(int set_id = 0; set_id< 2; set_id++) {
		asn1cSequenceAdd(out->resourceSetToAddModList->list, struct ASN_RRC_PUCCH_ResourceSet, pucch_res_set);
		pucch_res_set->pucch_ResourceSetId = set_id;
		for(int i = 0; i< 8; i++) {
			asn1cSequenceAdd(pucch_res_set->resourceList.list, struct ASN_RRC_PUCCH_ResourceId_t, pucch_res_id);
			if (cfg.is_standalone) {
			  *pucch_res_id = i + set_id * 8;
			} else {
			  *pucch_res_id = set_id;
			}
		}
	}

	// Make 3 possible resources
	out->resourceToAddModList = CALLOC(1, sizeof(*out->resourceToAddModList));
	uint32_t j = 0, j2 = 0;
	for(int m = 0; m< 2; m++) {
		asn1cSequenceAdd(out->resourceToAddModList->list, struct ASN_RRC_PUCCH_Resource, pucch_res);
		pucch_res->pucch_ResourceId = m;
		if (m < 8 or m == 16) {
			pucch_res->startingPRB = cell_cfg->phy_cell.carrier.nof_prb - 1;//51
			asn1cCallocOne(pucch_res->secondHopPRB, 0);
			pucch_res->format.present = ASN_RRC_PUCCH_Resource__format_PR_format1;
			pucch_res->format.choice.format1 = CALLOC(1, sizeof(struct ASN_RRC_PUCCH_format1));
			pucch_res->format.choice.format1->initialCyclicShift = (4 * (j % 3));
			pucch_res->format.choice.format1->nrofSymbols = 14;
			pucch_res->format.choice.format1->startingSymbolIndex = 0;
			pucch_res->format.choice.format1->timeDomainOCC = j / 3;
			j++;
		} else if (m < 15) {
			pucch_res->startingPRB = 1;
			asn1cCallocOne(pucch_res->secondHopPRB, cell_cfg->phy_cell.carrier.nof_prb-2);//50
			pucch_res->format.present = ASN_RRC_PUCCH_Resource__format_PR_format2;
			pucch_res->format.choice.format2 = CALLOC(1, sizeof(struct ASN_RRC_PUCCH_format2));
			pucch_res->format.choice.format2->nrofPRBs = 1;
			pucch_res->format.choice.format2->nrofSymbols = 2;
			pucch_res->format.choice.format2->startingSymbolIndex = 2 * (j2 % 7);
			j2++;
		} else {
			pucch_res->startingPRB = cell_cfg->phy_cell.carrier.nof_prb -2;//50
			asn1cCallocOne(pucch_res->secondHopPRB, 1);
			pucch_res->format.present = ASN_RRC_PUCCH_Resource__format_PR_format2;
			pucch_res->format.choice.format2 = CALLOC(1, sizeof(struct ASN_RRC_PUCCH_format2));
			pucch_res->format.choice.format2->nrofPRBs = 1;
			pucch_res->format.choice.format2->nrofSymbols = 2;
			pucch_res->format.choice.format2->startingSymbolIndex = 2 * (j2 % 7);
		}
	}

	out->format1 = CALLOC(1, sizeof(*struct ASN_RRC_SetupRelease_PUCCH_FormatConfig));
	out->format1->present = ASN_RRC_SetupRelease_PUCCH_FormatConfig_PR_setup;
	out->format1->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PUCCH_FormatConfig));
	
	out->format2 = CALLOC(1, sizeof(*struct ASN_RRC_SetupRelease_PUCCH_FormatConfig));
	out->format1->present = ASN_RRC_SetupRelease_PUCCH_FormatConfig_PR_setup;
	out->format1->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PUCCH_FormatConfig));
	asn1cCallocOne(out->format1->choice.setup->maxCodeRate, ASN_RRC_PUCCH_MaxCodeRate_zeroDot25);
	// NOTE: IMPORTANT!! The gNB expects the CSI to be reported along with HARQ-ACK
	// If simul_harq_ack_csi_present = false, PUCCH might not be decoded properly when CSI is reported
	asn1cCallocOne(out->format1->choice.setup->simultaneousHARQ_ACK_CSI, ASN_RRC_PUCCH_FormatConfig__simultaneousHARQ_ACK_CSI_true);

	// SR resources
	out->schedulingRequestResourceToAddModList = CALLOC(1, sizeof(*out->schedulingRequestResourceToAddModList));
	out->schedulingRequestResourceToAddModList->list = CALLOC(1, sizeof(*out->schedulingRequestResourceToAddModList));
	asn1cSequenceAdd(out->schedulingRequestResourceToAddModList->list, struct ASN_RRC_SchedulingRequestResourceConfig, sr_res1);
	sr_res1->schedulingRequestResourceId = 1;
	sr_res1->schedulingRequestID = 0;
	sr_res1->periodicityAndOffset = CALLOC(1, sizeof(*sr_res1->periodicityAndOffset));
	sr_res1->periodicityAndOffset->present = ASN_RRC_SchedulingRequestResourceConfig__periodicityAndOffset_PR_sl40;
	sr_res1->periodicityAndOffset->choice.sl40 = 8;
	asn1cCallocOne(sr_res1->resource, 2);

	// >>> dl-DataToUl-Ack
	// TS38.213, 9.1.2.1 - "If a UE is provided dl-DataToUL-ACK, the UE does not expect to be indicated by DCI format 1_0
	// a slot timing value for transmission of HARQ-ACK information that does not belong to the intersection of the set
	// of slot timing values {1, 2, 3, 4, 5, 6, 7, 8} and the set of slot timing values provided by dl-DataToUL-ACK for
	// the active DL BWP of a corresponding serving cell.
	// Inactive for format1_0."
	// Note2: Only k1 >= 4 supported.
	// DL data
	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
		out->dl_DataToUL_ACK = CALLOC(1, sizeof(*out->dl_DataToUL_ACK));
		asn1cSequenceAdd(out->dl_DataToUL_ACK->list, long, ul_ack0);
		*ul_ack0 = 4;
	} else {
		out->dl_DataToUL_ACK = CALLOC(1, sizeof(*out->dl_DataToUL_ACK));
		asn1cSequenceAdd(out->dl_DataToUL_ACK->list, long, ul_ack0);
		*ul_ack0 = 6;
		asn1cSequenceAdd(out->dl_DataToUL_ACK->list, long, ul_ack1);
		*ul_ack1 = 5;
		asn1cSequenceAdd(out->dl_DataToUL_ACK->list, long, ul_ack2);
		*ul_ack2 = 4;
		asn1cSequenceAdd(out->dl_DataToUL_ACK->list, long, ul_ack3);
		*ul_ack3 = 4;
		asn1cSequenceAdd(out->dl_DataToUL_ACK->list, long, ul_ack4);
		*ul_ack4 = 4;
		asn1cSequenceAdd(out->dl_DataToUL_ACK->list, long, ul_ack5);
		*ul_ack5 = 4;
	}
}

void fill_pusch_cfg_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_PUSCH_Config_t *out)
{
	out->dmrs_UplinkForPUSCH_MappingTypeA = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_DMRS_UplinkConfig));
	out->dmrs_UplinkForPUSCH_MappingTypeA->present = ASN_RRC_SetupRelease_DMRS_UplinkConfig_PR_setup;
	out->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_DMRS_UplinkConfig));
	asn1cCallocOne(out->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition, ASN_RRC_DMRS_UplinkConfig__dmrs_AdditionalPosition_pos1);
		
	// PUSH power control skipped
	out->resourceAllocation = ASN_RRC_PUSCH_Config__resourceAllocation_resourceAllocationType1;

	//UCI
	out->uci_OnPUSCH = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_UCI_OnPUSCH));
	out->uci_OnPUSCH->present = ASN_RRC_SetupRelease_UCI_OnPUSCH_PR_setup;
	out->uci_OnPUSCH->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_UCI_OnPUSCH));
	out->uci_OnPUSCH->choice.setup->betaOffsets = CALLOC(1, sizeof(*out->uci_OnPUSCH->choice.setup->betaOffsets));
	out->uci_OnPUSCH->choice.setup->betaOffsets->present = ASN_RRC_UCI_OnPUSCH__betaOffsets_PR_semiStatic;

	asn1cCalloc(out->uci_OnPUSCH->choice.setup->betaOffsets->choice.semiStatic, beta_offset_semi_static);
	asn1cCallocOne(beta_offset_semi_static->betaOffsetACK_Index1, 9);
	asn1cCallocOne(beta_offset_semi_static->betaOffsetACK_Index2, 9);
	asn1cCallocOne(beta_offset_semi_static->betaOffsetACK_Index3, 9);
	asn1cCallocOne(beta_offset_semi_static->betaOffsetCSI_Part1_Index1, 6);
	asn1cCallocOne(beta_offset_semi_static->betaOffsetCSI_Part1_Index2, 6);
	asn1cCallocOne(beta_offset_semi_static->betaOffsetCSI_Part2_Index1, 6);
	asn1cCallocOne(beta_offset_semi_static->betaOffsetCSI_Part2_Index2, 6);

	out->uci_OnPUSCH->choice.setup->scaling = ASN_RRC_UCI_OnPUSCH__scaling_f1;
}

/// Fill InitUlBwp with gNB config
void fill_init_ul_bwp_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BWP_UplinkDedicated *out)
{
	if (cfg->is_standalone) {
		out->pucch_Config = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PUCCH_Config));
		out->pucch_Config->present = ASN_RRC_SetupRelease_PUCCH_Config_PR_setup;
		out->pucch_Config->choice.setup = CALLOC(1, sizeof(*out->pucch_Config->choice.setup));
		fill_pucch_cfg_from_enb_cfg(cfg, cell_cfg, out->pucch_Config->choice.setup);

		out->pusch_Config = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PUSCH_Config));
		out->pusch_Config->present = ASN_RRC_SetupRelease_PUSCH_Config_PR_setup;
		out->pusch_Config->choice.setup = CALLOC(1, sizeof(*out->pusch_Config->choice.setup));
		fill_pusch_cfg_from_enb_cfg(cfg, cell_cfg, out->pusch_Config->choice.setup);
	}
}

/// Fill InitUlBwp with gNB config
void fill_ul_cfg_from_enb_cfg(rrc_nr_cfg_t *cfg, rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_UplinkConfig_t *out)
{
  out->initialUplinkBWP = CALLOC(1, sizeof(struct ASN_RRC_BWP_UplinkDedicated));
  fill_init_ul_bwp_from_enb_cfg(cfg, cell_cfg, out->initialUplinkBWP);
}

/// Fill ServingCellConfig with gNB config
int fill_serv_cell_from_enb_cfg(rrc_nr_cfg_t *cfg, uint32_t cc, ASN_RRC_ServingCellConfig_t *serv_cell)
{
	rrc_cell_cfg_nr_t *cell_cfg = oset_list2_find(cfg->cell_list, cc)->data;

	serv_cell->csi_MeasConfig = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_CSI_MeasConfig));
	serv_cell->csi_MeasConfig->present = ASN_RRC_SetupRelease_CSI_MeasConfig_PR_setup;
	serv_cell->csi_MeasConfig->choice->setup = CALLOC(1, sizeof(struct ASN_RRC_CSI_MeasConfig));
	HANDLE_ERROR(fill_csi_meas_from_enb_cfg(cfg, cell_cfg, serv_cell->csi_MeasConfig->choice->setup));


	serv_cell->initialDownlinkBWP = CALLOC(1, sizeof(struct ASN_RRC_BWP_DownlinkDedicated));
	fill_init_dl_bwp_from_enb_cfg(cfg, cell_cfg, serv_cell->initialDownlinkBWP);

	if (cell_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_FDD) {
		asn1cCallocOne(serv_cell->firstActiveDownlinkBWP_Id, 0);
	} else {
		asn1cCallocOne(serv_cell->firstActiveDownlinkBWP_Id, 1);
	}

	serv_cell->uplinkConfig = CALLOC(1, sizeof(struct ASN_RRC_UplinkConfig));
	fill_ul_cfg_from_enb_cfg(cfg, cell_cfg, serv_cell->uplinkConfig);
	// TODO: remaining fields
	return OSET_OK;
}


/// Fill spCellConfig with gNB config
int fill_sp_cell_cfg_from_enb_cfg(rrc_nr_cfg_t *cfg, uint32_t cc, ASN_RRC_SpCellConfig_t *sp_cell)
{
	if (!cfg->is_standalone) {
		//sp_cell->reconfigurationWithSync = CALLOC(1, sizeof(struct ASN_RRC_ReconfigurationWithSync));
		//HANDLE_ERROR(fill_recfg_with_sync_from_enb_cfg(cfg, cc, sp_cell->reconfigurationWithSync));
	}

	sp_cell->spCellConfigDedicated = CALLOC(1, sizeof(struct ASN_RRC_ServingCellConfig));
	HANDLE_ERROR(fill_serv_cell_from_enb_cfg(cfg, cc, sp_cell->spCellConfigDedicated));

	return OSET_OK;
}

/// Fill MasterCellConfig with gNB config
int fill_master_cell_cfg_from_enb_cfg(rrc_nr_cfg_t *cfg, uint32_t cc, ASN_RRC_CellGroupConfig_t *out)
{
	out =  CALLOC(1,sizeof(ASN_RRC_CellGroupConfig_t));
	out->cellGroupId = 0;

	out->rlc_BearerToReleaseList = NULL;
	out->rlc_BearerToAddModList = CALLOC(1, sizeof(*out->rlc_BearerToAddModList));
	for(int i = 0; i < 1; i++){
		asn1cSequenceAdd(out->rlc_BearerToAddModList.list, struct ASN_RRC_RLC_BearerConfig, rlc_BearerConfig);
		fill_srb(cfg, srb1, rlc_BearerConfig);
	}

	// mac-CellGroupConfig -- Need M
    out->mac_CellGroupConfig = CALLOC(1, sizeof(struct ASN_RRC_MAC_CellGroupConfig));
	out->mac_CellGroupConfig->schedulingRequestConfig = CALLOC(1, sizeof(struct ASN_RRC_SchedulingRequestConfig));
	out->mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList = CALLOC(1, sizeof(*out->mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList));
	for(int i = 0; i < 1; i++){
		asn1cSequenceAdd(out->mac_CellGroupConfig->schedulingRequestConfig->schedulingRequestToAddModList->list,
						struct ASN_RRC_SchedulingRequestToAddMod, sr_add_mod);
		sr_add_mod->schedulingRequestId = 0;
		sr_add_mod->sr_TransMax = ASN_RRC_SchedulingRequestToAddMod__sr_TransMax_n64;
	}

	out->mac_CellGroupConfig->bsr_Config = CALLOC(1, sizeof(struct ASN_RRC_BSR_Config));
	out->mac_CellGroupConfig->bsr_Config->periodicBSR_Timer = ASN_RRC_BSR_Config__periodicBSR_Timer_sf32;
	out->mac_CellGroupConfig->bsr_Config->retxBSR_Timer = ASN_RRC_BSR_Config__retxBSR_Timer_sf320;

    out->mac_CellGroupConfig->tag_Config = CALLOC(1, sizeof(struct ASN_RRC_TAG_Config));
    out->mac_CellGroupConfig->tag_Config->tag_ToAddModList = CALLOC(1, sizeof(*out->mac_CellGroupConfig->tag_Config->tag_ToAddModList));
	for(int i = 0; i < 1; i++){
		asn1cSequenceAdd(out->mac_CellGroupConfig->tag_Config->tag_ToAddModList->list,
						struct ASN_RRC_TAG, rrc_tag);
		rrc_tag->tag_Id = 0;
		rrc_tag->timeAlignmentTimer = ASN_RRC_TimeAlignmentTimer_infinity;
	}

	out->mac_CellGroupConfig->phr_Config = CALLOC(1, sizeof(struct ASN_RRC_SetupRelease_PHR_Config));
	out->mac_CellGroupConfig->phr_Config->present = ASN_RRC_SetupRelease_PHR_Config_PR_setup;
	out->mac_CellGroupConfig->phr_Config->choice.setup = CALLOC(1, sizeof(struct ASN_RRC_PHR_Config));
	out->mac_CellGroupConfig->phr_Config->choice.setup->phr_PeriodicTimer = ASN_RRC_PHR_Config__phr_PeriodicTimer_sf500;
	out->mac_CellGroupConfig->phr_Config->choice.setup->phr_ProhibitTimer = ASN_RRC_PHR_Config__phr_ProhibitTimer_sf200;
	out->mac_CellGroupConfig->phr_Config->choice.setup->phr_Tx_PowerFactorChange = ASN_RRC_PHR_Config__phr_Tx_PowerFactorChange_dB3;
	out->mac_CellGroupConfig->phr_Config->choice.setup->multiplePHR = 0;
	out->mac_CellGroupConfig->phr_Config->choice.setup->dummy = 0;
	out->mac_CellGroupConfig->phr_Config->choice.setup->phr_Type2OtherCell = 0;
	out->mac_CellGroupConfig->phr_Config->choice.setup->phr_ModeOtherCG = ASN_RRC_PHR_Config__phr_ModeOtherCG_real;
    out->mac_CellGroupConfig->skipUplinkTxDynamic = 0;


	// physicalCellGroupConfig -- Need M
	out->physicalCellGroupConfig = CALLOC(1, sizeof(struct ASN_RRC_PhysicalCellGroupConfig));
	asn1cCallocOne(out->physicalCellGroupConfig->p_NR_FR1, 10);
	out->physicalCellGroupConfig->pdsch_HARQ_ACK_Codebook = ASN_RRC_PhysicalCellGroupConfig__pdsch_HARQ_ACK_Codebook_dynamic;

	// spCellConfig -- Need M
	out->spCellConfig  = CALLOC(1, sizeof(struct ASN_RRC_SpCellConfig));
	fill_sp_cell_cfg_from_enb_cfg(cfg, cc, out->spCellConfig);

	return OSET_OK;
}




