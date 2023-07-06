/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RF_TIMESTAMP_H
#define RF_TIMESTAMP_H

#include "lib/srsran/phy/common/timestamp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rf_timestamp_s{
    srsran_timestamp_t	default_ts;
    srsran_timestamp_t  timestamps[SRSRAN_MAX_CHANNELS];
}rf_timestamp_t;


inline srsran_timestamp_t* get_timestamp(rf_timestamp_t *tx, uint32_t idx)
{
  if (idx >= SRSRAN_MAX_CHANNELS) {
	return tx->default_ts;
  }

  return tx->timestamps[idx];
}

#ifdef __cplusplus
}
#endif

#endif
