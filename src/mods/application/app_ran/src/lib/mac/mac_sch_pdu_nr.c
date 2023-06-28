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

/// Set the SDU pointer to use the internal buffer.
static uint8_t* use_internal_storage(mac_sch_subpdu_nr	*subpdu)
{
  subpdu->sdu.sdu = subpdu->sdu.ce_write_buffer;
  return subpdu->sdu.sdu;
}

/// Set the SDU pointer to point to the provided buffer.
static  uint8_t* set_storage_to(mac_sch_subpdu_nr	*subpdu, uint8_t* p)
{
	subpdu->sdu.sdu = p;
	return subpdu->sdu.sdu;
}

// Turn subPDU into a Con
static void set_ue_con_res_id_ce(mac_sch_pdu_nr	*mac_pdu_dl, mac_sch_subpdu_nr	*ce, ue_con_res_id_t id)
{
  ce->lcid          = CON_RES_ID;
  ce->header_length = 1; //sub header
  ce->sdu_length    = mac_sch_subpdu_nr_sizeof_ce(ce->lcid, mac_pdu_dl->ulsch);
  uint8_t* ptr  = use_internal_storage(ce);
  for (int32_t i = 0; i < ce->sdu_length; ++i) {
	ptr[i] = id[i];
  }
}

// Section 6.1.2
// 如果一个 MAC PDU 用于传输来自 PCCH 或 BCCH 的数据，则该 MAC PDU 只能包含来自一个
// 逻辑信道的数据。也就是说，PCCH 或 BCCH 的数据不能和其它逻辑信道的数据复用在同一个
// MAC PDU 中传输，因此 PCCH 和 BCCH 并不需要使用 LCID 域，也不需要 MAC subheader。其实
// PCCH 和 BCCH 使用的是 transparent MAC PDU（见 36.321 的 6.1.4 节），该 MAC PDU 只包含一个
// 独立的 MAC SDU。UE 通过 P-RNTI 或 SI-RNTI 就能够区分所携带的数据。简而言之，PCCH 或
// BCCH 在 MAC 层是透传的（见 36.300 的 6.1.2 节）
static uint32_t write_subpdu(mac_sch_subpdu_nr *subpdu, uint8_t *start_)
{
	// –LCID：逻辑信道标识。
	// –L：指示对应的MACSDU、MAC控制单元的长度，以字节为单位。
	// –F：指示L字段的长度。当该值为1时，L字段为8bits。当该值为0时，L字段为16bits
	// –R：保留比特, 0

  // make SDU header
  uint8_t* ptr = (uint8_t*)start_;
  // |R|R|E| LCID |  OCT1
  *ptr         = (uint8_t)((subpdu->F_bit ? 1 : 0) << 6) | ((uint8_t)subpdu->lcid & 0x3f);
  ptr += 1;

  if (subpdu->header_length == 3) {
    // 3 Byte R/F/LCID/L MAC subheader with 16-bit L field
    // |R|F| LCID   |  OCT1
    // |       L    |  OCT2
    // |       L    |  OCT3
    *ptr = ((subpdu->sdu_length & 0xff00) >> 8);
    ptr += 1;
    *ptr = (uint8_t)(subpdu->sdu_length);
    ptr += 1;

  } else if (subpdu->header_length == 2) {
    // 2 Byte R/F/LCID/L MAC subheader with 8-bit L field
    // |R|F| LCID   |  OCT1
    // |  	 L	    |  OCT2
    *ptr = (uint8_t)(subpdu->sdu_length);
    ptr += 1;
  } else if (subpdu->header_length == 1) {
	// |R|R|   LCID |  OCT1
	// eg :UL-SCH CCCH48/CCCH64   DL_SCH UE Contention Resolution Identity
    // do nothing
  } else {
    oset_error("Error while packing PDU. Unsupported header length (%d)", subpdu->header_length);
  }

  // copy SDU payload
  if (subpdu->sdu.sdu) {
    memcpy(ptr, subpdu->sdu.sdu, subpdu->sdu_length);
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

static bool is_ul_ccch(mac_sch_pdu_nr	*mac_pdu_dl, mac_sch_subpdu_nr *sch_pdu)
{
	return (mac_pdu_dl->ulsch && (sch_pdu->lcid == CCCH_SIZE_48 || sch_pdu->lcid == CCCH_SIZE_64));
}

static void set_sdu(mac_sch_pdu_nr	*mac_pdu_dl, mac_sch_subpdu_nr *sch_pdu, uint32_t lcid_, uint8_t* payload_, const uint32_t len_)
{
	// Use CCCH_SIZE_48 when SDU len fits
	sch_pdu->lcid = (lcid_ == CCCH_SIZE_64 && len_ == mac_sch_subpdu_nr_sizeof_ce(CCCH_SIZE_48, true)) ? CCCH_SIZE_48 : lcid_;
	set_storage_to(sch_pdu, payload_);
	sch_pdu->header_length = is_ul_ccch(mac_pdu_dl, sch_pdu) ? 1 : 2;
	sch_pdu->sdu_length    = len_;
	if (is_ul_ccch(mac_pdu_dl, sch_pdu)) {
		sch_pdu->F_bit      = false;
		sch_pdu->sdu_length = mac_sch_subpdu_nr_sizeof_ce(lcid_, mac_pdu_dl->ulsch);
		if (len_ != (uint32_t)(sch_pdu->sdu_length)) {
			oset_warn("Invalid SDU length of UL-SCH SDU (%d != %d)", len_, sch_pdu->sdu_length);
		}
	}

	if (sch_pdu->sdu_length >= MAC_SUBHEADER_LEN_THRESHOLD) {
		sch_pdu->F_bit = true;
		sch_pdu->header_length += 1;
	}
}

static void set_padding(mac_sch_subpdu_nr *sch_pdu, uint32_t len_)
{
  sch_pdu->lcid = PADDING;
  // 1 Byte R/LCID MAC subheader
  sch_pdu->sdu_length    = len_ - 1;
  sch_pdu->header_length = 1;
}


uint32_t mac_sch_pdu_nr_size_header_sdu(mac_sch_pdu_nr	*mac_pdu_dl, uint32_t lcid, uint32_t nbytes)
{
  if (mac_pdu_dl->ulsch && (lcid == CCCH_SIZE_48 || lcid == CCCH_SIZE_64)) {
  	return 1;
  }
  return nbytes < MAC_SUBHEADER_LEN_THRESHOLD ? 2 : 3;
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


uint32_t mac_sch_pdu_nr_add_ue_con_res_id_ce(mac_sch_pdu_nr	*mac_pdu_dl, ue_con_res_id_t id)
{
	mac_sch_subpdu_nr ce = {0};
	set_ue_con_res_id_ce(mac_pdu_dl, &ce, id);
	return add_sudpdu(mac_pdu_dl, &ce);
}

uint32_t mac_sch_pdu_nr_add_sdu(mac_sch_pdu_nr	*mac_pdu_dl, uint32_t lcid_, uint8_t* payload_, uint32_t len_)
{
	int header_size = mac_sch_pdu_nr_size_header_sdu(mac_pdu_dl, lcid_, len_);
	if (header_size + len_ > mac_pdu_dl->remaining_len) {
		oset_error("Header and SDU exceed space in PDU (%d + %d > %d)", header_size, len_,  mac_pdu_dl->remaining_len);
		return OSET_ERROR;
	}
	mac_sch_subpdu_nr sch_pdu = {0};
	set_sdu(mac_pdu_dl, &sch_pdu, lcid_, payload_, len_);
	return add_sudpdu(mac_pdu_dl, &sch_pdu);
}


void mac_sch_pdu_nr_pack(mac_sch_pdu_nr	*mac_pdu_dl)
{
  // SDU and CEs are written in-place, only add padding if needed
  if (mac_pdu_dl->remaining_len) {
    mac_sch_subpdu_nr padding_subpdu = {0};
    set_padding(&padding_subpdu, mac_pdu_dl->remaining_len);
    write_subpdu(&padding_subpdu, mac_pdu_dl->buffer->msg + mac_pdu_dl->buffer->N_bytes);

    // update length
    uint32_t padding_size = padding_subpdu.header_length + padding_subpdu.sdu_length;
    mac_pdu_dl->buffer->N_bytes += padding_size;
    mac_pdu_dl->remaining_len -= padding_size;
	cvector_push_back(mac_pdu_dl->subpdus, padding_subpdu);
  }
}


