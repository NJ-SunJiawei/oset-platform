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

oset_pkbuf_t *oset_asn_per_encode(const asn_TYPE_descriptor_t *td, enum asn_transfer_syntax type, void *sptr);
int oset_asn_per_decode(const asn_TYPE_descriptor_t *td, enum asn_transfer_syntax type,
                                void *struct_ptr, size_t struct_size, oset_pkbuf_t *pkbuf);
oset_pkbuf_t *oset_asn_aper_encode(const asn_TYPE_descriptor_t *td, void *sptr);
int oset_asn_aper_decode(const asn_TYPE_descriptor_t *td,
                                 void *struct_ptr, size_t struct_size, oset_pkbuf_t *pkbuf);
oset_pkbuf_t *oset_asn_uper_encode(const asn_TYPE_descriptor_t *td, void *sptr);
int oset_asn_uper_decode(const asn_TYPE_descriptor_t *td,
		                 void *struct_ptr, size_t struct_size, oset_pkbuf_t *pkbuf);
void oset_asn_free_contexts(const asn_TYPE_descriptor_t *td, void *sptr);
void oset_asn_free(const asn_TYPE_descriptor_t *td, void *sptr);

#ifdef __cplusplus
}
#endif

#endif

