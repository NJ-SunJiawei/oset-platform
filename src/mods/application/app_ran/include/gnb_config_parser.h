/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#ifndef GNB_CONFIG_PARSER_H_
#define GNB_CONFIG_PARSER_H_

#include "oset-core.h"
#include "lib/srsran/srsran.h"

#ifdef __cplusplus
extern "C" {
#endif

/*************args struct******************/
typedef struct enb_args_s{
  uint32_t enb_id;
  uint32_t dl_earfcn; // By default the EARFCN from rr.conf's cell list are used but this value can be used for single
					  // cell eNB
  uint32_t n_prb;
  uint32_t nof_ports;  //Number of Tx ports (1 port default, set to 2 for TM2/3/4)
  uint32_t transmission_mode;
  float    p_a;  //Power allocation rho_a (-6, -4.77, -3, -1.77, 0, 1, 2, 3)"
}enb_args_t;

typedef struct enb_files_s{
  char* sib_config;
  char* rr_config;
  char* rb_config;
}enb_files_t;

typedef struct general_args_s {
  uint32_t	  rrc_inactivity_timer;
  float 	  metrics_period_secs;
  bool		  metrics_csv_enable;
  char* 	  metrics_csv_filename;
  bool		  report_json_enable;
  char* 	  report_json_filename;
  bool		  report_json_asn1_oct;
  //bool		alarms_log_enable;
  //char*		alarms_filename;
  //bool		print_buffer_state;
  bool		  tracing_enable;
  char* 	  tracing_buffcapacity;
  char* 	  tracing_filename;
  char* 	  eia_pref_list;
  char* 	  eea_pref_list;
  uint32_t	  max_mac_dl_kos;
  uint32_t	  max_mac_ul_kos;
  uint32_t	  gtpu_indirect_tunnel_timeout;
  uint32_t	  rlf_release_timer_ms;
}general_args_t;

// RF/radio args
typedef struct rf_args_band_s{
  float min;
  float max;
}rf_args_band_t;

typedef struct rf_args_s{
  char* 	  type;
  double	  srate_hz;//Force Tx and Rx sampling rate in Hz
  float 	  dl_freq;
  float 	  ul_freq;
  float 	  freq_offset;
  float 	  rx_gain;
  float 	  rx_gain_ch[SRSRAN_MAX_CARRIERS];
  float 	  tx_gain;
  float 	  tx_gain_ch[SRSRAN_MAX_CARRIERS];
  float 	  tx_max_power;
  float 	  tx_gain_offset;
  float 	  rx_gain_offset;
  uint32_t	  nof_carriers; // Number of RF channels //cell num
  uint32_t	  nof_antennas; // Number of antennas per RF channel
  char* 	  device_name;
  char* 	  device_args;
  char* 	  time_adv_nsamples;
  char* 	  continuous_tx;

  rf_args_band_t ch_rx_bands[SRSRAN_MAX_CARRIERS];
  rf_args_band_t ch_tx_bands[SRSRAN_MAX_CARRIERS];

  FILE** rx_files;	// Array of pre-opened FILE* for rx instead of a real device
  FILE** tx_files;	// Array of pre-opened FILE* for tx instead of a real device
}rf_args_t;

// phy args
typedef struct channel_args_s{
  // General
  bool enable;

  // AWGN options
  bool	awgn_enable;
  float awgn_signal_power_dBfs;
  float awgn_snr_dB;

  // Fading options
  bool		  fading_enable;
  char* 	  fading_model;

  // High Speed Train options
  bool	hst_enable;
  float hst_fd_hz;
  float hst_period_s;
  float hst_init_time_s;

  // Delay options
  bool	delay_enable;
  float delay_min_us;
  float delay_max_us;
  float delay_period_s;
  float delay_init_time_s;

  // RLF options
  bool	   rlf_enable;
  uint32_t rlf_t_on_ms;
  uint32_t rlf_t_off_ms;
}channel_args_t;

typedef struct cfr_args_s{
  bool				enable;
  srsran_cfr_mode_t mode;
  float 			manual_thres;
  float 			strength;
  float 			auto_target_papr;
  float 			ema_alpha;
}cfr_args_t;

typedef struct phy_args_s{
  char* 				  type;
  float 				  rx_gain_offset;
  float 				  max_prach_offset_us;
  //uint32_t				  pusch_max_its;
  uint32_t				  nr_pusch_max_its;
  bool					  pusch_8bit_decoder;
  float 				  tx_amplitude;
  uint32_t				  nof_phy_threads;
  char* 				  equalizer_mode;
  float 				  estimator_fil_w;
  bool					  pusch_meas_epre;
  bool					  pusch_meas_evm;
  bool					  pusch_meas_ta;
  bool					  pucch_meas_ta;
  uint32_t				  nof_prach_threads;
  bool					  extended_cp;
  channel_args_t		  dl_channel_args;
  channel_args_t		  ul_channel_args;
  cfr_args_t			  cfr_args;
}phy_args_t;

//stack args
typedef struct pcap_args_s{
  bool		  enable;
  char* 	  filename;
} pcap_args_t;

typedef struct sched_args_s{
  bool		  pdsch_enabled;
  bool		  pusch_enabled;
  bool		  auto_refill_buffer;
  int		  fixed_dl_mcs;
  int		  fixed_ul_mcs;
}sched_args_t;

typedef struct mac_nr_args_s {
  uint32_t			    nof_prb; ///< Needed to dimension MAC softbuffers for all cells
  int					fixed_dl_mcs;
  int					fixed_ul_mcs;
  sched_args_t			sched_cfg;
  pcap_args_t			pcap;
}mac_nr_args_t;

//ngap args
typedef struct ngap_args_s{
  uint32_t	  gnb_id; // 20-bit id (lsb bits)
  uint8_t	  cell_id; // 8-bit cell id
  uint16_t	  tac; // 16-bit tac
  uint16_t	  mcc; // BCD-coded with 0xF filler
  uint16_t	  mnc; // BCD-coded with 0xF filler
  char* 	  amf_addr;
  char* 	  gtp_bind_addr;
  char* 	  ngc_bind_addr;
  char* 	  gnb_name;
}ngap_args_t;

typedef struct gnb_stack_args_s{
  mac_nr_args_t    mac_nr;
  ngap_args_t	   ngap;
  pcap_args_t	   ngap_pcap;
}gnb_stack_args_t;

typedef struct all_args_s{
  enb_args_t		enb;
  enb_files_t		enb_files;
  general_args_t	general;
  rf_args_t 		rf;
  phy_args_t		phy;
  gnb_stack_args_t	nr_stack;
}all_args_t;

int parse_cfg_files(all_args_t* args_, rrc_cfg_t* rrc_cfg_, rrc_nr_cfg_t* rrc_nr_cfg_, phy_cfg_t* phy_cfg_);
void gnb_arg_default(all_args_t      *args);	
void gnb_arg_second(all_args_t      *args);

#ifdef __cplusplus
}
#endif
	
#endif

