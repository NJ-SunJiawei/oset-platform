/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "asn1c/util/message.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "asn1c-per"

oset_pkbuf_t *oset_asn_aper_encode(const asn_TYPE_descriptor_t *td, void *sptr)
{
    asn_enc_rval_t enc_ret = {0};
    oset_pkbuf_t *pkbuf = NULL;

    oset_assert(td);
    oset_assert(sptr);

    pkbuf = oset_pkbuf_alloc(NULL, OSET_MAX_SDU_LEN);
    oset_expect_or_return_val(pkbuf, NULL);
    oset_pkbuf_put(pkbuf, OSET_MAX_SDU_LEN);

    enc_ret = aper_encode_to_buffer(td, NULL,
                    sptr, pkbuf->data, OSET_MAX_SDU_LEN);
    oset_asn_free(td, sptr);

    if (enc_ret.encoded < 0) {
        oset_error("Failed to encode ASN-PDU [%d]", (int)enc_ret.encoded);
        oset_pkbuf_free(pkbuf);
        return NULL;
    }

    oset_pkbuf_trim(pkbuf, ((enc_ret.encoded + 7) >> 3));

    return pkbuf;
}

int oset_asn_aper_decode(const asn_TYPE_descriptor_t *td,
        void *struct_ptr, size_t struct_size, oset_pkbuf_t *pkbuf)
{
    asn_dec_rval_t dec_ret = {0};

    oset_assert(td);
    oset_assert(struct_ptr);
    oset_assert(struct_size);
    oset_assert(pkbuf);
    oset_assert(pkbuf->data);
    oset_assert(pkbuf->len);

    memset(struct_ptr, 0, struct_size);
    dec_ret = aper_decode(NULL, td, (void **)&struct_ptr,
            pkbuf->data, pkbuf->len, 0, 0);

    if (dec_ret.code != RC_OK) {
        oset_warn("Failed to decode ASN-PDU [code:%d,consumed:%d]",
                dec_ret.code, (int)dec_ret.consumed);
        return OSET_ERROR;
    }

    return OSET_OK;
}

void oset_asn_free(const asn_TYPE_descriptor_t *td, void *sptr)
{
    oset_assert(td);
    oset_assert(sptr);

    ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);// if *sptr == oset_ngap_message_t/oset_rrc_message_t
	//ASN_STRUCT_FREE(*td, sptr);//if *sptr == NULL
}
