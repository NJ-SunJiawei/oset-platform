/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef OSET_ASN_MESSAGE_H
#define OSET_ASN_MESSAGE_H

#include "oset-core.h"

#include "asn_internal.h"
#include "constr_TYPE.h"

#ifdef __cplusplus
extern "C" {
#endif

int asn1_encoder_xer_print = 0;

#define OSET_MAX_SDU_LEN     8192

typedef enum {
	asn_struct_not_free,
	asn_struct_free_context,
	asn_struct_free_all,
}ASN_STRUCT_FREE_FLAG;

void* oset_asn_new_buffer_per_encode(const asn_TYPE_descriptor_t *td, enum asn_transfer_syntax type, void *sptr, ASN_STRUCT_FREE_FLAG free_flag);

oset_pkbuf_t *oset_asn_per_encode(const asn_TYPE_descriptor_t *td, enum asn_transfer_syntax type, void *sptr, ASN_STRUCT_FREE_FLAG free_flag);
int oset_asn_per_decode(const asn_TYPE_descriptor_t *td, enum asn_transfer_syntax type,
                                void *struct_ptr, size_t struct_size, oset_pkbuf_t *pkbuf);
oset_pkbuf_t *oset_asn_aper_encode(const asn_TYPE_descriptor_t *td, void *sptr, ASN_STRUCT_FREE_FLAG free_flag);
int oset_asn_aper_decode(const asn_TYPE_descriptor_t *td,
                                 void *struct_ptr, size_t struct_size, oset_pkbuf_t *pkbuf);
oset_pkbuf_t *oset_asn_uper_encode(const asn_TYPE_descriptor_t *td, void *sptr, ASN_STRUCT_FREE_FLAG free_flag);
int oset_asn_uper_decode(const asn_TYPE_descriptor_t *td,
		                 void *struct_ptr, size_t struct_size, oset_pkbuf_t *pkbuf);
void oset_asn_free_contexts(const asn_TYPE_descriptor_t *td, void *sptr);
void oset_asn_free_all(const asn_TYPE_descriptor_t *td, void *sptr);


#define asn1cCallocOne(VaR, VaLue) \
  VaR = CALLOC(1,sizeof(*VaR)); *VaR=VaLue;

#define asn1cCalloc(VaR, lOcPtr) \
  typeof(VaR) lOcPtr = VaR = CALLOC(1,sizeof(*VaR));

#define asn1cSequenceAdd(VaR, TyPe, lOcPtr) \
  TyPe *lOcPtr= CALLOC(1,sizeof(TyPe)); \
  ASN_SEQUENCE_ADD(&VaR,lOcPtr);

#ifdef __cplusplus
}
#endif

#endif

