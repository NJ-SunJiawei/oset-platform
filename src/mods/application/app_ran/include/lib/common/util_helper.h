/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef UTIL_HELPER_H_
#define UTIL_HELPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HANDLE_ERROR(x) \
  do { \
    if (x != OSET_OK) { \
      return OSET_ERROR; \
    } \
  } while (0)

#define ERROR_IF_NOT(x, ...) \
	  do { \
		if (oset_likely(x)); \
		else{ \
		  oset_error(##__VA_ARGS__); \
		  return OSET_ERROR; \
		} \
	  } while (0)

#define ERROR_IF_NOT_VOID(x, ...) \
	  do { \
		if (oset_likely(x)); \
		else{ \
		  oset_error(##__VA_ARGS__); \
		  return; \
		} \
	  } while (0)


#define ASSERT_IF_NOT(x, ...) \
	  do { \
		if (oset_likely(x)); \
		else{ \
		  oset_error(##__VA_ARGS__); \
          oset_abort(); \
		} \
	  } while (0)


inline bool bit_get(const uint8_t* ptr, uint32_t idx)
{
  uint32_t byte_idx = idx / 8;
  uint32_t offset   = idx % 8;
  return (ptr[byte_idx] & (1u << offset)) > 0;
}

inline void bit_set(uint8_t* ptr, uint32_t idx, bool value)
{
  uint32_t byte_idx = idx / 8;
  uint32_t offset   = idx % 8;
  if (value) {
    ptr[byte_idx] |= (1u << offset);
  } else {
    ptr[byte_idx] &= ((uint16_t)(1u << 8u) - 1u) - (1u << offset);//??? ~(1u << offset)
  }
}

inline void bit_from_number(uint8_t* ptr, uint64_t number, uint32_t nbits)
{
  if (nbits > 64u) {
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

#ifdef __cplusplus
}
#endif

#endif
