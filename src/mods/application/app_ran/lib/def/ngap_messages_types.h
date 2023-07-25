#ifndef NGAP_MESSAGES_TYPES_H_
#define NGAP_MESSAGES_TYPES_H_

#include "LTE_asn_constant.h"
//-------------------------------------------------------------------------------------------//
// Defines to access message fields.

#define NGAP_REGISTER_GNB_REQ(mSGpTR)           (mSGpTR)->msg_body.ngap_register_gnb_req

#define NGAP_REGISTER_GNB_CNF(mSGpTR)           (mSGpTR)->msg_body.ngap_register_gnb_cnf
#define NGAP_DEREGISTERED_GNB_IND(mSGpTR)       (mSGpTR)->msg_body.ngap_deregistered_gnb_ind

#define NGAP_NAS_FIRST_REQ(mSGpTR)              (mSGpTR)->msg_body.ngap_nas_first_req
#define NGAP_UPLINK_NAS(mSGpTR)                 (mSGpTR)->msg_body.ngap_uplink_nas
#define NGAP_UE_CAPABILITIES_IND(mSGpTR)        (mSGpTR)->msg_body.ngap_ue_cap_info_ind
#define NGAP_INITIAL_CONTEXT_SETUP_RESP(mSGpTR) (mSGpTR)->msg_body.ngap_initial_context_setup_resp
#define NGAP_INITIAL_CONTEXT_SETUP_FAIL(mSGpTR) (mSGpTR)->msg_body.ngap_initial_context_setup_fail
#define NGAP_UE_CONTEXT_RELEASE_RESP(mSGpTR)    (mSGpTR)->msg_body.ngap_ue_release_resp
#define NGAP_NAS_NON_DELIVERY_IND(mSGpTR)       (mSGpTR)->msg_body.ngap_nas_non_delivery_ind
#define NGAP_UE_CTXT_MODIFICATION_RESP(mSGpTR)  (mSGpTR)->msg_body.ngap_ue_ctxt_modification_resp
#define NGAP_UE_CTXT_MODIFICATION_FAIL(mSGpTR)  (mSGpTR)->msg_body.ngap_ue_ctxt_modification_fail
#define NGAP_PDUSESSION_SETUP_RESP(mSGpTR)           (mSGpTR)->msg_body.ngap_pdusession_setup_resp
#define NGAP_PDUSESSION_SETUP_FAIL(mSGpTR)           (mSGpTR)->msg_body.ngap_pdusession_setup_req_fail
#define NGAP_PDUSESSION_MODIFY_RESP(mSGpTR)           (mSGpTR)->msg_body.ngap_pdusession_modify_resp
#define NGAP_PATH_SWITCH_REQ(mSGpTR)            (mSGpTR)->msg_body.ngap_path_switch_req
#define NGAP_PATH_SWITCH_REQ_ACK(mSGpTR)        (mSGpTR)->msg_body.ngap_path_switch_req_ack
#define NGAP_PDUSESSION_MODIFICATION_IND(mSGpTR)     (mSGpTR)->msg_body.ngap_pdusession_modification_ind

#define NGAP_DOWNLINK_NAS(mSGpTR)               (mSGpTR)->msg_body.ngap_downlink_nas
#define NGAP_INITIAL_CONTEXT_SETUP_REQ(mSGpTR)  (mSGpTR)->msg_body.ngap_initial_context_setup_req
#define NGAP_UE_CTXT_MODIFICATION_REQ(mSGpTR)   (mSGpTR)->msg_body.ngap_ue_ctxt_modification_req
#define NGAP_UE_CONTEXT_RELEASE_COMMAND(mSGpTR) (mSGpTR)->msg_body.ngap_ue_release_command
#define NGAP_UE_CONTEXT_RELEASE_COMPLETE(mSGpTR) (mSGpTR)->msg_body.ngap_ue_release_complete
#define NGAP_PDUSESSION_SETUP_REQ(mSGpTR)              (mSGpTR)->msg_body.ngap_pdusession_setup_req
#define NGAP_PDUSESSION_MODIFY_REQ(mSGpTR)              (mSGpTR)->msg_body.ngap_pdusession_modify_req
#define NGAP_PAGING_IND(mSGpTR)                 (mSGpTR)->msg_body.ngap_paging_ind

#define NGAP_UE_CONTEXT_RELEASE_REQ(mSGpTR)     (mSGpTR)->msg_body.ngap_ue_release_req
#define NGAP_PDUSESSION_RELEASE_COMMAND(mSGpTR)      (mSGpTR)->msg_body.ngap_pdusession_release_command
#define NGAP_PDUSESSION_RELEASE_RESPONSE(mSGpTR)     (mSGpTR)->msg_body.ngap_pdusession_release_resp

#endif /* NGAP_MESSAGES_TYPES_H_ */
