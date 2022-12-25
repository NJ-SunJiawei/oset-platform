/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NR-InterNodeDefinitions"
 * 	found in "asn/nr-rrc-15.6.0.asn1"
 * 	`asn1c -fcompound-names -pdu=all -findirect-choice -fno-include-deps -gen-PER -no-gen-OER -no-gen-example -D rrc`
 */

#ifndef	_ASN_RRC_MRDC_AssistanceInfo_H_
#define	_ASN_RRC_MRDC_AssistanceInfo_H_


#include <asn_application.h>

/* Including external dependencies */
#include <asn_SEQUENCE_OF.h>
#include <constr_SEQUENCE_OF.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct ASN_RRC_AffectedCarrierFreqCombInfoMRDC;

/* ASN_RRC_MRDC-AssistanceInfo */
typedef struct ASN_RRC_MRDC_AssistanceInfo {
	struct ASN_RRC_MRDC_AssistanceInfo__affectedCarrierFreqCombInfoListMRDC {
		A_SEQUENCE_OF(struct ASN_RRC_AffectedCarrierFreqCombInfoMRDC) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} affectedCarrierFreqCombInfoListMRDC;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ASN_RRC_MRDC_AssistanceInfo_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ASN_RRC_MRDC_AssistanceInfo;
extern asn_SEQUENCE_specifics_t asn_SPC_ASN_RRC_MRDC_AssistanceInfo_specs_1;
extern asn_TYPE_member_t asn_MBR_ASN_RRC_MRDC_AssistanceInfo_1[1];

#ifdef __cplusplus
}
#endif

#endif	/* _ASN_RRC_MRDC_AssistanceInfo_H_ */
#include <asn_internal.h>
