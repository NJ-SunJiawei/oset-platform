/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_cfg.h"
#include "lib/mac/sched_nr_util.h"	
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-cfg"

void cell_config_manager_init(cell_config_manager *cell_cof_manager,
										uint32_t cc_,
										sched_nr_cell_cfg_t *cell,
										sched_args_t *sched_args_)
{
  cell_cof_manager->carrier.pci                    = cell->pci;
  cell_cof_manager->carrier.dl_center_frequency_hz = cell->dl_center_frequency_hz;
  cell_cof_manager->carrier.ul_center_frequency_hz = cell->ul_center_frequency_hz;
  cell_cof_manager->carrier.ssb_center_freq_hz     = cell->ssb_center_freq_hz;
  cell_cof_manager->carrier.nof_prb                = cell->dl_cell_nof_prb;
  cell_cof_manager->carrier.start                  = 0; // TODO: Check
  cell_cof_manager->carrier.max_mimo_layers        = cell->nof_layers;
  cell_cof_manager->carrier.offset_to_carrier      = byn_array_get_data(&cell->dl_cfg_common.freq_info_dl.scs_specific_carrier_list, 0)->offset_to_carrier;
  cell_cof_manager->carrier.scs = (srsran_subcarrier_spacing_t)cell->dl_cfg_common.init_dl_bwp.generic_params.subcarrier_spacing;

  // TDD-UL-DL-ConfigCommon
  cell_cof_manager->duplex.mode = SRSRAN_DUPLEX_MODE_FDD;//fdd模式，暂不支持tdd
  if (cell->tdd_ul_dl_cfg_common) {
    bool success = make_phy_tdd_cfg(cell->tdd_ul_dl_cfg_common, &cell_cof_manager->duplex);
    ASSERT_IF_NOT(success, "Failed to generate Cell TDD config");
  }

  // Set SSB params
  make_ssb_cfg(cell, &cell_cof_manager->ssb);

  // MIB
  make_mib_cfg(cell, &cell_cof_manager->mib);

  cell_cof_manager->bwps.reserve(cell.bwps.size());
  for (uint32_t i = 0; i < cell.bwps.size(); ++i) {
    cell_cof_manager->bwps.emplace_back(*this, i, cell.bwps[i]); //bwp_params_t::bwp_params_t
  }
  ASSERT_IF_NOT(!cell_cof_manager->bwps.empty(), "No BWPs were configured");
}

