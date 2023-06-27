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

enum class rlc_nr_si_field_t : unsigned {
  full_sdu                       = 0b00,
  first_segment                  = 0b01,
  last_segment                   = 0b10,
  neither_first_nor_last_segment = 0b11,
  nulltype
};
inline std::string to_string(const rlc_nr_si_field_t& si)
{
  constexpr static const char* options[] = {"Data field contains full SDU",
                                            "Data field contains first segment of SDU",
                                            "Data field contains last segment of SDU",
                                            "Data field contains neither first nor last segment of SDU"};
  return enum_to_text(options, (uint32_t)rlc_nr_si_field_t::nulltype, (uint32_t)si);
}

inline std::string to_string_short(const rlc_nr_si_field_t& si)
{
  constexpr static const char* options[] = {"full", "first", "last", "middle"};
  return enum_to_text(options, (uint32_t)rlc_nr_si_field_t::nulltype, (uint32_t)si);
}

static inline uint8_t operator&(rlc_nr_si_field_t lhs, int rhs)
{
  return static_cast<uint8_t>(static_cast<std::underlying_type<rlc_nr_si_field_t>::type>(lhs) &
                              static_cast<std::underlying_type<rlc_nr_si_field_t>::type>(rhs));
}

enum class rlc_am_nr_control_pdu_type_t : unsigned { status_pdu = 0b000, nulltype };
inline std::string to_string(const rlc_am_nr_control_pdu_type_t& type)
{
  constexpr static const char* options[] = {"Control PDU"};
  return enum_to_text(options, (uint32_t)rlc_am_nr_control_pdu_type_t::nulltype, (uint32_t)type);
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


#ifdef __cplusplus
}
#endif

#endif
