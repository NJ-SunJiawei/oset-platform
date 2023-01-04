/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef RADIO_H_
#define RADIO_H_

#include "channel_mapping.h"
#include "phy_common.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "radio"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rf_timestamp_s{
    srsran_timestamp_t	default_ts;
    srsran_timestamp_t  timestamps[SRSRAN_MAX_CHANNELS];
}rf_timestamp_t;

typedef struct rf_buffer_s{
	cf_t                                   *sample_buffer[SRSRAN_MAX_CHANNELS];
	bool								   allocated;
	uint32_t							   nof_subframes;
	uint32_t							   nof_samples;
}rf_buffer_t;


typedef struct {
  uint32_t rf_o;
  uint32_t rf_u;
  uint32_t rf_l;
  bool     rf_error;
}rf_metrics_t;


int radio_init(void);
int radio_destory(void);
void set_rx_srate(const double srate);
void set_tx_srate(const double srate);
void release_freq(const uint32_t carrier_idx);
void set_rx_freq(const uint32_t carrier_idx, const double freq);
void set_tx_freq(const uint32_t carrier_idx, const double freq);

#ifdef __cplusplus
}
#endif

#endif
