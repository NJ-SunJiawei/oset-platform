/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "rf/radio.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "radio"


static rf_manager_t rf_manager = {
	.max_resamp_buf_sz_ms = 5,
	.tx_max_gap_zeros = 4e-3,
	.uhd_default_tx_adv_samples  = 98,
	.uhd_default_tx_adv_offset_sec = 4 * 1e-6,
	.lime_default_tx_adv_samples  = 98,
	.lime_default_tx_adv_offset_sec = 4 * 1e-6,
	.blade_default_tx_adv_samples	= 27,
	.blade_default_tx_adv_offset_sec = 1e-6,
};

rf_manager_t *rf_manager_self(void)
{
    return &rf_manager;
}

static void rf_manager_init(void)
{
	rf_manager.app_pool = gnb_manager_self()->app_pool;

}

static void rf_manager_destory(void)
{
	rf_manager.app_pool = NULL; /*app_pool release by openset process*/
}


static void set_tx_adv(int nsamples)
{
  rf_manager.tx_adv_auto     = false;
  rf_manager.tx_adv_nsamples = nsamples;
  if (!nsamples) {
    rf_manager.tx_adv_sec = 0;
  }
}

static void set_tx_adv_neg(bool tx_adv_is_neg)
{
  rf_manager.tx_adv_negative = tx_adv_is_neg;
}

static bool start_agc(bool tx_gain_same_rx)
{
  if (!rf_manager.is_initialized) {
    return false;
  }
  for (int i = 0; i < rf_manager.nof_device_args; ++i) {
    if (srsran_rf_start_gain_thread(&rf_manager.rf_devices[i], tx_gain_same_rx)) {
      ERROR("Error starting AGC Thread RF device");
      return false;
    }
  }
  return true;
}

static void set_rx_gain(const float& gain)
{
  if (!rf_manager.is_initialized) {
    return;
  }
  for (int i = 0; i < rf_manager.nof_device_args; ++i) {
    srsran_rf_set_rx_gain(&rf_manager.rf_devices[i], gain);
  }
}

static void set_tx_gain(const float& gain)
{
  if (!rf_manager.is_initialized) {
    return;
  }
  for (int i = 0; i < rf_manager.nof_device_args; ++i) {
    srsran_rf_set_tx_gain(&rf_manager.rf_devices[i], gain);
  }
}


bool config_rf_channels(const rf_args_t* args)
{
  channel_cfg_t carriers[2][SRSRAN_MAX_CARRIERS] = {0};

  // Generate RF-Channel to Carrier map
  rf_manager.rx_channel_mapping.available_channels = oset_list2_create();
  rf_manager.tx_channel_mapping.available_channels = oset_list2_create();

  for(uint32_t i = 0; i < args->nof_carriers; i++){
    channel_cfg_t *c_rx = &carriers[0][i];
    c_rx->carrier_idx = i;
    c_rx->used = false;

    // Parse DL band for this channel
    c_rx->band.low_freq = args->ch_rx_bands[i].min;
    c_rx->band.high_freq = args->ch_rx_bands[i].max;

    oset_list2_add(rf_manager.rx_channel_mapping.available_channels, c_rx);
    oset_info("Configuring physical DL channel %d with band-pass filter (%.1f, %.1f)",
                i,
                c_rx->band.low_freq,
                c_rx->band.high_freq;

    // Parse UL band for this channel
	channel_cfg_t *c_tx = &carriers[1][i];
    c_tx->carrier_idx = i;
    c_tx->used = false;
    c_tx->band.low_freq = args->ch_tx_bands[i].min;
    c_tx->band.high_freq = args->ch_tx_bands[i].max;
    oset_list2_add(rf_manager.tx_channel_mapping.available_channels, c_tx);
    oset_info("Configuring physical UL channel %d with band-pass filter (%.1f, %.1f)",
                i,
                c_tx->band.low_freq,
                c_tx->band.high_freq);
  }
  return true;
}

static bool open_file_dev(const uint32_t device_idx, FILE** rx_files, FILE** tx_files, uint32_t nof_channels, uint32_t base_srate)
{
  srsran_rf_t* rf_device = &rf_manager.rf_devices[device_idx];

  oset_info("Opening channels idx %d in RF device abstraction\n", device_idx);

  if (srsran_rf_open_file(rf_device, rx_files, tx_files, nof_channels, base_srate)) {
    oset_error("Error opening RF device abstraction");
    return false;
  }

  // Suppress radio stdout
  srsran_rf_suppress_stdout(rf_device);

  // Register handler for processing O/U/L
  srsran_rf_register_error_handler(rf_device, rf_msg_callback, this);

  // Get device info
  rf_manager.rf_info[device_idx] = *srsran_rf_get_info(rf_device);

  return true;
}


static bool open_dev(const uint32_t device_idx, const char *device_name, const char *devive_args)
{
  srsran_rf_t* rf_device = &rf_manager.rf_devices[device_idx];

  char* dev_args = nullptr;
  if (devive_args != "auto") {
    dev_args = (char*)devive_args;
  }

  char* dev_name = nullptr;
  if (device_name != "auto") {
    dev_name = (char*)device_name;
  }

  oset_info("Opening %d channels in RF device=%s with args=%s\n",
                  rf_manager.nof_channels_x_dev,
                  dev_name ? dev_name : "default",
                  dev_args ? dev_args : "default");

  if (srsran_rf_open_devname(rf_device, dev_name, dev_args, rf_manager.nof_channels_x_dev)) {
    oset_error("Error opening RF device");
    return false;
  }

  // Suppress radio stdout
  srsran_rf_suppress_stdout(rf_device);//do nothings

  // Register handler for processing O/U/L
  srsran_rf_register_error_handler(rf_device, rf_msg_callback, this);

  // Get device info
  rf_manager.rf_info[device_idx] = *srsran_rf_get_info(rf_device);

  return true;
}

void set_rx_freq(const uint32_t carrier_idx, const double freq)
{
  if (!rf_manager.is_initialized) {
    return;
  }

  // Map carrier index to physical channel
  if (allocate_freq(&rf_manager.rx_channel_mapping, carrier_idx, freq)) {
    device_mapping_t device_mapping = get_device_mapping(&rf_manager.rx_channel_mapping, carrier_idx, 0);
    if (device_mapping.channel_idx >= rf_manager.nof_channels_x_dev) {
      oset_error("Invalid mapping physical channel %d to logical carrier %d on f_rx=%.1f MHz (nof_channels_x_dev=%d, device_idx=%d)",
                   device_mapping.channel_idx,
                   carrier_idx,
                   freq / 1e6, rf_manager.nof_channels_x_dev, device_mapping.device_idx);
      return;
    }

    oset_info("Mapping RF channel %d (device=%d, channel=%d) to logical carrier %d on f_rx=%.1f MHz",
                device_mapping.carrier_idx,
                device_mapping.device_idx,
                device_mapping.channel_idx,
                carrier_idx,
                freq / 1e6);
    if (rf_manager.cur_rx_freqs[device_mapping.carrier_idx] != freq) {
      if ((device_mapping.carrier_idx + 1) * rf_manager.nof_antennas <= rf_manager.nof_channels) {
        rf_manager.cur_rx_freqs[device_mapping.carrier_idx] = freq;
        for (uint32_t i = 0; i < rf_manager.nof_antennas; i++) {
          device_mapping_t dm = get_device_mapping(&rf_manager.rx_channel_mapping, carrier_idx, i);
          if (dm.device_idx >= rf_manager.nof_device_args || dm.channel_idx >= rf_manager.nof_channels_x_dev) {
            oset_error("Invalid port mapping %d:%d to logical carrier %d on f_rx=%.1f MHz",
                         dm.device_idx,
                         dm.channel_idx,
                         carrier_idx,
                         freq / 1e6);
            return;
          }

          srsran_rf_set_rx_freq(&rf_manager.rf_devices[dm.device_idx], dm.channel_idx, freq + rf_manager.freq_offset);
        }
      } else {
        oset_error("set_rx_freq: physical_channel_idx=%d for %d antennas exceeds maximum channels (%d)",
                     device_mapping.carrier_idx,
                     rf_manager.nof_antennas,
                     rf_manager.nof_channels);
      }
    } else {
      oset_info("RF Rx channel %d already on freq", device_mapping.carrier_idx);
    }
  } else {
    oset_error("set_rx_freq: Could not allocate frequency %.1f MHz to carrier %d", freq / 1e6, carrier_idx);
  }
}

void set_rx_srate(const double srate)
{
  int i = 0;

  if (!rf_manager.is_initialized) {
	  return;
  }

  // If fix sampling rate...
  if (rf_manager.fix_srate_hz) {
    rf_manager.decimator_busy = true;
	//oset_apr_mutex_lock(rf_manager.rx_mutex);

    // If the sampling rate was not set, set it
    if (!rf_manager.cur_rx_srate) {
      for (i = 0; i < rf_manager.nof_device_args; ++i) {
        rf_manager.cur_rx_srate = srsran_rf_set_rx_srate(&rf_manager.rf_devices[i], rf_manager.fix_srate_hz);
      }
    }

    // Assert ratio is integer
    ASSERT_IF_NOT(((uint32_t)rf_manager.cur_rx_srate % (uint32_t)srate) == 0,
                  "The sampling rate ratio is not integer (%.2f MHz / %.2f MHz = %.3f)",
                  rf_manager.cur_rx_srate / 1e6,
                  srate / 1e6,
                  rf_manager.cur_rx_srate / srate);

    // Update decimators
    uint32_t ratio = (uint32_t)ceil(rf_manager.cur_rx_srate / srate);
    for (uint32_t ch = 0; ch < rf_manager.nof_channels; ch++) {
      srsran_resampler_fft_init(&rf_manager.decimators[ch], SRSRAN_RESAMPLER_MODE_DECIMATE, ratio);
    }

    rf_manager.decimator_busy = false;
	//oset_apr_mutex_unlock(rf_manager.rx_mutex);
  } else {
      for (i = 0; i < rf_manager.nof_device_args; ++i) {
      rf_manager.cur_rx_srate = srsran_rf_set_rx_srate(&rf_manager.rf_devices[i], srate);
    }
  }
}


static double get_dev_cal_tx_adv_sec(const char *device_name)
{
  int nsamples = 0;
  /* Set time advance for each known device if in auto mode */
  if (tx_adv_auto) {
    /* This values have been calibrated using the prach_test_usrp tool in srsRAN */

    if (!strcmp(device_name, "uhd_b200")) {
      double srate_khz = round(rf_manager.cur_tx_srate / 1e3);
      if (srate_khz == 1.92e3) {
        // 6 PRB
        nsamples = 57;
      } else if (srate_khz == 3.84e3) {
        // 15 PRB
        nsamples = 60;
      } else if (srate_khz == 5.76e3) {
        // 25 PRB
        nsamples = 92;
      } else if (srate_khz == 11.52e3) {
        // 50 PRB
        nsamples = 120;
      } else if (srate_khz == 15.36e3) {
        // 75 PRB
        nsamples = 80;
      } else if (srate_khz == 23.04e3) {
        // 100 PRB
        nsamples = 160;
      } else {
        /* Interpolate from known values */
        oset_warn(
            "\nWarning TX/RX time offset for sampling rate %.0f KHz not calibrated. Using interpolated value\n",
            rf_manager.cur_tx_srate);
        nsamples = rf_manager.uhd_default_tx_adv_samples + (int)(rf_manager.cur_tx_srate * rf_manager.uhd_default_tx_adv_offset_sec);
      }

    } else if (!strcmp(device_name, "uhd_usrp2")) {
      double srate_khz = round(rf_manager.cur_tx_srate / 1e3);
      if (srate_khz == 1.92e3) {
        nsamples = 14; // estimated
      } else if (srate_khz == 3.84e3) {
        nsamples = 32;
      } else if (srate_khz == 5.76e3) {
        nsamples = 43;
      } else if (srate_khz == 11.52e3) {
        nsamples = 54;
      } else if (srate_khz == 15.36e3) {
        nsamples = 65; // to calc
      } else if (srate_khz == 23.04e3) {
        nsamples = 80; // to calc
      } else {
        /* Interpolate from known values */
        oset_warn(
            "\nWarning TX/RX time offset for sampling rate %.0f KHz not calibrated. Using interpolated value\n",
            rf_manager.cur_tx_srate);
        nsamples = rf_manager.uhd_default_tx_adv_samples + (int)(rf_manager.cur_tx_srate * rf_manager.uhd_default_tx_adv_offset_sec);
      }

    } else if (!strcmp(device_name, "lime")) {
      double srate_khz = round(rf_manager.cur_tx_srate / 1e3);
      if (srate_khz == 1.92e3) {
        nsamples = 28;
      } else if (srate_khz == 3.84e3) {
        nsamples = 51;
      } else if (srate_khz == 5.76e3) {
        nsamples = 74;
      } else if (srate_khz == 11.52e3) {
        nsamples = 78;
      } else if (srate_khz == 15.36e3) {
        nsamples = 86;
      } else if (srate_khz == 23.04e3) {
        nsamples = 102;
      } else {
        /* Interpolate from known values */
        oset_warn(
            "\nWarning TX/RX time offset for sampling rate %.0f KHz not calibrated. Using interpolated value\n",
            rf_manager.cur_tx_srate);
        nsamples = rf_manager.lime_default_tx_adv_samples + (int)(rf_manager.cur_tx_srate * rf_manager.lime_default_tx_adv_offset_sec);
      }

    } else if (!strcmp(device_name, "uhd_x300")) {
      // In X300 TX/RX offset is independent of sampling rate
      nsamples = 45;
    } else if (!strcmp(device_name, "bladerf")) {
      double srate_khz = round(rf_manager.cur_tx_srate / 1e3);
      if (srate_khz == 1.92e3) {
        nsamples = 16;
      } else if (srate_khz == 3.84e3) {
        nsamples = 18;
      } else if (srate_khz == 5.76e3) {
        nsamples = 16;
      } else if (srate_khz == 11.52e3) {
        nsamples = 21;
      } else if (srate_khz == 15.36e3) {
        nsamples = 14;
      } else if (srate_khz == 23.04e3) {
        nsamples = 21;
      } else {
        /* Interpolate from known values */
        oset_warn(
            "\nWarning TX/RX time offset for sampling rate %.0f KHz not calibrated. Using interpolated value\n",
            rf_manager.cur_tx_srate);
        nsamples = rf_manager.blade_default_tx_adv_samples + (int)(rf_manager.blade_default_tx_adv_offset_sec * rf_manager.cur_tx_srate);
      }
    } else if (!strcmp(device_name, "zmq")) {
      nsamples = 0;
    }
  } else {
    nsamples = rf_manager.tx_adv_nsamples;
    oset_warn("Setting manual TX/RX offset to %d samples\n", nsamples);
  }

  // Calculate TX advance in seconds from samples and sampling rate
  return (double)nsamples / rf_manager.cur_tx_srate;
}


void set_tx_freq(const uint32_t carrier_idx, const double freq)
{
  if (!rf_manager.is_initialized) {
    return;
  }

  // Map carrier index to physical channel
  if (allocate_freq(&rf_manager.tx_channel_mapping, carrier_idx, freq)) {
    device_mapping_t device_mapping = get_device_mapping(&rf_manager.tx_channel_mapping, carrier_idx, 0);
    if (device_mapping.channel_idx >= rf_manager.nof_channels_x_dev) {
      oset_error("Invalid mapping physical channel %d to logical carrier %d on f_tx=%.1f MHz",
                   device_mapping.channel_idx,
                   carrier_idx,
                   freq / 1e6);
      return;
    }

    oset_info("Mapping RF channel %d (device=%d, channel=%d) to logical carrier %d on f_tx=%.1f MHz",
                device_mapping.carrier_idx,
                device_mapping.device_idx,
                device_mapping.channel_idx,
                carrier_idx,
                freq / 1e6);
    if (rf_manager.cur_tx_freqs[device_mapping.carrier_idx] != freq) {
      if ((device_mapping.carrier_idx + 1) * rf_manager.nof_antennas <= rf_manager.nof_channels) {
        rf_manager.cur_tx_freqs[device_mapping.carrier_idx] = freq;
        for (uint32_t i = 0; i < rf_manager.nof_antennas; i++) {
          device_mapping = get_device_mapping(&rf_manager.tx_channel_mapping, carrier_idx, i);
          if (device_mapping.device_idx >= rf_manager.nof_device_args || device_mapping.channel_idx >= rf_manager.nof_channels_x_dev) {
            oset_error("Invalid port mapping %d:%d to logical carrier %d on f_rx=%.1f MHz",
                         device_mapping.device_idx,
                         device_mapping.channel_idx,
                         carrier_idx,
                         freq / 1e6);
            return;
          }

          srsran_rf_set_tx_freq(&rf_manager.rf_devices[device_mapping.device_idx], device_mapping.channel_idx, freq + rf_manager.freq_offset);
        }
      } else {
        oset_error("set_tx_freq: physical_channel_idx=%d for %d antennas exceeds maximum channels (%d)",
                     device_mapping.carrier_idx,
                     rf_manager.nof_antennas,
                     rf_manager.nof_channels);
      }
    } else {
      oset_info("RF Tx channel %d already on freq", device_mapping.carrier_idx);
    }
  } else {
    oset_error("set_tx_freq: Could not allocate frequency %.1f MHz to carrier %d", freq / 1e6, carrier_idx);
  }
}

void set_tx_srate(const double srate)
{
  int i = 0;

  //oset_apr_mutex_lock(rf_manager.tx_mutex);

  if (!rf_manager.is_initialized) {
    return;
  }

  // If fix sampling rate...
  if (rf_manager.fix_srate_hz) {
    // If the sampling rate was not set, set it
    if (!rf_manager.cur_tx_srate)) {
		for (i = 0; i < rf_manager.nof_device_args; ++i) {
            rf_manager.cur_tx_srate = srsran_rf_set_tx_srate(&rf_manager.rf_devices[i], rf_manager.fix_srate_hz);
      }
    }

    // Assert ratio is integer
    srsran_assert(((uint32_t)rf_manager.cur_tx_srate % (uint32_t)srate) == 0,
                  "The sampling rate ratio is not integer (%.2f MHz / %.2f MHz = %.3f)",
                  rf_manager.cur_rx_srate / 1e6,
                  srate / 1e6,
                  rf_manager.cur_rx_srate / srate);
    oset_assert(((uint32_t)rf_manager.cur_rx_srate % (uint32_t)srate) == 0);

    // Update interpolators
    uint32_t ratio = (uint32_t)ceil(rf_manager.cur_tx_srate / srate);
    for (uint32_t ch = 0; ch < rf_manager.nof_channels; ch++) {
      srsran_resampler_fft_init(&rf_manager.interpolators[ch], SRSRAN_RESAMPLER_MODE_INTERPOLATE, ratio);
    }
  } else {
	for (i = 0; i < rf_manager.nof_device_args; ++i) {
      rf_manager.cur_tx_srate = srsran_rf_set_tx_srate(&rf_manager.rf_devices[i], srate);
    }
  }

  // Get calibrated advanced
  rf_manager.tx_adv_sec = get_dev_cal_tx_adv_sec(srsran_rf_name(&rf_manager.rf_devices[0]));

  if (rf_manager.tx_adv_sec < 0) {
    rf_manager.tx_adv_sec *= -1;
    rf_manager.tx_adv_negative = true;
  }
}

void release_freq(const uint32_t carrier_idx)
{
  release_freq_(&rf_manager.rx_channel_mapping, carrier_idx);
  release_freq_(&rf_manager.tx_channel_mapping, carrier_idx);
}

int rf_init(void)
{
	rf_manager_init();

    int i = -1;
	rf_args_t *args = &gnb_manager_self()->args.rf;

	if (args->nof_antennas > SRSRAN_MAX_PORTS) {
	  oset_error("Maximum number of antennas exceeded (%d > %d)", args->nof_antennas, SRSRAN_MAX_PORTS);
	  return OSET_ERROR;
	}
	
	if (args->nof_carriers > SRSRAN_MAX_CARRIERS) {
	  oset_error("Maximum number of carriers exceeded (%d > %d)", args->nof_carriers, SRSRAN_MAX_CARRIERS);
	  return OSET_ERROR;
	}
	
	if (!config_rf_channels(args)) {
	  oset_error("Error configuring RF channels");
	  return OSET_ERROR;
	}

	rf_manager.nof_channels = args->nof_antennas * args->nof_carriers;
	rf_manager.nof_antennas = args->nof_antennas;
	rf_manager.nof_carriers = args->nof_carriers;
	rf_manager.fix_srate_hz = args->srate_hz;

	rf_manager.nof_device_args = oset_split(args->device_args, ';', rf_manager.device_args_list);
	// Add auto if list is empty
    if(0 == rf_manager.nof_device_args){
		rf_manager.nof_device_args = 1;
		rf_manager.device_args_list[0] = "auto";
	}

	// Avoid opening more RF devices than necessary
	if (rf_manager.nof_channels < rf_manager.nof_device_args) {
	   rf_manager.nof_device_args = rf_manager.nof_channels;
	}

	// Makes sure it is possible to have the same number of RF channels in each RF device
	if (rf_manager.nof_channels % rf_manager.nof_device_args != 0) {
		oset_error(
		    "Error: The number of required RF channels (%d) is not divisible between the number of RF devices (%zd).\n",
		    rf_manager.nof_channels,
		    rf_manager.nof_device_args);
	    return OSET_ERROR;
	}
    rf_manager.nof_channels_x_dev = rf_manager.nof_channels / rf_manager.nof_device_args;   

	// Allocate RF devices
	rf_manager.tx_channel_mapping.nof_channels_x_dev = rf_manager.nof_channels_x_dev;
	rf_manager.tx_channel_mapping.nof_antennas = rf_manager.nof_antennas;

	rf_manager.rx_channel_mapping.nof_channels_x_dev = rf_manager.nof_channels_x_dev;
	rf_manager.rx_channel_mapping.nof_antennas = rf_manager.nof_antennas;


	// Init and start Radios
	if (strcmp(args->device_name ,"file") || strcmp(rf_manager.device_args_list[0] ,"auto")) {
	  // regular RF device
	  for (uint32_t device_idx = 0; device_idx < (uint32_t)rf_manager.nof_device_args; device_idx++) {
		if (not open_dev(device_idx, args->device_name, rf_manager.device_args_list[device_idx])) {
		  oset_error("Error opening RF device %d", device_idx);
		  return OSET_ERROR;
		}
	  }
	} else {
	  // file-based RF device abstraction using pre-opened FILE* objects
	  if (args->rx_files == NULL && args->tx_files == NULL) {
		oset_error("File-based RF device abstraction requested, but no files provided");
		return OSET_ERROR;
	  }
	  for (uint32_t device_idx = 0; device_idx < (uint32_t)rf_manager.nof_device_args; device_idx++) {
		if (not open_file_dev(device_idx,
						 &args->rx_files[device_idx * rf_manager.nof_channels_x_dev],
						 &args->tx_files[device_idx * rf_manager.nof_channels_x_dev],
						 rf_manager.nof_channels_x_dev,
						 args->srate_hz)) {
		  oset_error("Error opening RF device %d", device_idx);
		  return OSET_ERROR;
		}
	  }
	}
	
	rf_manager.is_start_of_burst = true;
	rf_manager.is_initialized	 = true;

	// Set RF options
	rf_manager.tx_adv_auto = true;
	if (args->time_adv_nsamples != "auto") {
	  int t = (int)strtol(args->time_adv_nsamples, NULL, 10);
	  set_tx_adv(abs(t));
	  set_tx_adv_neg(t < 0);
	}
	rf_manager.continuous_tx = true;
	if (args->continuous_tx != "auto") {
	  rf_manager.continuous_tx = (args->continuous_tx == "yes");
	}
	
	// Set fixed gain options
	if (args->rx_gain < 0) {
	  start_agc(false);
	} else {
	  set_rx_gain(args->rx_gain);
	}
	// Set gain for all channels
	if (args->tx_gain > 0) {
	  set_tx_gain(args->tx_gain);
	} else {
	  // Set same gain than for RX until power control sets a gain
	  set_tx_gain(args->rx_gain);
	  oset_info("\nWarning: TX gain was not set. Using open-loop power control (not working properly)");
	}
	
	// Set individual gains
	for (uint32_t i = 0; i < args->nof_carriers; i++) {
	  if (args->tx_gain_ch[i] > 0) {
		for (uint32_t j = 0; j < rf_manager.nof_antennas; j++) {
		  uint32_t phys_antenna_idx = i * rf_manager.nof_antennas + j;
	
		  // From channel number deduce RF device index and channel
		  uint32_t rf_device_idx  = phys_antenna_idx / rf_manager.nof_channels_x_dev;
		  uint32_t rf_channel_idx = phys_antenna_idx % rf_manager.nof_channels_x_dev;
	
		  oset_info("Setting individual tx_gain=%.1f on dev=%d ch=%d", args->tx_gain_ch[i], rf_device_idx, rf_channel_idx);
		  if (srsran_rf_set_tx_gain_ch(&rf_manager.rf_devices[rf_device_idx], rf_channel_idx, args->tx_gain_ch[i]) < 0) {
			oset_error("Setting channel tx_gain=%.1f on dev=%d ch=%d", args->tx_gain_ch[i], rf_device_idx, rf_channel_idx);
		  }
		}
	  }
	}
	
	// Set individual gains
	for (uint32_t i = 0; i < args->nof_carriers; i++) {
	  if (args->rx_gain_ch[i] > 0) {
		for (uint32_t j = 0; j < rf_manager.nof_antennas; j++) {
		  uint32_t phys_antenna_idx = i * rf_manager.nof_antennas + j;
	
		  // From channel number deduce RF device index and channel
		  uint32_t rf_device_idx  = phys_antenna_idx / rf_manager.nof_channels_x_dev;
		  uint32_t rf_channel_idx = phys_antenna_idx % rf_manager.nof_channels_x_dev;
	
		  oset_info("Setting individual rx_gain=%.1f on dev=%d ch=%d", args->rx_gain_ch[i], rf_device_idx, rf_channel_idx);
		  if (srsran_rf_set_rx_gain_ch(&rf_manager.rf_devices[rf_device_idx], rf_channel_idx, args->rx_gain_ch[i]) < 0) {
			 oset_error("Setting channel rx_gain=%.1f on dev=%d ch=%d", args->rx_gain_ch[i], rf_device_idx, rf_channel_idx);
		  }
		}
	  }
	}
	
	// It is not expected that any application tries to receive more than max_resamp_buf_sz_ms
	if (rf_manager.fix_srate_hz) {
	  int resamp_buf_sz = (rf_manager.max_resamp_buf_sz_ms * rf_manager.fix_srate_hz) / 1000;
	  for (i = 0; i< SRSRAN_MAX_CHANNELS; ++i) {
		rf_manager.rx_buffer[i] = (cf_t *)oset_malloc(resamp_buf_sz*sizeof(cf_t));
	  }
	  for (i = 0; i< SRSRAN_MAX_CHANNELS; ++i) {
		rf_manager.tx_buffer[i] = (cf_t *)oset_malloc(resamp_buf_sz*sizeof(cf_t));
	  }
	}
	
	// Frequency offset
	rf_manager.freq_offset = args->freq_offset;


	rf_manager.zeros = (cf_t *)oset_malloc(SRSRAN_SF_LEN_MAX*sizeof(cf_t));

	for (i = 0; i < SRSRAN_MAX_CHANNELS; ++i) {
	  rf_manager.dummy_buffers[i] = (cf_t *)oset_malloc(SRSRAN_SF_LEN_MAX * SRSRAN_NOF_SF_X_FRAME*sizeof(cf_t));//rf rx buffer ~ 10*(15*2048)
	}

	oset_apr_mutex_init(&rf_manager.tx_mutex, OSET_MUTEX_NESTED, rf_manager.app_pool);
    oset_apr_mutex_init(&rf_manager.rx_mutex, OSET_MUTEX_NESTED, rf_manager.app_pool);
    oset_apr_mutex_init(&rf_manager.metrics_mutex, OSET_MUTEX_NESTED, rf_manager.app_pool);

	oset_info("radio layer init success");
	return OSET_OK;
}

int rf_destory(void)
{
    int i = -1;
	uint32_t ch = 0;

    //todo
	oset_apr_mutex_destroy(rf_manager.tx_mutex);
	oset_apr_mutex_destroy(rf_manager.rx_mutex);
	oset_apr_mutex_destroy(rf_manager.metrics_mutex);

    for (ch = 0; ch < rf_manager.nof_channels; ch++) {
      srsran_resampler_fft_free(&rf_manager.interpolators[ch]);
      srsran_resampler_fft_free(&rf_manager.decimators[ch]);
    }

	oset_free(rf_manager.zeros);

	for (i = 0; i< SRSRAN_MAX_CHANNELS; ++i) {
	  oset_free(rf_manager.dummy_buffers[i]);
	}

	if (rf_manager.fix_srate_hz) {
	  for (i = 0; i< SRSRAN_MAX_CHANNELS; ++i) {
		oset_free(rf_manager.rx_buffer[i]);
	    oset_free(rf_manager.tx_buffer[i]);
	  }
	}

	oset_list2_free(rf_manager.rx_channel_mapping.available_channels);
	oset_list2_free(rf_manager.tx_channel_mapping.available_channels);

	rf_manager_destory();
	oset_info("radio layer destory success");
    return OSET_OK;
}

static bool map_channels(channel_mapping             *map,
                       uint32_t                device_idx,
                       uint32_t                sample_offset,
                       rf_buffer_t             *buffer,
                       void                    *radio_buffers[SRSRAN_MAX_CHANNELS])
{
  // Conversion from safe C++ std::array to the unsafe C interface. We must ensure that the RF driver implementation
  // accepts up to SRSRAN_MAX_CHANNELS buffers
  for (uint32_t i = 0; i < rf_manager.nof_carriers; i++) {
    // Skip if not allocated
    if (!is_allocated(map, i)) {
      continue;
    }

    // Map each antenna
    for (uint32_t j = 0; j < rf_manager.nof_antennas; j++) {
      device_mapping_t physical_idx = get_device_mapping(map, i, j);

      // Detect mapping out-of-bounds
      if (physical_idx.channel_idx >= rf_manager.nof_channels_x_dev) {
        return false;
      }

      // Set pointer if device index matches
      if (physical_idx.device_idx == device_idx) {
        cf_t* ptr = buffer->sample_buffer[i*rf_manager.nof_antennas + j];

        // Add sample offset only if it is a valid pointer
        if (ptr != nullptr) {
          ptr += sample_offset;
        }

        radio_buffers[physical_idx.channel_idx] = ptr;
      }
    }
  }

  return true;
}


static bool rx_dev(const uint32_t device_idx, rf_buffer_t *buffer, srsran_timestamp_t *rxd_time)
{
  if (!rf_manager.is_initialized) {
    return false;
  }

  time_t* full_secs = rxd_time ? &rxd_time->full_secs : NULL;
  double* frac_secs = rxd_time ? &rxd_time->frac_secs : NULL;

  void* radio_buffers[SRSRAN_MAX_CHANNELS] = {NULL};

  // Discard channels not allocated, need to point to valid buffer
  for (uint32_t i = 0; i < SRSRAN_MAX_CHANNELS; i++) {
    radio_buffers[i] = rf_manager.dummy_buffers[i];//rf dev buf map
  }

  if (!map_channels(&rf_manager.rx_channel_mapping, device_idx, 0, buffer, radio_buffers)) {
    oset_error("Mapping logical channels to physical channels for transmission");
    return false;
  }

  // Apply Rx offset into the number of samples and reset value
  int      nof_samples_offset = rf_manager.rx_offset_n[device_idx];
  uint32_t nof_samples        = buffer->nof_samples;

  // Number of samples adjust from device time offset
  if (nof_samples_offset < 0 && (uint32_t)(-nof_samples_offset) > nof_samples) {
    // Avoid overflow subtraction
    nof_samples = 0;
  } else {
    // Limit the number of samples to a maximum of 2 times the requested number of samples
    nof_samples = SRSRAN_MIN(nof_samples + nof_samples_offset, 2 * nof_samples);
  }

  // Subtract number of offset samples
  rf_manager.rx_offset_n[device_idx] = nof_samples_offset - ((int)nof_samples - (int)buffer->nof_samples);

  int ret =
      srsran_rf_recv_with_time_multi(&rf_manager.rf_devices[device_idx], radio_buffers, nof_samples, true, full_secs, frac_secs);

  // If the number of received samples filled the buffer, there is nothing else to do
  if (buffer->nof_samples <= nof_samples) {
    return ret > 0;
  }

  // Otherwise, set rest of buffer to zero
  uint32_t nof_zeros = buffer->nof_samples - nof_samples;
  for (uint32_t ch = 0; ch < SRSRAN_MAX_CHANNELS; ch++) {
    if (radio_buffers[ch] != NULL) {
      cf_t* ptr = (cf_t*)radio_buffers[ch];
      srsran_vec_cf_zero(&ptr[nof_samples], nof_zeros);
    }
  }

  return ret > 0;
}

bool rx_now(rf_buffer_t *buffer, rf_timestamp_t *rxd_time)
{
  //std::unique_lock<std::mutex> lock(rx_mutex);
  int   i = -1;
  bool                         ret = false;
  rf_buffer_t                  buffer_rx = {0};
  int resamp_buf_sz = (rf_manager.max_resamp_buf_sz_ms * rf_manager.fix_srate_hz) / 1000;

  // Extract decimation ratio. As the decimation may take some time to set a new ratio, deactivate the decimation and
  // keep receiving samples to avoid stalling the RX stream
  uint32_t ratio = 1; // No decimation by default
  if (rf_manager.decimator_busy) {
    //oset_apr_mutex_unlock(rf_manager.rx_mutex);//???
  } else if (rf_manager.decimators[0].ratio > 1) {
    ratio = rf_manager.decimators[0].ratio;
  }

  // Calculate number of samples, considering the decimation ratio
  uint32_t nof_samples = buffer->nof_samples * ratio;

  // Check decimation buffer protection
  if (ratio > 1 && nof_samples > resamp_buf_sz) {
    // This is a corner case that could happen during sample rate change transitions, as it does not have a negative
    // impact, log it as info.
    oset_info("Rx number of samples ({}/{}) exceeds buffer size ({})",
                   buffer->nof_samples,
                   buffer->nof_samples * ratio,
                   resamp_buf_sz);
    // Limit number of samples to receive
    nof_samples = resamp_buf_sz;
  }

  // Set new buffer size
  buffer_rx.nof_samples= nof_samples;

  // If the interpolator have been set, interpolate
  for (uint32_t ch = 0; ch < rf_manager.nof_channels; ch++) {
    // Use rx buffer if decimator is required
	buffer_rx.sample_buffer[ch] = (ratio > 1 ? rf_manager.rx_buffer[ch] : buffer->sample_buffer[ch]);//?  :slot buffer
  }

  if (!rf_manager.radio_is_streaming) {
	for (i = 0; i < rf_manager.nof_device_args; ++i) {
      srsran_rf_start_rx_stream(&rf_manager.rf_devices[i], false);
    }
    rf_manager.radio_is_streaming = true;

    // Flush buffers to compensate settling time
    if (rf_manager.nof_device_args > 1) {
	  for (i = 0; i < rf_manager.nof_device_args; ++i) {
        srsran_rf_flush_buffer(&rf_manager.rf_devices[i]);
      }
    }
  }

  for (uint32_t device_idx = 0; device_idx < (uint32_t)rf_manager.nof_device_args; device_idx++) {
    ret = rx_dev(device_idx, &buffer_rx, &rxd_time->timestamps[device_idx]); //dev access
  }

  // Perform decimation
  if (ratio > 1) {
    for (uint32_t ch = 0; ch < rf_manager.nof_channels; ch++) {
      if (buffer->sample_buffer[ch] && buffer_rx.sample_buffer[ch]) {//buffer out
        srsran_resampler_fft_run(&rf_manager.decimators[ch], buffer_rx.sample_buffer[ch], buffer->sample_buffer[ch], buffer_rx.nof_samples;
      }
    }
  }

  return ret;
}

void tx_end()
{

  //std::unique_lock<std::mutex> lock(tx_mutex);
  oset_apr_mutex_lock(rf_manager.tx_mutex)
  tx_end_nolock();
  oset_apr_mutex_unlock(rf_manager.tx_mutex)
}

void tx_end_nolock()
{
  if (!rf_manager.is_initialized) {
    return;
  }
  if (!rf_manager.is_start_of_burst) {
    for (uint32_t i = 0; i < (uint32_t)rf_manager.nof_device_args; i++) {
      srsran_rf_send_timed2(
          &rf_manager.rf_devices[i], rf_manager.zeros, 0, rf_manager.end_of_burst_time[i].full_secs, rf_manager.end_of_burst_time[i].frac_secs, false, true);
    }
    rf_manager.is_start_of_burst = true;
  }
}

static bool tx_dev(const uint32_t device_idx, rf_buffer_t *buffer, const srsran_timestamp_t tx_time_)
{
  uint32_t     nof_samples   = buffer->nof_samples;
  uint32_t     sample_offset = 0;
  srsran_rf_t* rf_device     = &rf_manager.rf_devices[device_idx];

  // Return instantly if the radio module is not initialised
  if (!rf_manager.is_initialized) {
    return false;
  }

  // Copy timestamp and add Tx time offset calibration
  srsran_timestamp_t tx_time = tx_time_;
  if (!rf_manager.tx_adv_negative) {
    srsran_timestamp_sub(&tx_time, 0, rf_manager.tx_adv_sec);
  } else {
    srsran_timestamp_add(&tx_time, 0, rf_manager.tx_adv_sec);
  }

  // Calculates transmission time overlap with previous transmission
  srsran_timestamp_t ts_overlap = rf_manager.end_of_burst_time[device_idx];
  srsran_timestamp_sub(&ts_overlap, tx_time.full_secs, tx_time.frac_secs);

  // Calculates number of overlap samples with previous transmission
  int32_t past_nsamples = (int32_t)round(rf_manager.cur_tx_srate * srsran_timestamp_real(&ts_overlap));

  // if past_nsamples is positive, the current transmission overlaps with the previous transmission. If it is negative
  // there is a gap between the previous transmission and the current transmission.
  if (past_nsamples > 0) {
    // If the overlap length is greater than the current transmission length, it means the whole transmission is in
    // the past and it shall be ignored
    if ((int32_t)nof_samples < past_nsamples) {
      return true;
    }

    // Trim the first past_nsamples
    sample_offset = (uint32_t)past_nsamples;       // Sets an offset for moving first samples offset
    tx_time       = rf_manager.end_of_burst_time[device_idx]; // Keeps same transmission time
    nof_samples   = nof_samples - past_nsamples;   // Subtracts the number of trimmed samples

    // Prints discarded samples
    oset_debug("Detected RF overlap of %.1f us. Discarding %d samples.",
                 srsran_timestamp_real(&ts_overlap) * 1.0e6,
                 past_nsamples);

  } else if (past_nsamples < 0 && !rf_manager.is_start_of_burst) {
    // if the gap is bigger than TX_MAX_GAP_ZEROS, stop burst
    if (fabs(srsran_timestamp_real(&ts_overlap)) > rf_manager.tx_max_gap_zeros) {
      oset_info("Detected RF gap of %.1f us. Sending end-of-burst.", srsran_timestamp_real(&ts_overlap) * 1.0e6);
      tx_end_nolock();
    } else {
      oset_debug("Detected RF gap of %.1f us. Tx'ing zeroes.", srsran_timestamp_real(&ts_overlap) * 1.0e6);
      // Otherwise, transmit zeros
      uint32_t gap_nsamples = abs(past_nsamples);
      while (gap_nsamples > 0) {
        // Transmission cannot exceed SRSRAN_SF_LEN_MAX (zeros buffer size limitation)
        uint32_t nzeros = SRSRAN_MIN(gap_nsamples, SRSRAN_SF_LEN_MAX);

        // Zeros transmission
        int ret = srsran_rf_send_timed2(rf_device,
                                        rf_manager.zeros,
                                        nzeros,
                                        rf_manager.end_of_burst_time[device_idx].full_secs,
                                        rf_manager.end_of_burst_time[device_idx].frac_secs,
                                        false,
                                        false);
        if (ret < SRSRAN_SUCCESS) {
          return false;
        }

        // Substract gap samples
        gap_nsamples -= nzeros;

        // Increase timestamp
        srsran_timestamp_add(&rf_manager.end_of_burst_time[device_idx], 0, (double)nzeros / cur_tx_srate);
      }
    }
  }

  // Save possible end of burst time
  srsran_timestamp_copy(&rf_manager.end_of_burst_time[device_idx], &tx_time);
  srsran_timestamp_add(&rf_manager.end_of_burst_time[device_idx], 0, (double)nof_samples / cur_tx_srate);

  void* radio_buffers[SRSRAN_MAX_CHANNELS] = {};

  // Discard channels not allocated, need to point to valid buffer
  for (uint32_t i = 0; i < SRSRAN_MAX_CHANNELS; i++) {
    radio_buffers[i] = rf_manager.zeros;
  }

  if (!map_channels(&rf_manager.tx_channel_mapping, device_idx, sample_offset, buffer, radio_buffers)) {
    oset_error("Mapping logical channels to physical channels for transmission");
    return false;
  }

  int ret = srsran_rf_send_timed_multi(
      rf_device, radio_buffers, nof_samples, tx_time.full_secs, tx_time.frac_secs, true, rf_manager.is_start_of_burst, false);

  return ret > SRSRAN_SUCCESS;
}


bool tx(rf_buffer_t *buffer, rf_timestamp_t *tx_time)
{
  bool                         ret = false;
  //std::unique_lock<std::mutex> lock(tx_mutex);
  uint32_t                     ratio = rf_manager.interpolators[0].ratio;
  int resamp_buf_sz = (rf_manager.max_resamp_buf_sz_ms * rf_manager.fix_srate_hz) / 1000;

  // Get number of samples at the low rate
  uint32_t nof_samples = buffer->nof_samples;

  // Check that number of the interpolated samples does not exceed the buffer size
  if (ratio > 1 && nof_samples * ratio > resamp_buf_sz) {
    // This is a corner case that could happen during sample rate change transitions, as it does not have a negative
    // impact, log it as info.
    oset_info("Tx number of samples ({}/{}) exceeds buffer size ({})\n",
                   buffer->nof_samples,
                   buffer->nof_samples * ratio,
                   resamp_buf_sz);

    // Limit number of samples to transmit
    nof_samples = resamp_buf_sz / ratio;
  }

  // If the interpolator have been set, interpolate
  if (rf_manager.interpolators[0].ratio > 1) {
    for (uint32_t ch = 0; ch < rf_manager.nof_channels; ch++) {
      // Perform actual interpolation
      srsran_resampler_fft_run(&rf_manager.interpolators[ch], buffer->sample_buffer[ch], rf_manager.tx_buffer[ch], nof_samples);

      // Set the buffer pointer
	  buffer->sample_buffer[ch] = rf_manager.tx_buffer[ch];
    }

    // Set buffer size after applying the interpolation
    buffer->nof_samples = nof_samples * ratio;
  }

  for (uint32_t device_idx = 0; device_idx < (uint32_t)rf_manager.nof_device_args; device_idx++) {
    ret = tx_dev(device_idx, buffer, device_idx >= SRSRAN_MAX_CHANNELS ? tx_time->default_ts :tx_time->timestamps[device_idx]);
  }

  rf_manager.is_start_of_burst = false;

  return ret;
}


