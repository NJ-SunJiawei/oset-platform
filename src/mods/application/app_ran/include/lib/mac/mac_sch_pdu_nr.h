/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef MAC_SCH_PDU_NR_H_
#define MAC_SCH_PDU_NR_H_

#include "oset-core.h"
#include "lib/common/buffer_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// SDUs up to 256 B can use the short 8-bit L field 
#define MAC_SUBHEADER_LEN_THRESHOLD  256
#define max_num_lcg_lbsr    8
#define ue_con_res_id_len   6
#define mac_ce_payload_len  (8 + 1) // Long BSR has max. 9 octets (see sizeof_ce() too)

typedef uint8_t ue_con_res_id_t[ue_con_res_id_len];

// BSR
typedef struct lcg_bsr_s {
  uint8_t lcg_id;
  uint8_t buffer_size;
}lcg_bsr_t;

typedef struct lbsr_s {
  uint8_t		 bitmap; // the first octet of LBSR and Long Trunc BSR
  cvector_vector_t(lcg_bsr_t)  list;	 //std::vector<lcg_bsr_t> // one entry for each reported LCG
}lbsr_t;

// TA
typedef struct ta_s {
  uint8_t tag_id;
  uint8_t ta_command;
}ta_t;


// 3GPP 38.321 v15.3.0 Combined Tables 6.2.1-1, 6.2.1-2
typedef enum nr_lcid_sch_e {
  //MAC SDU type
  // Values for DL-SCH
  CCCH		 = 0b000000,
  DRX_CMD	 = 0b111100,
  TA_CMD	 = 0b111101,
  CON_RES_ID = 0b111110,//UE Contention Resolution Identity

  // Values for UL-SCH
  CRNTI 		  = 0b111010,
  SHORT_TRUNC_BSR = 0b111011,
  LONG_TRUNC_BSR  = 0b111100,
  CCCH_SIZE_48	  = 0b110100,
  CCCH_SIZE_64	  = 0b000000,
  SE_PHR		  = 0b111001, // Single Entry PHR

  SHORT_BSR = 0b111101,
  LONG_BSR	= 0b111110,

  // Common
  PADDING = 0b111111,
}nr_lcid_sch_t;


/// This helper class manages a SDU pointer that can point to either a user provided external buffer or to a small
/// internal buffer, useful for storing very short SDUs.
typedef struct {
	uint8_t  ce_write_buffer[mac_ce_payload_len];//Buffer for CE payload
	uint8_t  *sdu;// Point for ue->ue_rlc_buffer->msg/ce_write_buffer
}sdu_buffer;


typedef struct {
	uint32_t   lcid;
	int 	   header_length;
	int 	   sdu_length;
	bool	   F_bit;
	sdu_buffer sdu;
}mac_sch_subpdu_nr;

typedef struct {
  bool                  ulsch;
  cvector_vector_t(mac_sch_subpdu_nr)  subpdus;//std::vector<mac_sch_subpdu_nr>
  byte_buffer_t         *buffer;  // dl: pdsch->data[0] = slot_u->h_dl->pdu; //存放打包好的mac pdu
  uint32_t              pdu_len;
  uint32_t              remaining_len;
}mac_sch_pdu_nr;

///////////////////////////////////////////sch_subpdu/////////////////////////////////////////////////////////////////
uint32_t mac_sch_subpdu_nr_sizeof_ce(uint32_t lcid, bool is_ul);
///////////////////////////////////////////sch_pdu/////////////////////////////////////////////////////////////////
uint32_t mac_sch_pdu_nr_size_header_sdu(mac_sch_pdu_nr	*mac_pdu, uint32_t lcid, uint32_t nbytes);
int mac_sch_pdu_nr_init_tx(mac_sch_pdu_nr	*mac_pdu, byte_buffer_t* buffer_, uint32_t pdu_len_, bool ulsch_);
uint32_t mac_sch_pdu_nr_add_ue_con_res_id_ce(mac_sch_pdu_nr	*mac_pdu, ue_con_res_id_t id);
uint32_t mac_sch_pdu_nr_add_sdu(mac_sch_pdu_nr	*mac_pdu, uint32_t lcid_, uint8_t* payload_, uint32_t len_);
void mac_sch_pdu_nr_pack(mac_sch_pdu_nr	*mac_pdu);
void mac_sch_pdu_nr_to_string(mac_sch_pdu_nr *mac_pdu, uint16_t rnti);

#ifdef __cplusplus
}
#endif

#endif
