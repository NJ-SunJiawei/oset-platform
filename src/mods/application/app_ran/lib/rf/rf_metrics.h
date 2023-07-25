/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef RF_METRICS_H_
#define RF_METRICS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rf_metrics_s{
  uint32_t rf_o;
  uint32_t rf_u;
  uint32_t rf_l;
  bool     rf_error;
}rf_metrics_t;

extern uint32_t num_overflows;
extern uint32_t num_failures;

#ifdef __cplusplus
}
#endif

#endif
