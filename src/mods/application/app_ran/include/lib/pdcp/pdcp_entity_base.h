/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/

#ifndef PDCP_ENTITY_BASE_H
#define PDCP_ENTITY_BASE_H

#include "lib/common/security.h"
#include "lib/pdcp/pdcp_metrics.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Structs and Defines common to both LTE and NR
 * Ref: 3GPP TS 36.323 v10.1.0 and TS 38.323 v15.2.0
 ***************************************************************************/

#define PDCP_PDU_TYPE_PDCP_STATUS_REPORT 0x0
#define PDCP_PDU_TYPE_INTERSPERSED_ROHC_FEEDBACK_PACKET 0x1

// Maximum supported PDCP SDU size is 9000 bytes.
// See TS 38.323 v15.2.0, section 4.3.1
#define PDCP_MAX_SDU_SIZE 9000

typedef enum {
  PDCP_D_C_CONTROL_PDU = 0,
  PDCP_D_C_DATA_PDU,
  PDCP_D_C_N_ITEMS,
} pdcp_d_c_t;
static const char pdcp_d_c_text[PDCP_D_C_N_ITEMS][20] = {"Control PDU", "Data PDU"};

#ifdef __cplusplus
}
#endif

#endif
