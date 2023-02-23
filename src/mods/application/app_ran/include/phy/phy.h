/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef PHY_H_
#define PHY_H_

#include "gnb_config_parser.h"
#include "phy/txrx.h"
#include "phy/phy_nr_config.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t				 nof_phy_threads;
  uint32_t				 nof_prach_workers;
}phy_work_args_t;

typedef struct {
  uint32_t					  cell_index;
  uint32_t					  nof_max_prb;
  uint32_t					  nof_tx_ports;//1
  uint32_t					  nof_rx_ports;//1
  uint32_t					  rf_port;
  srsran_subcarrier_spacing_t scs;
  uint32_t					  pusch_max_its;
  float 					  pusch_min_snr_dB;
  double					  srate_hz;
}slot_worker_args_t;

typedef struct slot_worker_s{
	oset_thread_pool_t   *th_pools;
	uint32_t			sf_len;
	uint32_t			cell_index;
	uint32_t			rf_port;
	srsran_slot_cfg_t	dl_slot_cfg;
	srsran_slot_cfg_t	ul_slot_cfg;
	worker_context_t    *context;
	srsran_pdcch_cfg_nr_t pdcch_cfg;
	srsran_gnb_dl_t 	  gnb_dl;
	srsran_gnb_ul_t 	  gnb_ul;
	cf_t                  **tx_buffer; ///< Baseband transmit buffers ~1 subframe len
	cf_t                  **rx_buffer; ///< Baseband receive buffers ~1 subframe len
}slot_worker_t;

typedef struct {
	//oset_thread_mutex_t	 *grant_mutex;
	oset_list2_t            *cell_list_nr;//std::vector<phy_cell_cfg_nr_t>
	//oset_apr_mutex_t	     *cell_gain_mutex;
	srsran_cfr_cfg_t        cfr_config;
	srsran_refsignal_dmrs_pusch_cfg_t dmrs_pusch_cfg;

	phy_args_t              params;
}phy_common;

typedef struct phy_manager_s{
	oset_apr_memory_pool_t *app_pool;

	//txrx				   tx_rx;
	//srsran_prach_cfg_t   prach_cfg;
	common_cfg_t	       common_cfg;  //from rrc layer cofig
	phy_common			   workers_common;
	phy_work_args_t        worker_args;

	slot_worker_args_t     slot_args;
	slot_worker_t          slot_worker;

	oset_apr_mutex_t	   *mutex;
	oset_apr_thread_cond_t *cond;
	uint32_t               tti;//txrx
}phy_manager_t;


int phy_init(void);
int phy_destory(void);

uint32_t get_buffer_len();
cf_t* get_buffer_rx(uint32_t antenna_idx);
cf_t* get_buffer_tx(uint32_t antenna_idx);


#ifdef __cplusplus
}
#endif

#endif
