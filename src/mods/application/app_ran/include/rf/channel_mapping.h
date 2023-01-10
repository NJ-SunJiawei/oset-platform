/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef CHANNEL_MAPPING_H_
#define CHANNEL_MAPPING_H_

#include "oset-core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float low_freq;
    float high_freq;
} band_cfg;

typedef struct {
  bool     used;
  band_cfg band;
  uint32_t carrier_idx;
} channel_cfg_t;

typedef struct {
  uint32_t carrier_idx; // Physical channel index of all channels
  uint32_t device_idx;	// RF Device index
  uint32_t channel_idx; // Channel index in the RF Device
} device_mapping_t;

typedef struct {
	oset_list2_t		         *available_channels;//channel_cfg_t
	channel_cfg_t                *allocated_channels[SRSRAN_MAX_CARRIERS];//std::map<uint32_t, channel_cfg_t>
	//oset_apr_mutex_t	         *mutex;
	uint32_t				     nof_antennas;//1
	uint32_t				     nof_channels_x_dev;//1
} channel_mapping;

bool allocate_freq(channel_mapping *channel_mapping, const uint32_t logical_ch, const float freq);
void release_freq_(channel_mapping *channel_mapping, const uint32_t logical_ch);
device_mapping_t get_device_mapping(channel_mapping *channel_mapping, const uint32_t logical_ch, const uint32_t antenna_idx);
bool is_allocated(channel_mapping *channel_mapping, const uint32_t logical_ch);

#ifdef __cplusplus
}
#endif

#endif
