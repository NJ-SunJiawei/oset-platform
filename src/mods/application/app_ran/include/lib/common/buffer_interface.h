/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef BUFFER_INTERFACE_H_
#define BUFFER_INTERFACE_H_

#include "lib/common/common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct  buffer_metadata_s{
  uint32_t			  pdcp_sn;
  int64_t			  tp;//???//oset_time_t
} buffer_metadata_t;

typedef struct byte_buffer_s
{
	uint32_t N_bytes;//store msg length
	uint8_t  buffer[SRSRAN_MAX_BUFFER_SIZE_BYTES];
	uint8_t  *msg;//store ori msg//msg = &buffer[0]
	buffer_metadata_t md;
}byte_buffer_t;

#ifdef __cplusplus
}
#endif

#endif
