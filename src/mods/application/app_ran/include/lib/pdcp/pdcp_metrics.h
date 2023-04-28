/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef PDCP_METRICS_H_
#define PDCP_METRICS_H_

#include "oset-core.h"
#include "lib/srsran/common/common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // PDU metrics
  uint32_t num_tx_pdus;
  uint32_t num_rx_pdus;
  uint64_t num_tx_pdu_bytes;
  uint64_t num_rx_pdu_bytes;

  // ACK specific metrics (requires RLC AM)
  uint64_t num_tx_acked_bytes;         //< Cumulative number of bytes that the PDCP knows to be acknowledged
  uint64_t tx_notification_latency_ms; //< Average time in ms from PDU delivery to RLC to ACK notification from RLC
  uint32_t num_tx_buffered_pdus;       //< Number of PDUs waiting for ACK
  uint32_t num_tx_buffered_pdus_bytes; //< Number of bytes of PDUs waiting for ACK
} pdcp_bearer_metrics_t;

typedef struct {
  pdcp_bearer_metrics_t bearer[SRSRAN_N_RADIO_BEARERS];
} pdcp_ue_metrics_t;

typedef struct {
  cvector_vector_t(pdcp_ue_metrics_t) ues;
}pdcp_metrics_t;

#ifdef __cplusplus
}
#endif

#endif
