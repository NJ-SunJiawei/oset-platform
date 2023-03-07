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

  oset_assert(rrc_manager_self()->app_pool);
  cell = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(du_cell_config));
  oset_assert(cell);

  cell->cc = node->cell_idx;//byn_array_get_count(rrc_manager_self()->du_cfg.cells)

  // Fill general cell params
  cell->pci = node->phy_cell.carrier.pci;

  // fill MIB ASN.1
  if (fill_mib_from_enb_cfg(node, cell->mib) != OSET_OK) {
	oset_error("Couldn't generate MIB");
    return OSET_ERROR;
  }
  cell->packed_mib = oset_rrc_encode(&asn_DEF_ASN_RRC_BCCH_BCH_Message, cell->mib, true);
  oset_free(cell->packed_mib);

  // fill SIB1 ASN.1
  if (fill_sib1_from_enb_cfg(rrc_manager_self()->cfg, cell.sib1) != OSET_OK) {
    oset_error("Couldn't generate SIB1");
    return OSET_ERROR;
  }
  cell->packed_sib1 = oset_rrc_encode(&asn_DEF_ASN_RRC_BCCH_DL_SCH_Message, cell->sib1, true);
  oset_free(cell->packed_sib1);

  return OSET_OK;
}


