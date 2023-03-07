/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#ifndef BCD_HELPERS_H
#define BCD_HELPERS_H


#include <ctype.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
 * Convert between string and BCD-coded MCC.
 * Digits are represented by 4-bit nibbles. Unused nibbles are filled with 0xF.
 * MCC 001 results in 0xF001
 *****************************************************************************/
inline bool string_to_mcc(char* str, uint16_t* mcc)
{
  uint32_t len = strlen(str);
  if (len != 3) {
	return false;
  }
  if (!isdigit(str[0]) || !isdigit(str[1]) || !isdigit(str[2])) {
	return false;
  }
  *mcc = 0xF000;
  *mcc |= ((uint8_t)(str[0] - '0') << 8);
  *mcc |= ((uint8_t)(str[1] - '0') << 4);
  *mcc |= ((uint8_t)(str[2] - '0'));
  return true;
}

inline bool mcc_to_string(uint16_t mcc, char* str)
{
  if ((mcc & 0xF000) != 0xF000) {
    return false;
  }
  *str = "";
  *str += ((mcc & 0x0F00) >> 8) + '0';
  *str += ((mcc & 0x00F0) >> 4) + '0';
  *str += (mcc & 0x000F) + '0';
  return true;
}

inline bool bytes_to_mcc(uint8_t* bytes, uint16_t* mcc)
{
  *mcc = 0xF000;
  *mcc |= (((uint16_t)bytes[0]) << 8u);
  *mcc |= (((uint16_t)bytes[1]) << 4u);
  *mcc |= (uint16_t)bytes[2];
  return true;
}


inline bool mcc_to_bytes(uint16_t mcc, uint8_t* bytes)
{
  if ((mcc & 0xF000) != 0xF000) {
    return false;
  }
  bytes[0] = (uint8_t)((mcc & 0xF00) >> 8);
  bytes[1] = (uint8_t)((mcc & 0x0F0) >> 4);
  bytes[2] = (uint8_t)(mcc & 0x00F);
  return true;
}

/******************************************************************************
 * Convert between string and BCD-coded MNC.
 * Digits are represented by 4-bit nibbles. Unused nibbles are filled with 0xF.
 * MNC 001 results in 0xF001
 * MNC 01 results in 0xFF01
 *****************************************************************************/
inline bool string_to_mnc(char* str, uint16_t* mnc)
{
  uint32_t len = strlen(str);
  if (len != 3 && len != 2) {
    return false;
  }
  if (len == 3) {
    if (!isdigit(str[0]) || !isdigit(str[1]) || !isdigit(str[2])) {
      return false;
    }
    *mnc = 0xF000;
    *mnc |= ((uint8_t)(str[0] - '0') << 8);
    *mnc |= ((uint8_t)(str[1] - '0') << 4);
    *mnc |= ((uint8_t)(str[2] - '0'));
  }
  if (len == 2) {
    if (!isdigit(str[0]) || !isdigit(str[1])) {
      return false;
    }
    *mnc = 0xFF00;
    *mnc |= ((uint8_t)(str[0] - '0') << 4);
    *mnc |= ((uint8_t)(str[1] - '0'));
  }

  return true;
}

inline bool mnc_to_string(uint16_t mnc, char* str)
{
  if ((mnc & 0xF000) != 0xF000) {
    return false;
  }
  *str = "";
  if ((mnc & 0xFF00) != 0xFF00) {
    *str += ((mnc & 0x0F00) >> 8) + '0';
  }
  *str += ((mnc & 0x00F0) >> 4) + '0';
  *str += (mnc & 0x000F) + '0';
  return true;
}

inline bool bytes_to_mnc(uint8_t* bytes, uint16_t* mnc, uint8_t len)
{
  if (len != 3 && len != 2) {
    *mnc = 0;
    return false;
  } else if (len == 3) {
    *mnc = 0xF000;
    *mnc |= ((uint16_t)bytes[0]) << 8u;
    *mnc |= ((uint16_t)bytes[1]) << 4u;
    *mnc |= ((uint16_t)bytes[2]) << 0u;
  } else if (len == 2) {
    *mnc = 0xFF00;
    *mnc |= ((uint16_t)bytes[0]) << 4u;
    *mnc |= ((uint16_t)bytes[1]) << 0u;
  }
  return true;
}


inline bool mnc_to_bytes(uint16_t mnc, uint8_t* bytes, uint8_t* len)
{
  if ((mnc & 0xF000) != 0xF000) {
    *len = 0;
    return false;
  }
  uint8_t count = 0;
  if ((mnc & 0xFF00) != 0xFF00) {
    bytes[count++] = (mnc & 0xF00) >> 8u;
  }
  bytes[count++] = (mnc & 0x00F0) >> 4u;
  bytes[count++] = (mnc & 0x000F);
  *len           = count;
  return true;
}


#ifdef __cplusplus
}
#endif

#endif
