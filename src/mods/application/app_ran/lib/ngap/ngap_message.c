/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "lib/ngap/ngap_message_list.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-libngapMsg"

oset_pkbuf_t *oset_ngap_encode(oset_ngap_message_t *message)
{
    oset_pkbuf_t *pkbuf = NULL;

    oset_assert(message);

    if (oset_runtime()->hard_log_level >= OSET_LOG2_DEBUG){
	    if (asn1_encoder_xer_print){
			xer_fprint(stdout, &asn_DEF_ASN_NGAP_NGAP_PDU, message);
		}else{
			asn_fprint(stdout, &asn_DEF_ASN_NGAP_NGAP_PDU, message);
		}
	}

    pkbuf = oset_asn_aper_encode(&asn_DEF_ASN_NGAP_NGAP_PDU, message, false);
    oset_expect_or_return_val(pkbuf, NULL);

    return pkbuf;
}

int oset_ngap_decode(oset_ngap_message_t *message, oset_pkbuf_t *pkbuf)
{
    int rv;
    oset_assert(message);
    oset_assert(pkbuf);
    oset_assert(pkbuf->data);
    oset_assert(pkbuf->len);

    rv = oset_asn_aper_decode(&asn_DEF_ASN_NGAP_NGAP_PDU,
            message, sizeof(oset_ngap_message_t), pkbuf);
    if (rv != OSET_OK) {
        oset_warn("Failed to decode NGAP-PDU");
        return rv;
    }

    if (oset_runtime()->hard_log_level >= OSET_LOG2_DEBUG){
	    if (asn1_encoder_xer_print){
			xer_fprint(stdout, &asn_DEF_ASN_NGAP_NGAP_PDU, message);
		}else{
			asn_fprint(stdout, &asn_DEF_ASN_NGAP_NGAP_PDU, message);
		}
	}


    return OSET_OK;
}

void oset_ngap_free(oset_ngap_message_t *message)
{
    oset_assert(message);
    oset_asn_free_contexts(&asn_DEF_ASN_NGAP_NGAP_PDU, message);
}
