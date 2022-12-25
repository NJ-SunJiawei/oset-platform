/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NR-RRC-Definitions"
 * 	found in "asn/nr-rrc-15.6.0.asn1"
 * 	`asn1c -fcompound-names -pdu=all -findirect-choice -fno-include-deps -gen-PER -no-gen-OER -no-gen-example -D rrc`
 */

#ifndef	_ASN_RRC_PUSCH_TPC_CommandConfig_H_
#define	_ASN_RRC_PUSCH_TPC_CommandConfig_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeInteger.h>
#include "ASN_RRC_ServCellIndex.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ASN_RRC_PUSCH-TPC-CommandConfig */
typedef struct ASN_RRC_PUSCH_TPC_CommandConfig {
	long	*tpc_Index;	/* OPTIONAL */
	long	*tpc_IndexSUL;	/* OPTIONAL */
	ASN_RRC_ServCellIndex_t	*targetCell;	/* OPTIONAL */
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ASN_RRC_PUSCH_TPC_CommandConfig_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ASN_RRC_PUSCH_TPC_CommandConfig;
extern asn_SEQUENCE_specifics_t asn_SPC_ASN_RRC_PUSCH_TPC_CommandConfig_specs_1;
extern asn_TYPE_member_t asn_MBR_ASN_RRC_PUSCH_TPC_CommandConfig_1[3];

#ifdef __cplusplus
}
#endif

#endif	/* _ASN_RRC_PUSCH_TPC_CommandConfig_H_ */
#include <asn_internal.h>
