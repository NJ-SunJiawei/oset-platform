/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NR-RRC-Definitions"
 * 	found in "asn/nr-rrc-15.6.0.asn1"
 * 	`asn1c -fcompound-names -pdu=all -findirect-choice -fno-include-deps -gen-PER -no-gen-OER -no-gen-example -D rrc`
 */

#ifndef	_ASN_RRC_TDD_UL_DL_ConfigDedicated_H_
#define	_ASN_RRC_TDD_UL_DL_ConfigDedicated_H_


#include <asn_application.h>

/* Including external dependencies */
#include <asn_SEQUENCE_OF.h>
#include <constr_SEQUENCE_OF.h>
#include "ASN_RRC_TDD-UL-DL-SlotIndex.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct ASN_RRC_TDD_UL_DL_SlotConfig;

/* ASN_RRC_TDD-UL-DL-ConfigDedicated */
typedef struct ASN_RRC_TDD_UL_DL_ConfigDedicated {
	struct ASN_RRC_TDD_UL_DL_ConfigDedicated__slotSpecificConfigurationsToAddModList {
		A_SEQUENCE_OF(struct ASN_RRC_TDD_UL_DL_SlotConfig) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} *slotSpecificConfigurationsToAddModList;
	struct ASN_RRC_TDD_UL_DL_ConfigDedicated__slotSpecificConfigurationsToreleaseList {
		A_SEQUENCE_OF(ASN_RRC_TDD_UL_DL_SlotIndex_t) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} *slotSpecificConfigurationsToreleaseList;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ASN_RRC_TDD_UL_DL_ConfigDedicated_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ASN_RRC_TDD_UL_DL_ConfigDedicated;
extern asn_SEQUENCE_specifics_t asn_SPC_ASN_RRC_TDD_UL_DL_ConfigDedicated_specs_1;
extern asn_TYPE_member_t asn_MBR_ASN_RRC_TDD_UL_DL_ConfigDedicated_1[2];

#ifdef __cplusplus
}
#endif

#endif	/* _ASN_RRC_TDD_UL_DL_ConfigDedicated_H_ */
#include <asn_internal.h>
