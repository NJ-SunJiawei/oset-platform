/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_UTIL_H
#define SCHED_UTIL_H

#include "lib/mac/sched_interface.h"

#ifdef __cplusplus
extern "C" {
#endif
bool make_phy_tdd_cfg(tdd_ul_dl_cfg_common_s *tdd_ul_dl_cfg_common,
                      		srsran_duplex_config_nr_t *in_srsran_duplex_config_nr);
void make_mib_cfg(sched_nr_cell_cfg_t *cfg, srsran_mib_nr_t *mib);
void make_ssb_cfg(sched_nr_cell_cfg_t *cfg, ssb_cfg_t* ssb);
phy_cfg_nr_t get_common_ue_phy_cfg(sched_nr_cell_cfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif
