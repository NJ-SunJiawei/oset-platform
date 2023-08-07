/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_CELL_ASN_FILL_H_
#define RRC_CELL_ASN_FILL_H_

#include "rrc/rrc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

int fill_sib1_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BCCH_DL_SCH_Message_t **sib1_result);
int fill_mib_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BCCH_BCH_Message_t **mib_result);
int fill_master_cell_cfg(struct cell_group_cfg_s *cell_group_cfg, ASN_RRC_CellGroupConfig_t *out);


#ifdef __cplusplus
}
#endif

#endif
