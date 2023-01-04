/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "channel_mapping.h"
#include "radio.h"

static bool contains(band_cfg *band, float freq)
{
  if (band->low_freq == 0 && band->high_freq == 0) {
	return true;
  } else {
	return freq >= band->low_freq && freq <= band->high_freq;
  }
}

void release_freq_(channel_mapping *channel_mapping, const uint32_t logical_ch)
{
  if(channel_mapping->allocated_channels[logical_ch] && (true == channel_mapping->allocated_channels[logical_ch]->used)){
	  channel_cfg_t * c = channel_mapping->allocated_channels[logical_ch];
	  c->used = false;
	  oset_list2_add(channel_mapping->available_channels, c);
	  channel_mapping->allocated_channels[logical_ch] = NULL;
  }
}


bool allocate_freq(channel_mapping *channel_mapping, const uint32_t logical_ch, const float freq)
{
  oset_lnode2_t* lnode = NULL;
  channel_cfg_t * c = NULL;

  //std::lock_guard<std::mutex> lock(mutex);

  // Check if the logical channel has already been allocated
  if (channel_mapping->allocated_channels[logical_ch] && (true == channel_mapping->allocated_channels[logical_ch]->used)) {
    // If the current channel contains the frequency, do nothing else
    if (contains(&channel_mapping->allocated_channels[logical_ch]->band, freq)) {
      return true;
    }

    // Otherwise, release logical channel before searching for a new available channel
    release_freq_(logical_ch);
  }

  // Find first available channel that supports this frequency and allocated it
  oset_list2_for_each(channel_mapping->available_channels, lnode){
	  c = (channel_cfg_t *)lnode->data;
	  oset_assert(c);
	  if (contains(&c->band, freq)) {
	  	c->used = true;
		channel_mapping->allocated_channels[logical_ch] = c;
		oset_list2_remove(channel_mapping->available_channels, lnode);
		return true;
	  }
  }
  oset_error("allocate_freq: No channels available for frequency=%.1f", freq);
  return false;
}

device_mapping_t get_device_mapping(channel_mapping *channel_mapping, const uint32_t logical_ch, const uint32_t antenna_idx)
{
  //std::lock_guard<std::mutex> lock(mutex);
  if (channel_mapping->allocated_channels[logical_ch] && (true == channel_mapping->allocated_channels[logical_ch]->used)) {
    uint32_t carrier_idx = channel_mapping->allocated_channels[logical_ch].carrier_idx;
    uint32_t channel_idx = carrier_idx * rf_manager_self()->nof_antennas + antenna_idx;
    return {carrier_idx, channel_idx / rf_manager_self()->nof_channels_x_dev, channel_idx % rf_manager_self()->nof_channels_x_dev};
  }
  return {UINT32_MAX, UINT32_MAX, UINT32_MAX};
}

