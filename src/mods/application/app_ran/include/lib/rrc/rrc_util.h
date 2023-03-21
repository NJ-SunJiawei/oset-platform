/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef RRC_UTIL_H_
#define RRC_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

int rrc_read_pdu_bcch_dlsch(uint32_t sib_index, oset_pkbuf_t *buffer);

#ifdef __cplusplus
}
#endif

#endif
