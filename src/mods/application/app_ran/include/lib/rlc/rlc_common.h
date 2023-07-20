/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/

#ifndef RLC_COMMON_H_
#define RLC_COMMON_H_

#include "lib/rlc/rlc_metrics.h"
#include "lib/rlc/rlc_interface_types.h"
#include "lib/common/buffer_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Structs and Defines
 * Ref: 3GPP TS 36.322 v10.0.0
 ***************************************************************************/

#define MOD 1024
#define RLC_AM_WINDOW_SIZE 512
#define RLC_MAX_SDU_SIZE ((1 << 11) - 1) // Length of LI field is 11bits
#define RLC_AM_MIN_DATA_PDU_SIZE (3)     // AMD PDU with 10 bit SN (length of LI field is 11 bits) (No LI)

#define RLC_AM_NR_TYP_NACKS 512  // Expected number of NACKs in status PDU before expanding space by alloc
#define RLC_AM_NR_MAX_NACKS 2048 // Maximum number of NACKs in status PDU

#define RlcDebug(...) oset_debug(__VA_ARGS__)
#define RlcInfo(...) oset_info(__VA_ARGS__)
#define RlcWarning(...) oset_warn(__VA_ARGS__)
#define RlcError(...) oset_error(__VA_ARGS__)

#define RlcHexDebug(msg, bytes, ...) (oset_hex_print(OSET_LOG2_DEBUG, msg, bytes) && oset_debug(__VA_ARGS__))
#define RlcHexInfo(msg, bytes, ...) (oset_hex_print(OSET_LOG2_INFO, msg, bytes) && oset_info(__VA_ARGS__))
#define RlcHexWarning(msg, bytes, ...) (oset_hex_print(OSET_LOG2_WARNING, msg, bytes) && oset_warn(__VA_ARGS__))
#define RlcHexError(msg, bytes, ...) (oset_hex_print(OSET_LOG2_ERROR, msg, bytes) && oset_error(__VA_ARGS__))

typedef enum {
  RLC_FI_FIELD_START_AND_END_ALIGNED = 0,
  RLC_FI_FIELD_NOT_END_ALIGNED,
  RLC_FI_FIELD_NOT_START_ALIGNED,
  RLC_FI_FIELD_NOT_START_OR_END_ALIGNED,
  RLC_FI_FIELD_N_ITEMS,
} rlc_fi_field_t;

static const char rlc_fi_field_text[RLC_FI_FIELD_N_ITEMS][32] = {"Start and end aligned",
                                                                 "Not end aligned",
                                                                 "Not start aligned",
                                                                 "Not start or end aligned"};

typedef enum{
  full_sdu                       = 0b00,
  first_segment                  = 0b01,
  last_segment                   = 0b10,
  neither_first_nor_last_segment = 0b11,
  nulltype,
}rlc_nr_si_field_t;
  
inline char* rlc_nr_si_field_to_string(const rlc_nr_si_field_t si)
{
  constexpr static const char* options[] = {"Data field contains full SDU",
                                            "Data field contains first segment of SDU",
                                            "Data field contains last segment of SDU",
                                            "Data field contains neither first nor last segment of SDU"};
  return enum_to_text(options, (rlc_nr_si_field_t)nulltype, (uint32_t)si);
}

inline char* rlc_nr_si_field_to_string_short(rlc_nr_si_field_t si)
{
  constexpr static const char* options[] = {"full", "first", "last", "middle"};
  return enum_to_text(options, (rlc_nr_si_field_t)nulltype, (uint32_t)si);
}

typedef enum{
	status_pdu = 0b000,
	nulltype,
}rlc_am_nr_control_pdu_type_t;

inline char* rlc_am_nr_control_pdu_type_to_string(rlc_am_nr_control_pdu_type_t type)
{
  constexpr static const char* options[] = {"Control PDU"};
  return enum_to_text(options, (rlc_am_nr_control_pdu_type_t)nulltype, (uint32_t)type);
}

typedef enum {
  RLC_DC_FIELD_CONTROL_PDU = 0,
  RLC_DC_FIELD_DATA_PDU,
  RLC_DC_FIELD_N_ITEMS,
} rlc_dc_field_t;
 
static const char rlc_dc_field_text[RLC_DC_FIELD_N_ITEMS][20] = {"Control PDU", "Data PDU"};

// UMD PDU Header
typedef struct {
  uint8_t           fi;                     // Framing info
  rlc_umd_sn_size_t sn_size;                // Sequence number size (5 or 10 bits)
  uint16_t          sn;                     // Sequence number
  uint32_t          N_li;                   // Number of length indicators
  uint16_t          li[RLC_AM_WINDOW_SIZE]; // Array of length indicators
} rlc_umd_pdu_header_t;

typedef struct {
  rlc_nr_si_field_t   si;      // Segmentation info
  rlc_um_nr_sn_size_t sn_size; // Sequence number size (6 or 12 bits)
  uint16_t            sn;      // Sequence number
  uint16_t            so;      // Segment offset
} rlc_um_nr_pdu_header_t;

// AMD PDU Header
typedef struct {
  rlc_dc_field_t dc;//RLC_DC_FIELD_CONTROL_PDU           // Data or control
  uint8_t        rf0;                                    // Resegmentation flag
  uint8_t        p;                                      // Polling bit
  uint8_t        fi;//RLC_FI_FIELD_START_AND_END_ALIGNED // Framing info
  uint16_t       sn;                                     // Sequence number
  uint8_t        lsf;                                    // Last segment flag
  uint16_t       so;                                     // Segment offset
  uint32_t       N_li;                                   // Number of length indicators
  uint16_t       li[RLC_AM_WINDOW_SIZE];                 // Array of length indicators
} rlc_amd_pdu_header_t;

// NACK helper (for LTE and NR)
const static uint16_t so_end_of_sdu = 0xFFFF;

typedef struct {
  uint32_t nack_sn;        // Sequence Number (SN) of first missing SDU
  bool     has_so;         // NACKs continuous sequence of bytes [so_start..so_end]
  uint16_t so_start;       // First missing byte in SDU with SN=nack_sn
  uint16_t so_end;         // Last missing byte in SDU with SN=nack_sn or SN=nack_sn+nack_range-1 if has_nack_range.
  bool     has_nack_range; // NACKs continuous sequence of SDUs
  uint8_t  nack_range;     // Number of SDUs being NACKed (including SN=nack_sn)
}rlc_status_nack_t;

// STATUS PDU
typedef struct {
  uint16_t          ack_sn; // SN of the next not received RLC Data PDU
  uint32_t          N_nack;
  rlc_status_nack_t nacks[RLC_AM_WINDOW_SIZE];
}rlc_status_pdu_t;

/****************************************************************************
 * RLC Common interface
 * Common interface for all RLC entities
 ***************************************************************************/
#define	static_blocking_queue_size 256

typedef struct {
	char*    rb_name;
	bool     suspended;// 暂停
	rlc_mode_t mode;
	oset_apr_memory_pool_t	 *usepool;
	oset_ring_queue_t *rx_pdu_resume_queue;//static_blocking_queue<unique_byte_buffer_t, 256>
	oset_ring_queue_t *tx_sdu_resume_queue;//static_blocking_queue<unique_byte_buffer_t, 256>
}rlc_common;

typedef void (*bsr_callback_t)(uint32_t, uint32_t, uint32_t, uint32_t);

#ifdef __cplusplus
}
#endif

#endif
