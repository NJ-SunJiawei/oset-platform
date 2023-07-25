/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_UTIL_H_
#define RRC_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/common/asn_interface.h"
#include "lib/srsran/srsran.h"

bool make_pdsch_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_sch_hl_cfg_nr_t *sch_hl);
bool make_csi_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_csi_hl_cfg_t* csi_hl);

#ifdef __cplusplus
}
#endif

#endif
