/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#ifndef PDCP_INTERFACE_TYPES_H
#define PDCP_INTERFACE_TYPES_H

#include "lib/common/security_private.h"
#include <math.h>
#include <stdint.h>

/***************************
 *      PDCP Config
 **************************/
// LTE and NR common config
const uint8_t PDCP_SN_LEN_5  = 5;
const uint8_t PDCP_SN_LEN_7  = 7;
const uint8_t PDCP_SN_LEN_15 = 15;
const uint8_t PDCP_SN_LEN_12 = 12;
const uint8_t PDCP_SN_LEN_18 = 18;

typedef enum { PDCP_RB_IS_SRB, PDCP_RB_IS_DRB } pdcp_rb_type_t;

typedef enum {
	PDCP_DC_FIELD_CONTROL_PDU = 0,
	PDCP_DC_FIELD_DATA_PDU,
	PDCP_DC_FIELD_N_ITEMS,
}pdcp_dc_field_t;
static const char* pdcp_dc_field_text[PDCP_DC_FIELD_N_ITEMS] = {"Control PDU", "Data PDU"};

typedef enum {
	PDCP_PDU_TYPE_STATUS_REPORT = 0,
	PDCP_PDU_TYPE_INTERSPERSED_ROHC_FEEDBACK_PACKET,
	PDCP_PDU_TYPE_LWA_STATUS_REPORT,
	PDCP_PDU_TYPE_LWA_END_MARKER_PACKET,
	PDCP_PDU_TYPE_N_ITEMS,
}pdcp_pdu_type_t;
static const char* pdcp_pdu_type_text[PDCP_PDU_TYPE_N_ITEMS] = {"PDCP Report PDU",
                                                                "Interspersed ROCH Feedback Packet",
                                                                "LWA Status Report",
                                                                "LWA End-marker Packet"};

// Taken from PDCP-Config (TS 38.331 version 15.2.1)
typedef enum  {
	ms0      = 0,
	ms1      = 1,
	ms2      = 2,
	ms4      = 4,
	ms5      = 5,
	ms8      = 8,
	ms10     = 10,
	ms15     = 15,
	ms20     = 20,
	ms30     = 30,
	ms40     = 40,
	ms50     = 50,
	ms60     = 60,
	ms80     = 80,
	ms100    = 100,
	ms120    = 120,
	ms140    = 140,
	ms160    = 160,
	ms180    = 180,
	ms200    = 200,
	ms220    = 220,
	ms240    = 240,
	ms260    = 260,
	ms280    = 280,
	ms300    = 300,
	ms500    = 500,
	ms750    = 750,
	ms1000   = 1000,
	ms1250   = 1250,
	ms1500   = 1500,
	ms1750   = 1750,
	ms2000   = 2000,
	ms2250   = 2250,
	ms2500   = 2500,
	ms2750   = 2750,
	ms3000   = 3000,
	infinity = -1
}pdcp_t_reordering_t;

// Taken from PDCP-Config (TS 38.331 version 15.2.1)
typedef enum  {
	ms10     = 10,
	ms20     = 20,
	ms30     = 30,
	ms40     = 40,
	ms50     = 50,
	ms60     = 60,
	ms75     = 75,
	ms100    = 100,
	ms150    = 150,
	ms200    = 200,
	ms250    = 250,
	ms300    = 300,
	ms500    = 500,
	ms750    = 750,
	ms1500   = 1500,
	infinity = -1
}pdcp_discard_timer_t;

typedef struct {
	uint8_t              bearer_id;//     = 1
	pdcp_rb_type_t       rb_type;//       = PDCP_RB_IS_DRB
	security_direction_t tx_direction;//  = SECURITY_DIRECTION_DOWNLINK
	security_direction_t rx_direction;//  = SECURITY_DIRECTION_UPLINK
	uint8_t              sn_len;//        = PDCP_SN_LEN_12
	uint8_t              hdr_len_bytes;// = 2//pdcp header数据长度占位字节数

	pdcp_t_reordering_t  t_reordering;//  = (pdcp_t_reordering_t)ms500
	pdcp_discard_timer_t discard_timer;// = (pdcp_discard_timer_t)infinity
	srsran_rat_t         rat;//           = (srsran_rat_t)lte

	bool status_report_required;
}pdcp_config_t;

// Specifies in which direction security (integrity and ciphering) are enabled for PDCP
typedef enum { DIRECTION_NONE = 0, DIRECTION_TX, DIRECTION_RX, DIRECTION_TXRX, DIRECTION_N_ITEMS }srsran_direction_t;
static const char* srsran_direction_text[DIRECTION_N_ITEMS] = {"none", "tx", "rx", "tx/rx"};

// PDCP LTE internal state variables, as defined in TS 36 323, section 7.1
typedef struct {
	uint32_t next_pdcp_tx_sn;
	uint32_t tx_hfn;
	uint32_t rx_hfn;
	uint32_t next_pdcp_rx_sn;
	uint32_t last_submitted_pdcp_rx_sn;
	uint32_t reordering_pdcp_rx_count;
}pdcp_lte_state_t;

// Custom type for interface between PDCP and RLC to convey SDU delivery status
// Arbitrarily chosen limit, optimal value depends on the RLC (pollPDU) and PDCP config, channel BLER,
// traffic characterisitcs, etc. The chosen value has been tested with 100 PRB bi-dir TCP
#define MAX_SDUS_TO_NOTIFY (4096)
typedef cvector_vector_t(uint32_t) pdcp_sn_vector_t;//srsran::bounded_vector<uint32_t, MAX_SDUS_TO_NOTIFY>


#endif
