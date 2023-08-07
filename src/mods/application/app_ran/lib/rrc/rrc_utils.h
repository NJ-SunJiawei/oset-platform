/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/

#ifndef RRC_UTILS_H_
#define RRC_UTILS_H_

#include "lib/common/phy_cfg_nr.h"
#include "lib/common/asn_interface.h"

#ifdef __cplusplus
extern "C" {
#endif
bool make_phy_tdd_cfg_inner(srsran_duplex_config_nr_t	       	*srsran_duplex_config_nr,
					        srsran_subcarrier_spacing_t	  scs,
					        struct tdd_ul_dl_cfg_common_s *tdd_ul_dl_cfg_common);
bool make_phy_rach_cfg_inner(rach_cfg_common_s *asn1_type, srsran_prach_cfg_t *prach_cfg);
bool make_phy_coreset_cfg(struct ctrl_res_set_s *ctrl_res_set, srsran_coreset_t *in_srsran_coreset);
bool make_phy_search_space_cfg(struct search_space_s *search_space, srsran_search_space_t *in_srsran_search_space);
bool make_phy_nzp_csi_rs_resource(struct nzp_csi_rs_res_s *asn1_nzp_csi_rs_res,
                                             srsran_csi_rs_nzp_resource_t *out_csi_rs_nzp_resource);
bool make_phy_zp_csi_rs_resource(struct zp_csi_rs_res_s *zp_csi_rs_res,
								  srsran_csi_rs_zp_resource_t* out_zp_csi_rs_resource);
bool make_phy_csi_report(csi_report_cfg_s *csi_report_cfg,
								srsran_csi_hl_report_cfg_t* in_srsran_csi_hl_report_cfg);
bool make_phy_res_config(const pucch_res_s 		 *pucch_res,
								   uint32_t 				  format_2_max_code_rate,
								   srsran_pucch_nr_resource_t *in_srsran_pucch_nr_resource);
bool make_pdsch_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_sch_hl_cfg_nr_t *sch_hl);
bool make_csi_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_csi_hl_cfg_t* csi_hl);
int make_rlc_config_t(struct rlc_cfg_c *asn1_type, uint8_t bearer_id, rlc_config_t* cfg_out);

bool fill_phy_pdcch_cfg_common2(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_pdcch_cfg_nr_t *pdcch);
bool fill_ssb_pattern_scs(srsran_carrier_nr_t *carrier,
                                  srsran_ssb_pattern_t *pattern,
                                  srsran_subcarrier_spacing_t *ssb_scs);
bool fill_phy_ssb_cfg(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_ssb_cfg_t *out_ssb);
bool fill_rach_cfg_common_default_inner(srsran_prach_cfg_t *prach_cfg, struct rach_cfg_common_s *rach_cfg_com);

////////////////////////////////////////////////////////////////////////////////////////////////
bool API_rrc_mac_make_pdsch_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_sch_hl_cfg_nr_t *sch_hl);
bool API_rrc_mac_make_csi_cfg_from_serv_cell(struct serving_cell_cfg_s *serv_cell, srsran_csi_hl_cfg_t* csi_hl);

#ifdef __cplusplus
}
#endif

#endif
