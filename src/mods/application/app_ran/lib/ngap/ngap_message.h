/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#ifndef NGAP_MESSAGE_H
#define NGAP_MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ASN_NGAP_NGAP_PDU oset_ngap_message_t;

int oset_ngap_decode(oset_ngap_message_t *message, oset_pkbuf_t *pkbuf);
oset_pkbuf_t *oset_ngap_encode(oset_ngap_message_t *message);
void oset_ngap_free(oset_ngap_message_t *message);

#ifdef __cplusplus
}
#endif

#endif

