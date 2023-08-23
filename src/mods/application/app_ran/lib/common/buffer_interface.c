/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/

#include "lib/common/buffer_interface.h"

byte_buffer_t* byte_buffer_init(void)
{
	byte_buffer_t *p = oset_malloc(sizeof(byte_buffer_t));
	oset_assert(p);
	memset(p, 0, sizeof(byte_buffer_t));
	p->msg     = &p->buffer[SRSRAN_BUFFER_HEADER_OFFSET];
	p->N_bytes = 0;
	p->md	   = {0};
	return p;
}

byte_buffer_t* byte_buffer_dup(byte_buffer_t *buf)
{
	byte_buffer_t *p = oset_malloc(sizeof(byte_buffer_t));
	oset_assert(p);
	memset(p, 0, sizeof(byte_buffer_t));
	p->msg     = &p->buffer[SRSRAN_BUFFER_HEADER_OFFSET];
	p->N_bytes = buf->N_bytes;
	p->md	   = buf->md;
	memcpy(p->msg, buf->msg, p->N_bytes);
	return p;
}

byte_buffer_t* byte_buffer_dup_data(uint8_t* payload, uint32_t len)
{
	byte_buffer_t *p = byte_buffer_init();

	if (byte_buffer_get_tailroom(p) >= len) {
	  memcpy(p->msg, payload, len);
	  p->N_bytes = len;
	  return p;
	}
	oset_free(p);
	return NULL;
}

//???
byte_buffer_t* byte_buffer_copy(byte_buffer_t *p, byte_buffer_t *other)
{
	// avoid self assignment
	if (0 == memcmp(p, other, sizeof(byte_buffer_t))){
		return p;
	}
	p->msg	   = &p->buffer[SRSRAN_BUFFER_HEADER_OFFSET];
	p->N_bytes = other->N_bytes;
	p->md 	   = other->md;
	memcpy(p->msg, other->msg, p->N_bytes);
	return p;
}

byte_buffer_t* byte_buffer_change(oset_pkbuf_t *buf)
{
	byte_buffer_t *p = oset_malloc(sizeof(byte_buffer_t));
	oset_assert(p);
	memset(p, 0, sizeof(byte_buffer_t));
	p->msg     = &p->buffer[SRSRAN_BUFFER_HEADER_OFFSET];
	p->N_bytes = buf->len;
	p->md	   = {0};
	memcpy(p->msg, buf->data, p->N_bytes);
	return p;
}

byte_buffer_t* byte_buffer_clear(byte_buffer_t *p)
{
	memset(p, 0, sizeof(byte_buffer_t));
	p->msg	   = &p->buffer[SRSRAN_BUFFER_HEADER_OFFSET];
	p->N_bytes = 0;
	p->md	   = {0};
	return p;
}

uint32_t byte_buffer_get_headroom(byte_buffer_t *p)
{
	//msg - buffer = SRSRAN_BUFFER_HEADER_OFFSET
	return p->msg - p->buffer; 
}

// Returns the remaining space from what is reported to be the length of msg
uint32_t byte_buffer_get_tailroom(byte_buffer_t *p)
{
	return (sizeof(p->buffer) - byte_buffer_get_headroom(p) - p->N_bytes);
}

void byte_buffer_set_timestamp(byte_buffer_t *p)
{
	p->md.tp = oset_micro_time_now();//oset_time_now();//???gnb_get_current_time();
}

