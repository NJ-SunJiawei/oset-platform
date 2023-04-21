/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "lib/rf/rf_buffer.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librf-buf"

void set_nof_samples(rf_buffer_t *rfbuf, uint32_t n)
{
	rfbuf->nof_samples = n; 
}

uint32_t get_nof_samples(rf_buffer_t *rfbuf)
{ 
	return rfbuf->nof_samples;
}

