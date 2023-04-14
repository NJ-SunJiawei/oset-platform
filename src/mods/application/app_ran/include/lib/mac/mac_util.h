/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef MAC_UTIL_H_
#define MAC_UTIL_H_

#include "lib/mac/sched_nr_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

int mac_cell_cfg(cvector_vector_t(sched_nr_cell_cfg_t) sched_cells);

#ifdef __cplusplus
}
#endif

#endif
