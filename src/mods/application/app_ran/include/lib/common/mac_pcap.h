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

#include "oset-core.h"
#include "lib/srsran/srsran.h"
#include "lib/common/buffer_interface.h"
#include "lib/common/pcap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MAC_PCAP_QUE_SIZE 1024

typedef struct {
  // Different PCAP context for both RATs
  srsran_rat_t	        rat;
  MAC_Context_Info_t	context;
  mac_nr_context_info_t context_nr;
  byte_buffer_t	        *pdu;
} pcap_pdu_t;


typedef struct {
	oset_apr_mutex_t			*mutex;
	bool						running;//atomic_bool
	oset_ring_queue_t 			*queue;
	oset_ring_buf_t             *buf;//static_blocking_queue<pcap_pdu_t, 1024>
	uint16_t				    ue_id;
	//int 						emergency_handler_id;
}mac_pcap_base;


typedef struct {
	mac_pcap_base  base;
	FILE           *pcap_file;
	uint32_t       dlt; // The DLT used for the PCAP file
	char           *filename;
}mac_pcap;

void mac_pcap_base_enable(mac_pcap *pcap, bool enable_);
void mac_pcap_base_set_ue_id(mac_pcap *pcap, uint16_t ue_id_);
uint32_t mac_pcap_open(mac_pcap *pcap, char *filename_, uint32_t ue_id_);

#ifdef __cplusplus
}
#endif

#endif
