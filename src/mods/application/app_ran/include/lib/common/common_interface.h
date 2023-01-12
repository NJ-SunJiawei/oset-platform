/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef COMMON_INTERFACE_H_
#define COMMON_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

/***************************
 *        PLMN ID
 **************************/
typedef struct {
  uint8_t mcc[3];
  uint8_t mnc[3];
  uint8_t nof_mnc_digits;
}plmn_id_t;

/***************************
 *        s-TMSI
 **************************/
typedef struct {
  uint8_t  mmec;
  uint32_t m_tmsi;
}s_tmsi_t;


#ifdef __cplusplus
}
#endif

#endif
