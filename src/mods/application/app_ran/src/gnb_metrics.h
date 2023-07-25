/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef GNB_METRICS_H_
#define GNB_METRICS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/rrc/rrc_metrics.h"
#include "lib/pdcp/pdcp_metrics.h"
#include "lib/rlc/rlc_metrics.h"
#include "lib/mac/mac_metrics.h"
#include "lib/rf/rf_metrics.h"
#include "lib/phy/phy_metrics.h"

typedef struct {
  rrc_metrics_t  rrc;
  pdcp_metrics_t pdcp;  
  rlc_metrics_t  rlc;
  mac_metrics_t  mac;
}stack_metrics_t;

////////////////////////////////////////////
typedef struct {
  rf_metrics_t                    rf;
  cvector_vector_t(phy_metrics_t) phy;
  stack_metrics_t                 nr_stack;
}gnb_metrics_t;

#ifdef __cplusplus
}
#endif

#endif
