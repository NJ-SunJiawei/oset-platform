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
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

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
		  oset_error(__VA_ARGS__); \
		  return OSET_ERROR; \
		} \
	  } while (0)

#define ERROR_IF_NOT_VOID(x, ...) \
	  do { \
		if (oset_likely(x)); \
		else{ \
		  oset_error(__VA_ARGS__); \
		  return; \
		} \
	  } while (0)


#define ASSERT_IF_NOT(x, ...) \
	  do { \
		if (oset_likely(x)); \
		else{ \
		  oset_error(__VA_ARGS__); \
          oset_abort(); \
		} \
	  } while (0)
//////////////////////////////////////////////////////////////////////////////

#define span_t(T) \
		struct span { \
			T        *ptr;\
			size_t   len;\
		};

#define span(data, pointer, len) \
			  do { \
			  	data.ptr = pointer;\
				data.len = len;\
			  } while (0)

#define subspan(node, data, offset, count) \
			do { \
				ASSERT_IF_NOT(count <= data.len - offset, "size out of bounds!");\
				node.ptr = data.ptr + offset;\
				node.len = count;\
			} while (0)

/////////////////////////////////////////////////////////////////////
#define rolling_average_t(T) \
		struct rolling_average {\
			T 	   avg_;\
			uint32_t count_;\
		};

/////////////////////////////////////////////////////////
/******************************************************************************
 * Safe conversions between byte buffers and integer types.
 * Note: these don't perform endian conversion - use e.g. htonl/ntohl if required
 *****************************************************************************/
inline void uint8_to_uint32(const uint8_t* buf, uint32_t* i)
{
	*i = (uint32_t)buf[0] << 24 | (uint32_t)buf[1] << 16 | (uint32_t)buf[2] << 8 | (uint32_t)buf[3];
}

inline void uint32_to_uint8(uint32_t i, uint8_t* buf)
{
	buf[0] = (i >> 24) & 0xFF;
	buf[1] = (i >> 16) & 0xFF;
	buf[2] = (i >> 8) & 0xFF;
	buf[3] = i & 0xFF;
}

inline void uint8_to_uint16(uint8_t* buf, uint16_t* i)
{
  *i = (uint32_t)buf[0] << 8 | (uint32_t)buf[1];
}

inline void uint16_to_uint8(uint16_t i, uint8_t* buf)
{
	buf[0] = (i >> 8) & 0xFF;
	buf[1] = i & 0xFF;
}

inline void uint8_to_uint24(uint8_t* buf, uint32_t* i)
{
	*i = (uint32_t)buf[0] << 16 | (uint32_t)buf[1] << 8 | (uint32_t)buf[2];
}

inline void uint24_to_uint8(uint32_t i, uint8_t* buf)
{
	buf[0] = (i >> 16) & 0xFF;
	buf[1] = (i >> 8) & 0xFF;
	buf[2] = i & 0xFF;
}

#ifdef __cplusplus
}
#endif

#endif
