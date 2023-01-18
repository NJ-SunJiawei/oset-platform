/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef MAC_PCAP_H_
#define MAC_PCAP_H_

#include "lib/common/buffer_interface.h"
#include "lib/srsran/srsran.h"
#include "lib/common/pcap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // Different PCAP context for both RATs
  srsran_rat_t	        rat;
  MAC_Context_Info_t	context;
  mac_nr_context_info_t context_nr;
  byte_buffer_t	        *pdu;
} pcap_pdu_t;


typedef struct {
  FILE        *pcap_file;
  uint32_t    dlt; // The DLT used for the PCAP file
  char        *filename;
}mac_pcap;


#ifdef __cplusplus
}
#endif

#endif
