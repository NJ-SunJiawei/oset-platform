/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#ifndef RLC_INTERFACE_TYPES_H
#define RLC_INTERFACE_TYPES_H

#include "lib/rrc/rrc_interface_types.h"

/***************************
 *      RLC Config
 **************************/
typedef enum { tm, um, am, nulltype } rlc_mode_t;
inline const char* rlc_mode_to_string(rlc_mode_t mode, bool long_txt = true)
{
  static const char* long_options[]  = {"Transparent Mode", "Unacknowledged Mode", "Acknowledged Mode"};
  static const char* short_options[] = {"TM", "UM", "AM"};
  if (long_txt) {
    return enum_to_text(long_options, (rlc_mode_t)nulltype, (uint32_t)mode);
  }
  return enum_to_text(short_options, (rlc_mode_t)nulltype, (uint32_t)mode);
}

typedef enum { size5bits, size10bits, nulltype } rlc_umd_sn_size_t;
inline const char* rlc_umd_sn_size_to_string(rlc_umd_sn_size_t sn_size)
{
  static const char* options[] = {"5 bits", "10 bits"};
  return enum_to_text(options, (rlc_mode_t)nulltype, (uint32_t)sn_size);
}
inline uint16_t rlc_umd_sn_size_to_number(rlc_umd_sn_size_t sn_size)
{
  static uint16_t options[] = {5, 10};
  return enum_to_number(options, (rlc_mode_t)nulltype, (uint32_t)sn_size);
}

///< RLC UM NR sequence number field
typedef enum { size6bits, size12bits, nulltype } rlc_um_nr_sn_size_t;
inline const char* rlc_um_nr_sn_size_to_string(rlc_um_nr_sn_size_t sn_size)
{
  static const char* options[] = {"6 bits", "12 bits"};
  return enum_to_text(options, (rlc_mode_t)nulltype, (uint32_t)sn_size);
}
inline uint16_t rlc_um_nr_sn_size_to_number(rlc_um_nr_sn_size_t sn_size)
{
  static uint16_t options[] = {6, 12};
  return enum_to_number(options, (rlc_mode_t)nulltype, (uint32_t)sn_size);
}

///< RLC AM NR sequence number field
typedef enum  { size12bits, size18bits, nulltype } rlc_am_nr_sn_size_t;
inline const char* rlc_am_nr_sn_size_to_string(rlc_am_nr_sn_size_t sn_size)
{
  static const char* options[] = {"12 bits", "18 bits"};
  return enum_to_text(options, (rlc_mode_t)nulltype, (uint32_t)sn_size);
}
inline uint16_t rlc_am_nr_sn_size_to_number(rlc_am_nr_sn_size_t sn_size)
{
  uint16_t options[] = {12, 18};
  return enum_to_number(options, (rlc_mode_t)nulltype, (uint32_t)sn_size);
}
/**
 * @brief Value range of the serial numbers
 * @param sn_size Length of the serial number field in bits
 * @return cardianlity
 */
inline uint32_t cardinality(rlc_am_nr_sn_size_t sn_size)
{
  return (1 << rlc_am_nr_sn_size_to_number(sn_size));
}
/****************************************************************************
 * Tx constants
 * Ref: 3GPP TS 38.322 version 16.2.0 Section 7.2
 ***************************************************************************/
inline uint32_t am_window_size(rlc_am_nr_sn_size_t sn_size)
{
  return cardinality(sn_size) / 2;
}

typedef struct {
  /****************************************************************************
   * Configurable parameters
   * Ref: 3GPP TS 36.322 v10.0.0 Section 7
   ***************************************************************************/

  // TX configs
  int32_t  t_poll_retx;     // Poll retx timeout (ms)
  int32_t  poll_pdu;        // Insert poll bit after this many PDUs
  int32_t  poll_byte;       // Insert poll bit after this much data (KB)
  uint32_t max_retx_thresh; // Max number of retx

  // RX configs
  int32_t t_reordering;      // Timer used by rx to detect PDU loss  (ms)
  int32_t t_status_prohibit; // Timer used by rx to prohibit tx of status PDU (ms)
}rlc_am_config_t;

typedef struct {
  /****************************************************************************
   * Configurable parameters
   * Ref: 3GPP TS 38.322 Section 7
   ***************************************************************************/

  rlc_am_nr_sn_size_t tx_sn_field_length; // Number of bits used for tx (UL) sequence number
  rlc_am_nr_sn_size_t rx_sn_field_length; // Number of bits used for rx (DL) sequence number

  // Timers Ref: 3GPP TS 38.322 Section 7.3
  int32_t t_poll_retx;       // Poll retx timeout (ms)
  int32_t t_reassembly;      // Timer used by rx to detect PDU loss  (ms)
  int32_t t_status_prohibit; // Timer used by rx to prohibit tx of status PDU (ms)

  // Configurable Parameters. Ref: 3GPP TS 38.322 Section 7.4
  uint32_t max_retx_thresh; // Max number of retx
  int32_t  poll_pdu;        // Insert poll bit after this many PDUs
  int32_t  poll_byte;       // Insert poll bit after this much data (KB)
}rlc_am_nr_config_t;

typedef struct {
  /****************************************************************************
   * Configurable parameters
   * Ref: 3GPP TS 36.322 v10.0.0 Section 7
   ***************************************************************************/

  int32_t           t_reordering;       // Timer used by rx to detect PDU loss  (ms)
  rlc_umd_sn_size_t tx_sn_field_length; // Number of bits used for tx (UL) sequence number
  rlc_umd_sn_size_t rx_sn_field_length; // Number of bits used for rx (DL) sequence number

  uint32_t rx_window_size;
  uint32_t rx_mod; // Rx counter modulus
  uint32_t tx_mod; // Tx counter modulus
  bool     is_mrb; // Whether this is a multicast bearer
}rlc_um_config_t;

typedef struct {
  /****************************************************************************
   * Configurable parameters
   * Ref: 3GPP TS 38.322 v15.3.0 Section 7
   ***************************************************************************/

  rlc_um_nr_sn_size_t sn_field_length; // Number of bits used for sequence number
  int32_t             t_reassembly_ms; // Timer used by rx to detect PDU loss (ms)
  uint8_t             bearer_id;       // This is not in the 3GPP TS 38.322
}rlc_um_nr_config_t;

#define RLC_TX_QUEUE_LEN (256)

typedef struct  
{
  srsran_rat_t       rat;
  rlc_mode_t         rlc_mode;
  rlc_am_config_t    am;
  rlc_am_nr_config_t am_nr;
  rlc_um_config_t    um;
  rlc_um_nr_config_t um_nr;
  uint32_t           tx_queue_length;
}rlc_config_t;

#endif
