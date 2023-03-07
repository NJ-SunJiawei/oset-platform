/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "asn1c/util/conv.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "asn1c-conv"

void oset_asn_uint8_to_OCTET_STRING(
        uint8_t uint8, OCTET_STRING_t *octet_string)
{
    oset_assert(octet_string);

    octet_string->size = 1;
    octet_string->buf = CALLOC(octet_string->size, sizeof(uint8_t));

    octet_string->buf[0] = uint8;
}

void oset_asn_uint16_to_OCTET_STRING(
        uint16_t uint16, OCTET_STRING_t *octet_string)
{
    oset_assert(octet_string);

    octet_string->size = 2;
    octet_string->buf = CALLOC(octet_string->size, sizeof(uint8_t));

    octet_string->buf[0] = uint16 >> 8;
    octet_string->buf[1] = uint16;
}

void oset_asn_uint24_to_OCTET_STRING(
        oset_uint24_t uint24, OCTET_STRING_t *octet_string)
{
    oset_assert(octet_string);

    octet_string->size = 3;
    octet_string->buf = CALLOC(octet_string->size, sizeof(uint8_t));

    octet_string->buf[0] = uint24.v >> 16;
    octet_string->buf[1] = uint24.v >> 8;
    octet_string->buf[2] = uint24.v;
}

void oset_asn_uint32_to_OCTET_STRING(
        uint32_t uint32, OCTET_STRING_t *octet_string)
{
    oset_assert(octet_string);

    octet_string->size = 4;
    octet_string->buf = CALLOC(octet_string->size, sizeof(uint8_t));

    octet_string->buf[0] = uint32 >> 24;
    octet_string->buf[1] = uint32 >> 16;
    octet_string->buf[2] = uint32 >> 8;
    octet_string->buf[3] = uint32;
}

void oset_asn_OCTET_STRING_to_uint8(
        OCTET_STRING_t *octet_string, uint8_t *uint8)
{
    oset_assert(octet_string);
    oset_assert(uint8);

    *uint8 = octet_string->buf[0];
}
void oset_asn_OCTET_STRING_to_uint16(
        OCTET_STRING_t *octet_string, uint16_t *uint16)
{
    oset_assert(octet_string);
    oset_assert(uint16);

    *uint16 = (octet_string->buf[0] << 8) + octet_string->buf[1];
}
void oset_asn_OCTET_STRING_to_uint24(
        OCTET_STRING_t *octet_string, oset_uint24_t *uint24)
{
    oset_assert(octet_string);
    oset_assert(uint24);

    memcpy(uint24, octet_string->buf, sizeof(oset_uint24_t));
    *uint24 = oset_be24toh(*uint24);
}
void oset_asn_OCTET_STRING_to_uint32(
        OCTET_STRING_t *octet_string, uint32_t *uint32)
{
    oset_assert(octet_string);
    oset_assert(uint32);

    *uint32 = (octet_string->buf[0] << 24) + (octet_string->buf[1] << 16) +
                (octet_string->buf[2] << 8) + octet_string->buf[3];
}

void oset_asn_buffer_to_OCTET_STRING(
        void *buf, int size, OCTET_STRING_t *octet_string)
{
    octet_string->size = size;
    octet_string->buf = CALLOC(octet_string->size, sizeof(uint8_t));

    memcpy(octet_string->buf, buf, size);
}

void oset_asn_buffer_to_BIT_STRING(
        void *buf, int size, int unused, BIT_STRING_t *bit_string)
{
    bit_string->size = size;
    bit_string->buf = CALLOC(bit_string->size, sizeof(uint8_t));
    bit_string->bits_unused = unused;

    memcpy(bit_string->buf, buf, size);
}

void oset_asn_uint8_to_BIT_STRING(
		uint8_t uint8, int unused, BIT_STRING_t *bit_string)
{
	bit_string->size = 1;
	bit_string->buf = CALLOC(bit_string->size, sizeof(uint8_t));
	bit_string->bits_unused = unused;

	bit_string->buf[0] = uint8;
}


void oset_asn_uint32_to_BIT_STRING(
        uint32_t uint32, uint8_t bitsize, BIT_STRING_t *bit_string)
{
    char tmp[32] = {0};
    uint64_t uint64;
    oset_assert(bit_string);

    uint64 = uint32;
    oset_uint64_to_buffer(
            uint64 << ((32 - bitsize) % 8), (bitsize + 7) / 8, tmp);
    oset_asn_buffer_to_BIT_STRING(
            tmp, (bitsize + 7) / 8, (32 - bitsize) % 8, bit_string);
}

void oset_asn_uint64_to_BIT_STRING(
		uint64_t uint64, uint8_t bitsize, BIT_STRING_t *bit_string)
{
	char tmp[64] = {0};
	oset_assert(bit_string);

	oset_uint64_to_buffer(
			uint64 << ((64 - bitsize) % 8), (bitsize + 7) / 8, tmp);
	oset_asn_buffer_to_BIT_STRING(
			tmp, (bitsize + 7) / 8, (64 - bitsize) % 8, bit_string);
}


void oset_asn_BIT_STRING_to_uint32(BIT_STRING_t *bit_string, uint32_t *uint32)
{
    oset_assert(bit_string);
    oset_assert(uint32);

    *uint32 = oset_buffer_to_uint64(bit_string->buf, bit_string->size)
                    >> bit_string->bits_unused;
}

int oset_asn_BIT_STRING_to_ip(BIT_STRING_t *bit_string, oset_ip_t *ip)
{
    char buf[OSET_ADDRSTRLEN], buf2[OSET_ADDRSTRLEN];

    oset_assert(bit_string);
    oset_assert(ip);

    memset(ip, 0, sizeof(*ip));

    if (bit_string->size == OSET_IPV4V6_LEN) {
        ip->ipv4 = 1;
        ip->ipv6 = 1;
        memcpy(&ip->addr, bit_string->buf, OSET_IPV4_LEN);
        memcpy(&ip->addr6, bit_string->buf+OSET_IPV4_LEN, OSET_IPV6_LEN);
        oset_debug("    IPv4[%s] IPv6[%s]",
            OSET_INET_NTOP(&ip->addr, buf),
            OSET_INET6_NTOP(&ip->addr6, buf2));
    } else if (bit_string->size == OSET_IPV4_LEN) {
        ip->ipv4 = 1;
        memcpy(&ip->addr, bit_string->buf, OSET_IPV4_LEN);
        oset_debug("    IPv4[%s]", OSET_INET_NTOP(&ip->addr, buf));
    } else if (bit_string->size == OSET_IPV6_LEN) {
        ip->ipv6 = 1;
        memcpy(&ip->addr6, bit_string->buf, OSET_IPV6_LEN);
        oset_debug("    IPv6[%s]", OSET_INET_NTOP(&ip->addr6, buf));
    } else {
        oset_error("oset_asn_BIT_STRING_to_ip(size=%d) failed", bit_string->size);
        return OSET_ERROR;
    }

    ip->len = bit_string->size;

    return OSET_OK;
}
int oset_asn_ip_to_BIT_STRING(oset_ip_t *ip, BIT_STRING_t *bit_string)
{
    char buf[OSET_ADDRSTRLEN], buf2[OSET_ADDRSTRLEN];

    oset_assert(ip);
    oset_assert(bit_string);

    if (ip->ipv4 && ip->ipv6) {
        bit_string->size = OSET_IPV4V6_LEN;
        bit_string->buf = CALLOC(bit_string->size, sizeof(uint8_t));
        memcpy(bit_string->buf, &ip->addr, OSET_IPV4_LEN);
        memcpy(bit_string->buf+OSET_IPV4_LEN, &ip->addr6, OSET_IPV6_LEN);
        oset_debug("    IPv4[%s] IPv6[%s]",
            OSET_INET_NTOP(&ip->addr, buf),
            OSET_INET6_NTOP(&ip->addr6, buf2));
    } else if (ip->ipv4) {
        bit_string->size = OSET_IPV4_LEN;
        bit_string->buf = CALLOC(bit_string->size, sizeof(uint8_t));
        memcpy(bit_string->buf, &ip->addr, OSET_IPV4_LEN);
        oset_debug("    IPv4[%s]", OSET_INET_NTOP(&ip->addr, buf));
    } else if (ip->ipv6) {
        bit_string->size = OSET_IPV6_LEN;
        bit_string->buf = CALLOC(bit_string->size, sizeof(uint8_t));
        memcpy(bit_string->buf, &ip->addr6, OSET_IPV6_LEN);
        oset_debug("    IPv6[%s]", OSET_INET_NTOP(&ip->addr6, buf));
    } else
        oset_assert_if_reached();

    return OSET_OK;
}

int oset_asn_copy_ie(const asn_TYPE_descriptor_t *td, void *src, void *dst)
{
    asn_enc_rval_t enc_ret = {0};
    asn_dec_rval_t dec_ret = {0};
    uint8_t buffer[OSET_MAX_SDU_LEN];

    oset_assert(td);
    oset_assert(src);
    oset_assert(dst);

    enc_ret = aper_encode_to_buffer(td, NULL, src, buffer, OSET_MAX_SDU_LEN);
    if (enc_ret.encoded < 0) {
        oset_error("aper_encode_to_buffer() failed[%d]", (int)enc_ret.encoded);
        return OSET_ERROR;
    }

    dec_ret = aper_decode(NULL, td, (void **)&dst,
            buffer, ((enc_ret.encoded + 7) / 8), 0, 0);

    if (dec_ret.code != RC_OK) {
        oset_error("aper_decode() failed[%d]", dec_ret.code);
        return OSET_ERROR;
    }

    return OSET_OK;
}

/////////////////////////////////////////////////////////////////////////
static const char hex_to_ascii_table[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

static const signed char ascii_to_hex_table[0x100] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

void hexa_to_ascii(uint8_t *from, char *to, size_t length)
{
  int i;

  for(i = 0; i < length; i++) {
    uint8_t upper = (from[i] & 0xf0) >> 4;
    uint8_t lower =  from[i] & 0x0f;
    to[2 * i] = hex_to_ascii_table[upper];
    to[2 * i + 1] = hex_to_ascii_table[lower];
  }
}

int ascii_to_hex(uint8_t *dst, const char *h)
{
  const unsigned char *hex = (const unsigned char *) h;
  unsigned i = 0;

  for (;;) {
    int high, low;

    while (*hex && isspace(*hex))
      hex++;

    if (!*hex)
      return 1;

    high = ascii_to_hex_table[*hex++];

    if (high < 0)
      return 0;

    while (*hex && isspace(*hex))
      hex++;

    if (!*hex)
      return 0;

    low = ascii_to_hex_table[*hex++];

    if (low < 0)
      return 0;

    dst[i++] = (high << 4) | low;
  }
}

