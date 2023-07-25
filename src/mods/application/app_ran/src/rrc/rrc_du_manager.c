/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "rrc/rrc.h"
#include "rrc/rrc_du_manager.h"
#include "rrc/rrc_cell_asn_fill.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc"

//todo 对于MasterCellConfig需要管理和分配。
// pucch sr/csi的时频资源分配 ps:oran: du_pucch_resource_manager::alloc_resources函数

void du_config_manager_release_buf(du_cell_config *du_cell)
{
	//free du->packed_mib
	oset_free(du_cell->packed_mib);
	//free du->packed_sib1
	oset_free(du_cell->packed_sib1);
	//free du->sib1
	free_sib1_dyn_arrary(du_cell->sib1);
}

int du_config_manager_add_cell(rrc_cell_cfg_nr_t *node)
{
	// add cell
	ASN_RRC_BCCH_BCH_Message_t  *mib = NULL;
	ASN_RRC_BCCH_DL_SCH_Message_t *sib1 = NULL;

	oset_assert(rrc_manager_self()->app_pool);
	du_cell_config  *cell = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(du_cell_config));
	oset_assert(cell);
	cvector_push_back(rrc_manager_self()->du_cfg.cells, cell);

	cell->cc = node->cell_idx;

	// Fill general cell params
	cell->pci = node->phy_cell.carrier.pci;

	fill_mib_from_enb_cfg_inner(node, &cell->mib);
	// fill MIB ASN.1
	if (fill_mib_from_enb_cfg(node, &mib) != OSET_OK) {
		oset_error("Couldn't generate MIB");
		return OSET_ERROR;
	}
	cell->packed_mib = oset_rrc_encode(&asn_DEF_ASN_RRC_BCCH_BCH_Message, mib, asn_struct_free_all);
	//oset_free(cell->packed_mib);

	fill_sib1_from_enb_cfg_inner(node, &cell->sib1);
	// fill SIB1 ASN.1
	if (fill_sib1_from_enb_cfg(node, &sib1) != OSET_OK) {
		oset_error("Couldn't generate SIB1");
		return OSET_ERROR;
	}
	cell->packed_sib1 = oset_rrc_encode(&asn_DEF_ASN_RRC_BCCH_DL_SCH_Message, sib1, asn_struct_free_all);
	//oset_free(cell->packed_sib1);

	// Generate SSB SCS
	srsran_subcarrier_spacing_t ssb_scs;
	if (!fill_ssb_pattern_scs(node->phy_cell.carrier, &cell->ssb_pattern, &ssb_scs)) {
		return OSET_ERROR;
	}
	cell->ssb_scs = (enum subcarrier_spacing_e)ssb_scs;
	cell->ssb_center_freq_hz = node->ssb_freq_hz;
	cell->dl_freq_hz         = node->phy_cell.carrier.dl_center_frequency_hz;
	cell->is_standalone      = rrc_manager_self()->cfg.is_standalone;
	return OSET_OK;
}

//todo
void fill_phy_pdcch_cfg_common(du_cell_config *cell, rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_pdcch_cfg_nr_t *pdcch)
{
  bool    is_sa        = cell->is_standalone;
  uint8_t coreset0_idx = rrc_cell_cfg->coreset0_idx;//mib->pdcch_ConfigSIB1.controlResourceSetZero
  uint8_t scs     = rrc_cell_cfg->phy_cell.carrier.scs;
  uint8_t ssb_scs = cell->ssb_scs;
  uint32_t nof_prb = rrc_cell_cfg->phy_cell.carrier.nof_prb;//downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list

  if (is_sa) {
    // Generate CORESET#0
    pdcch->coreset_present[0] = true;
    // Get pointA and SSB absolute frequencies
    double pointA_abs_freq_Hz = cell->dl_freq_hz - nof_prb * SRSRAN_NRE * SRSRAN_SUBC_SPACING_NR(scs) / 2;
    double ssb_abs_freq_Hz    = cell->ssb_center_freq_hz;
    // Calculate integer SSB to pointA frequency offset in Hz
    // 计算coreset0
    uint32_t ssb_pointA_freq_offset_Hz =
        (ssb_abs_freq_Hz > pointA_abs_freq_Hz) ? (uint32_t)(ssb_abs_freq_Hz - pointA_abs_freq_Hz) : 0;
    int ret = srsran_coreset_zero(cell->pci, ssb_pointA_freq_offset_Hz, ssb_scs, scs, coreset0_idx, &pdcch->coreset[0]);
    ASSERT_IF_NOT(ret == SRSRAN_SUCCESS, "Failed to generate CORESET#0");

    //default  Generate SearchSpace#0 for sib1
    pdcch->search_space_present[0]           = true;
    pdcch->search_space[0].id                = 0;
    pdcch->search_space[0].coreset_id        = 0;
    pdcch->search_space[0].type              = srsran_search_space_type_common_0;
    pdcch->search_space[0].nof_candidates[0] = 1;
    pdcch->search_space[0].nof_candidates[1] = 1;
    pdcch->search_space[0].nof_candidates[2] = 1;
    pdcch->search_space[0].nof_candidates[3] = 0;
    pdcch->search_space[0].nof_candidates[4] = 0;
    pdcch->search_space[0].nof_formats       = 1;
    pdcch->search_space[0].formats[0]        = srsran_dci_format_nr_1_0;
    pdcch->search_space[0].duration          = 1;
  }

  // Generate Common CORESETs and Search Spaces
  bool ret = fill_phy_pdcch_cfg_common2(rrc_cell_cfg, pdcch);
  ASSERT_IF_NOT(ret, "PDCCH Config Common");
}

bool fill_phy_pdcch_cfg(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_pdcch_cfg_nr_t *pdcch)
{
	struct ctrl_res_set_s *coreset = NULL;
	cvector_for_each_in(coreset, rrc_cell_cfg->pdcch_cfg_ded.ctrl_res_set_to_add_mod_list){
		pdcch->coreset_present[coreset->ctrl_res_set_id] = true;
		make_phy_coreset_cfg(coreset, &pdcch->coreset[coreset->ctrl_res_set_id]);
	}

	struct search_space_s *ss = NULL;
	cvector_for_each_in(ss, rrc_cell_cfg->pdcch_cfg_ded.search_spaces_to_add_mod_list){
		pdcch->search_space_present[ss->search_space_id] = true;
		make_phy_search_space_cfg(ss, &pdcch->search_space[ss->search_space_id]);
	}
	return true;
}

