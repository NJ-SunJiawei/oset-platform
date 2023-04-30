/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef SLOT_WORKER_H_
#define SLOT_WORKER_H_

#include "oset-core.h"
#include "lib/rf/rf_timestamp.h"
#include "lib/srsran/srsran.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct worker_context_s {
  uint32_t		 sf_idx;	   ///< Subframe index
  void* 		 worker_ptr; ///< Worker pointer for wait/release semaphore
  bool			 last;   ///< Indicates this worker is the last one in the sub-frame processing
  rf_timestamp_t tx_time;	   ///< Transmit time, used only by last worker
}worker_context_t;


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
	uint32_t			  sf_len;
	uint32_t			  cell_index;
	uint32_t			  rf_port;
	srsran_slot_cfg_t	  dl_slot_cfg;
	srsran_slot_cfg_t	  ul_slot_cfg;
	worker_context_t      context;
	srsran_pdcch_cfg_nr_t pdcch_cfg;
	srsran_gnb_dl_t 	  gnb_dl;
	srsran_gnb_ul_t 	  gnb_ul;
	cf_t                  **tx_buffer; ///< Baseband transmit buffers ~1 subframe len //[args->nof_tx_ports]
	cf_t                  **rx_buffer; ///< Baseband receive buffers ~1 subframe len //[args->nof_rx_ports]
}slot_worker_t;

typedef struct slot_manager_s{
	//uint32_t               tti;//txrx
	slot_worker_t          slot_worker;//phy deal tti slot
}slot_manager_t;

slot_worker_t* slot_worker_alloc(void);
void slot_worker_free(slot_worker_t *slot_w);

void slot_worker_init(slot_worker_args_t *args);
void slot_worker_destory(void);

uint32_t get_buffer_len(slot_worker_t *slot_w);
cf_t* get_buffer_rx(slot_worker_t *slot_w, uint32_t antenna_idx);
cf_t* get_buffer_tx(slot_worker_t *slot_w, uint32_t antenna_idx);

void* slot_worker_process(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
