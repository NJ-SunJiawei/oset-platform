/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NR-RRC-Definitions"
 * 	found in "asn/nr-rrc-15.6.0.asn1"
 * 	`asn1c -fcompound-names -pdu=all -findirect-choice -fno-include-deps -gen-PER -no-gen-OER -no-gen-example -D rrc`
 */

#ifndef	_ASN_RRC_RAT_Type_H_
#define	_ASN_RRC_RAT_Type_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum ASN_RRC_RAT_Type {
	ASN_RRC_RAT_Type_nr	= 0,
	ASN_RRC_RAT_Type_eutra_nr	= 1,
	ASN_RRC_RAT_Type_eutra	= 2,
	ASN_RRC_RAT_Type_spare1	= 3
	/*
	 * Enumeration is extensible
	 */
} e_ASN_RRC_RAT_Type;

/* ASN_RRC_RAT-Type */
typedef long	 ASN_RRC_RAT_Type_t;

/* Implementation */
extern asn_per_constraints_t asn_PER_type_ASN_RRC_RAT_Type_constr_1;
extern asn_TYPE_descriptor_t asn_DEF_ASN_RRC_RAT_Type;
extern const asn_INTEGER_specifics_t asn_SPC_ASN_RRC_RAT_Type_specs_1;
asn_struct_free_f ASN_RRC_RAT_Type_free;
asn_struct_print_f ASN_RRC_RAT_Type_print;
asn_constr_check_f ASN_RRC_RAT_Type_constraint;
ber_type_decoder_f ASN_RRC_RAT_Type_decode_ber;
der_type_encoder_f ASN_RRC_RAT_Type_encode_der;
xer_type_decoder_f ASN_RRC_RAT_Type_decode_xer;
xer_type_encoder_f ASN_RRC_RAT_Type_encode_xer;
per_type_decoder_f ASN_RRC_RAT_Type_decode_uper;
per_type_encoder_f ASN_RRC_RAT_Type_encode_uper;
per_type_decoder_f ASN_RRC_RAT_Type_decode_aper;
per_type_encoder_f ASN_RRC_RAT_Type_encode_aper;

#ifdef __cplusplus
}
#endif

#endif	/* _ASN_RRC_RAT_Type_H_ */
#include <asn_internal.h>
