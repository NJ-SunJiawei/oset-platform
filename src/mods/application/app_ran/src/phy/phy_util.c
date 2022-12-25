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

