/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "rrc/rrc_asn_fill.h"
		
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rrc"


int fill_mib_from_enb_cfg(rrc_cell_cfg_nr_t *cell_cfg, ASN_RRC_MIB_t *mib)
{
  mib =  CALLOC(1,sizeof(struct ASN_RRC_MIB_t));

  mib->systemFrameNumber.buf = CALLOC(1,sizeof(uint8_t));
  mib->systemFrameNumber.buf[0] = 0;
  mib->systemFrameNumber.size = 1;
  mib->systemFrameNumber.bits_unused=2;

  switch (cell_cfg.phy_cell.carrier.scs) {
    case srsran_subcarrier_spacing_15kHz:
    case srsran_subcarrier_spacing_60kHz:
      mib->subCarrierSpacingCommon = ASN_RRC_MIB__subCarrierSpacingCommon_scs15or60;
      break;
    case srsran_subcarrier_spacing_30kHz:
    case srsran_subcarrier_spacing_120kHz:
      mib->subCarrierSpacingCommon = ASN_RRC_MIB__subCarrierSpacingCommon_scs30or120;
      break;
    default:
      oset_error("Invalid carrier SCS=%d Hz", SRSRAN_SUBC_SPACING_NR(cell_cfg.phy_cell.carrier.scs));
  }
  mib->ssb_SubcarrierOffset       = cell_cfg.ssb_offset;
  mib->dmrs_TypeA_Position        = ASN_RRC_MIB__dmrs_TypeA_Position_pos2;
  mib->pdcch_ConfigSIB1.controlResourceSetZero = 0;
  mib->pdcch_ConfigSIB1.searchSpaceZero = cell_cfg.coreset0_idx;
  mib->cellBarred                 = ASN_RRC_MIB__cellBarred_notBarred;
  mib->intraFreqReselection       = ASN_RRC_MIB__intraFreqReselection_allowed;
  mib->spare.buf = CALLOC(1,sizeof(uint8_t));
  mib->systemFrameNumber.buf[0] = 0;
  mib->spare.size = 1;
  mib->spare.bits_unused = 7;  // This makes a spare of 1 bits
  return OSET_OK;
}

int fill_sib1_from_enb_cfg(rrc_nr_cfg_t *cfg, uint32_t cc, sib1_s *sib1)
{
  const rrc_cell_cfg_nr_t& cell_cfg = cfg.cell_list[cc];

  sib1.cell_sel_info_present            = true;
  sib1.cell_sel_info.q_rx_lev_min       = -70;
  sib1.cell_sel_info.q_qual_min_present = true;
  sib1.cell_sel_info.q_qual_min         = -20;

  sib1.cell_access_related_info.plmn_id_list.resize(1);
  sib1.cell_access_related_info.plmn_id_list[0].plmn_id_list.resize(1);
  srsran::plmn_id_t plmn;
  plmn.from_number(cfg.mcc, cfg.mnc);
  srsran::to_asn1(&sib1.cell_access_related_info.plmn_id_list[0].plmn_id_list[0], plmn);
  sib1.cell_access_related_info.plmn_id_list[0].tac_present = true;
  sib1.cell_access_related_info.plmn_id_list[0].tac.from_number(cell_cfg.tac);
  sib1.cell_access_related_info.plmn_id_list[0].cell_id.from_number((cfg.enb_id << 8U) + cell_cfg.phy_cell.cell_id);
  sib1.cell_access_related_info.plmn_id_list[0].cell_reserved_for_oper.value =
      plmn_id_info_s::cell_reserved_for_oper_opts::not_reserved;

  sib1.conn_est_fail_ctrl_present                   = true;
  sib1.conn_est_fail_ctrl.conn_est_fail_count.value = conn_est_fail_ctrl_s::conn_est_fail_count_opts::n1;
  sib1.conn_est_fail_ctrl.conn_est_fail_offset_validity.value =
      conn_est_fail_ctrl_s::conn_est_fail_offset_validity_opts::s30;
  sib1.conn_est_fail_ctrl.conn_est_fail_offset_present = true;
  sib1.conn_est_fail_ctrl.conn_est_fail_offset         = 1;

  //  sib1.si_sched_info_present                                  = true;
  //  sib1.si_sched_info.si_request_cfg.rach_occasions_si_present = true;
  //  sib1.si_sched_info.si_request_cfg.rach_occasions_si.rach_cfg_si.ra_resp_win.value =
  //      rach_cfg_generic_s::ra_resp_win_opts::sl8;
  //  sib1.si_sched_info.si_win_len.value = si_sched_info_s::si_win_len_opts::s20;
  //  sib1.si_sched_info.sched_info_list.resize(1);
  //  sib1.si_sched_info.sched_info_list[0].si_broadcast_status.value =
  //  sched_info_s::si_broadcast_status_opts::broadcasting; sib1.si_sched_info.sched_info_list[0].si_periodicity.value =
  //  sched_info_s::si_periodicity_opts::rf16; sib1.si_sched_info.sched_info_list[0].sib_map_info.resize(1);
  //  // scheduling of SI messages
  //  sib1.si_sched_info.sched_info_list[0].sib_map_info[0].type.value        = sib_type_info_s::type_opts::sib_type2;
  //  sib1.si_sched_info.sched_info_list[0].sib_map_info[0].value_tag_present = true;
  //  sib1.si_sched_info.sched_info_list[0].sib_map_info[0].value_tag         = 0;

  sib1.serving_cell_cfg_common_present = true;
  HANDLE_ERROR(fill_serv_cell_cfg_common_sib(cfg, cc, sib1.serving_cell_cfg_common));

  sib1.ue_timers_and_consts_present    = true;
  sib1.ue_timers_and_consts.t300.value = ue_timers_and_consts_s::t300_opts::ms1000;
  sib1.ue_timers_and_consts.t301.value = ue_timers_and_consts_s::t301_opts::ms1000;
  sib1.ue_timers_and_consts.t310.value = ue_timers_and_consts_s::t310_opts::ms1000;
  sib1.ue_timers_and_consts.n310.value = ue_timers_and_consts_s::n310_opts::n1;
  sib1.ue_timers_and_consts.t311.value = ue_timers_and_consts_s::t311_opts::ms30000;
  sib1.ue_timers_and_consts.n311.value = ue_timers_and_consts_s::n311_opts::n1;
  sib1.ue_timers_and_consts.t319.value = ue_timers_and_consts_s::t319_opts::ms1000;

  return SRSRAN_SUCCESS;
}

