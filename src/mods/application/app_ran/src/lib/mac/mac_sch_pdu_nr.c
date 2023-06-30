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
#define OSET_LOG2_DOMAIN   "app-gnb-libmacSch"


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
static void set_ue_con_res_id_ce(mac_sch_pdu_nr	*mac_pdu, mac_sch_subpdu_nr	*ce, ue_con_res_id_t id)
{
  ce->lcid          = CON_RES_ID;
  ce->header_length = 1; //sub header
  ce->sdu_length    = mac_sch_subpdu_nr_sizeof_ce(ce->lcid, mac_pdu->ulsch);
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


static uint32_t add_sudpdu(mac_sch_pdu_nr	*mac_pdu, mac_sch_subpdu_nr *subpdu)
{
  uint32_t subpdu_len = subpdu->header_length + subpdu->sdu_length;
  if (subpdu_len > mac_pdu->remaining_len) {
    oset_warn("Not enough space to add subPDU to PDU (%d > %d)", subpdu_len, mac_pdu->remaining_len);
    return OSET_ERROR;
  }

  // Write subPDU straigt into provided buffer
  // 下行MAC PDU要求组装subPDU的时候，CEs优先于SDU，SDU优先于padding
  // 上行MAC PDU要求组装subPDU的时候，SDU优先于CEs，CEs优先于padding
  // |Sub header 1|MAC CE 1|Sub header 2|MAC CE 2|Sub header 3|MAC SDU|Padding|
  write_subpdu(subpdu, mac_pdu->buffer->msg + mac_pdu->buffer->N_bytes);

  // adopt buffer variables
  mac_pdu->buffer->N_bytes += subpdu_len;
  mac_pdu->remaining_len -= subpdu_len;
  cvector_push_back(mac_pdu->subpdus, *subpdu);

  return OSET_OK;
}

static bool is_ul_ccch(mac_sch_pdu_nr	*mac_pdu, mac_sch_subpdu_nr *sch_pdu)
{
	return (mac_pdu->ulsch && (sch_pdu->lcid == CCCH_SIZE_48 || sch_pdu->lcid == CCCH_SIZE_64));
}

bool is_sdu(mac_sch_pdu_nr	*mac_pdu, mac_sch_subpdu_nr *sch_pdu)
{
  // UL-CCCH handling in done as CE
  return (sch_pdu->lcid <= 32 && !is_ul_ccch(mac_pdu, sch_pdu));
}

static void set_sdu(mac_sch_pdu_nr	*mac_pdu, mac_sch_subpdu_nr *sch_pdu, uint32_t lcid_, uint8_t* payload_, const uint32_t len_)
{
	// Use CCCH_SIZE_48 when SDU len fits
	sch_pdu->lcid = (lcid_ == CCCH_SIZE_64 && len_ == mac_sch_subpdu_nr_sizeof_ce(CCCH_SIZE_48, true)) ? CCCH_SIZE_48 : lcid_;
	set_storage_to(sch_pdu, payload_);
	sch_pdu->header_length = is_ul_ccch(mac_pdu, sch_pdu) ? 1 : 2;
	sch_pdu->sdu_length    = len_;
	if (is_ul_ccch(mac_pdu, sch_pdu)) {
		sch_pdu->F_bit      = false;
		sch_pdu->sdu_length = mac_sch_subpdu_nr_sizeof_ce(lcid_, mac_pdu->ulsch);
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

static uint16_t get_c_rnti(mac_sch_pdu_nr *mac_pdu, mac_sch_subpdu_nr *sch_pdu)
{
  if (mac_pdu->ulsch && sch_pdu->lcid == CRNTI) {
    const uint8_t* ptr = sch_pdu->sdu.sdu;
    return le16toh((uint16_t)ptr[0] << 8 | ptr[1]);
  }
  return 0;
}

static ta_t get_ta(mac_sch_subpdu_nr *sch_pdu)
{
  ta_t ta = {0};
  if (sch_pdu->lcid == TA_CMD) {
    uint8_t* ptr  = sch_pdu->sdu.sdu;
    ta.tag_id     = (ptr[0] & 0xc0) >> 6;
    ta.ta_command = ptr[0] & 0x3f;
  }
  return ta;
}

static ue_con_res_id_t get_ue_con_res_id_ce(mac_sch_pdu_nr *mac_pdu, mac_sch_subpdu_nr *sch_pdu)
{
  ue_con_res_id_t id = {0};
  if (!mac_pdu->ulsch && sch_pdu->lcid == CON_RES_ID) {
    const uint8_t* ptr = sch_pdu->sdu.sdu;
    memcpy(id, ptr, ue_con_res_id_len);
  }
  return id;
}

// Short BSR 或 Truncated BSR 格式：只上报一个 LCG 的 BSR。其格式由一个 LCG ID 域和一个对应的 Buffer Size 域组成
// |LCG ID |   Buffer Size|
static lcg_bsr_t get_sbsr(mac_sch_pdu_nr *mac_pdu, mac_sch_subpdu_nr *sch_pdu)
{
  lcg_bsr_t sbsr = {0};
  if (mac_pdu->ulsch && (sch_pdu->lcid == SHORT_BSR || sch_pdu->lcid == SHORT_TRUNC_BSR)) {
    const uint8_t* ptr = sch_pdu->sdu.sdu;
    sbsr.lcg_id        = (ptr[0] & 0xe0) >> 5;
    sbsr.buffer_size   = ptr[0] & 0x1f;
  }
  return sbsr;
}

// Long BSR 格式：
// |LCG7|```|```|```|LCG0|
// |    Buffer Size 1    |
// |    `````````````    |
// |    Buffer Size m    |

// LCG ID 域长为 2 比特，指定了上报的 buffer 状态对应的 LCG，其值与 IE: LogicalChannelConfig 
// 的 logicalChannelGroup 字段对应
static lbsr_t get_lbsr(mac_sch_pdu_nr *mac_pdu, mac_sch_subpdu_nr *sch_pdu)
{
  lbsr_t lbsr = {0};
  cvector_reserve(lbsr.list, max_num_lcg_lbsr)

  if (mac_pdu->ulsch && (sch_pdu->lcid == LONG_BSR || sch_pdu->lcid == LONG_TRUNC_BSR)) {
    const uint8_t* ptr = sch_pdu->sdu.sdu;
    lbsr.bitmap        = *ptr; // read LCG bitmap
    ptr++;                     // skip LCG bitmap

    // early stop if LBSR is empty
    if (lbsr.bitmap == 0) {
      return lbsr;
    }

    int bsr_cnt = 0;
    for (int i = 0; i < max_num_lcg_lbsr; i++) {
      // If LCGi bit is enabled, it means the next 8-bit BSR value corresponds to it
      if (lbsr.bitmap & (0x1 << i)) {
        lcg_bsr_t bsr = {0};
        bsr.lcg_id    = i;
        // For the Long truncated, some BSR words can be not present, assume BSR > 0 in that case
        if (1 + bsr_cnt < sch_pdu->sdu_length) {
          bsr.buffer_size = ptr[bsr_cnt];
          bsr_cnt++;
        } else if (sch_pdu->lcid == LONG_TRUNC_BSR) {
          bsr.buffer_size = 63; // just assume it has 526 bytes to transmit
        } else {
          oset_error"Error parsing LongBSR CE: sdu_length=%d but there are %d active bsr", sch_pdu->sdu_length, bsr_cnt);
        }
		cvector_push_back(lbsr.list, bsr)
      }
    }
  }

  return lbsr;
}

static uint8_t get_phr(mac_sch_pdu_nr *mac_pdu, mac_sch_subpdu_nr *sch_pdu)
{
  if (mac_pdu->ulsch && sch_pdu->lcid == SE_PHR) {
    uint8_t* ptr = sch_pdu->sdu.sdu;
    return ptr[0] & 0x3f;
  }
  return 0;
}

static uint8_t get_pcmax(mac_sch_pdu_nr *mac_pdu, mac_sch_subpdu_nr *sch_pdu)
{
  if (mac_pdu->ulsch && sch_pdu->lcid == SE_PHR) {
    uint8_t* ptr = sch_pdu->sdu.sdu;
    return ptr[1] & 0x3f;
  }
  return 0;
}

static void subpdu_to_string(mac_sch_pdu_nr *mac_pdu, mac_sch_subpdu_nr *sch_pdu, char *p, char *last)
{
  // print subPDU
  if (is_sdu(mac_pdu, sch_pdu)) {
  	p = oset_slprintf(p, last, " LCID=%u len=%d", sch_pdu->lcid, sch_pdu->sdu_length);
  } else {
    if (mac_pdu->ulsch) {
      // UL-SCH case
      switch (sch_pdu->lcid) {
        case CCCH_SIZE_48:
          p = oset_slprintf(p, last, " CCCH48: len=%d", sch_pdu->sdu_length);
          break;
        case CCCH_SIZE_64:
          p = oset_slprintf(p, last, " CCCH64: len=%d", sch_pdu->sdu_length);
          break;
        case CRNTI:
          p = oset_slprintf(p, last, " C-RNTI: 0x%x", get_c_rnti(mac_pdu, sch_pdu));
          break;
        case SHORT_TRUNC_BSR:
          p = oset_slprintf(p, last, " SHORT_TRUNC_BSR: len=%d", sch_pdu->sdu_length + sch_pdu->header_length);
          break;
        case LONG_TRUNC_BSR:
          p = oset_slprintf(p, last, " LONG_TRUNC_BSR: len=%d", sch_pdu->sdu_length + sch_pdu->header_length);
          break;
        case SHORT_BSR: {
          lcg_bsr_t sbsr = get_sbsr(mac_pdu, sch_pdu);
          p = oset_slprintf(p, last, " SBSR: lcg=%u bs=%u", sbsr.lcg_id, sbsr.buffer_size);
        } break;
        case LONG_BSR: {
          lbsr_t lbsr = get_lbsr(mac_pdu, sch_pdu);
          p = oset_slprintf(p, last, " LBSR: bitmap=0x%x", lbsr.bitmap);
		  lcg_bsr_t *lcg = NULL;
		  cvector_for_each_in(lcg, lbsr.list){
          	p = oset_slprintf(p, last, " lcg=%u bs=%u", lcg->lcg_id, lcg->buffer_size);
          }
		  cvector_free(lbsr.list);
        } break;
        case SE_PHR:
          p = oset_slprintf(p, last, " SE_PHR: ph=%u pc=%u", get_phr(mac_pdu, sch_pdu), get_pcmax(mac_pdu, sch_pdu));
          break;
        case PADDING:
          p = oset_slprintf(p, last, " PAD: len=%d", sch_pdu->sdu_length);
          break;
        default:
          p = oset_slprintf(p, last, " CE=%u total_len=%d", sch_pdu->lcid, sch_pdu->sdu_length + sch_pdu->header_length);
          break;
      }
    } else {
      // DL-SCH PDU
      switch (sch_pdu->lcid) {
        case TA_CMD:
		  ta_t ta = get_ta(sch_pdu);
          p = oset_slprintf(p, last, " TA: id=%u command=%u", ta.tag_id, ta.ta_command);
          break;
        case CON_RES_ID: {
          ue_con_res_id_t id = get_ue_con_res_id_ce(mac_pdu, sch_pdu);
          p = oset_slprintf(p, last, " CON_RES: id=0x%x%x%x%x%x%x",
                         id[0],
                         id[1],
                         id[2],
                         id[3],
                         id[4],
                         id[5]);
        } break;
        case PADDING:
		  p = oset_slprintf(p, last, " PAD: len=%d", sch_pdu->sdu_length);
          break;
        default:
		  p = oset_slprintf(p, last, " CE=%u total_len=%d", sch_pdu->lcid, sch_pdu->sdu_length + sch_pdu->header_length);
          break;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t mac_sch_pdu_nr_size_header_sdu(mac_sch_pdu_nr	*mac_pdu, uint32_t lcid, uint32_t nbytes)
{
	if (mac_pdu->ulsch && (lcid == CCCH_SIZE_48 || lcid == CCCH_SIZE_64)) {
		return 1;
	}
	return nbytes < MAC_SUBHEADER_LEN_THRESHOLD ? 2 : 3;
}


int mac_sch_pdu_nr_init_tx(mac_sch_pdu_nr	*mac_pdu, byte_buffer_t* buffer_, uint32_t pdu_len_, bool ulsch_)
{
	if (buffer_ == NULL || buffer_->msg == NULL) {
		oset_error("Invalid buffer");
		return OSET_ERROR;
	}
	mac_pdu->buffer = buffer_;
	cvector_clear(mac_pdu->subpdus);
	mac_pdu->pdu_len = pdu_len_;//tbs
	mac_pdu->remaining_len = pdu_len_;
	mac_pdu->ulsch = ulsch_;
	return OSET_OK;
}


uint32_t mac_sch_pdu_nr_add_ue_con_res_id_ce(mac_sch_pdu_nr	*mac_pdu, ue_con_res_id_t id)
{
	mac_sch_subpdu_nr ce = {0};
	set_ue_con_res_id_ce(mac_pdu, &ce, id);
	return add_sudpdu(mac_pdu, &ce);
}

uint32_t mac_sch_pdu_nr_add_sdu(mac_sch_pdu_nr	*mac_pdu, uint32_t lcid_, uint8_t* payload_, uint32_t len_)
{
	int header_size = mac_sch_pdu_nr_size_header_sdu(mac_pdu, lcid_, len_);
	if (header_size + len_ > mac_pdu->remaining_len) {
		oset_error("Header and SDU exceed space in PDU (%d + %d > %d)", header_size, len_,  mac_pdu->remaining_len);
		return OSET_ERROR;
	}
	mac_sch_subpdu_nr sch_pdu = {0};
	set_sdu(mac_pdu, &sch_pdu, lcid_, payload_, len_);
	return add_sudpdu(mac_pdu, &sch_pdu);
}


void mac_sch_pdu_nr_pack(mac_sch_pdu_nr	*mac_pdu)
{
  // SDU and CEs are written in-place, only add padding if needed
  if (mac_pdu->remaining_len) {
    mac_sch_subpdu_nr padding_subpdu = {0};
    set_padding(&padding_subpdu, mac_pdu->remaining_len);
    write_subpdu(&padding_subpdu, mac_pdu->buffer->msg + mac_pdu->buffer->N_bytes);

    // update length
    uint32_t padding_size = padding_subpdu.header_length + padding_subpdu.sdu_length;
    mac_pdu->buffer->N_bytes += padding_size;
    mac_pdu->remaining_len -= padding_size;
	cvector_push_back(mac_pdu->subpdus, padding_subpdu);
  }
}

void mac_sch_pdu_nr_to_string(mac_sch_pdu_nr *mac_pdu, uint16_t rnti)
{
	char dumpstr[512] = {0};
	char *p = NULL, *last = NULL;

	last = dumpstr + 512;
	p = dumpstr;
	
  	p = oset_slprintf(p, last, "%s", mac_pdu->ulsch ? "UL" : "DL");

	mac_sch_subpdu_nr *subpdu = NULL;
	cvector_for_each_in(subpdu, mac_pdu->subpdus){
		subpdu_to_string(mac_pdu, subpdu, p, last);
	}
	oset_debug("0x%x %s", rnti, dumpstr);
}


