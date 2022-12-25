/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef PHY_H_
#define PHY_H_

#include "oset-core.h"
#include "srsran.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "phy"


#ifdef __cplusplus
extern "C" {
#endif

int phy_init(void);
int phy_destory(void);

#ifdef __cplusplus
}
#endif

#endif
