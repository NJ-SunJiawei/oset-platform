/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef TXRX_H_
#define TXRX_H_

#include "oset-core.h"
#include "lib/rf/rf_timestamp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct worker_context_s {
  uint32_t		 sf_idx;	   ///< Subframe index
  void* 		 worker_ptr; ///< Worker pointer for wait/release semaphore
  bool			 last;   ///< Indicates this worker is the last one in the sub-frame processing
  rf_timestamp_t tx_time;	   ///< Transmit time, used only by last worker
}worker_context_t;

void txrx_stop(void);
void txrx_init(void);
void txrx_task_init(void);
void *gnb_txrx_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
