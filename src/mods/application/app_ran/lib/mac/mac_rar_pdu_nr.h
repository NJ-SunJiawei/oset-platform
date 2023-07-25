/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/

#ifndef MAC_RAR_PDU_NR_H_
#define MAC_RAR_PDU_NR_H_

#include "oset-core.h"
#include "lib/common/buffer_interface.h"
#include "lib/srsran/srsran.h"

#ifdef __cplusplus
extern "C" {
#endif

// Possible types of RAR subpdus (same like EUTRA)
typedef enum { BACKOFF = 0, RAPID } rar_subh_type_t;

// RAR content length in bits (38.321 Sec 6.2.3)
#define  UL_GRANT_NBITS  SRSRAN_RAR_UL_GRANT_NBITS
#define  TA_COMMAND_NBITS  12
#define  MAC_RAR_NBYTES  7 // see TS 38.321 Sec 6.2.3

// 3GPP 38.321 v15.3.0 Sec 6.1.5
typedef struct {
  int         header_length;  //1 // RAR PDU subheader is always 1 B
  int         payload_length; //0 // only used if MAC RAR is included
  uint8_t     ul_grant[UL_GRANT_NBITS];
  uint16_t    ta; // 12bit TA
  uint16_t    temp_crnti;
  uint16_t    rapid;
  uint8_t     backoff_indicator;
  rar_subh_type_t type; // BACKOFF
  bool        E_bit;
}mac_rar_subpdu_nr;

typedef struct {
  cvector_vector_t(mac_rar_subpdu_nr) subpdus;
  uint32_t        pdu_len;
  uint32_t        remaining_len;

  uint16_t        si_rapid;
  bool            si_rapid_set;
  byte_buffer_t*  buffer; //存放打包好的mac pdu buffer
}mac_rar_pdu_nr;

void  mac_rar_pdu_nr_set_si_rapid(mac_rar_pdu_nr *rar_pdu, uint16_t si_rapid_);
int mac_rar_pdu_nr_init_tx(mac_rar_pdu_nr rar_pdu, byte_buffer_t* buffer_, uint32_t pdu_len_);
int mac_rar_pdu_nr_pack(mac_rar_pdu_nr *rar_pdu);
void mac_rar_pdu_nr_to_string(mac_rar_pdu_nr *rar_pdu);

#ifdef __cplusplus
}
#endif

#endif
