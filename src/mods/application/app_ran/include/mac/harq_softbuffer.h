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

const static uint32_t MAX_HARQ = 16;

typedef struct {
  srsran_softbuffer_tx_t buffer;
}tx_harq_softbuffer;

typedef struct {
  srsran_softbuffer_rx_t buffer;
}rx_harq_softbuffer;

typedef struct {
  uint32_t batch_size;

  tx_harq_softbuffer  *tx_pool[SRSRAN_MAX_PRB_NR];
  uint32_t tx_buffer_idnex[SRSRAN_MAX_PRB_NR];

  rx_harq_softbuffer  *rx_pool[SRSRAN_MAX_PRB_NR];
  uint32_t rx_buffer_idnex[SRSRAN_MAX_PRB_NR];

}harq_softbuffer_pool;

void rx_harq_softbuffer_reset(rx_harq_softbuffer *pool, uint32_t tbs_bits);


void harq_softbuffer_pool_init(harq_softbuffer_pool *harq_pool, uint32_t nof_prb, uint32_t batch_size);
void harq_softbuffer_pool_destory(harq_softbuffer_pool *harq_pool, uint32_t nof_prb, uint32_t batch_size);

tx_harq_softbuffer *harq_softbuffer_pool_get_tx(harq_softbuffer_pool *harq_pool, uint32_t nof_prb);
rx_harq_softbuffer *harq_softbuffer_pool_get_rx(harq_softbuffer_pool *harq_pool, uint32_t nof_prb);

#ifdef __cplusplus
}
#endif

#endif
