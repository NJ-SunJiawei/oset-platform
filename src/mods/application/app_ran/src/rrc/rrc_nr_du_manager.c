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
#include "rrc/rrc_asn_fill.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc"


int du_config_manager_add_cell(rrc_cell_cfg_nr_t *node)
{
  // add cell
  du_cell_config  *cell = NULL;

  oset_assert(rrc_manager_self()->app_pool);
  cell = oset_core_alloc(rrc_manager_self()->app_pool, sizeof(du_cell_config));
  oset_assert(cell);

  cell->cc = byn_array_get_count(rrc_manager_self()->du_cfg.cells);

  // Fill general cell params
  cell->pci = node->phy_cell.carrier.pci;

  // fill MIB ASN.1
  if (fill_mib_from_enb_cfg(node, cell->mib) != OSET_OK) {
    return OSET_ERROR;
  }
  cell->packed_mib = oset_asn_uper_encode(&asn_DEF_ASN_RRC_MIB, cell->mib, true);
  if (oset_runtime()->hard_log_level >= OSET_LOG2_DEBUG){
	  if (asn1_encoder_xer_print){
		  xer_fprint(stdout, &asn_DEF_ASN_NGAP_NGAP_PDU, cell->mib);
	  }else{
		  asn_fprint(stdout, &asn_DEF_ASN_NGAP_NGAP_PDU, cell->mib);
	  }
  }
  oset_free(cell->packed_mib);


  return OSET_OK;
}


