/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef OSET_ASN_CONV_H
#define OSET_ASN_CONV_H

#include "oset-core.h"

#include "asn_internal.h"
#include "OCTET_STRING.h"
#include "BIT_STRING.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OSET_ASN_CLEAR_DATA(__dATA) \
    do { \
        oset_assert((__dATA)); \
        if ((__dATA)->buf) { \
            FREEMEM((__dATA)->buf); \
            (__dATA)->buf = NULL; \
            (__dATA)->size = 0; \
        } \
    } while(0)
#define OSET_ASN_STORE_DATA(__dST, __sRC) \
    do { \
        oset_assert((__sRC)); \
        oset_assert((__sRC)->buf); \
        oset_assert((__dST)); \
        OSET_ASN_CLEAR_DATA(__dST); \
        (__dST)->size = (__sRC)->size; \
        (__dST)->buf = CALLOC((__dST)->size, sizeof(uint8_t)); \
        memcpy((__dST)->buf, (__sRC)->buf, (__dST)->size); \
    } while(0)

void oset_asn_uint8_to_OCTET_STRING(
        uint8_t uint8, OCTET_STRING_t *octet_string);
void oset_asn_uint16_to_OCTET_STRING(
        uint16_t uint16, OCTET_STRING_t *octet_string);
void oset_asn_uint24_to_OCTET_STRING(
        oset_uint24_t uint24, OCTET_STRING_t *octet_string);
void oset_asn_uint32_to_OCTET_STRING(
        uint32_t uint32, OCTET_STRING_t *octet_string);

void oset_asn_OCTET_STRING_to_uint8(
        OCTET_STRING_t *octet_string, uint8_t *uint8);
void oset_asn_OCTET_STRING_to_uint16(
        OCTET_STRING_t *octet_string, uint16_t *uint16);
void oset_asn_OCTET_STRING_to_uint24(
        OCTET_STRING_t *octet_string, oset_uint24_t *uint24);
void oset_asn_OCTET_STRING_to_uint32(
        OCTET_STRING_t *octet_string, uint32_t *uint32);

void oset_asn_buffer_to_OCTET_STRING(
        void *buf, int size, OCTET_STRING_t *octet_string);

void oset_asn_buffer_to_BIT_STRING(
        void *buf, int size, int unused, BIT_STRING_t *bit_string);
void oset_asn_uint32_to_BIT_STRING(
        uint32_t uint32, uint8_t bitsize, BIT_STRING_t *bit_string);
void oset_asn_BIT_STRING_to_uint32(BIT_STRING_t *bit_string, uint32_t *uint32);

int oset_asn_BIT_STRING_to_ip(
        BIT_STRING_t *bit_string, oset_ip_t *ip);
int oset_asn_ip_to_BIT_STRING(
        oset_ip_t *ip, BIT_STRING_t *bit_string);

int oset_asn_copy_ie(
        const asn_TYPE_descriptor_t *td, void *src, void *dst);

#ifdef __cplusplus
}
#endif

#endif /* OSET_ASN_CONV_H */
