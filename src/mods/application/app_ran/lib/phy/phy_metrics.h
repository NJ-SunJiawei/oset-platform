/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#ifndef PHY_METRICS_H_
#define PHY_METRICS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// PHY metrics per user
typedef struct ul_metrics_s {
  float   n;
  float   pusch_sinr;
  float   pusch_rssi;
  int64_t pusch_tpc;
  float   pucch_sinr;
  float   pucch_rssi;
  float   pucch_ni;
  float   turbo_iters;
  float   mcs;
  int     n_samples;
  int     n_samples_pucch;
}ul_metrics_t;

typedef struct dl_metrics_s {
  float mcs;
  int64_t pucch_tpc;
  int     n_samples;
}dl_metrics_t;

typedef struct phy_metrics_s {
  dl_metrics_t dl;
  ul_metrics_t ul;
}phy_metrics_t;

#ifdef __cplusplus
}
#endif

#endif
