/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_NR_DU_MANAGER_H_
#define RRC_NR_DU_MANAGER_H_

#include "lib/srsran/srsran.h"
#include "rrc/rrc_nr_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t cc;
  uint32_t pci;

  //ASN_RRC_BCCH_BCH_Message_t  *mib;
  struct mib_s   mib;//inner
  byte_buffer_t  *packed_mib;

  //ASN_RRC_BCCH_DL_SCH_Message_t *sib1;
  struct sib1_s  sib1;//inner
  byte_buffer_t  *packed_sib1;

  enum subcarrier_spacing_e          ssb_scs;
  srsran_ssb_pattern_t               ssb_pattern;
  double                             ssb_center_freq_hz;
  double                             dl_freq_hz;
  bool                               is_standalone;
}du_cell_config;

typedef struct{
  cvector_vector_t(du_cell_config *) cells;//std::vector<std::unique_ptr<du_cell_config> >
}du_config_manager;

void du_config_manager_release_buf(du_cell_config *du_cell);
int du_config_manager_add_cell(rrc_cell_cfg_nr_t *node);
void fill_phy_pdcch_cfg_common(du_cell_config *cell, srsran_pdcch_cfg_nr_t *pdcch);
bool fill_phy_pdcch_cfg(rrc_cell_cfg_nr_t *rrc_cell_cfg, srsran_pdcch_cfg_nr_t *pdcch);

#ifdef __cplusplus
}
#endif

#endif
