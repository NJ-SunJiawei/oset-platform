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
#define OSET_LOG2_DOMAIN   "app-gnb-librfBuf"

void set_nof_samples(rf_buffer_t *rfbuf, uint32_t n)
{
	rfbuf->nof_samples = n; 
}

uint32_t get_nof_samples(rf_buffer_t *rfbuf)
{ 
	return rfbuf->nof_samples;
}

void set_combine(rf_buffer_t *rfbuf, uint32_t channel_idx, cf_t* ptr)
{
  if (rfbuf->sample_buffer[channel_idx] == NULL) {
	rfbuf->sample_buffer[channel_idx] = ptr;
  } else if (ptr != NULL) {
	srsran_vec_sum_ccc(ptr, rfbuf->sample_buffer[channel_idx], rfbuf->sample_buffer[channel_idx], rfbuf->nof_samples);
  }
}

void set_combine_all(rf_buffer_t *rfbuf, rf_buffer_t* other)
{
  // Take the other number of samples always
  set_nof_samples(rfbuf, get_nof_samples(other));
  for (uint32_t ch = 0; ch < SRSRAN_MAX_CHANNELS; ch++) {
	set_combine(rfbuf, ch, other->sample_buffer[ch]);
  }
}

