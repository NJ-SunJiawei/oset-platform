/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#include "gnb_common.h"
#include "lib/mac/mac_sch_pdu_nr.h"


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-lib-mac-sch"


uint32_t mac_sch_subpdu_nr_sizeof_ce(uint32_t lcid, bool is_ul)
{
  if (is_ul) {
    switch (lcid) {
      case CCCH_SIZE_48:
        return 6;
      case CCCH_SIZE_64:
        return 8;
      case CRNTI:
        return 2;
      case SHORT_BSR:
      case SHORT_TRUNC_BSR:
        return 1;
      case SE_PHR:
        return 2;
      case LONG_BSR:
      case LONG_TRUNC_BSR:
        return 1; // minimum size, could be more than that
      case PADDING:
        return 0;
    }
  } else {
    switch (lcid) {
      case CON_RES_ID:
        return 6;
      case TA_CMD:
        return 1;
      case DRX_CMD:
        return 0;
      case PADDING:
        return 0;
    }
  }
  return 0;
}

int mac_sch_pdu_nr_init_tx(mac_sch_pdu_nr	*mac_pdu_dl, byte_buffer_t* buffer_, uint32_t pdu_len_, bool ulsch_)
{
  if (buffer_ == NULL || buffer_->msg == NULL) {
    oset_error("Invalid buffer");
    return OSET_ERROR;
  }
  mac_pdu_dl->buffer = buffer_;
  cvector_clear(mac_pdu_dl->subpdus);
  mac_pdu_dl->pdu_len = pdu_len_;//tbs
  mac_pdu_dl->remaining_len = pdu_len_;
  mac_pdu_dl->ulsch = ulsch_;
  return OSET_OK;
}

// Turn subPDU into a Con
static void set_ue_con_res_id_ce(mac_sch_pdu_nr	*mac_pdu_dl, mac_sch_subpdu_nr	*ce, ue_con_res_id_t id)
{
  ce->lcid          = CON_RES_ID;
  ce->header_length = 1; //sub header
  ce->sdu_length    = mac_sch_subpdu_nr_sizeof_ce(ce->lcid, mac_pdu_dl->ulsch);
  uint8_t* ptr  = ce->sdu.ce_write_buffer;
  for (int32_t i = 0; i < ce->sdu_length; ++i) {
	ptr[i] = id[i];
  }
}

// Section 6.1.2
static uint32_t write_subpdu(mac_sch_subpdu_nr *subpdu, uint8_t *start_)
{
  // make SDU header
  uint8_t* ptr = (uint8_t*)start_;
  // |R|R|E| LCID |  OCT1
  *ptr         = (uint8_t)((subpdu->F_bit ? 1 : 0) << 6) | ((uint8_t)subpdu->lcid & 0x3f);
  ptr += 1;

  if (subpdu->header_length == 3) {
    // 3 Byte R/F/LCID/L MAC subheader with 16-bit L field
    // |R|R|E| LCID |  OCT1
    // |F|     L    |  OCT2
    // |       L    |  OCT3
    *ptr = ((subpdu->sdu_length & 0xff00) >> 8);
    ptr += 1;
    *ptr = (uint8_t)(subpdu->sdu_length);
    ptr += 1;

  } else if (subpdu->header_length == 2) {
    // 2 Byte R/F/LCID/L MAC subheader with 8-bit L field
    // |R|R|E| LCID |  OCT1
    // |F|	 L	    |  OCT2
    *ptr = (uint8_t)(subpdu->sdu_length);
    ptr += 1;
  } else if (subpdu->header_length == 1) {
	// |R|R|E| LCID |  OCT1
    // do nothing
  } else {
    oset_error("Error while packing PDU. Unsupported header length (%d)", subpdu->header_length);
  }

  // copy SDU payload
  if (subpdu->sdu) {
    memcpy(ptr, subpdu->sdu.ce_write_buffer, subpdu->sdu_length);
  } else {
    // clear memory
    memset(ptr, 0, subpdu->sdu_length);
  }

  ptr += subpdu->sdu_length;

  // return total length of subpdu
  return ptr - start_;
}


static uint32_t add_sudpdu(mac_sch_pdu_nr	*mac_pdu_dl, mac_sch_subpdu_nr *subpdu)
{
  uint32_t subpdu_len = subpdu->header_length + subpdu->sdu_length;
  if (subpdu_len > mac_pdu_dl->remaining_len) {
    oset_warn("Not enough space to add subPDU to PDU (%d > %d)", subpdu_len, mac_pdu_dl->remaining_len);
    return OSET_ERROR;
  }

  // Write subPDU straigt into provided buffer
  // 下行MAC PDU要求组装subPDU的时候，CEs优先于SDU，SDU优先于padding
  // 上行MAC PDU要求组装subPDU的时候，SDU优先于CEs，CEs优先于padding
  // |Sub header 1|MAC CE 1|Sub header 2|MAC CE 2|Sub header 3|MAC SDU|Padding|
  write_subpdu(subpdu, mac_pdu_dl->buffer->msg + mac_pdu_dl->buffer->N_bytes);

  // adopt buffer variables
  mac_pdu_dl->buffer->N_bytes += subpdu_len;
  mac_pdu_dl->remaining_len -= subpdu_len;
  cvector_push_back(mac_pdu_dl->subpdus, *subpdu);

  return OSET_OK;
}

uint32_t mac_sch_pdu_nr_add_ue_con_res_id_ce(mac_sch_pdu_nr	*mac_pdu_dl, ue_con_res_id_t id)
{
	mac_sch_subpdu_nr ce = {0};
	set_ue_con_res_id_ce(mac_pdu_dl, &ce, id);
	return add_sudpdu(mac_pdu_dl, &ce);
}

