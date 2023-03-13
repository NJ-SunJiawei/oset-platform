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
#include "rrc/rrc_nr_du_manager.h"
#include "rrc/rrc_cell_asn_fill.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc"


int du_config_manager_add_cell(rrc_cell_cfg_nr_t *node)
{
  // add cell
  du_cell_config  *cell = NULL;
  ASN_RRC_BCCH_BCH_Message_t  *mib = NULL;
  ASN_RRC_BCCH_DL_SCH_Message_t *sib1 = NULL;

  oset_assert(rrc_manager_self()->app_pool);
  cell = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(du_cell_config));
  oset_assert(cell);
  byn_array_add(&rrc_manager_self()->du_cfg.cells, cell);

  cell->cc = node->cell_idx;

  // Fill general cell params
  cell->pci = node->phy_cell.carrier.pci;

  // fill MIB ASN.1
  if (fill_mib_from_enb_cfg(node, mib) != OSET_OK) {
	oset_error("Couldn't generate MIB");
    return OSET_ERROR;
  }
  cell->packed_mib = oset_rrc_encode(&asn_DEF_ASN_RRC_BCCH_BCH_Message, mib, asn_struct_free_all);
  //oset_free(cell->packed_mib);

  // fill SIB1 ASN.1
  if (fill_sib1_from_enb_cfg(node, sib1) != OSET_OK) {
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

bool make_phy_coreset_cfg(ctrl_res_set_s *ctrl_res_set, srsran_coreset_t *in_srsran_coreset)
{
	srsran_coreset_t srsran_coreset = {0};
	srsran_coreset.id               = ctrl_res_set->ctrl_res_set_id;
	switch (ctrl_res_set->precoder_granularity) {
	case same_as_reg_bundle:
	  srsran_coreset.precoder_granularity = srsran_coreset_precoder_granularity_reg_bundle;
	  break;
	case all_contiguous_rbs:
	  srsran_coreset.precoder_granularity = srsran_coreset_precoder_granularity_contiguous;
	default:
	  oset_error("Invalid option for precoder_granularity %d", ctrl_res_set->precoder_granularity);
	  return false;
	};

	switch (ctrl_res_set->cce_reg_map_type.types) {
	case interleaved:
	  srsran_coreset.mapping_type = srsran_coreset_mapping_type_interleaved;
	  break;
	case non_interleaved:
	  srsran_coreset.mapping_type = srsran_coreset_mapping_type_non_interleaved;
	  break;
	default:
	  oset_error("Invalid option for cce_reg_map_type: %d", ctrl_res_set->cce_reg_map_type.types);
	  return false;
	}
	srsran_coreset.duration = ctrl_res_set->dur;
	for (uint32_t i = 0; i < SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE; i++) {
	srsran_coreset.freq_resources[i] = bit_get(ctrl_res_set->freq_domain_res,SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE - 1 - i);
	}
	*in_srsran_coreset = srsran_coreset;
	return true;
}

bool make_phy_search_space_cfg(struct search_space_s *search_space, srsran_search_space_t *in_srsran_search_space)
{
  srsran_search_space_t srsran_search_space = {0};
  srsran_search_space.id                    = search_space->search_space_id;
  if (!search_space->ctrl_res_set_id_present) {
    oset_error("ctrl_res_set_id option not present");
    return false;
  }
  srsran_search_space.coreset_id = search_space->ctrl_res_set_id;

  srsran_search_space.duration = 1;
  if (search_space->dur_present) {
    srsran_search_space.duration = search_space->dur;
  }

  if (!search_space->nrof_candidates_present) {
    oset_error("nrof_candidates_present option not present");
    return false;
  }
  uint8_t options[] = {0, 1, 2, 3, 4, 5, 6, 8};
  srsran_search_space.nof_candidates[0] = options[search_space->nrof_candidates.aggregation_level1];
  srsran_search_space.nof_candidates[1] = options[search_space->nrof_candidates.aggregation_level2];
  srsran_search_space.nof_candidates[2] = options[search_space->nrof_candidates.aggregation_level4];
  srsran_search_space.nof_candidates[3] = options[search_space->nrof_candidates.aggregation_level8];
  srsran_search_space.nof_candidates[4] = options[search_space->nrof_candidates.aggregation_level16];

  if (!search_space->search_space_type_present) {
    oset_error("nrof_candidates option not present");
    return false;
  }
  switch (search_space->search_space_type.types) {
    case common:
      srsran_search_space.type = srsran_search_space_type_common_3;

      // dci-Format0-0-AndFormat1-0
      // If configured, the UE monitors the DCI formats 0_0 and 1_0 according to TS 38.213 [13], clause 10.1.
      if (search_space->search_space_type.c.common.dci_format0_minus0_and_format1_minus0_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_0_0;
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_1_0;
      }

      // dci-Format2-0
      // If configured, UE monitors the DCI format 2_0 according to TS 38.213 [13], clause 10.1, 11.1.1.
      if (search_space->search_space_type.c.common.dci_format2_minus0_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_2_0;
      }

      // dci-Format2-1
      // If configured, UE monitors the DCI format 2_1 according to TS 38.213 [13], clause 10.1, 11.2.
      if (search_space->search_space_type.c.common.dci_format2_minus1_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_2_1;
      }

      // dci-Format2-2
      // If configured, UE monitors the DCI format 2_2 according to TS 38.213 [13], clause 10.1, 11.3.
      if (search_space->search_space_type.c.common.dci_format2_minus2_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_2_2;
      }

      // dci-Format2-3
      // If configured, UE monitors the DCI format 2_3 according to TS 38.213 [13], clause 10.1, 11.4
      if (search_space->search_space_type.c.common.dci_format2_minus3_present) {
        srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_2_3;
      }

      break;
    case ue_specific:
      srsran_search_space.type = srsran_search_space_type_ue;
      switch (search_space->search_space_type.c.ue_spec.dci_formats.value) {
        case formats0_minus0_and_minus1_minus0:
          srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_0_0;
          srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_1_0;
          break;
        case formats0_minus1_and_minus1_minus1:
          srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_0_1;
          srsran_search_space.formats[srsran_search_space.nof_formats++] = srsran_dci_format_nr_1_1;
          break;
      }
      break;
    default:
      oset_error("Invalid option for search_space_type %d", search_space->search_space_type.types);
      return false;
  }
  // Copy struct and return value
  *in_srsran_search_space = srsran_search_space;
  return true;
}


bool fill_phy_pdcch_cfg_common2(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_pdcch_cfg_nr_t *pdcch)
{
	if (rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set_present) {
		//is_sa no enter
		pdcch->coreset_present[rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set.ctrl_res_set_id] = true;
		make_phy_coreset_cfg(rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set, &pdcch->coreset[rrc_cell_cfg->pdcch_cfg_common.common_ctrl_res_set.ctrl_res_set_id]);
	}

	for (int i = 0; i < byn_array_get_count(&rrc_cell_cfg->pdcch_cfg_common.common_search_space_list); i++) {
		struct search_space_s *ss = byn_array_get_data(&rrc_cell_cfg->pdcch_cfg_common.common_search_space_list, i);

		pdcch->search_space_present[ss->search_space_id] = true;
		if (! make_phy_search_space_cfg(ss, &pdcch->search_space[ss->search_space_id])) {
		  oset_error("Failed to convert SearchSpace Configuration");
		  return false;
		}
		if (rrc_cell_cfg->pdcch_cfg_common.ra_search_space_present &&\
			rrc_cell_cfg->pdcch_cfg_common.ra_search_space == ss->search_space_id) {
			pdcch->ra_search_space_present     = true;
			pdcch->ra_search_space             = pdcch->search_space[ss->search_space_id];
			pdcch->ra_search_space.type        = srsran_search_space_type_common_1;
			pdcch->ra_search_space.nof_formats = 1;
			pdcch->ra_search_space.formats[0]  = srsran_dci_format_nr_1_0;
		}
	}
	return true;
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


