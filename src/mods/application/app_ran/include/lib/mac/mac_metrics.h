/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef MAC_METRICS_H_
#define MAC_METRICS_H_

#include "oset-core.h"
//#include "lib/srsran/phy/common/phy_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MAC metrics per user
typedef struct mac_ue_metrics_s {
  uint16_t rnti;
  uint32_t pci;
  uint32_t nof_tti;
  uint32_t cc_idx;
  int      tx_pkts;
  int      tx_errors;
  int      tx_brate;
  int      rx_pkts;
  int      rx_errors;
  int      rx_brate;
  int      ul_buffer;
  int      dl_buffer;
  float    dl_cqi;
  float    dl_ri;
  float    dl_pmi;
  float    phr;
  float    dl_cqi_offset;
  float    ul_snr_offset;

  // NR-only UL PHY metrics
  float pusch_sinr;
  float pucch_sinr;
  float ul_rssi;
  float fec_iters;
  float dl_mcs;
  int   dl_mcs_samples;
  float ul_mcs;
  int   ul_mcs_samples;
}mac_ue_metrics_t;
/// MAC misc information for each cc.
typedef struct mac_cc_info_s {
  /// PCI value.
  uint32_t pci;
  /// RACH preamble counter per cc.
  uint32_t cc_rach_counter;
}mac_cc_info_t;

/// Main MAC metrics.
typedef struct mac_metrics_s {
  /// Per CC info.
  cvector_vector_t(mac_cc_info_t) cc_info;//SRSRAN_MAX_CARRIERS//std::vector
  /// Per UE MAC metrics.
  cvector_vector_t(mac_ue_metrics_t) ues;//SRSENB_MAX_UES//std::vector
}mac_metrics_t;


#ifdef __cplusplus
}
#endif

#endif
