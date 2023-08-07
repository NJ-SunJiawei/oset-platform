/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_NR_SECURITY_CONTEXT_H
#define RRC_NR_SECURITY_CONTEXT_H

#include "lib/common/security.h"
#include "rrc/rrc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

class nr_security_context
{
  rrc_nr_cfg_t                    *cfg;
  bool                            k_gnb_present;
  struct ue_security_cap_s        security_capabilities;
  uint8_t                         k_gnb[32]; // Provided by MME
  struct nr_as_security_config_t  sec_cfg;
  uint8_t                         ncc;
};

#ifdef __cplusplus
}
#endif

#endif
