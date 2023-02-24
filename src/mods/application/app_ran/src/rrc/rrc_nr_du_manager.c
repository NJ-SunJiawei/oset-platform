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
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc"

int du_config_manager_add_cell(rrc_cell_cfg_nr_t *node)
{
  // add cell
  du_cell_config  *cell = NULL;
  
  cell = oset_malloc(sizeof(du_cell_config));
  oset_assert(cell);

  cell->cc = byn_array_get_count(rrc_manager_self()->du_cfg.cells);

  // Fill general cell params
  cell->pci = node->phy_cell.carrier.pci;

  // fill MIB ASN.1
  if (fill_mib_from_enb_cfg(node, cell->mib) != SRSRAN_SUCCESS) {
    return SRSRAN_ERROR;
  }

  // Pack MIB
  cell.packed_mib = srsran::make_byte_buffer();
  if (cell.packed_mib == nullptr) {
    logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
    return SRSRAN_ERROR;
  }
  {
    asn1::bit_ref  bref(cell.packed_mib->msg, cell.packed_mib->get_tailroom());
    bcch_bch_msg_s bch_msg;
    bch_msg.msg.set_mib() = cell.mib;
    if (bch_msg.pack(bref) != asn1::SRSASN_SUCCESS) {
      logger.error("Couldn't pack mib msg");
      return SRSRAN_ERROR;
    }
    cell.packed_mib->N_bytes = bref.distance_bytes();
  }
  logger.info(
      cell.packed_mib->data(), cell.packed_mib->size(), "BCCH-BCH Message (with MIB) (%d B)", cell.packed_mib->size());
  asn1::json_writer js;
  cell.mib.to_json(js);
  logger.info("MIB content: %s", js.to_string().c_str());

  // fill SIB1 ASN.1
  if (fill_sib1_from_enb_cfg(cfg, cell.cc, cell.sib1) != SRSRAN_SUCCESS) {
    logger.error("Couldn't generate SIB1");
    return SRSRAN_ERROR;
  }

  // Pack SIB1
  cell.packed_sib1 = srsran::make_byte_buffer();
  if (cell.packed_sib1 == nullptr) {
    logger.error("Couldn't allocate PDU in %s().", __FUNCTION__);
    return SRSRAN_ERROR;
  }
  {
    asn1::bit_ref     bref(cell.packed_sib1->msg, cell.packed_sib1->get_tailroom());
    bcch_dl_sch_msg_s bcch_msg;
    bcch_msg.msg.set_c1().set_sib_type1() = cell.sib1;
    if (bcch_msg.pack(bref) != asn1::SRSASN_SUCCESS) {
      logger.error("Couldn't pack SIB1 msg");
      return SRSRAN_ERROR;
    }
    cell.packed_sib1->N_bytes = bref.distance_bytes();
  }
  if (cfg.is_standalone) {
    logger.info(cell.packed_sib1->data(),
                cell.packed_sib1->size(),
                "BCCH-DL-SCH-Message (with SIB1) (%d B)",
                cell.packed_sib1->size());
    cell.sib1.to_json(js);
    logger.info("SIB1 content: %s", js.to_string().c_str());
  }

  // Generate SSB SCS
  srsran_subcarrier_spacing_t ssb_scs;
  if (not srsran::fill_ssb_pattern_scs(cfg.cell_list[cell.cc].phy_cell.carrier, &cell.ssb_pattern, &ssb_scs)) {
    return SRSRAN_ERROR;
  }
  cell.ssb_scs.value      = (subcarrier_spacing_e::options)ssb_scs;
  cell.ssb_center_freq_hz = cfg.cell_list[cell.cc].ssb_freq_hz;
  cell.dl_freq_hz         = cfg.cell_list[cell.cc].phy_cell.carrier.dl_center_frequency_hz;
  cell.is_standalone      = cfg.is_standalone;

  cells.push_back(std::move(obj));
  return SRSRAN_SUCCESS;
}


