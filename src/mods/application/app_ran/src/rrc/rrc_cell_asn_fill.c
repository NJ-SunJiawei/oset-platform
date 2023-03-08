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

bool fill_ssb_pattern_scs(srsran_carrier_nr_t *carrier,
                                  srsran_ssb_pattern_t *pattern,
                                  srsran_subcarrier_spacing_t *ssb_scs)
{
	band_helper_t *band_helper = gnb_manager_self()->band_helper;
	uint16_t band = get_band_from_dl_freq_Hz_2c(band_helper, carrier->ssb_center_freq_hz);
	if (band == UINT16_MAX) {
		oset_error("Invalid band for SSB frequency %.3f MHz", carrier->ssb_center_freq_hz);
		return false;
	}

	// TODO: Generalize conversion for other SCS
	*pattern = get_ssb_pattern_2c(band_helper, band, srsran_subcarrier_spacing_15kHz);
	if (*pattern == SRSRAN_SSB_PATTERN_A) {
		*ssb_scs = carrier->scs;
	} else {
		// try to optain SSB pattern for same band with 30kHz SCS
		*pattern = get_ssb_pattern_2c(band_helper, band, srsran_subcarrier_spacing_30kHz);
		if (*pattern == SRSRAN_SSB_PATTERN_B || *pattern == SRSRAN_SSB_PATTERN_C) {
		  // SSB SCS is 30 kHz
		  *ssb_scs = srsran_subcarrier_spacing_30kHz;
		} else {
		  oset_error("Can't derive SSB pattern from band %d", band);
		  return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
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

	out->commonSearchSpaceList = CALLOC(1,sizeof(*out->commonSearchSpaceList));
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
	rach->prach_RootSequenceIndex.choice.l839 = cell_cfg.prach_root_seq_idx;
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

	//si-SchedulingInfo Todo
	/*sib1->si_SchedulingInfo = CALLOC(1, sizeof(struct ASN_RRC_SI_SchedulingInfo));
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
	}*/

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



	// mac-CellGroupConfig -- Need M
	out.mac_cell_group_cfg.tag_cfg_present                  = true;
	out.mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list.resize(1);
	out.mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list[0].tag_id                 = 0;
	out.mac_cell_group_cfg.tag_cfg.tag_to_add_mod_list[0].time_align_timer.value = time_align_timer_opts::infinity;
	out.mac_cell_group_cfg.phr_cfg_present                                       = true;
	phr_cfg_s& phr                            = out.mac_cell_group_cfg.phr_cfg.set_setup();
	phr.phr_periodic_timer.value              = asn1::rrc_nr::phr_cfg_s::phr_periodic_timer_opts::sf500;
	phr.phr_prohibit_timer.value              = asn1::rrc_nr::phr_cfg_s::phr_prohibit_timer_opts::sf200;
	phr.phr_tx_pwr_factor_change.value        = asn1::rrc_nr::phr_cfg_s::phr_tx_pwr_factor_change_opts::db3;
	phr.multiple_phr                          = false;
	phr.dummy                                 = false;
	phr.phr_type2_other_cell                  = false;
	phr.phr_mode_other_cg.value               = asn1::rrc_nr::phr_cfg_s::phr_mode_other_cg_opts::real;
	out.mac_cell_group_cfg.skip_ul_tx_dynamic = false;
	out.mac_cell_group_cfg.phr_cfg_present    = false; // Note: not supported

	// physicalCellGroupConfig -- Need M
	out.phys_cell_group_cfg_present          = true;
	out.phys_cell_group_cfg.p_nr_fr1_present = true;
	out.phys_cell_group_cfg.p_nr_fr1         = 10;
	out.phys_cell_group_cfg.pdsch_harq_ack_codebook.value =
	  phys_cell_group_cfg_s::pdsch_harq_ack_codebook_opts::dynamic_value;

	// spCellConfig -- Need M
	out.sp_cell_cfg_present = true;
	fill_sp_cell_cfg_from_enb_cfg(cfg, cc, out.sp_cell_cfg);

	return SRSRAN_SUCCESS;
}




