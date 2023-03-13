/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#ifndef RRC_MESSAGE_H
#define RRC_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

oset_pkbuf_t *oset_rrc_encode(const asn_TYPE_descriptor_t *td, void *message, ASN_STRUCT_FREE_FLAG free_flag);
int oset_rrc_decode(const asn_TYPE_descriptor_t *td, void *message, oset_pkbuf_t *pkbuf);
void oset_rrc_free(const asn_TYPE_descriptor_t *td, void *message);

#ifdef __cplusplus
}
#endif

#endif

