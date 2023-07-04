/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef PHY_UTIL_H_
#define PHY_UTIL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t get_nof_carriers_nr(void);
uint32_t get_nof_carriers(void);
uint32_t get_nof_ports(uint32_t cc_idx);
uint32_t get_nof_prb(uint32_t cc_idx);
uint32_t get_nof_rf_channels();
double get_ul_freq_hz(uint32_t cc_idx);
double get_dl_freq_hz(uint32_t cc_idx);
double get_ssb_freq_hz(uint32_t cc_idx);
uint32_t get_rf_port(uint32_t cc_idx);


#ifdef __cplusplus
}
#endif

#endif
