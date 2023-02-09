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

#include "lib/srsran/phy/common/phy_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rf_buffer_s{
	cf_t                                   *sample_buffer[SRSRAN_MAX_CHANNELS];
	//uint32_t							   nof_subframes;
	uint32_t							   nof_samples;
}rf_buffer_t;

#ifdef __cplusplus
}
#endif

#endif
