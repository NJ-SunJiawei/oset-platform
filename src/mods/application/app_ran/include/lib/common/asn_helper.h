/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef ASN_HELPER_H_
#define ASN_HELPER_H_

#include "oset-core.h"

#ifdef __cplusplus
extern "C" {
#endif

inline uint32_t ceil_frac(uint32_t n, uint32_t d)
{
  return (n + (d - 1)) / d;
}

inline uint32_t nof_octets(uint32_t nof_bits) 
{ 

    return ceil_frac(nof_bits, 8u);
}

typedef struct {
	uint8_t *buf;	/* BIT STRING body */
	size_t size;	/* Size of the above buffer */
	int bits_unused;/* Unused trailing bits in the last octet (0..7) */
} bitstring;

typedef A_DYN_ARRAY_OF(uint8_t) dyn_octstring;
typedef uint8_t fixed_octstring;

/************************
	bitstring util
************************/
inline bool bitstring_get(const uint8_t* ptr, uint32_t idx)
{
  uint32_t byte_idx = idx / 8;
  uint32_t offset   = idx % 8;
  return (ptr[byte_idx] & (1u << offset)) > 0;
}

//((uint16_t)(1u << 8u) - 1u) = 0xFF
inline void bitstring_set(uint8_t* ptr, uint32_t idx, bool value)
{
  uint32_t byte_idx = idx / 8;
  uint32_t offset   = idx % 8;
  if (value) {
    ptr[byte_idx] |= (1u << offset);
  } else {
    ptr[byte_idx] &= ((uint16_t)(1u << 8u) - 1u) - (1u << offset);//??? ~(1u << offset)
  }
}

/************************
    common bitstring
************************/

inline void bitstring_from_number(uint8_t* ptr, uint64_t number, uint32_t nbits)
{
  if (nbits > 64u) {
	oset_error("bitstring of size=%d does not fit in an uint64_t", nbits);
    return;
  }
  uint32_t nof_bytes = ((nbits + (8u - 1)) / 8u);
  for (uint32_t i = 0; i < nof_bytes; ++i) {
    ptr[i] = (number >> (i * 8u)) & 0xFFu;
  }
  uint32_t offset = nbits % 8; // clean up any extra set bit
  if (offset > 0) {
    ptr[nof_bytes - 1] &= (uint8_t)((1u << offset) - 1u);
  }
}

inline char* bitstring_to_string(uint8_t* ptr, uint32_t nbits)
{
  char str[64+1] = {0};
  for (uint32_t i = 0; i < nbits; ++i) {
    str[i] = bitstring_get(ptr, nbits - 1 - i) ? '1' : '0';
  }
  return str;
}

inline uint64_t bitstring_to_number(uint8_t* ptr, uint32_t nbits)
{
  if (nbits > 64u) {
    oset_error("bitstring of size=%d does not fit in an uint64_t", nbits);
    return 0;
  }
  uint64_t val       = 0;
  uint32_t nof_bytes = ceil_frac(nbits, 8u);
  for (uint32_t i = 0; i < nof_bytes; ++i) {
    val += ptr[i] << (i * 8u);
  }
  return val;
}

/************************
    common octstring
************************/
inline uint64_t octstring_to_number(const uint8_t* ptr, uint32_t nbytes)
{
  if (nbytes > 8) {
	oset_error("octstring of size=%d does not fit in an uint64_t", nbytes);
	return 0;
  }
  uint64_t val = 0;
  for (uint32_t i = 0; i < nbytes; ++i) {
	val += ((uint64_t)ptr[nbytes - 1 - i]) << (uint64_t)(i * 8);
  }
  return val;
}

inline void number_to_octstring(uint8_t* ptr, uint64_t number, uint32_t nbytes)
{
  if (nbytes > 8) {
	oset_error("octstring of size=%d does not fit in an uint64_t", nbytes);
	return;
  }
  for (uint32_t i = 0; i < nbytes; ++i) {
	ptr[nbytes - 1 - i] = (number >> (uint64_t)(i * 8u)) & 0xFFu;
  }
}

inline void to_hex(char* cstr, uint8_t val)
{
  sprintf(cstr, "%02x", val);
}


#ifdef __cplusplus
}
#endif

#endif
