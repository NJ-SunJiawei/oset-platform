/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "phy.h"

uint32_t get_nof_carriers_nr() { return phy_manager_self()->workers_common.cell_list_nr.count; }
uint32_t get_nof_carriers() { return phy_manager_self()->workers_common.cell_list_nr.count; }


uint32_t get_nof_prb(uint32_t cc_idx)
{
  uint32_t ret = 0;
  oset_lnode2_t* lnode = NULL;
  struct phy_cell_cfg_nr_t * cell = NULL;

  if (cc_idx >= get_nof_carriers()) {
	// invalid CC index
	return ret;
  }

  lnode = oset_list2_find(phy_manager_self()->workers_common.cell_list_nr, cc_idx);
  cell = (struct phy_cell_cfg_nr_t *)lnode->data;
  ret = cell->carrier.nof_prb;
  return ret;
}

uint32_t get_nof_ports(uint32_t cc_idx)
{
  uint32_t ret = 0;

  if ((cc_idx == 0 || cc_idx == 1) && !(0 == phy_manager_self()->workers_common.cell_list_nr.count)) {
	// one RF port for basic NSA/SA config
	ret = 1;
  }

  return ret;
}

uint32_t get_nof_rf_channels()
{
  uint32_t count = 0;
  oset_lnode2_t* lnode = NULL;
  struct phy_cell_cfg_nr_t * cell = NULL;

  oset_list2_for_each(phy_manager_self()->workers_common.cell_list_nr, lnode){
	  cell = (struct phy_cell_cfg_nr_t *)lnode->data;
	  count += cell->carrier.max_mimo_layers;
  }
  return count;
}

