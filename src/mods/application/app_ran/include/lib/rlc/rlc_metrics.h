/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef RLC_METRICS_H_
#define RLC_METRICS_H_

#include "oset-core.h"
#include "lib/srsran/common/common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // SDU metrics
  uint32_t num_tx_sdus;
  uint32_t num_rx_sdus;
  uint64_t num_tx_sdu_bytes;
  uint64_t num_rx_sdu_bytes;
  uint32_t num_lost_sdus; //< Count dropped SDUs at Tx due to bearer inactivity or empty buffer
  uint64_t rx_latency_ms; //< Average time in ms from first RLC segment to full SDU

  // PDU metrics
  uint32_t num_tx_pdus;
  uint32_t num_rx_pdus;
  uint64_t num_tx_pdu_bytes;
  uint64_t num_rx_pdu_bytes;
  uint32_t num_lost_pdus; //< Lost PDUs registered at Rx

  // misc metrics
  uint32_t rx_buffered_bytes; //< sum of payload of PDUs buffered in rx_window
} rlc_bearer_metrics_t;

typedef struct {
  rlc_bearer_metrics_t bearer[SRSRAN_N_RADIO_BEARERS];
  rlc_bearer_metrics_t mrb_bearer[SRSRAN_N_MCH_LCIDS];
} rlc_ue_metrics_t;

typedef struct {
  cvector_vector_t(rlc_ue_metrics_t) ues;
}rlc_metrics_t;

#ifdef __cplusplus
}
#endif

#endif
