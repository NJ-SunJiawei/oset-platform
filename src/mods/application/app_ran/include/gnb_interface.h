/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef GNB_INTERFACE_H_
#define GNB_INTERFACE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rach_info_s {
  uint32_t slot_index;
  uint32_t preamble;
  uint32_t time_adv;
}rach_info_t;

#ifdef __cplusplus
}
#endif

#endif
