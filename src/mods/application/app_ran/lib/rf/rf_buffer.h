/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RF_BUFFER_H_
#define RF_BUFFER_H_

//#include "lib/srsran/phy/common/phy_common.h"
#include "lib/srsran/srsran.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rf_buffer_s{
	cf_t             *sample_buffer[SRSRAN_MAX_CHANNELS];
	bool             allocated;
	uint32_t		 nof_subframes;
	uint32_t		 nof_samples;
}rf_buffer_t;

void set_nof_samples(rf_buffer_t *rfbuf, uint32_t n);
uint32_t get_nof_samples(rf_buffer_t *rfbuf);
void set_combine(rf_buffer_t *rfbuf, uint32_t channel_idx, cf_t* ptr);
void set_combine_all(rf_buffer_t *rfbuf, rf_buffer_t* other);


#ifdef __cplusplus
}
#endif

#endif
