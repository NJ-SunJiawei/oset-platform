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
  oset_time_t         tp;//???//oset_time_t
} buffer_metadata_t;

typedef struct byte_buffer_s
{
	uint32_t N_bytes;//store msg length
	uint8_t  buffer[SRSRAN_MAX_BUFFER_SIZE_BYTES];
	uint8_t  *msg;//store ori msg//msg = &buffer[0]
	buffer_metadata_t md;
}byte_buffer_t;

byte_buffer_t* byte_buffer_init(void);
byte_buffer_t* byte_buffer_init(byte_buffer_t *buf);
byte_buffer_t* byte_buffer_change(oset_pkbuf_t *buf);
byte_buffer_t* byte_buffer_copy(uint8_t* payload, uint32_t len);
byte_buffer_t* byte_buffer_copy(byte_buffer_t *p, byte_buffer_t *other);
byte_buffer_t*  byte_buffer_clear(byte_buffer_t *p);
uint32_t byte_buffer_get_headroom(byte_buffer_t *p);
uint32_t byte_buffer_get_tailroom(byte_buffer_t *p);
void byte_buffer_set_timestamp(byte_buffer_t *p);


#ifdef __cplusplus
}
#endif

#endif
