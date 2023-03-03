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

int fill_mib_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_BCCH_BCH_Message_t *pdu);
int fill_sib1_from_enb_cfg(rrc_nr_cfg_t *cfg, uint32_t cc, ASN_RRC_SIB1_t *sib1);


#ifdef __cplusplus
}
#endif

#endif
