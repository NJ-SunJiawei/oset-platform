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

#ifdef __cplusplus
extern "C" {
#endif

uint32_t get_nof_carriers_nr();
uint32_t get_nof_carriers();
uint32_t get_nof_ports(uint32_t cc_idx);
uint32_t get_nof_prb(uint32_t cc_idx);
uint32_t get_nof_rf_channels();

#ifdef __cplusplus
}
#endif

#endif
