/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#include "gnb_common.h"
#include "lib/mac/mac_sch_pdu_nr.h"


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-lib-mac-sch"


uint32_t mac_sch_subpdu_nr_sizeof_ce(uint32_t lcid, bool is_ul)
{
  if (is_ul) {
    switch (lcid) {
      case CCCH_SIZE_48:
        return 6;
      case CCCH_SIZE_64:
        return 8;
      case CRNTI:
        return 2;
      case SHORT_BSR:
      case SHORT_TRUNC_BSR:
        return 1;
      case SE_PHR:
        return 2;
      case LONG_BSR:
      case LONG_TRUNC_BSR:
        return 1; // minimum size, could be more than that
      case PADDING:
        return 0;
    }
  } else {
    switch (lcid) {
      case CON_RES_ID:
        return 6;
      case TA_CMD:
        return 1;
      case DRX_CMD:
        return 0;
      case PADDING:
        return 0;
    }
  }
  return 0;
}

