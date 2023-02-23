/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_INTERFACE_TYPES_H_
#define RRC_INTERFACE_TYPES_H_

#include "lib/srsran/srsran.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  srsran_softbuffer_tx_t buffer;
}tx_harq_softbuffer;

typedef struct {
  srsran_softbuffer_rx_t buffer;
}rx_harq_softbuffer;

const static uint32_t MAX_HARQ = 16;

typedef struct {
  tx_harq_softbuffer  *tx_pool[SRSRAN_MAX_PRB_NR];
  rx_harq_softbuffer  *rx_pool[SRSRAN_MAX_PRB_NR];
}harq_softbuffer_pool;

#ifdef __cplusplus
}
#endif

#endif
