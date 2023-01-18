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

typedef struct byte_buffer_s
{
  uint32_t N_bytes = 0;//store msg length
  uint8_t  buffer[SRSRAN_MAX_BUFFER_SIZE_BYTES];
  uint8_t* msg;//store ori msg

  struct buffer_metadata_t {
    uint32_t            pdcp_sn;
    int64_t             tp;//???//oset_time_t
  } md;
}byte_buffer_t;

#ifdef __cplusplus
}
#endif

#endif
