/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef NGAP_MESSAGE_LIST_H
#define NGAP_MESSAGE_LIST_H

#include "oset-core.h"

#include "ASN_NGAP_AdditionalDLUPTNLInformationForHOItem.h"
#include "ASN_NGAP_AdditionalDLUPTNLInformationForHOList.h"
#include "ASN_NGAP_AdditionalQosFlowInformation.h"
#include "ASN_NGAP_AllocationAndRetentionPriority.h"
#include "ASN_NGAP_AllowedNSSAI.h"
#include "ASN_NGAP_AllowedNSSAI-Item.h"
#include "ASN_NGAP_AllowedTACs.h"
#include "ASN_NGAP_AMFConfigurationUpdateAcknowledge.h"
#include "ASN_NGAP_AMFConfigurationUpdateFailure.h"
#include "ASN_NGAP_AMFConfigurationUpdate.h"
#include "ASN_NGAP_AMFName.h"
#include "ASN_NGAP_AMFPagingTarget.h"
#include "ASN_NGAP_AMFPointer.h"
#include "ASN_NGAP_AMFRegionID.h"
#include "ASN_NGAP_AMFSetID.h"
#include "ASN_NGAP_AMFStatusIndication.h"
#include "ASN_NGAP_AMF-TNLAssociationSetupItem.h"
#include "ASN_NGAP_AMF-TNLAssociationSetupList.h"
#include "ASN_NGAP_AMF-TNLAssociationToAddItem.h"
#include "ASN_NGAP_AMF-TNLAssociationToAddList.h"
#include "ASN_NGAP_AMF-TNLAssociationToRemoveItem.h"
#include "ASN_NGAP_AMF-TNLAssociationToRemoveList.h"
#include "ASN_NGAP_AMF-TNLAssociationToUpdateItem.h"
#include "ASN_NGAP_AMF-TNLAssociationToUpdateList.h"
#include "ASN_NGAP_AMF-UE-NGAP-ID.h"
#include "ASN_NGAP_AreaOfInterestCellItem.h"
#include "ASN_NGAP_AreaOfInterestCellList.h"
#include "ASN_NGAP_AreaOfInterest.h"
#include "ASN_NGAP_AreaOfInterestItem.h"
#include "ASN_NGAP_AreaOfInterestList.h"
#include "ASN_NGAP_AreaOfInterestRANNodeItem.h"
#include "ASN_NGAP_AreaOfInterestRANNodeList.h"
#include "ASN_NGAP_AreaOfInterestTAIItem.h"
#include "ASN_NGAP_AreaOfInterestTAIList.h"
#include "ASN_NGAP_asn_constant.h"
#include "ASN_NGAP_AssistanceDataForPaging.h"
#include "ASN_NGAP_AssistanceDataForRecommendedCells.h"
#include "ASN_NGAP_AssociatedQosFlowItem.h"
#include "ASN_NGAP_AssociatedQosFlowList.h"
#include "ASN_NGAP_AveragingWindow.h"
#include "ASN_NGAP_BitRate.h"
#include "ASN_NGAP_BroadcastCancelledAreaList.h"
#include "ASN_NGAP_BroadcastCompletedAreaList.h"
#include "ASN_NGAP_BroadcastPLMNItem.h"
#include "ASN_NGAP_BroadcastPLMNList.h"
#include "ASN_NGAP_CancelAllWarningMessages.h"
#include "ASN_NGAP_CancelledCellsInEAI-EUTRA.h"
#include "ASN_NGAP_CancelledCellsInEAI-EUTRA-Item.h"
#include "ASN_NGAP_CancelledCellsInEAI-NR.h"
#include "ASN_NGAP_CancelledCellsInEAI-NR-Item.h"
#include "ASN_NGAP_CancelledCellsInTAI-EUTRA.h"
#include "ASN_NGAP_CancelledCellsInTAI-EUTRA-Item.h"
#include "ASN_NGAP_CancelledCellsInTAI-NR.h"
#include "ASN_NGAP_CancelledCellsInTAI-NR-Item.h"
#include "ASN_NGAP_Cause.h"
#include "ASN_NGAP_CauseMisc.h"
#include "ASN_NGAP_CauseNas.h"
#include "ASN_NGAP_CauseProtocol.h"
#include "ASN_NGAP_CauseRadioNetwork.h"
#include "ASN_NGAP_CauseTransport.h"
#include "ASN_NGAP_CellIDBroadcastEUTRA.h"
#include "ASN_NGAP_CellIDBroadcastEUTRA-Item.h"
#include "ASN_NGAP_CellIDBroadcastNR.h"
#include "ASN_NGAP_CellIDBroadcastNR-Item.h"
#include "ASN_NGAP_CellIDCancelledEUTRA.h"
#include "ASN_NGAP_CellIDCancelledEUTRA-Item.h"
#include "ASN_NGAP_CellIDCancelledNR.h"
#include "ASN_NGAP_CellIDCancelledNR-Item.h"
#include "ASN_NGAP_CellIDListForRestart.h"
#include "ASN_NGAP_CellSize.h"
#include "ASN_NGAP_CellTrafficTrace.h"
#include "ASN_NGAP_CellType.h"
#include "ASN_NGAP_CNAssistedRANTuning.h"
#include "ASN_NGAP_CNTypeRestrictionsForEquivalent.h"
#include "ASN_NGAP_CNTypeRestrictionsForEquivalentItem.h"
#include "ASN_NGAP_CNTypeRestrictionsForServing.h"
#include "ASN_NGAP_CommonNetworkInstance.h"
#include "ASN_NGAP_CompletedCellsInEAI-EUTRA.h"
#include "ASN_NGAP_CompletedCellsInEAI-EUTRA-Item.h"
#include "ASN_NGAP_CompletedCellsInEAI-NR.h"
#include "ASN_NGAP_CompletedCellsInEAI-NR-Item.h"
#include "ASN_NGAP_CompletedCellsInTAI-EUTRA.h"
#include "ASN_NGAP_CompletedCellsInTAI-EUTRA-Item.h"
#include "ASN_NGAP_CompletedCellsInTAI-NR.h"
#include "ASN_NGAP_CompletedCellsInTAI-NR-Item.h"
#include "ASN_NGAP_ConcurrentWarningMessageInd.h"
#include "ASN_NGAP_ConfidentialityProtectionIndication.h"
#include "ASN_NGAP_ConfidentialityProtectionResult.h"
#include "ASN_NGAP_ConfiguredNSSAI.h"
#include "ASN_NGAP_CoreNetworkAssistanceInformationForInactive.h"
#include "ASN_NGAP_COUNTValueForPDCP-SN12.h"
#include "ASN_NGAP_COUNTValueForPDCP-SN18.h"
#include "ASN_NGAP_CPTransportLayerInformation.h"
#include "ASN_NGAP_CriticalityDiagnostics.h"
#include "ASN_NGAP_CriticalityDiagnostics-IE-Item.h"
#include "ASN_NGAP_CriticalityDiagnostics-IE-List.h"
#include "ASN_NGAP_Criticality.h"
#include "ASN_NGAP_DataCodingScheme.h"
#include "ASN_NGAP_DataForwardingAccepted.h"
#include "ASN_NGAP_DataForwardingNotPossible.h"
#include "ASN_NGAP_DataForwardingResponseDRBItem.h"
#include "ASN_NGAP_DataForwardingResponseDRBList.h"
#include "ASN_NGAP_DataForwardingResponseERABList.h"
#include "ASN_NGAP_DataForwardingResponseERABListItem.h"
#include "ASN_NGAP_DeactivateTrace.h"
#include "ASN_NGAP_DelayCritical.h"
#include "ASN_NGAP_DirectForwardingPathAvailability.h"
#include "ASN_NGAP_DLForwarding.h"
#include "ASN_NGAP_DL-NGU-TNLInformationReused.h"
#include "ASN_NGAP_DownlinkNASTransport.h"
#include "ASN_NGAP_DownlinkNonUEAssociatedNRPPaTransport.h"
#include "ASN_NGAP_DownlinkRANConfigurationTransfer.h"
#include "ASN_NGAP_DownlinkRANStatusTransfer.h"
#include "ASN_NGAP_DownlinkRIMInformationTransfer.h"
#include "ASN_NGAP_DownlinkUEAssociatedNRPPaTransport.h"
#include "ASN_NGAP_DRB-ID.h"
#include "ASN_NGAP_DRBsSubjectToStatusTransferItem.h"
#include "ASN_NGAP_DRBsSubjectToStatusTransferList.h"
#include "ASN_NGAP_DRBStatusDL12.h"
#include "ASN_NGAP_DRBStatusDL18.h"
#include "ASN_NGAP_DRBStatusDL.h"
#include "ASN_NGAP_DRBStatusUL12.h"
#include "ASN_NGAP_DRBStatusUL18.h"
#include "ASN_NGAP_DRBStatusUL.h"
#include "ASN_NGAP_DRBsToQosFlowsMappingItem.h"
#include "ASN_NGAP_DRBsToQosFlowsMappingList.h"
#include "ASN_NGAP_Dynamic5QIDescriptor.h"
#include "ASN_NGAP_EmergencyAreaIDBroadcastEUTRA.h"
#include "ASN_NGAP_EmergencyAreaIDBroadcastEUTRA-Item.h"
#include "ASN_NGAP_EmergencyAreaIDBroadcastNR.h"
#include "ASN_NGAP_EmergencyAreaIDBroadcastNR-Item.h"
#include "ASN_NGAP_EmergencyAreaIDCancelledEUTRA.h"
#include "ASN_NGAP_EmergencyAreaIDCancelledEUTRA-Item.h"
#include "ASN_NGAP_EmergencyAreaIDCancelledNR.h"
#include "ASN_NGAP_EmergencyAreaIDCancelledNR-Item.h"
#include "ASN_NGAP_EmergencyAreaID.h"
#include "ASN_NGAP_EmergencyAreaIDListForRestart.h"
#include "ASN_NGAP_EmergencyAreaIDList.h"
#include "ASN_NGAP_EmergencyFallbackIndicator.h"
#include "ASN_NGAP_EmergencyFallbackRequestIndicator.h"
#include "ASN_NGAP_EmergencyServiceTargetCN.h"
#include "ASN_NGAP_EN-DCSONConfigurationTransfer.h"
#include "ASN_NGAP_EndpointIPAddressAndPort.h"
#include "ASN_NGAP_EPS-TAC.h"
#include "ASN_NGAP_EPS-TAI.h"
#include "ASN_NGAP_EquivalentPLMNs.h"
#include "ASN_NGAP_E-RAB-ID.h"
#include "ASN_NGAP_E-RABInformationItem.h"
#include "ASN_NGAP_E-RABInformationList.h"
#include "ASN_NGAP_ErrorIndication.h"
#include "ASN_NGAP_EUTRACellIdentity.h"
#include "ASN_NGAP_EUTRA-CGI.h"
#include "ASN_NGAP_EUTRA-CGIListForWarning.h"
#include "ASN_NGAP_EUTRA-CGIList.h"
#include "ASN_NGAP_EUTRAencryptionAlgorithms.h"
#include "ASN_NGAP_EUTRAintegrityProtectionAlgorithms.h"
#include "ASN_NGAP_EventType.h"
#include "ASN_NGAP_ExpectedActivityPeriod.h"
#include "ASN_NGAP_ExpectedHOInterval.h"
#include "ASN_NGAP_ExpectedIdlePeriod.h"
#include "ASN_NGAP_ExpectedUEActivityBehaviour.h"
#include "ASN_NGAP_ExpectedUEBehaviour.h"
#include "ASN_NGAP_ExpectedUEMobility.h"
#include "ASN_NGAP_ExpectedUEMovingTrajectory.h"
#include "ASN_NGAP_ExpectedUEMovingTrajectoryItem.h"
#include "ASN_NGAP_ExtendedRATRestrictionInformation.h"
#include "ASN_NGAP_ExtendedRNC-ID.h"
#include "ASN_NGAP_FiveG-S-TMSI.h"
#include "ASN_NGAP_FiveG-TMSI.h"
#include "ASN_NGAP_FiveQI.h"
#include "ASN_NGAP_ForbiddenAreaInformation.h"
#include "ASN_NGAP_ForbiddenAreaInformation-Item.h"
#include "ASN_NGAP_ForbiddenTACs.h"
#include "ASN_NGAP_GBR-QosInformation.h"
#include "ASN_NGAP_GlobalGNB-ID.h"
#include "ASN_NGAP_GlobalN3IWF-ID.h"
#include "ASN_NGAP_GlobalNgENB-ID.h"
#include "ASN_NGAP_GlobalRANNodeID.h"
#include "ASN_NGAP_GNB-ID.h"
#include "ASN_NGAP_GNBSetID.h"
#include "ASN_NGAP_GTP-TEID.h"
#include "ASN_NGAP_GTPTunnel.h"
#include "ASN_NGAP_GUAMI.h"
#include "ASN_NGAP_GUAMIType.h"
#include "ASN_NGAP_HandoverCancelAcknowledge.h"
#include "ASN_NGAP_HandoverCancel.h"
#include "ASN_NGAP_HandoverCommand.h"
#include "ASN_NGAP_HandoverCommandTransfer.h"
#include "ASN_NGAP_HandoverFailure.h"
#include "ASN_NGAP_HandoverFlag.h"
#include "ASN_NGAP_HandoverNotify.h"
#include "ASN_NGAP_HandoverPreparationFailure.h"
#include "ASN_NGAP_HandoverPreparationUnsuccessfulTransfer.h"
#include "ASN_NGAP_HandoverRequestAcknowledge.h"
#include "ASN_NGAP_HandoverRequestAcknowledgeTransfer.h"
#include "ASN_NGAP_HandoverRequest.h"
#include "ASN_NGAP_HandoverRequired.h"
#include "ASN_NGAP_HandoverRequiredTransfer.h"
#include "ASN_NGAP_HandoverResourceAllocationUnsuccessfulTransfer.h"
#include "ASN_NGAP_HandoverType.h"
#include "ASN_NGAP_IMSVoiceSupportIndicator.h"
#include "ASN_NGAP_IndexToRFSP.h"
#include "ASN_NGAP_InfoOnRecommendedCellsAndRANNodesForPaging.h"
#include "ASN_NGAP_InitialContextSetupFailure.h"
#include "ASN_NGAP_InitialContextSetupRequest.h"
#include "ASN_NGAP_InitialContextSetupResponse.h"
#include "ASN_NGAP_InitialUEMessage.h"
#include "ASN_NGAP_InitiatingMessage.h"
#include "ASN_NGAP_IntegrityProtectionIndication.h"
#include "ASN_NGAP_IntegrityProtectionResult.h"
#include "ASN_NGAP_IntendedNumberOfPagingAttempts.h"
#include "ASN_NGAP_InterfacesToTrace.h"
#include "ASN_NGAP_LAC.h"
#include "ASN_NGAP_LAI.h"
#include "ASN_NGAP_LastVisitedCellInformation.h"
#include "ASN_NGAP_LastVisitedCellItem.h"
#include "ASN_NGAP_LastVisitedEUTRANCellInformation.h"
#include "ASN_NGAP_LastVisitedGERANCellInformation.h"
#include "ASN_NGAP_LastVisitedNGRANCellInformation.h"
#include "ASN_NGAP_LastVisitedUTRANCellInformation.h"
#include "ASN_NGAP_LocationReport.h"
#include "ASN_NGAP_LocationReportingAdditionalInfo.h"
#include "ASN_NGAP_LocationReportingControl.h"
#include "ASN_NGAP_LocationReportingFailureIndication.h"
#include "ASN_NGAP_LocationReportingReferenceID.h"
#include "ASN_NGAP_LocationReportingRequestType.h"
#include "ASN_NGAP_MaskedIMEISV.h"
#include "ASN_NGAP_MaximumDataBurstVolume.h"
#include "ASN_NGAP_MaximumIntegrityProtectedDataRate.h"
#include "ASN_NGAP_MessageIdentifier.h"
#include "ASN_NGAP_MICOModeIndication.h"
#include "ASN_NGAP_MobilityRestrictionList.h"
#include "ASN_NGAP_N3IWF-ID.h"
#include "ASN_NGAP_NASNonDeliveryIndication.h"
#include "ASN_NGAP_NAS-PDU.h"
#include "ASN_NGAP_NASSecurityParametersFromNGRAN.h"
#include "ASN_NGAP_NetworkInstance.h"
#include "ASN_NGAP_NewSecurityContextInd.h"
#include "ASN_NGAP_NextHopChainingCount.h"
#include "ASN_NGAP_NextPagingAreaScope.h"
#include "ASN_NGAP_NGAP-PDU.h"
#include "ASN_NGAP_NgENB-ID.h"
#include "ASN_NGAP_NGRAN-CGI.h"
#include "ASN_NGAP_NGRAN-TNLAssociationToRemoveItem.h"
#include "ASN_NGAP_NGRAN-TNLAssociationToRemoveList.h"
#include "ASN_NGAP_NGRANTraceID.h"
#include "ASN_NGAP_NGResetAcknowledge.h"
#include "ASN_NGAP_NGReset.h"
#include "ASN_NGAP_NGSetupFailure.h"
#include "ASN_NGAP_NGSetupRequest.h"
#include "ASN_NGAP_NGSetupResponse.h"
#include "ASN_NGAP_NonDynamic5QIDescriptor.h"
#include "ASN_NGAP_NotAllowedTACs.h"
#include "ASN_NGAP_NotificationCause.h"
#include "ASN_NGAP_NotificationControl.h"
#include "ASN_NGAP_NRCellIdentity.h"
#include "ASN_NGAP_NR-CGI.h"
#include "ASN_NGAP_NR-CGIListForWarning.h"
#include "ASN_NGAP_NR-CGIList.h"
#include "ASN_NGAP_NRencryptionAlgorithms.h"
#include "ASN_NGAP_NRintegrityProtectionAlgorithms.h"
#include "ASN_NGAP_NRPPa-PDU.h"
#include "ASN_NGAP_NumberOfBroadcasts.h"
#include "ASN_NGAP_NumberOfBroadcastsRequested.h"
#include "ASN_NGAP_OverloadAction.h"
#include "ASN_NGAP_OverloadResponse.h"
#include "ASN_NGAP_OverloadStart.h"
#include "ASN_NGAP_OverloadStartNSSAIItem.h"
#include "ASN_NGAP_OverloadStartNSSAIList.h"
#include "ASN_NGAP_OverloadStop.h"
#include "ASN_NGAP_PacketDelayBudget.h"
#include "ASN_NGAP_PacketErrorRate.h"
#include "ASN_NGAP_PacketLossRate.h"
#include "ASN_NGAP_PagingAttemptCount.h"
#include "ASN_NGAP_PagingAttemptInformation.h"
#include "ASN_NGAP_PagingDRX.h"
#include "ASN_NGAP_Paging.h"
#include "ASN_NGAP_PagingOrigin.h"
#include "ASN_NGAP_PagingPriority.h"
#include "ASN_NGAP_PathSwitchRequestAcknowledge.h"
#include "ASN_NGAP_PathSwitchRequestAcknowledgeTransfer.h"
#include "ASN_NGAP_PathSwitchRequestFailure.h"
#include "ASN_NGAP_PathSwitchRequest.h"
#include "ASN_NGAP_PathSwitchRequestSetupFailedTransfer.h"
#include "ASN_NGAP_PathSwitchRequestTransfer.h"
#include "ASN_NGAP_PathSwitchRequestUnsuccessfulTransfer.h"
#include "ASN_NGAP_PDUSessionAggregateMaximumBitRate.h"
#include "ASN_NGAP_PDUSessionID.h"
#include "ASN_NGAP_PDUSessionResourceAdmittedItem.h"
#include "ASN_NGAP_PDUSessionResourceAdmittedList.h"
#include "ASN_NGAP_PDUSessionResourceFailedToModifyItemModCfm.h"
#include "ASN_NGAP_PDUSessionResourceFailedToModifyItemModRes.h"
#include "ASN_NGAP_PDUSessionResourceFailedToModifyListModCfm.h"
#include "ASN_NGAP_PDUSessionResourceFailedToModifyListModRes.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupItemCxtFail.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupItemCxtRes.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupItemHOAck.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupItemPSReq.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupItemSURes.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupListCxtFail.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupListCxtRes.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupListHOAck.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupListPSReq.h"
#include "ASN_NGAP_PDUSessionResourceFailedToSetupListSURes.h"
#include "ASN_NGAP_PDUSessionResourceHandoverItem.h"
#include "ASN_NGAP_PDUSessionResourceHandoverList.h"
#include "ASN_NGAP_PDUSessionResourceInformationItem.h"
#include "ASN_NGAP_PDUSessionResourceInformationList.h"
#include "ASN_NGAP_PDUSessionResourceItemCxtRelCpl.h"
#include "ASN_NGAP_PDUSessionResourceItemCxtRelReq.h"
#include "ASN_NGAP_PDUSessionResourceItemHORqd.h"
#include "ASN_NGAP_PDUSessionResourceListCxtRelCpl.h"
#include "ASN_NGAP_PDUSessionResourceListCxtRelReq.h"
#include "ASN_NGAP_PDUSessionResourceListHORqd.h"
#include "ASN_NGAP_PDUSessionResourceModifyConfirm.h"
#include "ASN_NGAP_PDUSessionResourceModifyConfirmTransfer.h"
#include "ASN_NGAP_PDUSessionResourceModifyIndication.h"
#include "ASN_NGAP_PDUSessionResourceModifyIndicationTransfer.h"
#include "ASN_NGAP_PDUSessionResourceModifyIndicationUnsuccessfulTransfer.h"
#include "ASN_NGAP_PDUSessionResourceModifyItemModCfm.h"
#include "ASN_NGAP_PDUSessionResourceModifyItemModInd.h"
#include "ASN_NGAP_PDUSessionResourceModifyItemModReq.h"
#include "ASN_NGAP_PDUSessionResourceModifyItemModRes.h"
#include "ASN_NGAP_PDUSessionResourceModifyListModCfm.h"
#include "ASN_NGAP_PDUSessionResourceModifyListModInd.h"
#include "ASN_NGAP_PDUSessionResourceModifyListModReq.h"
#include "ASN_NGAP_PDUSessionResourceModifyListModRes.h"
#include "ASN_NGAP_PDUSessionResourceModifyRequest.h"
#include "ASN_NGAP_PDUSessionResourceModifyRequestTransfer.h"
#include "ASN_NGAP_PDUSessionResourceModifyResponse.h"
#include "ASN_NGAP_PDUSessionResourceModifyResponseTransfer.h"
#include "ASN_NGAP_PDUSessionResourceModifyUnsuccessfulTransfer.h"
#include "ASN_NGAP_PDUSessionResourceNotify.h"
#include "ASN_NGAP_PDUSessionResourceNotifyItem.h"
#include "ASN_NGAP_PDUSessionResourceNotifyList.h"
#include "ASN_NGAP_PDUSessionResourceNotifyReleasedTransfer.h"
#include "ASN_NGAP_PDUSessionResourceNotifyTransfer.h"
#include "ASN_NGAP_PDUSessionResourceReleaseCommand.h"
#include "ASN_NGAP_PDUSessionResourceReleaseCommandTransfer.h"
#include "ASN_NGAP_PDUSessionResourceReleasedItemNot.h"
#include "ASN_NGAP_PDUSessionResourceReleasedItemPSAck.h"
#include "ASN_NGAP_PDUSessionResourceReleasedItemPSFail.h"
#include "ASN_NGAP_PDUSessionResourceReleasedItemRelRes.h"
#include "ASN_NGAP_PDUSessionResourceReleasedListNot.h"
#include "ASN_NGAP_PDUSessionResourceReleasedListPSAck.h"
#include "ASN_NGAP_PDUSessionResourceReleasedListPSFail.h"
#include "ASN_NGAP_PDUSessionResourceReleasedListRelRes.h"
#include "ASN_NGAP_PDUSessionResourceReleaseResponse.h"
#include "ASN_NGAP_PDUSessionResourceReleaseResponseTransfer.h"
#include "ASN_NGAP_PDUSessionResourceSecondaryRATUsageItem.h"
#include "ASN_NGAP_PDUSessionResourceSecondaryRATUsageList.h"
#include "ASN_NGAP_PDUSessionResourceSetupItemCxtReq.h"
#include "ASN_NGAP_PDUSessionResourceSetupItemCxtRes.h"
#include "ASN_NGAP_PDUSessionResourceSetupItemHOReq.h"
#include "ASN_NGAP_PDUSessionResourceSetupItemSUReq.h"
#include "ASN_NGAP_PDUSessionResourceSetupItemSURes.h"
#include "ASN_NGAP_PDUSessionResourceSetupListCxtReq.h"
#include "ASN_NGAP_PDUSessionResourceSetupListCxtRes.h"
#include "ASN_NGAP_PDUSessionResourceSetupListHOReq.h"
#include "ASN_NGAP_PDUSessionResourceSetupListSUReq.h"
#include "ASN_NGAP_PDUSessionResourceSetupListSURes.h"
#include "ASN_NGAP_PDUSessionResourceSetupRequest.h"
#include "ASN_NGAP_PDUSessionResourceSetupRequestTransfer.h"
#include "ASN_NGAP_PDUSessionResourceSetupResponse.h"
#include "ASN_NGAP_PDUSessionResourceSetupResponseTransfer.h"
#include "ASN_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer.h"
#include "ASN_NGAP_PDUSessionResourceSwitchedItem.h"
#include "ASN_NGAP_PDUSessionResourceSwitchedList.h"
#include "ASN_NGAP_PDUSessionResourceToBeSwitchedDLItem.h"
#include "ASN_NGAP_PDUSessionResourceToBeSwitchedDLList.h"
#include "ASN_NGAP_PDUSessionResourceToReleaseItemHOCmd.h"
#include "ASN_NGAP_PDUSessionResourceToReleaseItemRelCmd.h"
#include "ASN_NGAP_PDUSessionResourceToReleaseListHOCmd.h"
#include "ASN_NGAP_PDUSessionResourceToReleaseListRelCmd.h"
#include "ASN_NGAP_PDUSessionType.h"
#include "ASN_NGAP_PDUSessionUsageReport.h"
#include "ASN_NGAP_PeriodicRegistrationUpdateTimer.h"
#include "ASN_NGAP_PLMNIdentity.h"
#include "ASN_NGAP_PLMNSupportItem.h"
#include "ASN_NGAP_PLMNSupportList.h"
#include "ASN_NGAP_PortNumber.h"
#include "ASN_NGAP_Pre-emptionCapability.h"
#include "ASN_NGAP_Pre-emptionVulnerability.h"
#include "ASN_NGAP_Presence.h"
#include "ASN_NGAP_PriorityLevelARP.h"
#include "ASN_NGAP_PriorityLevelQos.h"
#include "ASN_NGAP_PrivateIE-Container.h"
#include "ASN_NGAP_PrivateIE-Field.h"
#include "ASN_NGAP_PrivateIE-ID.h"
#include "ASN_NGAP_PrivateMessage.h"
#include "ASN_NGAP_ProcedureCode.h"
#include "ASN_NGAP_ProtocolExtensionContainer.h"
#include "ASN_NGAP_ProtocolExtensionField.h"
#include "ASN_NGAP_ProtocolExtensionID.h"
#include "ASN_NGAP_ProtocolIE-Container.h"
#include "ASN_NGAP_ProtocolIE-ContainerList.h"
#include "ASN_NGAP_ProtocolIE-Field.h"
#include "ASN_NGAP_ProtocolIE-ID.h"
#include "ASN_NGAP_ProtocolIE-SingleContainer.h"
#include "ASN_NGAP_PWSCancelRequest.h"
#include "ASN_NGAP_PWSCancelResponse.h"
#include "ASN_NGAP_PWSFailedCellIDList.h"
#include "ASN_NGAP_PWSFailureIndication.h"
#include "ASN_NGAP_PWSRestartIndication.h"
#include "ASN_NGAP_QosCharacteristics.h"
#include "ASN_NGAP_QosFlowAcceptedItem.h"
#include "ASN_NGAP_QosFlowAcceptedList.h"
#include "ASN_NGAP_QosFlowAddOrModifyRequestItem.h"
#include "ASN_NGAP_QosFlowAddOrModifyRequestList.h"
#include "ASN_NGAP_QosFlowAddOrModifyResponseItem.h"
#include "ASN_NGAP_QosFlowAddOrModifyResponseList.h"
#include "ASN_NGAP_QosFlowIdentifier.h"
#include "ASN_NGAP_QosFlowInformationItem.h"
#include "ASN_NGAP_QosFlowInformationList.h"
#include "ASN_NGAP_QosFlowItemWithDataForwarding.h"
#include "ASN_NGAP_QosFlowLevelQosParameters.h"
#include "ASN_NGAP_QosFlowListWithCause.h"
#include "ASN_NGAP_QosFlowListWithDataForwarding.h"
#include "ASN_NGAP_QosFlowModifyConfirmItem.h"
#include "ASN_NGAP_QosFlowModifyConfirmList.h"
#include "ASN_NGAP_QosFlowNotifyItem.h"
#include "ASN_NGAP_QosFlowNotifyList.h"
#include "ASN_NGAP_QosFlowPerTNLInformation.h"
#include "ASN_NGAP_QosFlowPerTNLInformationItem.h"
#include "ASN_NGAP_QosFlowPerTNLInformationList.h"
#include "ASN_NGAP_QosFlowSetupRequestItem.h"
#include "ASN_NGAP_QosFlowSetupRequestList.h"
#include "ASN_NGAP_QoSFlowsUsageReport-Item.h"
#include "ASN_NGAP_QoSFlowsUsageReportList.h"
#include "ASN_NGAP_QosFlowToBeForwardedItem.h"
#include "ASN_NGAP_QosFlowToBeForwardedList.h"
#include "ASN_NGAP_QosFlowWithCauseItem.h"
#include "ASN_NGAP_QosMonitoringRequest.h"
#include "ASN_NGAP_RANConfigurationUpdateAcknowledge.h"
#include "ASN_NGAP_RANConfigurationUpdateFailure.h"
#include "ASN_NGAP_RANConfigurationUpdate.h"
#include "ASN_NGAP_RANNodeName.h"
#include "ASN_NGAP_RANPagingPriority.h"
#include "ASN_NGAP_RANStatusTransfer-TransparentContainer.h"
#include "ASN_NGAP_RAN-UE-NGAP-ID.h"
#include "ASN_NGAP_RAT-Information.h"
#include "ASN_NGAP_RATRestrictionInformation.h"
#include "ASN_NGAP_RATRestrictions.h"
#include "ASN_NGAP_RATRestrictions-Item.h"
#include "ASN_NGAP_RecommendedCellItem.h"
#include "ASN_NGAP_RecommendedCellList.h"
#include "ASN_NGAP_RecommendedCellsForPaging.h"
#include "ASN_NGAP_RecommendedRANNodeItem.h"
#include "ASN_NGAP_RecommendedRANNodeList.h"
#include "ASN_NGAP_RecommendedRANNodesForPaging.h"
#include "ASN_NGAP_RedirectionVoiceFallback.h"
#include "ASN_NGAP_ReflectiveQosAttribute.h"
#include "ASN_NGAP_RejectedNSSAIinPLMN.h"
#include "ASN_NGAP_RejectedNSSAIinTA.h"
#include "ASN_NGAP_RelativeAMFCapacity.h"
#include "ASN_NGAP_RepetitionPeriod.h"
#include "ASN_NGAP_ReportArea.h"
#include "ASN_NGAP_RerouteNASRequest.h"
#include "ASN_NGAP_ResetAll.h"
#include "ASN_NGAP_ResetType.h"
#include "ASN_NGAP_RIMInformation.h"
#include "ASN_NGAP_RIMInformationTransfer.h"
#include "ASN_NGAP_RNC-ID.h"
#include "ASN_NGAP_RoutingID.h"
#include "ASN_NGAP_RRCContainer.h"
#include "ASN_NGAP_RRCEstablishmentCause.h"
#include "ASN_NGAP_RRCInactiveTransitionReport.h"
#include "ASN_NGAP_RRCInactiveTransitionReportRequest.h"
#include "ASN_NGAP_RRCState.h"
#include "ASN_NGAP_SCTP-TLAs.h"
#include "ASN_NGAP_SD.h"
#include "ASN_NGAP_SecondaryRATDataUsageReport.h"
#include "ASN_NGAP_SecondaryRATDataUsageReportTransfer.h"
#include "ASN_NGAP_SecondaryRATUsageInformation.h"
#include "ASN_NGAP_SecurityContext.h"
#include "ASN_NGAP_SecurityIndication.h"
#include "ASN_NGAP_SecurityKey.h"
#include "ASN_NGAP_SecurityResult.h"
#include "ASN_NGAP_SerialNumber.h"
#include "ASN_NGAP_ServedGUAMIItem.h"
#include "ASN_NGAP_ServedGUAMIList.h"
#include "ASN_NGAP_ServiceAreaInformation.h"
#include "ASN_NGAP_ServiceAreaInformation-Item.h"
#include "ASN_NGAP_SgNB-UE-X2AP-ID.h"
#include "ASN_NGAP_SliceOverloadItem.h"
#include "ASN_NGAP_SliceOverloadList.h"
#include "ASN_NGAP_SliceSupportItem.h"
#include "ASN_NGAP_SliceSupportList.h"
#include "ASN_NGAP_S-NSSAI.h"
#include "ASN_NGAP_SONConfigurationTransfer.h"
#include "ASN_NGAP_SONInformation.h"
#include "ASN_NGAP_SONInformationReply.h"
#include "ASN_NGAP_SONInformationRequest.h"
#include "ASN_NGAP_SourceNGRANNode-ToTargetNGRANNode-TransparentContainer.h"
#include "ASN_NGAP_SourceOfUEActivityBehaviourInformation.h"
#include "ASN_NGAP_SourceRANNodeID.h"
#include "ASN_NGAP_SourceToTarget-AMFInformationReroute.h"
#include "ASN_NGAP_SourceToTarget-TransparentContainer.h"
#include "ASN_NGAP_SRVCCOperationPossible.h"
#include "ASN_NGAP_SST.h"
#include "ASN_NGAP_SuccessfulOutcome.h"
#include "ASN_NGAP_SupportedTAItem.h"
#include "ASN_NGAP_SupportedTAList.h"
#include "ASN_NGAP_TAC.h"
#include "ASN_NGAP_TAIBroadcastEUTRA.h"
#include "ASN_NGAP_TAIBroadcastEUTRA-Item.h"
#include "ASN_NGAP_TAIBroadcastNR.h"
#include "ASN_NGAP_TAIBroadcastNR-Item.h"
#include "ASN_NGAP_TAICancelledEUTRA.h"
#include "ASN_NGAP_TAICancelledEUTRA-Item.h"
#include "ASN_NGAP_TAICancelledNR.h"
#include "ASN_NGAP_TAICancelledNR-Item.h"
#include "ASN_NGAP_TAI.h"
#include "ASN_NGAP_TAIListForInactive.h"
#include "ASN_NGAP_TAIListForInactiveItem.h"
#include "ASN_NGAP_TAIListForPaging.h"
#include "ASN_NGAP_TAIListForPagingItem.h"
#include "ASN_NGAP_TAIListForRestart.h"
#include "ASN_NGAP_TAIListForWarning.h"
#include "ASN_NGAP_TargeteNB-ID.h"
#include "ASN_NGAP_TargetID.h"
#include "ASN_NGAP_TargetNGRANNode-ToSourceNGRANNode-TransparentContainer.h"
#include "ASN_NGAP_TargetRANNodeID.h"
#include "ASN_NGAP_TargetRNC-ID.h"
#include "ASN_NGAP_TargetToSource-TransparentContainer.h"
#include "ASN_NGAP_TimerApproachForGUAMIRemoval.h"
#include "ASN_NGAP_TimeStamp.h"
#include "ASN_NGAP_TimeToWait.h"
#include "ASN_NGAP_TimeUEStayedInCellEnhancedGranularity.h"
#include "ASN_NGAP_TimeUEStayedInCell.h"
#include "ASN_NGAP_TNLAddressWeightFactor.h"
#include "ASN_NGAP_TNLAssociationItem.h"
#include "ASN_NGAP_TNLAssociationList.h"
#include "ASN_NGAP_TNLAssociationUsage.h"
#include "ASN_NGAP_TraceActivation.h"
#include "ASN_NGAP_TraceDepth.h"
#include "ASN_NGAP_TraceFailureIndication.h"
#include "ASN_NGAP_TraceStart.h"
#include "ASN_NGAP_TrafficLoadReductionIndication.h"
#include "ASN_NGAP_TransportLayerAddress.h"
#include "ASN_NGAP_TriggeringMessage.h"
#include "ASN_NGAP_TypeOfError.h"
#include "ASN_NGAP_UEAggregateMaximumBitRate.h"
#include "ASN_NGAP_UE-associatedLogicalNG-connectionItem.h"
#include "ASN_NGAP_UE-associatedLogicalNG-connectionList.h"
#include "ASN_NGAP_UEContextModificationFailure.h"
#include "ASN_NGAP_UEContextModificationRequest.h"
#include "ASN_NGAP_UEContextModificationResponse.h"
#include "ASN_NGAP_UEContextReleaseCommand.h"
#include "ASN_NGAP_UEContextReleaseComplete.h"
#include "ASN_NGAP_UEContextReleaseRequest.h"
#include "ASN_NGAP_UEContextRequest.h"
#include "ASN_NGAP_UEHistoryInformation.h"
#include "ASN_NGAP_UEIdentityIndexValue.h"
#include "ASN_NGAP_UE-NGAP-ID-pair.h"
#include "ASN_NGAP_UE-NGAP-IDs.h"
#include "ASN_NGAP_UEPagingIdentity.h"
#include "ASN_NGAP_UEPresence.h"
#include "ASN_NGAP_UEPresenceInAreaOfInterestItem.h"
#include "ASN_NGAP_UEPresenceInAreaOfInterestList.h"
#include "ASN_NGAP_UERadioCapabilityCheckRequest.h"
#include "ASN_NGAP_UERadioCapabilityCheckResponse.h"
#include "ASN_NGAP_UERadioCapabilityForPaging.h"
#include "ASN_NGAP_UERadioCapabilityForPagingOfEUTRA.h"
#include "ASN_NGAP_UERadioCapabilityForPagingOfNR.h"
#include "ASN_NGAP_UERadioCapability.h"
#include "ASN_NGAP_UERadioCapabilityInfoIndication.h"
#include "ASN_NGAP_UERetentionInformation.h"
#include "ASN_NGAP_UESecurityCapabilities.h"
#include "ASN_NGAP_UETNLABindingReleaseRequest.h"
#include "ASN_NGAP_ULForwarding.h"
#include "ASN_NGAP_UL-NGU-UP-TNLModifyItem.h"
#include "ASN_NGAP_UL-NGU-UP-TNLModifyList.h"
#include "ASN_NGAP_UnavailableGUAMIItem.h"
#include "ASN_NGAP_UnavailableGUAMIList.h"
#include "ASN_NGAP_UnsuccessfulOutcome.h"
#include "ASN_NGAP_UplinkNASTransport.h"
#include "ASN_NGAP_UplinkNonUEAssociatedNRPPaTransport.h"
#include "ASN_NGAP_UplinkRANConfigurationTransfer.h"
#include "ASN_NGAP_UplinkRANStatusTransfer.h"
#include "ASN_NGAP_UplinkRIMInformationTransfer.h"
#include "ASN_NGAP_UplinkUEAssociatedNRPPaTransport.h"
#include "ASN_NGAP_UPTransportLayerInformation.h"
#include "ASN_NGAP_UPTransportLayerInformationItem.h"
#include "ASN_NGAP_UPTransportLayerInformationList.h"
#include "ASN_NGAP_UPTransportLayerInformationPairItem.h"
#include "ASN_NGAP_UPTransportLayerInformationPairList.h"
#include "ASN_NGAP_UserLocationInformationEUTRA.h"
#include "ASN_NGAP_UserLocationInformation.h"
#include "ASN_NGAP_UserLocationInformationN3IWF.h"
#include "ASN_NGAP_UserLocationInformationNR.h"
#include "ASN_NGAP_UserPlaneSecurityInformation.h"
#include "ASN_NGAP_VolumeTimedReport-Item.h"
#include "ASN_NGAP_VolumeTimedReportList.h"
#include "ASN_NGAP_WarningAreaCoordinates.h"
#include "ASN_NGAP_WarningAreaList.h"
#include "ASN_NGAP_WarningMessageContents.h"
#include "ASN_NGAP_WarningSecurityInfo.h"
#include "ASN_NGAP_WarningType.h"
#include "ASN_NGAP_WriteReplaceWarningRequest.h"
#include "ASN_NGAP_WriteReplaceWarningResponse.h"
#include "ASN_NGAP_XnExtTLA-Item.h"
#include "ASN_NGAP_XnExtTLAs.h"
#include "ASN_NGAP_XnGTP-TLAs.h"
#include "ASN_NGAP_XnTLAs.h"
#include "ASN_NGAP_XnTNLConfigurationInfo.h"
#include "ASN_NGAP_EXTERNAL.h"
#include "ASN_NGAP_ExtendedUEIdentityIndexValue.h"
#include "ASN_NGAP_MicoAllPLMN.h"
#include "ASN_NGAP_QosFlowFeedbackItem.h"
#include "ASN_NGAP_QosFlowFeedbackList.h"
#include "ASN_NGAP_UpdateFeedback.h"

#include "asn1c/util/conv.h"
#include "asn1c/util/message.h"

#include "lib/ngap/ngap_message.h"


#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif
