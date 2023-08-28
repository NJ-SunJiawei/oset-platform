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

#include "lib/common/security_private.h"
#include "lib/common/util_helper.h"
#include "lib/pdcp/pdcp_metrics.h"
#include "lib/pdcp/pdcp_interface_types.h"

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

/****************************************************************************
 * PDCP Entity interface
 * Common interface for LTE and NR PDCP entities
 ***************************************************************************/
typedef struct {
	bool               active;// false
	uint16_t 		   rnti;
	uint32_t           lcid;
	srsran_direction_t integrity_direction;
	srsran_direction_t encryption_direction;
	int32_t            enable_security_tx_sn; // -1 // TX SN at which security will be enabled
	int32_t            enable_security_rx_sn; // -1 // RX SN at which security will be enabled
	pdcp_config_t 	   cfg;
	char               *rb_name;
	struct as_security_config_t   sec_cfg;
	// Metrics helpers
	pdcp_bearer_metrics_t  metrics;
	//rolling_average_t(double) tx_pdu_ack_latency_ms;//lte
}pdcp_entity_base;


bool is_srb(pdcp_entity_base *base);
bool is_drb(pdcp_entity_base *base);
uint32_t pdcp_HFN(pdcp_entity_base *base, uint32_t count);
uint32_t pdcp_SN(pdcp_entity_base *base, uint32_t count);
uint32_t pdcp_COUNT(pdcp_entity_base *base, uint32_t hfn, uint32_t sn);
void pdcp_entity_base_init(pdcp_entity_base *base, uint32_t         lcid_, uint16_t rnti_);
void pdcp_entity_base_stop(pdcp_entity_base *base);
void pdcp_entity_base_config_security(pdcp_entity_base *base, struct as_security_config_t *sec_cfg_);
void pdcp_entity_base_enable_integrity(pdcp_entity_base *base, srsran_direction_t direction);
void pdcp_entity_base_enable_encryption(pdcp_entity_base *base, srsran_direction_t direction);
uint32_t pdcp_entity_base_read_data_header(pdcp_entity_base *base, byte_buffer_t *pdu);
void pdcp_entity_base_cipher_decrypt(pdcp_entity_base *base, uint8_t *ct, uint32_t ct_len, uint32_t count, uint8_t *msg);
void pdcp_entity_base_extract_mac(byte_buffer_t *pdu, uint8_t *mac);
bool pdcp_entity_base_integrity_verify(pdcp_entity_base *base, uint8_t *msg, uint32_t msg_len, uint32_t count, uint8_t *mac);
void pdcp_entity_base_discard_data_header(pdcp_entity_base *base, byte_buffer_t *pdu);
void pdcp_entity_base_write_data_header(pdcp_entity_base *base, byte_buffer_t *sdu, uint32_t count);
#ifdef __cplusplus
}
#endif

#endif
