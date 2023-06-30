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
#define OSET_LOG2_DOMAIN   "app-gnb-asn1cPer"

static oset_pkbuf_pool_t *asn_buffer_pool = NULL;

int asn_buffer_pool_init(void)
{
    oset_pkbuf_config_t config;
    memset(&config, 0, sizeof config);

    config.cluster_8192_pool = 1024;
    asn_buffer_pool = oset_pkbuf_pool_create(&config);

    return OSET_OK;
}

void asn_buffer_pool_final(void)
{
    oset_pkbuf_pool_destroy(asn_buffer_pool);
}

void* oset_asn_new_buffer_per_encode(const asn_TYPE_descriptor_t *td, enum asn_transfer_syntax type, void *sptr, ASN_STRUCT_FREE_FLAG free_flag)
{
    asn_encode_to_new_buffer_result_t res = {0};
    oset_assert(td);
    oset_assert(sptr);

    res = asn_encode_to_new_buffer(NULL, type, td, sptr)

    if(free_flag == asn_struct_free_all){
		oset_asn_free_all(td, sptr);
	}else if(free_flag == asn_struct_free_context){
		oset_asn_free_contexts(td, sptr);
	}

    if (res.result.encoded < 0) {
        oset_error("Failed to encode ASN-PDU [%d]", (int)res.result.encoded);
        FREEMEM(res.buffer);
        return NULL;
    }

    return res.buffer;
}


oset_pkbuf_t *oset_asn_per_encode(const asn_TYPE_descriptor_t *td, enum asn_transfer_syntax type, void *sptr, ASN_STRUCT_FREE_FLAG free_flag)
{
    asn_enc_rval_t enc_ret = {0};
    oset_pkbuf_t *pkbuf = NULL;

    oset_assert(td);
    oset_assert(sptr);

    pkbuf = oset_pkbuf_alloc(asn_buffer_pool, OSET_MAX_SDU_LEN);
    oset_expect_or_return_val(pkbuf, NULL);
    oset_pkbuf_put(pkbuf, OSET_MAX_SDU_LEN);

    enc_ret = asn_encode_to_buffer(NULL, type, td, sptr, pkbuf->data, OSET_MAX_SDU_LEN)

    if(free_flag == asn_struct_free_all){
		oset_asn_free_all(td, sptr);
	}else if(free_flag == asn_struct_free_context){
		oset_asn_free_contexts(td, sptr);
	}

    if (enc_ret.encoded < 0) {
        oset_error("Failed to encode ASN-PDU [%d]", (int)enc_ret.encoded);
        oset_pkbuf_free(pkbuf);
        return NULL;
    }

    oset_pkbuf_trim(pkbuf, ((enc_ret.encoded + 7) >> 3));

    return pkbuf;
}

int oset_asn_per_decode(const asn_TYPE_descriptor_t *td, enum asn_transfer_syntax type,
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
    dec_ret = asn_decode(NULL, type, td, (void **)&struct_ptr, pkbuf->data, pkbuf->len);

    if (dec_ret.code != RC_OK) {
        oset_warn("Failed to decode ASN-PDU [code:%d,consumed:%d]",
                dec_ret.code, (int)dec_ret.consumed);
        return OSET_ERROR;
    }

    return OSET_OK;
}


oset_pkbuf_t *oset_asn_aper_encode(const asn_TYPE_descriptor_t *td, void *sptr, ASN_STRUCT_FREE_FLAG free_flag)
{
    asn_enc_rval_t enc_ret = {0};
    oset_pkbuf_t *pkbuf = NULL;

    oset_assert(td);
    oset_assert(sptr);

    pkbuf = oset_pkbuf_alloc(asn_buffer_pool, OSET_MAX_SDU_LEN);
    oset_expect_or_return_val(pkbuf, NULL);
    oset_pkbuf_put(pkbuf, OSET_MAX_SDU_LEN);

    enc_ret = aper_encode_to_buffer(td, NULL,
                    sptr, pkbuf->data, OSET_MAX_SDU_LEN);
    if(free_flag == asn_struct_free_all){
		oset_asn_free_all(td, sptr);
	}else if(free_flag == asn_struct_free_context){
		oset_asn_free_contexts(td, sptr);
	}

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

oset_pkbuf_t *oset_asn_uper_encode(const asn_TYPE_descriptor_t *td, void *sptr, ASN_STRUCT_FREE_FLAG free_flag)
{
	asn_enc_rval_t enc_ret = {0};
	oset_pkbuf_t *pkbuf = NULL;

	oset_assert(td);
	oset_assert(sptr);

	pkbuf = oset_pkbuf_alloc(asn_buffer_pool, OSET_MAX_SDU_LEN);
	oset_expect_or_return_val(pkbuf, NULL);
	oset_pkbuf_put(pkbuf, OSET_MAX_SDU_LEN);

	enc_ret = uper_encode_to_buffer(td, NULL,
					sptr, pkbuf->data, OSET_MAX_SDU_LEN);
    if(free_flag == asn_struct_free_all){
		oset_asn_free_all(td, sptr);
	}else if(free_flag == asn_struct_free_context){
		oset_asn_free_contexts(td, sptr);
	}

	if (enc_ret.encoded < 0) {
		oset_error("Failed to encode ASN-PDU [%d]", (int)enc_ret.encoded);
		oset_pkbuf_free(pkbuf);
		return NULL;
	}

	oset_pkbuf_trim(pkbuf, ((enc_ret.encoded + 7) >> 3));

	return pkbuf;
}

int oset_asn_uper_decode(const asn_TYPE_descriptor_t *td,
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
	dec_ret = uper_decode(NULL, td, (void **)&struct_ptr,
			pkbuf->data, pkbuf->len, 0, 0);

	if (dec_ret.code != RC_OK) {
		oset_warn("Failed to decode ASN-PDU [code:%d,consumed:%d]",
				dec_ret.code, (int)dec_ret.consumed);
		return OSET_ERROR;
	}

	return OSET_OK;
}


void oset_asn_free_contexts(const asn_TYPE_descriptor_t *td, void *sptr)
{
    oset_assert(td);
    oset_assert(sptr);

    ASN_STRUCT_FREE_CONTENTS_ONLY(*td, sptr);// if *sptr == oset_ngap_message_t/oset_rrc_message_t
}

void oset_asn_free_all(const asn_TYPE_descriptor_t *td, void *sptr)
{
    oset_assert(td);
    oset_assert(sptr);

	ASN_STRUCT_FREE(*td, sptr);//if *sptr == NULL
}

