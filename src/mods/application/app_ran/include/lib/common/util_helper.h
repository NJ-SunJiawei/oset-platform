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
//////////////////////////////////////////////////////////////////////////////

#define span_t(T) \
		struct { \
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

#ifdef __cplusplus
}
#endif

#endif
