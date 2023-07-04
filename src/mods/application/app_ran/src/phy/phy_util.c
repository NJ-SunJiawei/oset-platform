/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "phy/phy.h"

uint32_t get_nof_carriers_nr(void)
{
	return cvector_size(phy_manager_self()->workers_common.cell_list_nr);
}
uint32_t get_nof_carriers(void)
{
	return cvector_size(phy_manager_self()->workers_common.cell_list_nr);
}

uint32_t get_nof_prb(uint32_t cc_idx)
{
  uint32_t ret = 0;
  phy_cell_cfg_nr_t * cell = NULL;

  if (cc_idx >= get_nof_carriers()) {
	// invalid CC index
	return ret;
  }

  cell = &phy_manager_self()->workers_common.cell_list_nr[cc_idx];
  oset_assert(cell);
  ret = cell->carrier.nof_prb;
  return ret;
}

uint32_t get_nof_ports(uint32_t cc_idx)
{
  uint32_t ret = 0;

  if ((cc_idx == 0 || cc_idx == 1) && (!cvector_empty(phy_manager_self()->workers_common.cell_list_nr)) {
	// one RF port for basic NSA/SA config
	ret = 1;
  }

  return ret;
}

uint32_t get_nof_rf_channels()
{
  uint32_t count = 0;
  phy_cell_cfg_nr_t *cell = NULL;

  cvector_for_each_in(cell, phy_manager_self()->workers_common.cell_list_nr){
	  count += cell->carrier.max_mimo_layers;
  }
  return count;
}

double get_ul_freq_hz(uint32_t cc_idx)
{
  double ret = 0.0;
  phy_cell_cfg_nr_t *cell = NULL;

  cell = &phy_manager_self()->workers_common.cell_list_nr[cc_idx];
  oset_assert(cell);
  ret = cell->carrier.ul_center_frequency_hz;
  return ret;
}

double get_dl_freq_hz(uint32_t cc_idx)
{
  double ret = 0.0;
  phy_cell_cfg_nr_t *cell = NULL;

  cell = &phy_manager_self()->workers_common.cell_list_nr[cc_idx];
  oset_assert(cell);
  ret = cell->carrier.dl_center_frequency_hz;
  return ret;
}

double get_ssb_freq_hz(uint32_t cc_idx)
{
  double ret = 0.0;
  struct phy_cell_cfg_nr_t * cell = NULL;

  cell = &phy_manager_self()->workers_common.cell_list_nr[cc_idx];
  oset_assert(cell);
  ret = cell->carrier.ssb_center_freq_hz;
  return ret;
}

uint32_t get_rf_port(uint32_t cc_idx)
{
  uint32_t ret = 0;
  struct phy_cell_cfg_nr_t * cell = NULL;
  
  cell = &phy_manager_self()->workers_common.cell_list_nr[cc_idx];
  oset_assert(cell);
  ret = cell->rf_port;
  return ret;
}

