/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_INTERFACE_TYPES_H_
#define RRC_INTERFACE_TYPES_H_

#include "rrc/rrc_config.h"
#include "lib/rrc/rrc_utils.h"

#ifdef __cplusplus
extern "C" {
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////
int fill_mib_from_enb_cfg_inner(rrc_cell_cfg_nr_t *cell_cfg, struct mib_s *mib);
void free_sib1_dyn_arrary(struct sib1_s *sib1);
int fill_sib1_from_enb_cfg_inner(rrc_cell_cfg_nr_t *cell_cfg, struct sib1_s *sib1);
void free_master_cell_cfg_vector(struct cell_group_cfg_s *master_cell_group);
int fill_master_cell_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, uint32_t cc, struct cell_group_cfg_s *out);
int fill_cellgroup_with_radio_bearer_cfg_inner(rrc_nr_cfg_t *              cfg,
                                         uint32_t                  rnti,
                                         enb_bearer_manager        *bearer_mapper,
                                         struct radio_bearer_cfg_s *bearers,
                                         struct cell_group_cfg_s   *out);


#ifdef __cplusplus
}
#endif

#endif
