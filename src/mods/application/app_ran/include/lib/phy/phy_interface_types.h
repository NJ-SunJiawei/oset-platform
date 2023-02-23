/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#ifndef PHY_INTERFACE_TYPES_H
#define PHY_INTERFACE_TYPES_H

#include "lib/common/common.h"
#include "lib/srsran/srsran.h"
typedef struct {
  float    rsrp;
  float    rsrq;
  float    sinr;
  float    cfo_hz;
  uint32_t arfcn_nr;
  uint32_t pci_nr;
}phy_meas_nr_t;

typedef struct {
  srsran_rat_t         rat; ///< LTE or NR
  float                rsrp;
  float                rsrq;
  float                cfo_hz;
  uint32_t             earfcn;
  uint32_t             pci;
}phy_meas_t;

typedef struct {
  uint32_t pci;
  uint32_t earfcn;
  float    cfo_hz;
}phy_cell_t;

#endif
