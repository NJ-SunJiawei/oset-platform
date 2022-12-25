/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef RADIO_H_
#define RADIO_H_

#include "channel_mapping.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "radio"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t rf_o;
  uint32_t rf_u;
  uint32_t rf_l;
  bool     rf_error;
}rf_metrics_t;


int radio_init(void);
int radio_destory(void);

#ifdef __cplusplus
}
#endif

#endif
