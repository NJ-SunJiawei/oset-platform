/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_ASN_FILL_H_
#define RRC_ASN_FILL_H_

#include "rrc/rrc_nr_config.h"

#ifdef __cplusplus
extern "C" {
#endif

int fill_mib_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, mib_s *mib);
int fill_sib1_from_enb_cfg(rrc_nr_cfg_t *cfg, uint32_t cc, sib1_s *sib1);


#ifdef __cplusplus
}
#endif

#endif
