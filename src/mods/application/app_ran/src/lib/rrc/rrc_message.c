/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "lib/rrc/rrc_message_list.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc-msg"


oset_pkbuf_t *oset_rrc_encode(const asn_TYPE_descriptor_t *td, void *message, ASN_STRUCT_FREE_FLAG free_flag)
{
    oset_pkbuf_t *pkbuf = NULL;

    oset_assert(message);

    if (oset_runtime()->hard_log_level >= OSET_LOG2_DEBUG){
	    if (asn1_encoder_xer_print){
			xer_fprint(stdout, td, message);
		}else{
			asn_fprint(stdout, td, message);
		}
	}

    pkbuf = oset_asn_uper_encode(td, message, free_flag);
    oset_expect_or_return_val(pkbuf, NULL);

    return pkbuf;
}

int oset_rrc_decode(const asn_TYPE_descriptor_t *td, void *message, oset_pkbuf_t *pkbuf)
{
    int rv;
    oset_assert(message);
    oset_assert(pkbuf);
    oset_assert(pkbuf->data);
    oset_assert(pkbuf->len);

    rv = oset_asn_uper_decode(td,
            message, sizeof(*message), pkbuf);
    if (rv != OSET_OK) {
        oset_warn("Failed to decode NGAP-PDU");
        return rv;
    }

    if (oset_runtime()->hard_log_level >= OSET_LOG2_DEBUG){
	    if (asn1_encoder_xer_print){
			xer_fprint(stdout, td, message);
		}else{
			asn_fprint(stdout, td, message);
		}
	}

    return OSET_OK;
}

void oset_rrc_free(const asn_TYPE_descriptor_t *td, void *message)
{
    oset_assert(message);
    oset_asn_free_contexts(td, message);
}
