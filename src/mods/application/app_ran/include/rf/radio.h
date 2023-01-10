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

#include "oset-core.h"
#include "channel_mapping.h"
#include "phy_common.h"
#include "timestamp.h"


#ifdef __cplusplus
extern "C" {
#endif

#define  MAX_DEVICE_NUM 10

typedef struct rf_timestamp_s{
    srsran_timestamp_t	default_ts;
    srsran_timestamp_t  timestamps[SRSRAN_MAX_CHANNELS];
}rf_timestamp_t;

typedef struct rf_buffer_s{
	cf_t                                   *sample_buffer[SRSRAN_MAX_CHANNELS];
	//uint32_t							   nof_subframes;
	uint32_t							   nof_samples;
}rf_buffer_t;


typedef struct {
  uint32_t rf_o;
  uint32_t rf_u;
  uint32_t rf_l;
  bool     rf_error;
}rf_metrics_t;

typedef struct rf_manager_s{
	oset_apr_memory_pool_t *app_pool;

	channel_mapping     rx_channel_mapping;
	channel_mapping     tx_channel_mapping;
	srsran_rf_t		    rf_devices[MAX_DEVICE_NUM];//std::vector
	srsran_rf_info_t	rf_info[MAX_DEVICE_NUM];//std::vector
	int32_t			    rx_offset_n[MAX_DEVICE_NUM];//std::vector
	rf_metrics_t		rf_metrics;
	oset_apr_mutex_t	*metrics_mutex;
	cf_t             	*zeros;
	cf_t		        *dummy_buffers[SRSRAN_MAX_CHANNELS];//rf rx buffer ~ 1 frame len
	oset_apr_mutex_t	*tx_mutex;
	oset_apr_mutex_t	*rx_mutex;
	cf_t                *tx_buffer[SRSRAN_MAX_CHANNELS];
	cf_t                *rx_buffer[SRSRAN_MAX_CHANNELS];
	srsran_resampler_fft_t            interpolators[SRSRAN_MAX_CHANNELS];
	srsran_resampler_fft_t            decimators[SRSRAN_MAX_CHANNELS];
	bool         decimator_busy; ///< Indicates the decimator is changing the rate
	
	srsran_timestamp_t	  end_of_burst_time[SRSRAN_MAX_CHANNELS];//????//rf_timestamp_t    end_of_burst_time
	bool              is_start_of_burst;
	uint32_t		  tx_adv_nsamples;
	double			  tx_adv_sec; // Transmission time advance to compensate for antenna->timestamp delay
	bool			  tx_adv_auto;
	bool			  tx_adv_negative;
	bool			  is_initialized;
	bool			  radio_is_streaming;
	bool			  continuous_tx;
	double			  freq_offset;
	double			  cur_tx_srate;
	double			  cur_rx_srate;
	double			  fix_srate_hz;//Force Tx and Rx sampling rate in Hz
	uint32_t		  nof_antennas;
	uint32_t		  nof_channels;//logical channel
	uint32_t		  nof_channels_x_dev;
	uint32_t		  nof_carriers;
	
	double cur_tx_freqs[SRSRAN_MAX_CARRIERS]; //std::vector<double>
	double cur_rx_freqs[SRSRAN_MAX_CARRIERS]; //std::vector<double>
	
	uint32_t max_resamp_buf_sz_ms; ///< Maximum buffer size in ms for intermediate resampling
															  ///< buffers
	double tx_max_gap_zeros; ///< Maximum transmission gap to fill with zeros, otherwise the burst
													 ///< shall be stopped
	// Define default values for known radios
	int	uhd_default_tx_adv_samples;
	double uhd_default_tx_adv_offset_sec;
	int	lime_default_tx_adv_samples;
	double lime_default_tx_adv_offset_sec;
	int	blade_default_tx_adv_samples;
	double blade_default_tx_adv_offset_sec;

	int nof_device_args;
	char *device_args_list[MAX_DEVICE_NUM];
}rf_manager_t;


int rf_init(void);
int rf_destory(void);
void set_rx_srate(const double srate);
void set_tx_srate(const double srate);
void release_freq(const uint32_t carrier_idx);
void set_rx_freq(const uint32_t carrier_idx, const double freq);
void set_tx_freq(const uint32_t carrier_idx, const double freq);
bool rx_now(rf_buffer_t *buffer, rf_timestamp_t *rxd_time);
void tx_end();
void tx_end_nolock();
bool tx(rf_buffer_t *buffer, rf_timestamp_t *tx_time);

#ifdef __cplusplus
}
#endif

#endif
