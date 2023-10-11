/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.09
************************************************************************/
#include "lib/common/common_nr.h"
#include "rlc/rlc_um.h"
#include "pdcp/pdcp.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-rlcAM"

#define AM_RXTX

//////////////////////////////////////////////////////////////////////////////
uint32_t rlc_am_base_rx_get_status_pdu(rlc_am_nr_status_pdu_t *status, uint32_t max_len);
uint32_t rlc_am_base_rx_get_status_pdu_length(rlc_am_base_rx *base_rx);

/////////////////////////////////////////////////////////////////////////////
//  AMD PDU都有SN，这是因为AM RLC需要基于SN确保每个RLC PDU都成功接收
// SN: 编号
// D/C: AM报文包含两大类，一个是数据报文(1)，一个是控制报文(0)
// SI:  full_sdu 0b00, first_segment 0b01, last_segment 0b10, neither_first_nor_last_segment 0b11
// P(polling): P字段是为重传而准备的, 回复状态状告(1), 不需要回复状态报告(0)
// SO: 用于指示RLC SDU segment在原始RLC SDU中的位置，以byte为单位, 携带RLC SDU第一个分段的UMD PDU不需要SO域

// no so(sn 12 bit)                     no so(sn 18 bit)
// |D/C|P|SI | SN   |                   |D/C|P|SI|R|R| SN |   
// |      SN        |                   |      SN         |
// |      Data      |                   |      SN         |
// |      ...       |                   |      Data       |


// with so(sn 12 bit)                   with so(sn 18 bit)
// |D/C|P|SI | SN   |                   |D/C|P|SI|R|R| SN |
// |      SN        |                   |      SN         |
// |      SO        |                   |      SN         |
// |      SO        |                   |      SO         |
// |      Data      |                   |      SO         |
// |      ...       |                   |      Data       |

///////////////////////////////////////////////////////////////////////////
// D/C: AM报文包含两大类，一个是数据报文(1)，一个是控制报文(0)
// CPT: 字段是为了标识不同种类的控制报文，当前为000
// ACK_SN: 此序列号及之后的报文没有被接收到，也没有明确告知没有接收到
// E1: 表示后面是否跟随一个NACK_SN/E1/E2/E3字段。 跟随(1)
// NACK_SN: 标示丢失没有收到的报文
// E2: 标识是否跟随有SoStart和SoEnd。主要用于报文的不完整接收场景的处理
// E3: 字段表示后面是否跟着关于一连串RLC SDU未被接收的消息。是否有range字段
// NACK range: 字段表示从NACK_SN开始(包括NACK_SN)，有几个连续的RLC SDU丢失
// SOstart, Soend. 表示被RLC接收端发现丢失的SN=NACK_SN的SDU的某个部分。SOstart的值表示该丢失的SDU部分在原始SDU中的哪一个byte处开始.Soend表示哪个byte结束

// acK (sn 12 bit)                      acK (sn 18 bit)
// |D/C|CPT | ACK_SN  |                 |D/C|CPT |    ACK_SN   |
// |     ACK_SN       |                 |         ACK_SN       |
// | E1 |R|R|R|R|R|R|R|                 |  ACK_SN         |E1|R|
// |     NACK_SN      |                 |         NACK_SN      |
// |NACK_SN|E1|E2|E3|R|                 |         NACK_SN      |
// |     NACK_SN      |                 |NACK_SN|E1|E2|E3|R|R|R|
// |NACK_SN|E1|E2|E3|R|                 |         NACK_SN      |
// |     SOstart      |                 |         NACK_SN      |
// |     SOstart      |                 |NACK_SN|E1|E2|E3|R|R|R|
// |     SOend        |                 |         SOstart      |
// |     SOend        |                 |         SOstart      |
// |     NACK_range   |                 |         SOend        |
// |     NACK_SN      |                 |         SOend        |
// |NACK_SN|E1|E2|E3|R|                 |         NACK_range   |
// |      ...         |                 |         NACK_SN      |
//                                      |         NACK_SN      |
//                                      |NACK_SN|E1|E2|E3|R|R|R|
//                                      |          ...         |
////////////////////////////////////////////////////////////////////////////////
static void log_rlc_am_nr_pdu_header_to_string(rlc_am_nr_pdu_header_t *header, char *rb_name)
{
	char buffer[512] = {0};
	char *p = NULL, *last = NULL;

	last = buffer + 512;
	p = buffer;

	p = oset_slprintf(p, last, 
	             "%s: [%s, P=%s, SI=%s, SN_SIZE=%s, SN=%u, SO=%u",
	             rb_name,
	             rlc_dc_field_text[header->dc],
	             (header->p ? "1" : "0"),
	             rlc_nr_si_field_to_string(header->si),
	             rlc_am_nr_sn_size_to_string(header->sn_size),
	             header->sn,
	             header->so);
	p = oset_slprintf(p, last, "]");

	oset_debug("%s", buffer);
}

static uint32_t rlc_am_nr_packed_length(rlc_am_nr_pdu_header_t *header)
{
  uint32_t len = 0;
  if (header->si == (rlc_nr_si_field_t)full_sdu || header->si == (rlc_nr_si_field_t)first_segment) {
    len = 2;
    if (header->sn_size == (rlc_am_nr_sn_size_t)size18bits) {
      len++;
    }
  } else {
    // PDU contains SO
    len = 4;
    if (header->sn_size == (rlc_am_nr_sn_size_t)size18bits) {
      len++;
    }
  }
  return len;
}

static uint32_t write_data_pdu_header(rlc_am_nr_pdu_header_t *header, uint8_t *payload)
{
	// no so(sn 12 bit) 					no so(sn 18 bit)
	// |D/C|P|SI | SN	|					|D/C|P|SI|R|R| SN |   
	// |	  SN		|					|	   SN		  |
	// |	  Data		|					|	   SN		  |
	// |	  ...		|					|	   Data 	  |

	// 携带RLC SDU第一个分段的UMD PDU不需要SO域

	
	// with so(sn 12 bit)					with so(sn 18 bit)
	// |D/C|P|SI | SN	|					|D/C|P|SI|R|R| SN |
	// |	  SN		|					|	   SN		  |
	// |	  SO		|					|	   SN		  |
	// |	  SO		|					|	   SO		  |
	// |	  Data		|					|	   SO		  |
	// |	  ...		|					|	   Data 	  |

	uint8_t *ptr = payload;

	// fixed header part
	*ptr = (header->dc & 0x01) << 7;  ///< 1 bit D/C field
	*ptr |= (header->p & 0x01) << 6;  ///< 1 bit P flag
	*ptr |= (header->si & 0x03) << 4; ///< 2 bits SI

	if (header->sn_size == (rlc_am_nr_sn_size_t)size12bits) {
		// write first 4 bit of SN
		*ptr |= (header->sn >> 8) & 0x0f; // 4 bit SN
		ptr++;
		*ptr = header->sn & 0xff; // remaining 8 bit of SN
		ptr++;
	} else {
		// 18bit SN
		*ptr |= (header->sn >> 16) & 0x3; // 2 bit SN
		ptr++;
		*ptr = header->sn >> 8; // bit 3 - 10 of SN
		ptr++;
		*ptr = (header->sn & 0xff); // remaining 8 bit of SN
		ptr++;
	}

	if (header->so) {
		// write SO
		*ptr = header->so >> 8; // first part of SO
		ptr++;
		*ptr = (header->so & 0xff); // second part of SO
		ptr++;
	}
	return rlc_am_nr_packed_length(header);
}

static uint32_t rlc_am_nr_write_data_pdu_header(rlc_am_nr_pdu_header_t *header, byte_buffer_t* pdu)
{
  // Make room for the header
  uint32_t len = rlc_am_nr_packed_length(header);
  pdu->msg -= len;
  pdu->N_bytes += len;
  write_data_pdu_header(header, pdu->msg);
  return len;
}

/////////////////////////////status/////////////////////////////////////////////
static void log_rlc_am_nr_status_pdu_to_string(char *string, rlc_am_nr_status_pdu_t *status, char *rb_name)
{
	char buffer[512] = {0};
	char *p = NULL, *last = NULL;

	last = buffer + 512;
	p = buffer;

	p = oset_slprintf(p, last, "ACK_SN = %u, N_nack = %d", status->ack_sn, cvector_size(status->nacks));
	if (cvector_size(status->nacks) > 0) {
		p = oset_slprintf(p, last, ", NACK_SN = ");
		for (uint32_t i = 0; i < cvector_size(status->nacks); ++i) {
		  if (status->nacks[i].has_nack_range) {
		    if (status->nacks[i].has_so) {
		      p = oset_slprintf(p, last, 
		                     "[%u %u:%u r%u]",
		                     status->nacks[i].nack_sn,
		                     status->nacks[i].so_start,
		                     status->nacks[i].so_end,
		                     status->nacks[i].nack_range);
		    } else {
		      p = oset_slprintf(p, last, "[%u r%u]", status->nacks[i].nack_sn, status->nacks[i].nack_range);
		    }
		  } else {
		    if (status->nacks[i].has_so) {
		      p = oset_slprintf(p, last, "[%u %u:%u]", status->nacks[i].nack_sn, status->nacks[i].so_start, status->nacks[i].so_end);
		    } else {
		      p = oset_slprintf(p, last, "[%u]", status->nacks[i].nack_sn);
		    }
		  }
		}
	}
	oset_debug("%s: %s status PDU - %s", rb_name, string, buffer);
}

static void rlc_am_nr_status_pdu_reset(rlc_am_nr_status_pdu_t *status)
{
	status->cpt   = (rlc_am_nr_control_pdu_type_t)status_pdu;
	status->ack_sn = INVALID_RLC_SN;
	cvector_clear(status->nacks);//cvector_clear
	status->packed_size = rlc_am_nr_status_pdu_sizeof_header_ack_sn;
}

static bool rlc_am_nr_status_pdu_is_continuous_sequence(rlc_am_nr_status_pdu_t *status, rlc_status_nack_t prev, rlc_status_nack_t now)
{
  // SN must be continuous
  if (now.nack_sn != ((prev.has_nack_range ? prev.nack_sn + prev.nack_range : (prev.nack_sn + 1)) % status->mod_nr)) {
    return false;
  }

  // Segments on left side (if present) must reach the end of sdu
  if (prev.has_so && prev.so_end != so_end_of_sdu) {
    return false;
  }

  // Segments on right side (if present) must start from the beginning
  if (now.has_so && now.so_start != 0) {
    return false;
  }

  return true;
}

static uint32_t rlc_am_nr_status_pdu_nack_size(rlc_am_nr_status_pdu_t *status, rlc_status_nack_t nack)
{
	///////////////////////////////////////////////////////////////////////////
	// D/C: AM报文包含两大类，一个是数据报文(1)，一个是控制报文(0)
	// CPT: 字段是为了标识不同种类的控制报文，当前为000
	// ACK_SN: 此序列号及之后的报文没有被接收到，也没有明确告知没有接收到
	// E1: 表示后面是否跟随一个NACK_SN/E1/E2/E3字段。 跟随(1)
	// NACK_SN: 标示丢失没有收到的报文
	// E2: 标识是否跟随有SoStart和SoEnd。主要用于报文的不完整接收场景的处理
	// E3: 字段表示后面是否跟着关于一连串RLC SDU未被接收的消息。是否有range字段
	// NACK range: 字段表示从NACK_SN开始(包括NACK_SN)，有几个连续的RLC SDU丢失
	// SOstart, Soend. 表示被RLC接收端发现丢失的SN=NACK_SN的SDU的某个部分。SOstart的值表示该丢失的SDU部分在原始SDU中的哪一个byte处开始.Soend表示哪个byte结束

	// acK (sn 12 bit)						acK (sn 18 bit)
	// |D/C|CPT | ACK_SN  | 				|D/C|CPT |	  ACK_SN   |
	// |	 ACK_SN 	  | 				|		  ACK_SN	   |
	// | E1 |R|R|R|R|R|R|R| 				|  ACK_SN		  |E1|R|
	// |	 NACK_SN	  | 				|		  NACK_SN	   |
	// |NACK_SN|E1|E2|E3|R| 				|		  NACK_SN	   |
	// |	 NACK_SN	  | 				|NACK_SN|E1|E2|E3|R|R|R|
	// |NACK_SN|E1|E2|E3|R| 				|		  NACK_SN	   |
	// |	 SOstart	  | 				|		  NACK_SN	   |
	// |	 SOstart	  | 				|NACK_SN|E1|E2|E3|R|R|R|
	// |	 SOend		  | 				|		  SOstart	   |
	// |	 SOend		  | 				|		  SOstart	   |
	// |	 NACK_range   | 				|		  SOend 	   |
	// |	 NACK_SN	  | 				|		  SOend 	   |
	// |NACK_SN|E1|E2|E3|R| 				|		  NACK_range   |
	// |	  ...		  | 				|		  NACK_SN	   |
	//										|		  NACK_SN	   |
	//										|NACK_SN|E1|E2|E3|R|R|R|
	//	

	uint32_t result = status->sn_size == (rlc_am_nr_sn_size_t)size12bits ? rlc_am_nr_status_pdu_sizeof_nack_sn_ext_12bit_sn
	                                                                     : rlc_am_nr_status_pdu_sizeof_nack_sn_ext_18bit_sn;
	if (nack.has_so) {
		result += rlc_am_nr_status_pdu_sizeof_nack_so;
	}
	if (nack.has_nack_range) {
		result += rlc_am_nr_status_pdu_sizeof_nack_range;
	}
	return result;
}


static void rlc_am_nr_status_pdu_push_nack(rlc_am_nr_status_pdu_t *status, rlc_status_nack_t nack)
{  

  if (cvector_size(status->nacks) == 0) {
	cvector_push_back(status->nacks, nack);
    status->packed_size += rlc_am_nr_status_pdu_nack_size(status, nack);
    return;
  }

  rlc_status_nack_t *prev = cvector_back(status->nacks);
  //sn连续判断
  if (rlc_am_nr_status_pdu_is_continuous_sequence(status, *prev, nack) == false) {
    cvector_push_back(status->nacks, nack);
    status->packed_size += rlc_am_nr_status_pdu_nack_size(status, nack);
    return;
  }

  // sn连续
  // expand previous NACK
  // subtract size of previous NACK (add updated size later)
  status->packed_size -= rlc_am_nr_status_pdu_nack_size(status, *prev);

  // enable and update NACK range
  if (nack.has_nack_range == true) {
    if (prev->has_nack_range == true) {
      // [NACK range][NACK range]
      prev->nack_range += nack.nack_range;
    } else {
      // [NACK SDU][NACK range]
      prev->nack_range     = nack.nack_range + 1;
      prev->has_nack_range = true;
    }
  } else {
    if (prev->has_nack_range == true) {
      // [NACK range][NACK SDU]
      prev->nack_range++;
    } else {
      // [NACK SDU][NACK SDU]
      prev->nack_range     = 2;
      prev->has_nack_range = true;
    }
  }

  // enable and update segment offsets (if required)
  if (nack.has_so == true) {
    if (prev->has_so == false) {
      // [NACK SDU][NACK segm]
      prev->has_so   = true;
      prev->so_start = 0;
    }
    // [NACK SDU][NACK segm] or [NACK segm][NACK segm]
    prev->so_end = nack.so_end;
  } else {
    if (prev->has_so == true) {
      // [NACK segm][NACK SDU]
      prev->so_end = so_end_of_sdu;
    }
    // [NACK segm][NACK SDU] or [NACK SDU][NACK SDU]
  }

  // add updated size
  status->packed_size += rlc_am_nr_status_pdu_nack_size(status, *prev);
}

static bool rlc_am_nr_status_pdu_trim(rlc_am_nr_status_pdu_t *status, uint32_t max_packed_size)
{
  if (max_packed_size >= status->packed_size) {
    // no trimming required
    return true;
  }
  if (max_packed_size < rlc_am_nr_status_pdu_sizeof_header_ack_sn) {
    // too little space for smallest possible status PDU (only header + ACK).
    return false;
  }

  // remove NACKs (starting from the back) until it fits into given space
  // note: when removing a NACK for a segment, we have to remove all other NACKs with the same SN as well,
  // see TS 38.322 Sec. 5.3.4:
  //   "set the ACK_SN to the SN of the next not received RLC SDU
  //   which is not indicated as missing in the resulting STATUS PDU."
  rlc_status_nack_t *back =  NULL;
  while (cvector_size(status->nacks) > 0 &&\
  			back =  cvector_back(status->nacks) &&\
  			(status->packed_size > max_packed_size || back->nack_sn == status->ack_sn)) {
    status->packed_size -= rlc_am_nr_status_pdu_push_nack(status, *back);
    status->ack_sn = back->nack_sn;
  	cvector_pop_back(status->nacks);
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////
// D/C: AM报文包含两大类，一个是数据报文(1)，一个是控制报文(0)
// CPT: 字段是为了标识不同种类的控制报文，当前为000
// ACK_SN: 此序列号及之后的报文没有被接收到，也没有明确告知没有接收到
// E1: 表示后面是否跟随一个NACK_SN/E1/E2/E3字段。 跟随(1)
// NACK_SN: 标示丢失没有收到的报文
// E2: 标识是否跟随有SoStart和SoEnd。主要用于报文的不完整接收场景的处理
// E3: 字段表示后面是否跟着关于一连串RLC SDU未被接收的消息。是否有range字段
// NACK range: 字段表示从NACK_SN开始(包括NACK_SN)，有几个连续的RLC SDU丢失
// SOstart, Soend. 表示被RLC接收端发现丢失的SN=NACK_SN的SDU的某个部分。SOstart的值表示该丢失的SDU部分在原始SDU中的哪一个byte处开始.Soend表示哪个byte结束

// acK (sn 12 bit)						acK (sn 18 bit)
// |D/C|CPT | ACK_SN  | 				|D/C|CPT |	  ACK_SN   |
// |	 ACK_SN 	  | 				|		  ACK_SN	   |
// | E1 |R|R|R|R|R|R|R| 				|  ACK_SN		  |E1|R|
// |	 NACK_SN	  | 				|		  NACK_SN	   |
// |NACK_SN|E1|E2|E3|R| 				|		  NACK_SN	   |
// |	 NACK_SN	  | 				|NACK_SN|E1|E2|E3|R|R|R|
// |NACK_SN|E1|E2|E3|R| 				|		  NACK_SN	   |
// |	 SOstart	  | 				|		  NACK_SN	   |
// |	 SOstart	  | 				|NACK_SN|E1|E2|E3|R|R|R|
// |	 SOend		  | 				|		  SOstart	   |
// |	 SOend		  | 				|		  SOstart	   |
// |	 NACK_range   | 				|		  SOend 	   |
// |	 NACK_SN	  | 				|		  SOend 	   |
// |NACK_SN|E1|E2|E3|R| 				|		  NACK_range   |
// |	  ...		  | 				|		  NACK_SN	   |
//										|		  NACK_SN	   |
//										|NACK_SN|E1|E2|E3|R|R|R|
//	
static int32_t rlc_am_nr_write_status_pdu_12bit_sn(rlc_am_nr_status_pdu_t *status_pdu, byte_buffer_t* pdu)
{
  uint8_t *ptr = pdu->msg;

  // fixed header part
  *ptr = 0; ///< 1 bit D/C field and 3bit CPT are all zero

  // write first 4 bit of ACK_SN
  *ptr |= (status_pdu->ack_sn >> 8) & 0x0f; // 4 bit ACK_SN
  ptr++;
  *ptr = status_pdu->ack_sn & 0xff; // remaining 8 bit of SN
  ptr++;

  // write E1 flag in octet 3
  if (cvector_size(status_pdu->nacks) > 0) {
    *ptr = 0x80;
  } else {
    *ptr = 0x00;
  }
  ptr++;

  if (cvector_size(status_pdu->nacks) > 0) {
    for (uint32_t i = 0; i < cvector_size(status_pdu->nacks); i++) {
      // write first 8 bit of NACK_SN
      *ptr = (status_pdu->nacks[i].nack_sn >> 4) & 0xff;
      ptr++;

      // write remaining 4 bits of NACK_SN
      *ptr = (status_pdu->nacks[i].nack_sn & 0x0f) << 4;
      // Set E1 if necessary
      if (i < (uint32_t)(cvector_size(status_pdu->nacks) - 1)) {
        *ptr |= 0x08;
      }

      if (status_pdu->nacks[i].has_so) {
        // Set E2
        *ptr |= 0x04;
      }

      if (status_pdu->nacks[i].has_nack_range) {
        // Set E3
        *ptr |= 0x02;
      }

      ptr++;
      if (status_pdu->nacks[i].has_so) {
        (*ptr) = status_pdu->nacks[i].so_start >> 8;
        ptr++;
        (*ptr) = status_pdu->nacks[i].so_start;
        ptr++;
        (*ptr) = status_pdu->nacks[i].so_end >> 8;
        ptr++;
        (*ptr) = status_pdu->nacks[i].so_end;
        ptr++;
      }
      if (status_pdu->nacks[i].has_nack_range) {
        (*ptr) = status_pdu->nacks[i].nack_range;
        ptr++;
      }
    }
  }

  pdu->N_bytes = ptr - pdu->msg;

  return OSET_OK;
}

static int32_t rlc_am_nr_write_status_pdu_18bit_sn(rlc_am_nr_status_pdu_t *status_pdu, byte_buffer_t* pdu)
{
  uint8_t* ptr = pdu->msg;

  // fixed header part
  *ptr = 0; ///< 1 bit D/C field and 3bit CPT are all zero

  *ptr |= (status_pdu->ack_sn >> 14) & 0x0F; // upper 4 bits of SN
  ptr++;
  *ptr = (status_pdu->ack_sn >> 6) & 0xFF; // center 8 bits of SN
  ptr++;
  *ptr = (status_pdu->ack_sn << 2) & 0xFC; // lower 6 bits of SN

  // set E1 flag if necessary
  if (cvector_size(status_pdu->nacks) > 0) {
    *ptr |= 0x02;
  }
  ptr++;

  if (cvector_size(status_pdu->nacks) > 0) {
    for (uint32_t i = 0; i < cvector_size(status_pdu->nacks); i++) {
      *ptr = (status_pdu->nacks[i].nack_sn >> 10) & 0xFF; // upper 8 bits of SN
      ptr++;
      *ptr = (status_pdu->nacks[i].nack_sn >> 2) & 0xFF; // center 8 bits of SN
      ptr++;
      *ptr = (status_pdu->nacks[i].nack_sn << 6) & 0xC0; // lower 2 bits of SN

      if (i < (uint32_t)(cvector_size(status_pdu->nacks) - 1)) {
        *ptr |= 0x20; // Set E1
      }
      if (status_pdu->nacks[i].has_so) {
        *ptr |= 0x10; // Set E2
      }
      if (status_pdu->nacks[i].has_nack_range) {
        *ptr |= 0x08; // Set E3
      }

      ptr++;
      if (status_pdu->nacks[i].has_so) {
        (*ptr) = status_pdu->nacks[i].so_start >> 8;
        ptr++;
        (*ptr) = status_pdu->nacks[i].so_start;
        ptr++;
        (*ptr) = status_pdu->nacks[i].so_end >> 8;
        ptr++;
        (*ptr) = status_pdu->nacks[i].so_end;
        ptr++;
      }
      if (status_pdu->nacks[i].has_nack_range) {
        (*ptr) = status_pdu->nacks[i].nack_range;
        ptr++;
      }
    }
  }

  pdu->N_bytes = ptr - pdu->msg;

  return OSET_OK;
}


/**
 * Write a RLC AM NR status PDU to a PDU buffer and eets the length of the generate PDU accordingly
 * @param status_pdu The status PDU
 * @param pdu A pointer to a unique bytebuffer
 * @return OSET_OK if PDU was written, SRSRAN_ERROR otherwise
 */
static int32_t rlc_am_nr_write_status_pdu(rlc_am_nr_status_pdu_t *status_pdu,
                                   rlc_am_nr_sn_size_t     sn_size,
                                   byte_buffer_t*          pdu)
{
  if (sn_size == (rlc_am_nr_sn_size_t)size12bits) {
    return rlc_am_nr_write_status_pdu_12bit_sn(status_pdu, pdu);
  } else { // 18bit SN
    return rlc_am_nr_write_status_pdu_18bit_sn(status_pdu, pdu);
  }
}

#if AM_RXTX
/*static int modulus_tx(nr_rlc_entity_am_t *entity, int a)
{
  int r = a - entity->tx_next_ack;
  if (r < 0) r += entity->sn_modulus;
  return r;
}*/

static uint32_t TX_MOD_NR_BASE(rlc_am_nr_tx *tx, uint32_t sn)
{
  return (sn - tx->st.tx_next_ack) % tx->mod_nr;
}

static rlc_amd_tx_pdu_nr* tx_window_has_sn(rlc_am_nr_tx *tx, uint32_t sn)
{
	/* as per 38.322 7.1, modulus base is rx_next */
	return oset_hash_get(tx->tx_window, &sn, sizeof(sn));
}

static void empty_queue_no_lock(rlc_am_base_tx *base_tx)
{
	// Drop all messages in TX queue
	rlc_um_base_tx_sdu_t *next_node, *node = NULL;
	oset_list_for_each_safe(&base_tx->tx_sdu_queue, next_node, node){
			byte_buffer_t *sdu = node->buffer;
			oset_list_remove(&base_tx->tx_sdu_queue, node);
			oset_free(node);
			oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
			base_tx->unread_bytes -= sdu->N_bytes;
			oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
			oset_free(sdu)
	}

	oset_assert(0 == base_tx->unread_bytes);
	oset_assert(0 == oset_list_count(base_tx->tx_sdu_queue));
}

static void empty_queue(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	empty_queue_no_lock(base_tx);
	oset_thread_mutex_unlock(&base_tx->mutex);
}

// polling的目的：发送端获取接收端的RLC PDU接收状态，从而进行数据重传；

// polling的触发条件：
// 累计传输AMD PDU（不包括重传）个数不小于预设阈值，即PDU_WITHOUT_POLL >= pollPDU；
// 累计传输AMD PDU（不包括重传）总字节数不小于预设阈值，即BYTE_WITHOUT_POLL >= pollByte；
// 传输完当前AMD PDU后传输buffer和重传buffer（等待ACK/NACK的RLC SDU或RLC SDU segment 除外）为空；
// 传输完当前AMD PDU后不能再传输新的AMD PDU（如，传输窗口stalling）

// t-PollRetransmit 定时器：
// P域设置为“1”的AMD PDU递交下层后，启动或重启t-PollRetransmit定时器，并记录POLL_SN；
// 当接收到反馈(NACK/ACK)的AMD PDU SN等于POLL_SN时，停止t-PollRetransmit定时器；
// 当t-PollRetransmit定时器超时，如果没有待发数据，考虑重传POLL_SN对应的AMD PDU，同时设置P域；或者重传指示为NACK的AMD PDU并设置P域：

/*
 * Check whether the polling bit needs to be set, as specified in
 * TS 38.322, section 5.3.3.2
 */
uint8_t get_pdu_poll(rlc_am_base_tx *base_tx, uint32_t sn, bool is_retx, uint32_t sdu_bytes)
{
  rlc_am_nr_tx *tx = base_tx->tx;
  rlc_am_base *base = base_tx->base;

  RlcDebug("Checking poll bit requirements for PDU. SN=%d, retx=%s, sdu_bytes=%d, POLL_SN=%d",
           sn,
           is_retx ? "true" : "false",
           sdu_bytes,
           tx->st.poll_sn);
  /* For each AMD PDU or AMD PDU segment that has not been previoulsy tranmitted:
   * - increment PDU_WITHOUT_POLL by one;
   * - increment BYTE_WITHOUT_POLL by every new byte of Data field element that it maps to the Data field of the AMD
   * PDU;
   *   - if PDU_WITHOUT_POLL >= pollPDU; or
   *   - if BYTE_WITHOUT_POLL >= pollByte:
   *   	- include a poll in the AMD PDU as described below.
   */
	// 累计传输AMD PDU（不包括重传）个数不小于预设阈值，即PDU_WITHOUT_POLL >= pollPDU；
	// 累计传输AMD PDU（不包括重传）总字节数不小于预设阈值，即BYTE_WITHOUT_POLL >= pollByte；
  uint8_t poll = 0;
  if (!is_retx) {
    tx->st.pdu_without_poll++;
    tx->st.byte_without_poll += sdu_bytes;
    if (base_tx->cfg.poll_pdu > 0 && tx->st.pdu_without_poll >= (uint32_t)base_tx->cfg.poll_pdu) {
      poll = 1;
      RlcDebug("Setting poll bit due to PollPDU. SN=%d, POLL_SN=%d", sn, tx->st.poll_sn);
    }
    if (base_tx->cfg.poll_byte > 0 && tx->st.byte_without_poll >= (uint32_t)base_tx->cfg.poll_byte) {
      poll = 1;
      RlcDebug("Setting poll bit due to PollBYTE. SN=%d, POLL_SN=%d", sn, tx->st.poll_sn);
    }
  }

  /*
   * - if both the transmission buffer and the retransmission buffer becomes empty
   *   (excluding transmitted RLC SDUs or RLC SDU segments awaiting acknowledgements)
   *   after the transmission of the AMD PDU; or
   * - if no new RLC SDU can be transmitted after the transmission of the AMD PDU (e.g. due to window stalling);
   *   - include a poll in the AMD PDU as described below.
   */
  // 传输完当前AMD PDU后传输buffer和重传buffer（等待ACK/NACK的RLC SDU或RLC SDU segment 除外）为空；
  // 传输完当前AMD PDU后不能再传输新的AMD PDU（如，传输窗口stalling）
  // 因为在发送完最后一个新传RLC data PDU后，UE有可能很长时间都不会发送新的数据了，这时候前两个条件就无法触发了。因此，这时候需要向网络发送polling，获取网络的接收状态报告，然后及时地将前面还没发送成功的包给发送出去。
  if (oset_list_empty(&base_tx->tx_sdu_queue) && oset_list_empty(&tx->retx_queue) && tx->sdu_under_segmentation_sn == INVALID_RLC_SN) ||
      oset_hash_count(tx->tx_window) > base->cfg.tx_queue_length) {
    RlcDebug("Setting poll bit due to empty buffers/inablity to TX. SN=%d, POLL_SN=%d", sn, tx->st.poll_sn);
    poll = 1;
  }

  /*
   * - If poll bit is included:
   *     - set PDU_WITHOUT_POLL to 0;
   *     - set BYTE_WITHOUT_POLL to 0.
   */
  if (poll == 1) {
    tx->st.pdu_without_poll  = 0;
    tx->st.byte_without_poll = 0;
    /*
     * - set POLL_SN to the highest SN of the AMD PDU among the AMD PDUs submitted to lower layer;
     * - if t-PollRetransmit is not running:
     *   - start t-PollRetransmit.
     * - else:
     *   - restart t-PollRetransmit.
     */

	// t-PollRetransmit 定时器：
	// P域设置为“1”的AMD PDU递交下层后，启动或重启t-PollRetransmit定时器，并记录POLL_SN；
	// 当接收到反馈(NACK/ACK)的AMD PDU SN等于POLL_SN时，停止t-PollRetransmit定时器；
	// 当t-PollRetransmit定时器超时，如果没有待发数据，考虑重传POLL_SN对应的AMD PDU，同时设置P域；或者重传指示为NACK的AMD PDU并设置P域：


	// 假如在上面的polling触发，并且重传RLC PDU后，网络侧还是有一些包没收到，而此时UE不再有新的包发送了，那么网络侧岂不是再也收不到这些没收到的包了？针对这个问题，RLC层提供了最后一个polling触发条件，保证了在这种场景下，UE还能向网络发送这些丢失的包。UE在RLC层有一个 t-PollRetransmit 定时器，每当UE发送了polling后，都会启动 t-PollRetransmit 定时器(或者重启，假如此时定时器还在运行)。当 t-PollRetransmit 定时器超时时，假如
    //    buffer(包括RLC data PDU新传队列和重传队列)为空
    //    或者无法发送新传RLC PDU(比如此时因为收不到给VT(A)的ACK，导致window stall了) 
	// 那么UE需要向网络重传一包RLC data PDU，并将该RLC data PDU的P位置1，向网络发送polling，这样就保证了在无法发送新传的数据时，UE还能收到网络的状态报告，从而重传之前丢失的包，补齐网络侧的接收窗口，而不至于让网络侧长时间无法完全收到这些包。

    if (!is_retx) {
      // This is not an RETX, but a new transmission
      // As such it should be the highest SN submitted to the lower layers
      tx->st.poll_sn = sn;
      RlcDebug("Setting new POLL_SN. POLL_SN=%d", sn);
    }

    if (base_tx->cfg.t_poll_retx > 0) {
	  gnb_timer_start(tx->poll_retransmit_timer, base_tx->cfg.t_poll_retx);
      RlcInfo("Started t-PollRetransmit. POLL_SN=%d", tx->st.poll_sn);
    }
  }
  return poll;
}


// t-StatusProhibit ARQ-状态报告定时器(防止频繁上报):
//    状态报告触发后，如果t-StatusProhibit 定时器没有运行，则在新传时（MAC 指示的），将构建的STATUS PDU递交底层，同时开启定时器；
//    状态报告触发后，如果t-StatusProhibit 定时器在运行，状态报告是不能发送的。需要等到定时器超时后，在第一个传输机会到来时，将构建的STATUS PDU递交给底层，同时开启定时器；
//  构建STATUS PDU：
//    需要注意的是，t-Reassembly定时器超时会触发状态变量（RX_Highest_Status）更新以及状态报告。因此，应该以更新状态变量后的包接收状态来构建STATUS PDU(可理解为状态报告的触发点在状态变量更新之后)

bool do_status(rlc_am_base_rx *base_rx)
{
  return base_rx->do_status && !base_rx->rx->status_prohibit_timer.running;
}

static byte_buffer_t* tx_sdu_queue_read(rlc_am_base_tx *base_tx)
{
	rlc_am_base_tx_sdu_t *node = oset_list_first(&base_tx->tx_sdu_queue);
	if(node){
		byte_buffer_t *sdu = node->buffer;
		oset_list_remove(&base_tx->tx_sdu_queue, node);
		oset_free(node);
		oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
		base_tx->unread_bytes -= sdu->N_bytes;
		oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
		return sdu;
	}
	return NULL;
}

static bool is_retx_segmentation_required(rlc_am_base_tx *base_tx, rlc_amd_retx_nr_t *retx, uint32_t nof_bytes)
{
	rlc_am_nr_tx *tx = base_tx->tx;
	bool segmentation_required = false;

	if (retx->base.is_segment) {
		uint32_t expected_hdr_size = retx->base.current_so == 0 ? tx->min_hdr_size : tx->max_hdr_size;
		if (nof_bytes < ((retx->base.so_start + retx->segment_length - retx->base.current_so) + expected_hdr_size)) {
		  RlcInfo("Re-segmentation required for RETX. SN=%d", retx->base.sn);
		  segmentation_required = true;
		}
	} else {
		rlc_amd_tx_pdu_nr *pdu = tx_window_has_sn(tx, retx->base.sn);
		if (nof_bytes < (pdu->sdu_buf->N_bytes + tx->min_hdr_size)) {
		  RlcInfo("Segmentation required for RETX. SN=%d", retx->base.sn);
		  segmentation_required = true;
		}
	}
	return segmentation_required;
}

static uint32_t get_retx_expected_hdr_len(rlc_am_base_tx *base_tx, rlc_amd_retx_nr_t *retx)
{
	rlc_am_nr_tx *tx = base_tx->tx;

	uint32_t expected_hdr_len = tx->min_hdr_size;
	if (retx->base.is_segment && retx->base.current_so != 0) {
		expected_hdr_len = tx->max_hdr_size;
	}
	return expected_hdr_len;
}


static rlc_amd_retx_nr_t* retx_queue_pop_first(rlc_am_nr_tx *tx)
{
	rlc_amd_retx_nr_t *node = oset_list_first(&tx->retx_queue);
	if(node){
		oset_list_remove(&tx->retx_queue, node);
		return node;
	}
	return NULL;
}

static void rlc_am_base_tx_discard_sdu(rlc_am_base_tx *base_tx, uint32_t discard_sn)
{
	bool discarded = false;

	oset_thread_mutex_lock(&base_tx->mutex);
	rlc_am_base_tx_sdu_t *next_node, *node = NULL;
	oset_list_for_each_safe(&base_tx->tx_sdu_queue, next_node, node){
	  if (node != NULL && node->buffer->md.pdcp_sn == discard_sn) {
		  byte_buffer_t *sdu = node->buffer;
		  oset_list_remove(&base_tx->tx_sdu_queue, node);
	  	  oset_free(node);
		  oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
		  base_tx->unread_bytes -= sdu->N_bytes;
		  oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);
		  oset_free(sdu);
		  node = NULL;
		  discarded = true;
	  }
	}   

	// Discard fails when the PDCP PDU is already in Tx window.
	RlcInfo("%s PDU with PDCP_SN=%d", discarded ? "Discarding" : "Couldn't discard", discard_sn);
}


static uint32_t rlc_am_base_tx_build_status_pdu(rlc_am_base_tx *base_tx, byte_buffer_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr_tx *tx = base_tx->tx;
	rlc_am_nr_rx *rx = base_tx->rx;
	rlc_am_base *base = base_tx->base;

	RlcInfo("generating status PDU. Bytes available:%d", nof_bytes);

	rlc_am_nr_status_pdu_t status = {
									 .mod_nr = cardinality(base_tx->cfg.rx_sn_field_length);
									 .sn_size = base_tx->cfg.rx_sn_field_length,
									};
	cvector_reserve(status.nacks, RLC_AM_NR_TYP_NACKS)

	int pdu_len = rlc_am_base_rx_get_status_pdu(&rx->base_rx, &status, nof_bytes);
	if (pdu_len == OSET_ERROR) {
		RlcDebug("deferred status PDU. Cause: Failed to acquire rx lock");
		pdu_len = 0;
	} else if (pdu_len > 0 && nof_bytes >= pdu_len)) {
		RlcDebug("generated status PDU. Bytes:%d", pdu_len);
		log_rlc_am_nr_status_pdu_to_string("Tx" ,&status, base_tx->rb_name);
		pdu_len = rlc_am_nr_write_status_pdu(&status, base_tx->cfg.tx_sn_field_length, payload);
	} else {
		RlcInfo("cannot tx status PDU - %d bytes available, %d bytes required", nof_bytes, pdu_len);
		pdu_len = 0;
	}

	cvector_free(status.nacks);
	return payload->N_bytes;
}

/**
 * Builds a retx RLC PDU, without requiring (re-)segmentation.
 *
 * The RETX PDU may be transporting a full SDU or an SDU segment.
 *
 * \param [retx] is the retx info contained in the retx_queue. This is passed by copy, to avoid
 *               issues when using retx after pop'ing it from the queue.
 * \param [payload] is a pointer to the MAC buffer that will hold the PDU segment.
 * \param [nof_bytes] is the number of bytes the RLC is allowed to fill.
 *
 * \returns the number of bytes written to the payload buffer.
 * \remark this function will not update the SI. This means that if the retx is of the last
 * SDU segment, the SI should already be of the `last_segment` type.
 */
static uint32_t build_retx_pdu_without_segmentation(rlc_am_base_tx *base_tx, rlc_amd_retx_nr_t *retx, uint8_t *payload, uint32_t nof_bytes)
{
  rlc_am_nr_tx *tx = base_tx->tx;
  rlc_amd_tx_pdu_nr *tx_pdu = tx_window_has_sn(tx, retx->base.sn);

  ASSERT_IF_NOT(NULL != tx_pdu, "Called without checking retx SN");
  ASSERT_IF_NOT(!is_retx_segmentation_required(retx, nof_bytes), "Called without checking if segmentation was required");

  // Get expected header and payload len
  uint32_t expected_hdr_len = get_retx_expected_hdr_len(retx);
  uint32_t retx_payload_len = retx->base.is_segment ? (retx->base.so_start + retx->segment_length - retx->base.current_so)
                                              : tx_pdu->sdu_buf->N_bytes;
  ASSERT_IF_NOT(nof_bytes >= (expected_hdr_len + retx_payload_len),
                "Called but segmentation is required. nof_bytes=%d, expeced_hdr_len=%d, retx_payload_len=%d",
                nof_bytes,
                expected_hdr_len,
                retx_payload_len);

  // Log RETX info
  RlcDebug("SDU%scan be fully re-transmitted. SN=%d, nof_bytes=%d, expected_hdr_len=%d, "
           "current_so=%d, so_start=%d, segment_length=%d",
           retx->base.is_segment ? " segment " : " ",
           retx->base.sn,
           nof_bytes,
           expected_hdr_len,
           retx->base.current_so,
           retx->base.so_start,
           retx->segment_length);

  // Get RETX SN, current SO and SI
  rlc_nr_si_field_t si = (rlc_nr_si_field_t)full_sdu;
  if (retx->base.is_segment) {
    if (retx->base.current_so == 0) {
      si = (rlc_nr_si_field_t)first_segment;
    } else if ((retx->base.current_so + retx_payload_len) < tx_pdu.sdu_buf->N_bytes) {
      si = (rlc_nr_si_field_t)neither_first_nor_last_segment;
    } else {
      si = (rlc_nr_si_field_t)last_segment;
    }
  }

  // Get RETX PDU payload size
  uint32_t retx_pdu_payload_size = 0;
  if (!retx->base.is_segment) {
    // RETX full SDU
    retx_pdu_payload_size = tx_pdu->sdu_buf->N_bytes;
  } else {
    // RETX SDU segment
    retx_pdu_payload_size = (retx->base.so_start + retx.segment_length - retx->base.current_so);
  }

  // Update RETX queue. This must be done before calculating
  // the polling bit, to make sure the poll bit is calculated correctly
  retx_queue.pop();

  // Write header to payload
  rlc_am_nr_pdu_header_t new_header = tx_pdu.header;
  new_header.si                     = si;
  new_header.so                     = retx.current_so;
  new_header.p                      = get_pdu_poll(retx.sn, true, 0);
  uint32_t hdr_len                  = rlc_am_nr_write_data_pdu_header(new_header, payload);

  // Write SDU/SDU segment to payload
  uint32_t pdu_bytes = hdr_len + retx_pdu_payload_size;
  srsran_assert(pdu_bytes <= nof_bytes, "Error calculating hdr_len and pdu_payload_len");
  memcpy(&payload[hdr_len], &tx_pdu.sdu_buf->msg[retx.current_so], retx_pdu_payload_size);

  // Log RETX
  RlcHexInfo((*tx_window)[retx.sn].sdu_buf->msg,
             (*tx_window)[retx.sn].sdu_buf->N_bytes,
             "Original SDU SN=%d (%d B) (attempt %d/%d)",
             retx.sn,
             (*tx_window)[retx.sn].sdu_buf->N_bytes,
             (*tx_window)[retx.sn].retx_count + 1,
             cfg.max_retx_thresh);
  RlcHexInfo(payload, pdu_bytes, "RETX PDU SN=%d (%d B)", retx.sn, pdu_bytes);
  log_rlc_am_nr_pdu_header_to_string(logger.debug, new_header, rb_name);

  debug_state();
  return pdu_bytes;
}

/**
 * Builds a retx RLC PDU that requires (re-)segmentation.
 *
 * \param [tx_pdu] is the tx_pdu info contained in the tx_window.
 * \param [payload] is a pointer to the MAC buffer that will hold the PDU segment.
 * \param [nof_bytes] is the number of bytes the RLC is allowed to fill.
 *
 * \returns the number of bytes written to the payload buffer.
 * \remark: This functions assumes that the SDU has already been copied to tx_pdu.sdu_buf.
 */
static uint32_t build_retx_pdu_with_segmentation(rlc_am_base_tx *base_tx, rlc_amd_retx_nr_t *retx, uint8_t *payload, uint32_t nof_bytes)
{
  // Get tx_pdu info from tx_window
  srsran_assert(tx_window->has_sn(retx.sn), "Called %s without checking retx SN", __FUNCTION__);
  srsran_assert(is_retx_segmentation_required(retx, nof_bytes),
                "Called %s without checking if segmentation was not required",
                __FUNCTION__);

  rlc_amd_tx_pdu_nr& tx_pdu = (*tx_window)[retx.sn];

  // Is this an SDU segment or a full SDU?
  if (not retx.is_segment) {
    RlcDebug("Creating SDU segment from full SDU. SN=%d Tx SDU (%d B), nof_bytes=%d B ",
             retx.sn,
             tx_pdu.sdu_buf->N_bytes,
             nof_bytes);

  } else {
    RlcDebug("Creating SDU segment from SDU segment. SN=%d, current_so=%d, so_start=%d, segment_length=%d",
             retx.sn,
             retx.current_so,
             retx.so_start,
             retx.segment_length);
  }

  uint32_t          expected_hdr_len = min_hdr_size;
  rlc_nr_si_field_t si               = rlc_nr_si_field_t::first_segment;
  if (retx.current_so != 0) {
    si               = rlc_nr_si_field_t::neither_first_nor_last_segment;
    expected_hdr_len = max_hdr_size;
  }

  // Sanity check: are there enough bytes for header plus data?
  if (nof_bytes <= expected_hdr_len) {
    RlcInfo("Not enough bytes for RETX payload plus header. SN=%d, nof_bytes=%d, hdr_len=%d",
            retx.sn,
            nof_bytes,
            expected_hdr_len);
    return 0;
  }

  // Sanity check: could this have been transmitted without segmentation?
  if (nof_bytes > (tx_pdu.sdu_buf->N_bytes + expected_hdr_len)) {
    RlcError("called %s, but there are enough bytes to avoid segmentation. SN=%d", __FUNCTION__, retx.sn);
    return 0;
  }

  // Can the RETX PDU be transmitted in a single PDU?
  uint32_t retx_pdu_payload_size = nof_bytes - expected_hdr_len;

  // Write header
  rlc_am_nr_pdu_header_t hdr = tx_pdu.header;
  hdr.p                      = get_pdu_poll(retx.sn, true, 0);
  hdr.so                     = retx.current_so;
  hdr.si                     = si;
  uint32_t hdr_len           = rlc_am_nr_write_data_pdu_header(hdr, payload);
  if (hdr_len >= nof_bytes || hdr_len != expected_hdr_len) {
    log_rlc_am_nr_pdu_header_to_string(logger.error, hdr, rb_name);
    RlcError("Error writing AMD PDU header. nof_bytes=%d, hdr_len=%d", nof_bytes, hdr_len);
    return 0;
  }
  log_rlc_am_nr_pdu_header_to_string(logger.info, hdr, rb_name);

  // Copy SDU segment into payload
  srsran_assert((hdr_len + retx_pdu_payload_size) <= nof_bytes, "Error calculating hdr_len and segment_payload_len");
  memcpy(&payload[hdr_len], &tx_pdu.sdu_buf->msg[retx.current_so], retx_pdu_payload_size);

  // Store PDU segment info into tx_window
  RlcDebug("Updating RETX segment info. SN=%d, is_segment=%s", retx.sn, retx.is_segment ? "true" : "false");
  if (!retx.is_segment) {
    // Retx is not a segment yet
    rlc_amd_tx_pdu_nr::pdu_segment seg1 = {};
    seg1.so                             = retx.current_so;
    seg1.payload_len                    = retx_pdu_payload_size;
    rlc_amd_tx_pdu_nr::pdu_segment seg2 = {};
    seg2.so                             = retx.current_so + retx_pdu_payload_size;
    seg2.payload_len                    = retx.segment_length - retx_pdu_payload_size;
    tx_pdu.segment_list.push_back(seg1);
    tx_pdu.segment_list.push_back(seg2);
    RlcDebug("New segment: SN=%d, SO=%d len=%d", retx.sn, seg1.so, seg1.payload_len);
    RlcDebug("New segment: SN=%d, SO=%d len=%d", retx.sn, seg2.so, seg2.payload_len);
  } else {
    // Retx is already a segment
    // Find current segment in segment list.
    std::list<rlc_amd_tx_pdu_nr::pdu_segment>::iterator it;
    for (it = tx_pdu.segment_list.begin(); it != tx_pdu.segment_list.end(); ++it) {
      if (it->so == retx.current_so) {
        break;
      }
    }
    if (it != tx_pdu.segment_list.end()) {
      rlc_amd_tx_pdu_nr::pdu_segment seg1 = {};
      seg1.so                             = it->so;
      seg1.payload_len                    = retx_pdu_payload_size;
      rlc_amd_tx_pdu_nr::pdu_segment seg2 = {};
      seg2.so                             = it->so + retx_pdu_payload_size;
      seg2.payload_len                    = it->payload_len - retx_pdu_payload_size;

      std::list<rlc_amd_tx_pdu_nr::pdu_segment>::iterator begin_it   = tx_pdu.segment_list.erase(it);
      std::list<rlc_amd_tx_pdu_nr::pdu_segment>::iterator insert_it  = tx_pdu.segment_list.insert(begin_it, seg2);
      std::list<rlc_amd_tx_pdu_nr::pdu_segment>::iterator insert_it2 = tx_pdu.segment_list.insert(insert_it, seg1);
      RlcDebug("Old segment SN=%d, SO=%d len=%d", retx.sn, retx.current_so, retx.segment_length);
      RlcDebug("New segment SN=%d, SO=%d len=%d", retx.sn, seg1.so, seg1.payload_len);
      RlcDebug("New segment SN=%d, SO=%d len=%d", retx.sn, seg2.so, seg2.payload_len);
    } else {
      RlcDebug("Could not find segment. SN=%d, SO=%d length=%d", retx.sn, retx.current_so, retx.segment_length);
    }
  }

  // Update retx queue
  retx.is_segment = true;
  retx.current_so = retx.current_so + retx_pdu_payload_size;

  RlcDebug("Updated RETX info. is_segment=%s, current_so=%d, so_start=%d, segment_length=%d",
           retx.is_segment ? "true" : "false",
           retx.current_so,
           retx.so_start,
           retx.segment_length);

  if (retx.current_so >= tx_pdu.sdu_buf->N_bytes) {
    RlcError("Current SO larger or equal to SDU size when creating SDU segment. SN=%d, current SO=%d, SO_start=%d, "
             "segment_length=%d",
             retx.sn,
             retx.current_so,
             retx.so_start,
             retx.segment_length);
    return 0;
  }

  if (retx.current_so >= retx.so_start + retx.segment_length) {
    RlcError("Current SO larger than SO_start + segment_length. SN=%d, current SO=%d, SO_start=%d, segment_length=%s",
             retx.sn,
             retx.current_so,
             retx.so_start,
             retx.segment_length);
    return 0;
  }

  return hdr_len + retx_pdu_payload_size;
}


/**
 * Builds a retx RLC PDU.
 *
 * This will use the retx_queue to get information about the RLC PDU
 * being retx'ed. The retx may have been previously transmitted as
 * a full SDU or an SDU segment.
 *
 * \param [tx_pdu] is the tx_pdu info contained in the tx_window.
 * \param [payload] is a pointer to the MAC buffer that will hold the PDU segment.
 * \param [nof_bytes] is the number of bytes the RLC is allowed to fill.
 *
 * \returns the number of bytes written to the payload buffer.
 * \remark: This functions assumes that the SDU has already been copied to tx_pdu.sdu_buf.
 */
static uint32_t rlc_am_base_tx_build_retx_pdu(rlc_am_base_tx *base_tx, uint8_t* payload, uint32_t nof_bytes)
{
	rlc_am_nr_tx *tx = base_tx->tx;

	// Check there is at least 1 element before calling front()
	if (oset_list_empty(&tx->retx_queue)) {
		RlcError("in build_retx_pdu(): retx_queue is empty");
		return 0;
	}

	rlc_amd_retx_nr_t *retx = oset_list_first(&tx->retx_queue);

	// Sanity check - drop any retx SNs not present in tx_window
	while (NULL == tx_window_has_sn(tx, retx->base.sn)) {
		RlcInfo("SN=%d not in tx window, probably already ACKed. Skip and remove from retx queue", retx.sn);
		oset_list_remove(&tx->retx_queue, retx);
		oset_free(retx);
		if (!oset_list_empty(&tx->retx_queue)) {
			retx = oset_list_first(&tx->retx_queue);
		} else {
			RlcInfo("empty retx queue, cannot provide any retx PDU");
			return 0;
		}
	}

	RlcDebug("RETX - SN=%d, is_segment=%s, current_so=%d, so_start=%d, segment_length=%d",
	       retx->base.sn,
	       retx->base.is_segment ? "true" : "false",
	       retx->base.current_so,
	       retx->base.so_start,
	       retx->segment_length);

	// Is segmentation/re-segmentation required?
	// 重传是否需要二次分片
	bool segmentation_required = is_retx_segmentation_required(retx, nof_bytes);

	if (segmentation_required) {
		//重传二次分片
		return build_retx_pdu_with_segmentation(base_tx, retx, payload, nof_bytes);
	}

	//重传不再二次分片
	return build_retx_pdu_without_segmentation(base_tx, retx, payload, nof_bytes);
}


/**
 * Builds a new RLC PDU segment, from a RLC SDU.
 *
 * \param [tx_pdu] is the tx_pdu info contained in the tx_window.
 * \param [payload] is a pointer to the MAC buffer that will hold the PDU segment.
 * \param [nof_bytes] is the number of bytes the RLC is allowed to fill.
 *
 * \returns the number of bytes written to the payload buffer.
 * \remark: This functions assumes that the SDU has already been copied to tx_pdu.sdu_buf.
 */
uint32_t rlc_am_base_tx_build_new_sdu_segment(rlc_am_base_tx *base_tx, rlc_amd_tx_pdu_nr *tx_pdu, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr_tx *tx = base_tx->tx;
	rlc_am_base *base = base_tx->base;

	RlcInfo("creating new SDU segment. Tx SDU (%d B), nof_bytes=%d B ", tx_pdu.sdu_buf->N_bytes, nof_bytes);

	// Sanity check: can this SDU be sent this in a single PDU?
	if ((tx_pdu->sdu_buf->N_bytes + tx->min_hdr_size) < nof_bytes) {
		RlcError("calling build_new_sdu_segment(), but there are enough bytes to tx in a single PDU. Tx SDU (%d B), "
		         "nof_bytes=%d B ",
		         tx_pdu.sdu_buf->N_bytes,
		         nof_bytes);
		return 0;
	}

	// Sanity check: can this SDU be sent considering header overhead?
	if (nof_bytes <= tx->min_hdr_size) { // Small header as SO is not present
		RlcInfo("cannot build new sdu_segment, there are not enough bytes allocated to tx header plus data. nof_bytes=%d, "
		        "min_hdr_size=%d",
		        nof_bytes,
		        tx->min_hdr_size);
		return 0;
	}

	uint32_t segment_payload_len = nof_bytes - tx->min_hdr_size;

	// Save SDU currently being segmented
	// This needs to be done before calculating the polling bit
	// To make sure we check correctly that the buffers are empty.
	tx->sdu_under_segmentation_sn = tx->st.tx_next;

	// Prepare header
	rlc_am_nr_pdu_header_t hdr = {0};
	hdr.dc                     = RLC_DC_FIELD_DATA_PDU;
	hdr.p                      = get_pdu_poll(base_tx, tx->st.tx_next, false, segment_payload_len);
	hdr.si                     = (rlc_nr_si_field_t)first_segment;
	hdr.sn_size                = base_tx->cfg.tx_sn_field_length;
	hdr.sn                     = tx->st.tx_next;
	hdr.so                     = 0;
	tx_pdu->header              = hdr;
	log_rlc_am_nr_pdu_header_to_string(&hdr, base_tx->rb_name);

	// Write header
	uint32_t hdr_len = rlc_am_nr_write_data_pdu_header(hdr, payload);
	if (hdr_len >= nof_bytes || hdr_len != tx->min_hdr_size) {
		RlcError("error writing AMD PDU header");
		return 0;
	}

	// Copy PDU to payload
	ASSERT_IF_NOT((hdr_len + segment_payload_len) <= nof_bytes, "Error calculating hdr_len and segment_payload_len");
	memcpy(&payload[hdr_len], tx_pdu->sdu_buf->msg, segment_payload_len);

	// Store Segment Info
	pdu_segment *segment_info = oset_malloc(sizeof(pdu_segment));
	segment_info->payload_len = segment_payload_len;
	oset_list_add(&tx_pdu->segment_list, segment_info);
	return hdr_len + segment_payload_len;
}


/**
 * Builds a new RLC PDU.
 *
 * This will be called after checking whether control, retransmission,
 * or segment PDUs needed to be transmitted first.
 *
 * This will read an SDU from the SDU queue, build a new PDU, and add it to the tx_window.
 * SDU segmentation will be done if necessary.
 *
 * \param [payload] is a pointer to the buffer that will hold the PDU.
 * \param [nof_bytes] is the number of bytes the RLC is allowed to fill.
 *
 * \returns the number of bytes written to the payload buffer.
 */
static uint32_t rlc_am_base_tx_build_new_pdu(rlc_am_base_tx *base_tx, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr_tx *tx = base_tx->tx;
	rlc_am_nr_rx *rx = base_tx->rx;
	rlc_am_base *base = base_tx->base;
	uint32_t ret = 0;

	if (nof_bytes <= tx->min_hdr_size) {
		RlcInfo("Not enough bytes for payload plus header. nof_bytes=%d", nof_bytes);
		return 0;
	}

	// do not build any more PDU if window is already full
	if (oset_hash_count(tx->tx_window) > base->cfg.tx_queue_length) {
		RlcInfo("Cannot build data PDU - Tx window full.");
		return 0;
	}

	// Read new SDU from TX queue
	byte_buffer_t *tx_sdu = NULL;
	RlcDebug("Reading from RLC SDU queue. Queue size %d", oset_list_count(&base_tx->tx_sdu_queue);
	do {
		tx_sdu = tx_sdu_queue_read(&base_tx->tx_sdu_queue);
	} while (tx_sdu == NULL && !oset_list_empty(&base_tx->tx_sdu_queue));

	if (tx_sdu != NULL) {
		RlcDebug("Read RLC SDU - RLC_SN=%d, PDCP_SN=%d, %d bytes", tx->st.tx_next, tx_sdu->md.pdcp_sn, tx_sdu->N_bytes);
	} else {
		RlcDebug("No SDUs left in the tx queue.");
		return 0;
	}

	// insert newly assigned SN into window and use reference for in-place operations
	// NOTE: from now on, we can't return from this function anymore before increasing tx_next
	rlc_amd_tx_pdu_nr *tx_pdu = tx_window_has_sn(tx, tx->st.tx_next);
	if(NULL == tx_pdu){
	  tx_pdu = oset_malloc(rlc_amd_tx_pdu_nr);
	  oset_assert(tx_pdu);
	  oset_hash_set(rx->rx_window, &tx->st.tx_next, sizeof(tx->st.tx_next), NULL);
	  oset_hash_set(rx->rx_window, &tx->st.tx_next, sizeof(tx->st.tx_next), tx_pdu);
	}else{
	  RlcWarning("The same SN=%zd should not be added twice", tx->st.tx_next);
	}

	tx_pdu->pdcp_sn = tx_sdu->md.pdcp_sn;
	tx_pdu->sdu_buf = byte_buffer_init();
	oset_assert(tx_pdu->sdu_buf);

	// Copy SDU into TX window SDU info
	memcpy(tx_pdu->sdu_buf->msg, tx_sdu->msg, tx_sdu->N_bytes);
	tx_pdu->sdu_buf->N_bytes = tx_sdu->N_bytes;
	oset_free(tx_sdu);

	// Segment new SDU if necessary
	if (tx_pdu->sdu_buf->N_bytes + tx->min_hdr_size > nof_bytes) {
		RlcInfo("trying to build PDU segment from SDU.");
		return rlc_am_base_tx_build_new_sdu_segment(base_tx, tx_pdu, payload, nof_bytes);
	}

	// Prepare header
	rlc_am_nr_pdu_header_t hdr = {0};
	hdr.dc                     = RLC_DC_FIELD_DATA_PDU;
	hdr.p                      = get_pdu_poll(base_tx, tx->st.tx_next, false, tx_pdu->sdu_buf->N_bytes);
	hdr.si                     = (rlc_nr_si_field_t)full_sdu;
	hdr.sn_size                = base_tx->cfg.tx_sn_field_length;
	hdr.sn                     = tx->st.tx_next;
	tx_pdu->header             = hdr;
	log_rlc_am_nr_pdu_header_to_string(&hdr, base_tx->rb_name);

	// Write header
	uint32_t len = rlc_am_nr_write_data_pdu_header(hdr, tx_pdu->sdu_buf);
	if (len > nof_bytes) {
		RlcError("error writing AMD PDU header");
	}

	// Update TX Next
	tx->st.tx_next = (tx->st.tx_next + 1) % tx->mod_nr;

	memcpy(payload, tx_pdu->sdu_buf->msg, tx_pdu->sdu_buf->N_bytes);
	RlcDebug("wrote RLC PDU - %d bytes", tx_pdu->sdu_buf->N_bytes);

	ret = tx_pdu->sdu_buf->N_bytes;

	return ret;
}


/**
 * Builds the RLC PDU.
 *
 * Called by the MAC, trough one of the PHY worker threads.
 *
 * \param [payload] is a pointer to the buffer that will hold the PDU.
 * \param [nof_bytes] is the number of bytes the RLC is allowed to fill.
 *
 * \returns the number of bytes written to the payload buffer.
 * \remark: This will be called multiple times from the MAC,
 * while there is something to TX and enough space in the TB.
 */
static uint32_t rlc_am_base_tx_read_dl_pdu(rlc_am_base_tx *base_tx, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr_tx *tx = base_tx->tx;
	rlc_am_nr_rx *rx = base_tx->rx;
	rlc_am_base *base = base_tx->base;

	oset_thread_mutex_lock(&base_tx->mutex);

	if (!base->tx_enabled) {
		RlcDebug("RLC entity not active. Not generating PDU.");
		return 0;
	}
	RlcDebug("MAC opportunity - bytes=%d, tx_window size=%zu PDUs", nof_bytes, oset_hash_count(tx->tx_window));

	// Tx STATUS if requested
	// 状态报告
	if (do_status(&rx->base_rx)) {
		byte_buffer_t *tx_pdu = byte_buffer_init();
		if (tx_pdu == NULL) {
			RlcError("Couldn't allocate PDU");
			return 0;
		}
		rlc_am_base_tx_build_status_pdu(base_tx, tx_pdu, nof_bytes);
		memcpy(payload, tx_pdu->msg, tx_pdu->N_bytes);
		RlcDebug("Status PDU built - %d bytes", tx_pdu->N_bytes);
		uint32_t read_bytes = tx_pdu->N_bytes;
		oset_free(tx_pdu);
		return read_bytes;
	}

	// Retransmit if required
	// 重传
	if (!oset_list_empty(&tx->retx_queue)) {
		RlcInfo("Re-transmission required. Retransmission queue size: %d", oset_list_count(&tx->retx_queue));
		return rlc_am_base_tx_build_retx_pdu(base_tx, payload, nof_bytes);
	}

	// Send remaining segment, if it exists
	// 发送剩余段（如果存在）
	if (tx->sdu_under_segmentation_sn != INVALID_RLC_SN) {
		rlc_amd_tx_pdu_nr *pdu_node = tx_window_has_sn(tx, tx->sdu_under_segmentation_sn);
		if (NULL == pdu_node) {
		  tx->sdu_under_segmentation_sn = INVALID_RLC_SN;
		  RlcError("SDU currently being segmented does not exist in tx_window. Aborting segmentation SN=%d",
				   tx->sdu_under_segmentation_sn);
		  return 0;
		}
		return build_continuation_sdu_segment(pdu_node, payload, nof_bytes);
	}

	// Check whether there is something to TX
	if (oset_list_empty(&base_tx->tx_sdu_queue)) {
		RlcInfo("No data available to be sent");
		return 0;
	}

	//发送新数据
	return rlc_am_base_tx_build_new_pdu(base_tx, payload, nof_bytes);
}


static int rlc_am_base_tx_write_dl_sdu(rlc_am_base_tx *base_tx, byte_buffer_t *sdu)
{
	oset_thread_mutex_lock(&base_tx->mutex);
	if (sdu) {
		// Get SDU info
		uint32_t sdu_pdcp_sn = sdu->md.pdcp_sn;

		// Store SDU
		uint8_t*   msg_ptr	 = sdu->msg;
		uint32_t   nof_bytes = sdu->N_bytes;
		int count = oset_list_count(&base_tx->tx_sdu_queue);

		if((uint32_t)count <= base_tx->base->cfg.tx_queue_length) {
			rlc_am_base_tx_sdu_t *sdu_node = oset_malloc(rlc_um_base_tx_sdu_t);
			oset_assert(sdu_node);
			sdu_node->buffer = byte_buffer_dup(sdu);
			oset_list_add(&base_tx->tx_sdu_queue, sdu_node);
			oset_thread_mutex_lock(&base_tx->unread_bytes_mutex);
			base_tx->unread_bytes += sdu->N_bytes;
			oset_thread_mutex_unlock(&base_tx->unread_bytes_mutex);

			RlcHexInfo(msg_ptr,
			           nof_bytes,
			           "Tx SDU (%d B, PDCP_SN=%ld tx_sdu_queue_len=%d)",
			           nof_bytes,
			           sdu_pdcp_sn,
			           oset_list_count(&base_tx->tx_sdu_queue));
			oset_thread_mutex_unlock(&base_tx->mutex);
			return OSET_OK;
		} else {
			// in case of fail, the try_write returns back the sdu
			RlcHexWarning(msg_ptr,
			              nof_bytes,
			              "[Dropped SDU] Tx SDU (%d B, PDCP_SN=%ld, tx_sdu_queue_len=%d)",
			              nof_bytes,
			              sdu_pdcp_sn,
			              count);
		}else{
			RlcWarning("NULL SDU pointer in write_sdu()");
		}
	}
	oset_thread_mutex_unlock(&base_tx->mutex);
	return OSET_ERROR;
}


static void rlc_am_base_tx_get_buffer_state(rlc_am_base_tx *base_tx, uint32_t *n_bytes_new, uint32_t *n_bytes_prio)
{
  rlc_am_nr_tx *tx = base_tx->tx;
  rlc_am_nr_rx *rx = base_tx->rx;
  rlc_am_base *base = base_tx->base;

  oset_thread_mutex_lock(&base_tx->mutex);
  RlcDebug("buffer state - do_status=%s", do_status() ? "yes" : "no");

  if (!base->tx_enabled) {
    RlcError("get_buffer_state() failed: TX is not enabled.");
    goto end;
  }

  // Bytes needed for status report
  if (do_status(&rx->base_rx)) {
    n_bytes_prio += rlc_am_base_rx_get_status_pdu_length(&rx->base_rx);
    RlcDebug("buffer state - total status report: %d bytes", n_bytes_prio);
  }

  // Bytes needed for retx
  for (const rlc_amd_retx_nr_t *retx : retx_queue.get_inner_queue()) {
    RlcDebug("buffer state - retx - SN=%d, Segment: %s, %d:%d",
             retx.sn,
             retx.is_segment ? "true" : "false",
             retx.so_start,
             retx.so_start + retx.segment_length - 1);
    if (tx_window_has_sn(tx, retx->sn)) {
      int req_bytes     = retx.segment_length;
      int hdr_req_bytes = (retx.is_segment && retx.current_so != 0) ? max_hdr_size : min_hdr_size;
      if (req_bytes <= 0) {
        RlcError("buffer state - retx - invalid length=%d for SN=%d", req_bytes, retx.sn);
      } else {
        n_bytes_prio += (req_bytes + hdr_req_bytes);
        RlcDebug("buffer state - retx: %d bytes", n_bytes_prio);
      }
    } else {
      RlcWarning("buffer state - retx for SN=%d is outside the tx_window", retx.sn);
    }
  }

  // Bytes needed for tx of the rest of the SDU that is currently under segmentation (if any)
  if (sdu_under_segmentation_sn != INVALID_RLC_SN) {
    if (tx_window->has_sn(sdu_under_segmentation_sn)) {
      rlc_amd_tx_pdu_nr& seg_pdu = (*tx_window)[sdu_under_segmentation_sn];
      if (not seg_pdu.segment_list.empty()) {
        // obtain amount of already transmitted Bytes
        const rlc_amd_tx_pdu_nr::pdu_segment& seg       = seg_pdu.segment_list.back();
        uint32_t                              last_byte = seg.so + seg.payload_len;
        if (last_byte <= seg_pdu.sdu_buf->N_bytes) {
          // compute remaining bytes pending for transmission
          uint32_t remaining_bytes = seg_pdu.sdu_buf->N_bytes - last_byte;
          n_bytes_new += remaining_bytes + max_hdr_size;
        } else {
          RlcError(
              "buffer state - last segment of SDU under segmentation exceeds SDU len. SDU len=%d B, last_byte=%d B",
              seg_pdu.sdu_buf->N_bytes,
              last_byte);
        }
      } else {
        RlcError("buffer state - SDU under segmentation has empty segment list. Ignoring SN=%d",
                 sdu_under_segmentation_sn);
      }
    } else {
      sdu_under_segmentation_sn = INVALID_RLC_SN;
      RlcError("buffer state - SDU under segmentation does not exist in tx_window. Aborting segmentation SN=%d",
               sdu_under_segmentation_sn);
    }
  }

  // Bytes needed for tx SDUs in queue
  uint32_t n_sdus = tx_sdu_queue.get_n_sdus();
  n_bytes_new += tx_sdu_queue.size_bytes();

  // Room needed for fixed header of data PDUs
  n_bytes_new += min_hdr_size * n_sdus;
  RlcDebug("total buffer state - %d SDUs (%d B)", n_sdus, n_bytes_new + n_bytes_prio);

  if (base_tx->bsr_callback) {
    RlcDebug("calling BSR callback - %d new_tx, %d priority bytes", n_bytes_new, n_bytes_prio);
    base_tx->bsr_callback(base->common.rnti, base->common.lcid, n_bytes_new, n_bytes_prio);
  }

end:
  oset_thread_mutex_lock(&base_tx->mutex);
  return;
}

static void rlc_am_base_tx_timer_expired(void *data)
{
  rlc_am_base_tx *base_tx = (rlc_am_base_tx *)data;
  rlc_am_nr_tx *tx = base_tx->tx;
  rlc_am_base *base = base_tx->base;

  oset_thread_mutex_lock(&base_tx->mutex);

  // t-PollRetransmit
  if (tx->poll_retransmit_timer) {
    RlcDebug("Poll retransmission timer expired after %dms", poll_retransmit_timer.duration());
    debug_state();
    /*
     * - if both the transmission buffer and the retransmission buffer are empty
     *   (excluding transmitted RLC SDU or RLC SDU segment awaiting acknowledgements); or
     * - if no new RLC SDU or RLC SDU segment can be transmitted (e.g. due to window stalling):
     *   - consider the RLC SDU with the highest SN among the RLC SDUs submitted to lower layer for
     *   retransmission; or
     *   - consider any RLC SDU which has not been positively acknowledged for retransmission.
     * - include a poll in an AMD PDU as described in section 5.3.3.2.
     */
    if ((tx_sdu_queue.is_empty() && retx_queue.empty()) || tx_window->full()) {
      if (tx_window->empty()) {
        RlcError("t-PollRetransmit expired, but the tx_window is empty. POLL_SN=%d, Tx_Next_Ack=%d, tx_window_size=%d",
                 st.poll_sn,
                 st.tx_next_ack,
                 tx_window->size());
        goto end;
      }
      if (not tx_window->has_sn(st.tx_next_ack)) {
        RlcError("t-PollRetransmit expired, but Tx_Next_Ack is not in the tx_widow. POLL_SN=%d, Tx_Next_Ack=%d, "
                 "tx_window_size=%d",
                 st.poll_sn,
                 st.tx_next_ack,
                 tx_window->size());
        goto end;
      }
      // RETX first RLC SDU that has not been ACKed
      // or first SDU segment of the first RLC SDU
      // that has not been acked
      rlc_amd_retx_nr_t& retx = retx_queue.push();
      retx.sn                 = st.tx_next_ack;
      if ((*tx_window)[st.tx_next_ack].segment_list.empty()) {
        // Full SDU
        retx.is_segment     = false;
        retx.so_start       = 0;
        retx.segment_length = (*tx_window)[st.tx_next_ack].sdu_buf->N_bytes;
        retx.current_so     = 0;
      } else {
        // To make sure we do not mess up the segment list
        // We RETX an SDU segment instead of the full SDU
        // if the SDU has been segmented before.
        // As we cannot know which segments have been ACKed before
        // we simply RETX the first one.
        retx.is_segment     = true;
        retx.so_start       = 0;
        retx.current_so     = 0;
        retx.segment_length = (*tx_window)[st.tx_next_ack].segment_list.begin()->payload_len;
      }
      RlcDebug("Retransmission because of t-PollRetransmit. RETX SN=%d, is_segment=%s, so_start=%d, segment_length=%d",
               retx.sn,
               retx.is_segment ? "true" : "false",
               retx.so_start,
               retx.segment_length);
    }
    goto end;
  }
end:
	oset_thread_mutex_unlock(&base_tx->mutex);
	return;
}


static void rlc_am_base_tx_stop(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_destroy(&base_tx->unread_bytes_mutex);
	oset_thread_mutex_destroy(&base_tx->mutex);
}

static void rlc_am_base_tx_init(rlc_am_base_tx *base_tx)
{
	oset_thread_mutex_init(&base_tx->mutex);
	oset_thread_mutex_init(&base_tx->unread_bytes_mutex);
}


/*static int modulus_rx(nr_rlc_entity_am_t *entity, int a)
{
  // as per 38.322 7.1, modulus base is rx_next
  int r = a - entity->rx_next;
  if (r < 0) r += entity->sn_modulus;
  return r;
}*/

static uint32_t RX_MOD_NR_BASE(rlc_am_nr_rx *rx, uint32_t sn)
{
  /* as per 38.322 7.1, modulus base is rx_next */
  return (sn - rx->st.rx_next) % rx->mod_nr;
}


static int rlc_amd_rx_pdu_nr_cmp(rlc_amd_rx_pdu_nr *a, rlc_amd_rx_pdu_nr *b) 
{
	if (a.header.so == b.header.so)
		return 0;
	else if (a.header.so < b.header.so)
		return -1;//从小到大排序
	else
		return 1;
};

static rlc_amd_rx_sdu_nr_t* rx_window_has_sn(rlc_am_nr_rx *rx, uint32_t sn)
{
	/* as per 38.322 7.1, modulus base is rx_next */
	return oset_hash_get(rx->rx_window, &sn, sizeof(sn));
}

static void rlc_am_base_rx_timer_expired(void *data)
{
  rlc_am_base_rx *base_rx = (rlc_am_base_rx *)data;
  rlc_am_nr_rx *rx = base_rx->rx;
  rlc_am_base *base = base_rx->base;

  oset_thread_mutex_lock(&base_rx->mutex);

  // Status Prohibit
  if (status_prohibit_timer.is_valid() && status_prohibit_timer.id() == timeout_id) {
    RlcDebug("Status prohibit timer expired after %dms", status_prohibit_timer.duration());
    goto end;
  }

  // Reassembly
  if (reassembly_timer.is_valid() && reassembly_timer.id() == timeout_id) {
    RlcDebug("Reassembly timer expired after %dms", reassembly_timer.duration());
    /*
     * 5.2.3.2.4 Actions when t-Reassembly expires:
     * - update RX_Highest_Status to the SN of the first RLC SDU with SN >= RX_Next_Status_Trigger for which not
     *   all bytes have been received;
     * - if RX_Next_Highest> RX_Highest_Status +1: or
     * - if RX_Next_Highest = RX_Highest_Status + 1 and there is at least one missing byte segment of the SDU
     *   associated with SN = RX_Highest_Status before the last byte of all received segments of this SDU:
     *   - start t-Reassembly;
     *   - set RX_Next_Status_Trigger to RX_Next_Highest.
     */
    uint32_t sn_upd = {};
    for (sn_upd = rx->st.rx_next_status_trigger; RX_MOD_NR_BASE(sn_upd) < RX_MOD_NR_BASE(st.rx_next_highest);
         sn_upd = (sn_upd + 1) % mod_nr) {
      if (not rx_window->has_sn(sn_upd) || (rx_window->has_sn(sn_upd) && not(*rx_window)[sn_upd].fully_received)) {
        break;
      }
    }
    rx->st.rx_highest_status = sn_upd;
    if (not valid_ack_sn(st.rx_highest_status)) {
      RlcError("Rx_Highest_Status not inside RX window");
      debug_state();
    }
    srsran_assert(valid_ack_sn(st.rx_highest_status), "Error: rx_highest_status assigned outside rx window");

    bool restart_reassembly_timer = false;
    if (RX_MOD_NR_BASE(st.rx_next_highest) > RX_MOD_NR_BASE(st.rx_highest_status + 1)) {
      restart_reassembly_timer = true;
    }
    if (RX_MOD_NR_BASE(st.rx_next_highest) == RX_MOD_NR_BASE(st.rx_highest_status + 1)) {
      if (rx_window->has_sn(st.rx_highest_status) && (*rx_window)[st.rx_highest_status].has_gap) {
        restart_reassembly_timer = true;
      }
    }
    if (restart_reassembly_timer) {
      reassembly_timer.run();
      st.rx_next_status_trigger = st.rx_next_highest;
    }

    /* 5.3.4 Status reporting:
     * - The receiving side of an AM RLC entity shall trigger a STATUS report when t-Reassembly expires.
     *   NOTE 2: The expiry of t-Reassembly triggers both RX_Highest_Status to be updated and a STATUS report to be
     *   triggered, but the STATUS report shall be triggered after RX_Highest_Status is updated.
     */
    base_rx->do_status = true;
    debug_state();
    debug_window();
    goto end;
  }
end:
  oset_thread_mutex_unlock(&base_rx->mutex);
  return;
}

/*
 * Status PDU
 */
uint32_t rlc_am_base_rx_get_status_pdu(rlc_am_base_rx *base_rx, rlc_am_nr_status_pdu_t *status, uint32_t max_len)
{
  rlc_am_nr_rx *rx = base_rx->rx;

  if (0 != oset_thread_mutex_trylock(&base_rx->mutex)) {
	return OSET_ERROR;
  }

  rlc_am_nr_status_pdu_reset(status);

  /*
   * - for the RLC SDUs with SN such that RX_Next <= SN < RX_Highest_Status that has not been completely
   *   received yet, in increasing SN order of RLC SDUs and increasing byte segment order within RLC SDUs,
   *   starting with SN = RX_Next up to the point where the resulting STATUS PDU still fits to the total size of RLC
   *   PDU(s) indicated by lower layer:
   */
  RlcDebug("Generating status PDU");
  for (uint32_t i = rx->st.rx_next; RX_MOD_NR_BASE(rx, i) < RX_MOD_NR_BASE(rx, rx->st.rx_highest_status); i = (i + 1) % rx->mod_nr) {
	rlc_amd_rx_sdu_nr_t* rx_node = rx_window_has_sn(rx, i);
	if (NULL != rx_node && rx_node->fully_received) {
      RlcDebug("SDU SN=%d is fully received", i);
    } else {
      if (NULL == rx_node) {
        // No segment received, NACK the whole SDU
        RlcDebug("Adding NACK for full SDU. NACK SN=%d", i);
        rlc_status_nack_t nack = {0};
        nack.nack_sn = i;
        nack.has_so  = false;
		rlc_am_nr_status_pdu_push_nack(status, nack);
      } else if (!rx_node->fully_received) {
        // Some segments were received, but not all.
        // NACK non consecutive missing bytes
        RlcDebug("Adding NACKs for segmented SDU. NACK SN=%d", i);
        uint32_t last_so         = 0;
        bool     last_segment_rx = false;
		rlc_amd_rx_pdu_nr  *segm = NULL;
		oset_list_for_each(&rx_node->segments, segm){
		  // 如果不缺失按道理segm->header.so == last_so
          if (segm->header.so != last_so) {
            // Some bytes were not received
            rlc_status_nack_t nack = {0};
            nack.nack_sn  = i;
            nack.has_so   = true;
            nack.so_start = last_so;
			// set to last missing byte 
            nack.so_end   = segm->header.so - 1; //segm->header.so为下一个开始，所以so_end=so-1
            rlc_am_nr_status_pdu_push_nack(status, nack);

            if (nack.so_start > nack.so_end) {
              // Print segment list
              rlc_amd_rx_pdu_nr  *segm_it = NULL;
			  oset_list_for_each(&rx_node->segments, segm_it){
                RlcError("Segment: segm.header.so=%d, segm.buf.N_bytes=%d", segm_it->header.so, segm_it->buf->N_bytes);
              }
              RlcError("Error: SO_start=%d > SO_end=%d. NACK_SN=%d. SO_start=%d, SO_end=%d, seg.so=%d",
                       nack.so_start,
                       nack.so_end,
                       nack.nack_sn,
                       nack.so_start,
                       nack.so_end,
                       segm->header.so);
              ASSERT_IF_NOT(nack.so_start <= nack.so_end,
                            "Error: SO_start=%d > SO_end=%d. NACK_SN=%d",
                            nack.so_start,
                            nack.so_end,
                            nack.nack_sn);
            } else {
              RlcDebug("First/middle segment missing. NACK_SN=%d. SO_start=%d, SO_end=%d",
                       nack.nack_sn,
                       nack.so_start,
                       nack.so_end);
            }
          }

          if (segm->header.si == (rlc_nr_si_field_t)last_segment) {
            last_segment_rx = true;
          }

          last_so = segm->header.so + segm->buf->N_bytes;
        } // Segment loop

		// Final segment missing
        if (!last_segment_rx) {
          rlc_status_nack_t nack = {0};
          nack.nack_sn  = i;
          nack.has_so   = true;
          nack.so_start = last_so;
          nack.so_end   = so_end_of_sdu;
          rlc_am_nr_status_pdu_push_nack(status, nack);
          RlcDebug(
              "Final segment missing. NACK_SN=%d. SO_start=%d, SO_end=%d", nack.nack_sn, nack.so_start, nack.so_end);
          ASSERT_IF_NOT(nack.so_start <= nack.so_end, "Error: SO_start > SO_end. NACK_SN=%d", nack.nack_sn);
        }
      }
    }
  } // NACK loop

  /*
   * - set the ACK_SN to the SN of the next not received RLC SDU which is not
   * indicated as missing in the resulting STATUS PDU.
   */
  status->ack_sn = rx->st.rx_highest_status;

  // trim PDU if necessary
  if (status->packed_size > max_len) {
    RlcInfo("Trimming status PDU with %d NACKs and packed_size=%d into max_len=%d",
            cvector_size(status->nacks),
            status->packed_size,
            max_len);
    log_rlc_am_nr_status_pdu_to_string("Untrimmed", status, base_rx->rb_name);
    if (!rlc_am_nr_status_pdu_trim(status, max_len)) {
      RlcError("Failed to trim status PDU into provided space: max_len=%d", max_len);
    }
  }

  if (max_len != UINT32_MAX) {
    // UINT32_MAX is used just to query the status PDU length
    // t-StatusProhibit 定时器（防止频繁上报）
    if (rx->status_prohibit_timer) {
	  gnb_timer_start(rx->status_prohibit_timer, base_rx->cfg.t_status_prohibit);
    }
    base_rx->do_status = false;
  }

  oset_thread_mutex_unlock(&base_rx->mutex);

  return status->packed_size;
}

uint32_t rlc_am_base_rx_get_status_pdu_length(rlc_am_base_rx *base_rx)
{
	rlc_am_nr_status_pdu_t tmp_status = {
									 .mod_nr = cardinality(base_rx->cfg.rx_sn_field_length);
									 .sn_size = base_rx->cfg.rx_sn_field_length,
									};
	cvector_reserve(tmp_status.nacks, RLC_AM_NR_TYP_NACKS);

	rlc_am_base_rx_get_status_pdu(base_rx, &tmp_status, UINT32_MAX);
	cvector_free(tmp_status.nacks);
	return tmp_status.packed_size;
}

static uint32_t rlc_am_base_rx_get_sdu_rx_latency_ms(rlc_am_base_rx *base_rx)
{
  return 0;
}

static uint32_t rlc_am_base_rx_get_rx_buffered_bytes(rlc_am_base_rx *base_rx)
{
  return 0;
}

static void rlc_am_base_rx_stop(rlc_am_base_rx *base_rx)
{
	//todo
	base_rx->do_status = false;
	oset_thread_mutex_destroy(&base_rx->mutex);
}

static void rlc_am_base_rx_init(rlc_um_base_rx *base_rx)
{
	oset_thread_mutex_init(&base_rx->mutex);
}

static void rlc_am_base_init(rlc_am_base *base, uint32_t lcid_, uint16_t rnti_)
{
	rlc_common_init(&base->common, NULL, rnti_, lcid_, (rlc_mode_t)um, NULL);
	base->rx_enabled = false;
	base->tx_enabled = false;
	base->metrics = {0};
	oset_thread_mutex_init(&base->metrics_mutex);
}

static void rlc_am_base_stop(rlc_am_base *base)
{
	rlc_common_destory(&base->common);
	base->rx_enabled = false;
	base->tx_enabled = false;
	oset_thread_mutex_destroy(&base->metrics_mutex);
	base->metrics = {0};
}

static uint32_t rlc_am_base_read_dl_pdu(rlc_am_base *base, rlc_am_base_tx *base_tx, uint8_t* payload, uint32_t nof_bytes)
{
	uint32_t read_bytes = rlc_am_base_tx_read_dl_pdu(base_tx, payload, nof_bytes);

	oset_thread_mutex_lock(&base->metrics_mutex);
	base->metrics.num_tx_pdus += read_bytes > 0 ? 1 : 0;
	base->metrics.num_tx_pdu_bytes += read_bytes;
	oset_thread_mutex_unlock(&base->metrics_mutex);

	return read_bytes;
}


static void rlc_am_base_write_dl_sdu(rlc_am_base *base, rlc_am_base_tx *base_tx, byte_buffer_t *sdu)
{
	if (! base->tx_enabled || ! base_tx) {
		RlcDebug("RB is currently deactivated. Dropping SDU (%d B)", sdu->N_bytes);
		oset_thread_mutex_lock(&base->metrics_mutex);
		base->metrics.num_lost_sdus++;
		oset_thread_mutex_unlock(&base->metrics_mutex);
		return;
	}

	int sdu_bytes = sdu->N_bytes; //< Store SDU length for book-keeping
	if (rlc_am_base_tx_write_dl_sdu(base_tx, sdu) == OSET_OK) {
		oset_thread_mutex_lock(&base->metrics_mutex);
		base->metrics.num_tx_sdus++;
		base->metrics.num_tx_sdu_bytes += sdu_bytes;
		oset_thread_mutex_unlock(&base->metrics_mutex);
	} else {
		oset_thread_mutex_lock(&base->metrics_mutex);
		base->metrics.num_lost_sdus++;
		oset_thread_mutex_unlock(&base->metrics_mutex);
	}
}


#endif
/////////////////////////////////////////////////////////////////////////////
#if AM_RXTX
static void suspend_tx(rlc_am_nr_tx *tx)
{
	oset_thread_mutex_lock(&tx->base_tx.mutex);
	empty_queue_no_lock(&tx->base_tx);

	if(tx->poll_retransmit_timer) gnb_timer_delete(tx->poll_retransmit_timer);

	tx->st = {0};
	tx->sdu_under_segmentation_sn = INVALID_RLC_SN;

	// Drop all messages in TX window
	tx->tx_window->clear();
	oset_hash_destroy(tx->tx_window);

	// Drop all messages in RETX queue
	tx->retx_queue.clear();

	tx->base_tx.base.tx_enabled = false;
	oset_thread_mutex_unlock(&tx->base_tx.mutex);
}

static void rlc_am_nr_tx_stop(rlc_am_nr_tx *tx)
{
	suspend_tx(tx);
	rlc_am_base_tx_stop(&tx->base_tx);
}

static void rlc_am_nr_tx_reestablish(rlc_am_nr_tx *tx)
{
	suspend_tx(tx);
}

static bool rlc_am_nr_tx_configure(rlc_am_nr_tx *tx, rlc_config_t *cfg_, char *rb_name_)
{
	tx->base_tx.rb_name = rb_name_;
	tx->base_tx.cfg = cfg_->am_nr;

	if (cfg_->tx_queue_length > max_tx_queue_size) {
	  RlcError("configuring tx queue length of %d PDUs too big. Maximum value is %d.",
			   cfg_->tx_queue_length,
			   max_tx_queue_size);
	  return false;
	}
	
	tx->mod_nr = cardinality(tx->base_tx.cfg.tx_sn_field_length);
	tx->AM_Window_Size = am_window_size(tx->base_tx.cfg.tx_sn_field_length);

	// no so(sn 12 bit) 					no so(sn 18 bit)
	// |D/C|P|SI | SN	|					|D/C|P|SI|R|R| SN |   
	// |	  SN		|					|	   SN		  |
	// |	  Data		|					|	   SN		  |
	// |	  ...		|					|	   Data 	  |
	
	
	// with so(sn 12 bit)					with so(sn 18 bit)
	// |D/C|P|SI | SN	|					|D/C|P|SI|R|R| SN |
	// |	  SN		|					|	   SN		  |
	// |	  SO		|					|	   SN		  |
	// |	  SO		|					|	   SO		  |
	// |	  Data		|					|	   SO		  |
	// |	  ...		|					|	   Data 	  |

	switch (tx->base_tx.cfg.tx_sn_field_length) {
	  case (rlc_am_nr_sn_size_t)size12bits:
		tx->min_hdr_size = 2;
		break;
	  case (rlc_am_nr_sn_size_t)size18bits:
		tx->min_hdr_size = 3;
		break;
	  default:
		RlcError("attempt to configure unsupported tx_sn_field_length %s", rlc_am_nr_sn_size_to_string(tx->base_tx.cfg.tx_sn_field_length));
		return false;
	}
	
	tx->max_hdr_size = tx->min_hdr_size + tx->so_size;

	// make sure Tx queue is empty before attempting to resize
	oset_list_init(&tx->base_tx.tx_sdu_queue);//cfg_->tx_queue_length
	
	// Configure t_poll_retransmission timer
	if (tx->base_tx.cfg.t_poll_retx > 0) {
	  tx->poll_retransmit_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_tx_timer_expired, &tx->base_tx);
	}
	
	tx->base_tx.base.tx_enabled = true;
	
	RlcDebug("RLC AM NR configured tx entity");

	return true;
}

static void rlc_am_nr_tx_init(rlc_am_nr_tx *tx, rlc_am_nr_rx *rx, rlc_um_base *base)
{
	tx->base_tx.base = base;
	tx->base_tx.rx = rx;
	tx->base_tx.tx = tx;

	tx->min_hdr_size = 2;
	tx->so_size      = 2;
	tx->max_hdr_size = 4;
	tx->mod_nr       = 0;

	tx->tx_window = oset_hash_make();
	oset_list_init(&tx->retx_queue);

	rlc_am_base_tx_init(&tx->base_tx);
}

static void suspend_rx(rlc_am_nr_rx *rx)
{
	oset_thread_mutex_lock(&rx->base_rx.mutex);
	if(rx->status_prohibit_timer) gnb_timer_delete(rx->status_prohibit_timer);
	if(rx->reassembly_timer) gnb_timer_delete(rx->reassembly_timer);

	rx->st = {0};
	// Drop all messages in RX window
	oset_hash_destroy(rx->rx_window);
	oset_thread_mutex_unlock(&rx->base_rx.mutex);
}

static void rlc_am_nr_rx_reestablish(rlc_am_nr_rx *rx)
{
	suspend_rx(rx);
}

static void rlc_am_nr_rx_stop(rlc_am_nr_rx *rx)
{
	suspend_rx(rx);
	rlc_am_base_rx_stop(&rx->base_rx);
}

static bool rlc_am_nr_rx_configure(rlc_am_nr_rx *rx, rlc_config_t *cfg_, char *rb_name_)
{
	rx->base_rx.rb_name = rb_name_;
	rx->base_rx.cfg = cfg_->am_nr;

	// configure timer
	if (rx->base_rx.cfg.t_status_prohibit > 0) {
		rx->status_prohibit_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_rx_timer_expired, &rx->base_rx);
	}

	// Configure t_reassembly timer
	if (rx->base_rx.cfg.t_reassembly > 0) {
		rx->reassembly_timer = gnb_timer_add(gnb_manager_self()->app_timer, rlc_am_base_rx_timer_expired, &rx->base_rx);
	}

	rx->mod_nr = cardinality(rx->base_rx.cfg.rx_sn_field_length);
	rx->AM_Window_Size = am_window_size(rx->base_rx.cfg.rx_sn_field_length);

	rx->base_rx.base.rx_enabled = true;

	RlcDebug("RLC AM NR configured rx entity.");

	return true;
}

static void rlc_am_nr_rx_init(rlc_am_nr_rx *rx, rlc_am_nr_tx *tx, rlc_um_base *base)
{
	rx->base_rx.base = base;
	rx->base_rx.tx = tx;
	rx->base_rx.rx = rx;

	rx->rx_window = oset_hash_make();
	rlc_am_base_rx_init(&rx->base_rx, base)
}

#endif

/////////////////////////////////////////////////////////////////////////////
void rlc_am_nr_stop(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_um_nr *)am_common;

	rlc_am_nr_rx_stop(&am->rx);
	rlc_am_nr_tx_stop(&am->tx);
	rlc_am_base_stop(&am->base);
}

void rlc_am_nr_get_buffer_state(rlc_common *am_common, uint32_t *n_bytes_newtx, uint32_t *n_bytes_prio)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	rlc_am_base_tx_get_buffer_state(&am->tx.base_tx, n_bytes_newtx, n_bytes_prio);
}


bool rlc_am_nr_configure(rlc_common *am_common, rlc_config_t *cfg_)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	// store config
	am->base.cfg = *cfg_;

	// determine bearer name and configure Rx/Tx objects
	am->base.common.rb_name = oset_strdup(get_rb_name(am->base.common.lcid));

	if (! rlc_am_nr_rx_configure(&am->rx, &am->base.cfg, am->base.common.rb_name)) {
		RlcError("Error configuring bearer (RX)");
		return false;
	}

	if (! rlc_am_nr_tx_configure(&am->tx, &am->base.cfg, am->base.common.rb_name)) {
		RlcError("Error configuring bearer (TX)");
		return false;
	}

	RlcInfo("AM NR configured - tx_sn_field_length=%d, rx_sn_field_length=%d, "
	        "t_poll_retx=%d, poll_pdu=%d, poll_byte=%d, "
	        "max_retx_thresh=%d, t_reassembly=%d, t_status_prohibit=%d, tx_queue_length=%d",
	        rlc_am_nr_sn_size_to_number(am->base.cfg.am_nr.tx_sn_field_length),
	        rlc_am_nr_sn_size_to_number(am->base.cfg.am_nr.rx_sn_field_length),
	        am->base.cfg.am_nr.t_poll_retx,
	        am->base.cfg.am_nr.poll_pdu,
	        am->base.cfg.am_nr.poll_byte,
	        am->base.cfg.am_nr.max_retx_thresh,
	        am->base.cfg.am_nr.t_reassembly,
	        am->base.cfg.am_nr.t_status_prohibit,
	        am->base.cfg.tx_queue_length);

  return true;
}

void rlc_am_nr_set_bsr_callback(rlc_common *am_common, bsr_callback_t callback)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	am->tx.base_tx.bsr_callback = callback;
}

rlc_bearer_metrics_t rlc_am_nr_get_metrics(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	// update values that aren't calculated on the fly
	uint32_t latency        = rlc_am_base_rx_get_sdu_rx_latency_ms(&am->rx.base_rx);
	uint32_t buffered_bytes = rlc_am_base_rx_get_rx_buffered_bytes(&am->rx.base_rx);

	oset_thread_mutex_lock(&am->base.metrics_mutex);
	am->base.metrics.rx_latency_ms     = latency;
	am->base.metrics.rx_buffered_bytes = buffered_bytes;
	oset_thread_mutex_unlock(&am->base.metrics_mutex);
	return am->base.metrics;
}

void rlc_am_nr_reset_metrics(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	oset_thread_mutex_lock(&am->base.metrics_mutex);
	am->base.metrics = {0};
	oset_thread_mutex_unlock(&am->base.metrics_mutex);
}

void rlc_am_nr_reestablish(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	RlcDebug("Reestablished bearer");
	rlc_am_nr_tx_reestablish(&am->tx); // calls stop and enables tx again
	rlc_am_nr_rx_reestablish(&am->rx); // calls only stop
	am->base.tx_enabled = true;
}

// NR AM RLC采用PUSH window+t-reassembly timer的形式接收下层递交的AMD PDU；
// 如果接收到的AMD PDU落在接收窗口外，直接丢弃；
// 如果接收到的AMD PDU落在接收窗口内，需要先进行重复性检测，没通过重复性检测的包或部分字节将被丢弃，通过重复性检测的包或字节放在接收buffer中；
// 如果一个原始RLC SDU的所有SDU segment都接收到了，将重组成原始RLC SDU并递交上层
void rlc_am_nr_write_ul_pdu(rlc_common *am_common, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	rlc_am_base_write_ul_pdu(&am->base, &am->rx.base_rx, payload, nof_bytes);
}

void rlc_am_nr_read_dl_pdu(rlc_common *am_common, uint8_t *payload, uint32_t nof_bytes)
{
	rlc_am_nr *am = (rlc_um_nr *)am_common;

	rlc_am_base_read_dl_pdu(&am->base, &am->tx.base_tx, payload, nof_bytes);
}


void rlc_am_nr_write_dl_sdu(rlc_common *am_common, byte_buffer_t *sdu)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	rlc_am_base_write_dl_sdu(&am->base, &am->tx.base_tx, sdu);
}

rlc_mode_t rlc_am_nr_get_mode(void)
{
	return rlc_mode_t(am);
}

bool rlc_am_nr_sdu_queue_is_full(rlc_common *am_common)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	return oset_list_count(am->tx.base_tx.tx_sdu_queue) == am->base.cfg.tx_queue_length;
}

void rlc_am_nr_discard_sdu(rlc_common *am_common, uint32_t discard_sn)
{
	rlc_am_nr *am = (rlc_am_nr *)am_common;

	if (!am->base.tx_enabled) {
		return;
	}

	rlc_am_base_tx_discard_sdu(&am->tx.base_tx, discard_sn);
}

rlc_am_nr *rlc_am_nr_init(uint32_t lcid_,	uint16_t rnti_)
{
	rlc_am_nr *am = oset_malloc(sizeof(rlc_am_nr));
	ASSERT_IF_NOT(am, "lcid %u Could not allocate pdcp nr context from pool", lcid_);
	memset(am, 0, sizeof(rlc_am_nr));

	rlc_am_base_init(&am->base, lcid_, rnti_);
	rlc_am_nr_tx_init(&am->tx, &am->rx, &am->base);
	rlc_am_nr_rx_init(&am->rx, &am->tx, &am->base);

	am->base.common.func = {
						._get_buffer_state  = rlc_am_nr_get_buffer_state,
						._configure         = rlc_am_nr_configure,
						._set_bsr_callback  = rlc_am_nr_set_bsr_callback,
						._get_metrics 	    = rlc_am_nr_get_metrics,
						._reset_metrics     = rlc_am_nr_reset_metrics,
						._reestablish       = rlc_am_nr_reestablish,
						._write_ul_pdu      = rlc_am_nr_write_ul_pdu,
						._read_dl_pdu       = rlc_am_nr_read_dl_pdu;
						._write_dl_sdu      = rlc_am_nr_write_dl_sdu,
						._get_mode		    = rlc_am_nr_get_mode,
						._sdu_queue_is_full	= rlc_am_nr_sdu_queue_is_full,
						._discard_sdu	    = rlc_am_nr_discard_sdu,
						._stop              = rlc_am_nr_stop,
					  };
}

