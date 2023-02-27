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

  struct mib_s   mib;
  byte_buffer_t  *packed_mib;

  struct sib1_s  sib1;
  byte_buffer_t  *packed_sib1;

  enum subcarrier_spacing_e          ssb_scs;
  srsran_ssb_pattern_t               ssb_pattern;
  double                             ssb_center_freq_hz;
  double                             dl_freq_hz;
  bool                               is_standalone;
}du_cell_config;

typedef struct{
  A_DYN_ARRAY_OF(du_cell_config) cells;//std::vector<std::unique_ptr<du_cell_config> >
}du_config_manager;


#ifdef __cplusplus
}
#endif

#endif