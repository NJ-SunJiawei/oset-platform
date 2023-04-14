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

#include "rrc/rrc_nr_config.h"

#ifdef __cplusplus
extern "C" {
#endif
bool make_phy_tdd_cfg_inner(srsran_duplex_config_nr_t	       	*srsran_duplex_config_nr,
					        srsran_subcarrier_spacing_t	  scs,
					        struct tdd_ul_dl_cfg_common_s *tdd_ul_dl_cfg_common);
bool make_phy_rach_cfg_inner(rach_cfg_common_s *asn1_type, srsran_prach_cfg_t *prach_cfg);
bool make_phy_coreset_cfg(struct ctrl_res_set_s *ctrl_res_set, srsran_coreset_t *in_srsran_coreset);
bool make_phy_search_space_cfg(struct search_space_s *search_space, srsran_search_space_t *in_srsran_search_space);
bool fill_phy_pdcch_cfg_common2(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_pdcch_cfg_nr_t *pdcch);
bool fill_ssb_pattern_scs(srsran_carrier_nr_t *carrier,
                                  srsran_ssb_pattern_t *pattern,
                                  srsran_subcarrier_spacing_t *ssb_scs);
bool fill_phy_ssb_cfg(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_ssb_cfg_t *out_ssb);
bool fill_rach_cfg_common_default_inner(srsran_prach_cfg_t *prach_cfg, struct rach_cfg_common_s *rach_cfg_com);
bool make_pdsch_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_sch_hl_cfg_nr_t *sch_hl);
bool make_csi_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_csi_hl_cfg_t* csi_hl);
//////////////////////////////////////////////////////////////////////////////////////////////////
int fill_mib_from_enb_cfg_inner(rrc_cell_cfg_nr_t *cell_cfg, struct mib_s *mib);
void free_sib1_dyn_arrary(struct sib1_s *sib1);
int fill_sib1_from_enb_cfg_inner(rrc_cell_cfg_nr_t *cell_cfg, struct sib1_s *sib1);
void free_master_cell_cfg_dyn_array(struct cell_group_cfg_s *master_cell_group);
int fill_master_cell_cfg_from_enb_cfg_inner(rrc_nr_cfg_t *cfg, uint32_t cc, struct cell_group_cfg_s *out);

#ifdef __cplusplus
}
#endif

#endif
