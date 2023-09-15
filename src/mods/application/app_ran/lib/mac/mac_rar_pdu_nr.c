/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.06
************************************************************************/
#include "gnb_common.h"
#include "lib/mac/mac_rar_pdu_nr.h"
	
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-libmacRAR"

static uint32_t get_total_length(mac_rar_subpdu_nr *subpdu)
{
  return (subpdu->header_length + subpdu->payload_length);
}

// Section 6.1.2
// |MAC subPDU 1(BI only)|MAC subPDU 2(RAPID only)|MAC subPDU 3(RAPID + RAR)|
// |E|T|R|R|	 BI 	 |E|T|	   RAPID		  |E|T|   RAPID|  MAC	RAR |

// |E|T|R|R|   BI  | OCT1	MAC sub header

// |E|T 	 PAPID | OCT1	MAC sub header
// |R|R|R|	 TA    | OCT1
// | TA   |UL Grant| OCT2
// |	UL Grant   | OCT3
// |	UL Grant   | OCT4
// |	UL Grant   | OCT5
// |	TC-RNTI    | OCT6
// |	TC-RNTI    | OCT7

static void write_subpdu(mac_rar_pdu_nr *rar_pdu, mac_rar_subpdu_nr *subpdu, const uint8_t* start_)
{
  uint8_t* ptr = const_cast<uint8_t*>(start_);

  if (subpdu->type == RAPID) {
    // write E/T/RAPID MAC subheader
    *ptr = (uint8_t)((subpdu->E_bit ? 1 : 0) << 7) | ((int)RAPID << 6) | ((uint8_t)subpdu->rapid & 0x3f);
    ptr += 1;

    // if PDU is not configured with SI request, insert MAC RAR
    if (rar_pdu->si_rapid_set == false) {
      // high 7 bits of TA go into first octet
      *ptr = (uint8_t)((subpdu->ta >> 5) & 0x7f);
      ptr += 1;

      // low 5 bit of TA and first 3 bit of UL grant
      *ptr = (uint8_t)((subpdu->ta & 0x1f) << 3) | (subpdu->ul_grant[0] << 2) | (subpdu->ul_grant[1] << 1) | (subpdu->ul_grant[2]);
      ptr += 1;

      // add remaining 3 full octets of UL grant
      uint8_t* x = &subpdu->ul_grant[3];
      *(ptr + 0) = (uint8_t)srsran_bit_pack(&x, 8);
      *(ptr + 1) = (uint8_t)srsran_bit_pack(&x, 8);
      *(ptr + 2) = (uint8_t)srsran_bit_pack(&x, 8);
      ptr += 3;

      // 2 byte C-RNTI
      *(ptr + 0) = (uint8_t)((subpdu->temp_crnti & 0xff00) >> 8);
      *(ptr + 1) = (uint8_t)(subpdu->temp_crnti & 0x00ff);
      ptr += 2;
    }
  } else {
    // write E/T/R/R/BI MAC subheader
    *ptr = (uint8_t)((subpdu->E_bit ? 1 : 0) << 7) | ((int)BACKOFF << 6) | ((uint8_t)subpdu->backoff_indicator & 0xf);
    ptr += 1;
  }
}

static void subpdu_to_string(mac_rar_subpdu_nr *subpdu, char *p, char *last)
{
  // Add space for new subPDU
  p = oset_slprintf(p, last, " ");

  if (subpdu->type == RAPID) {
    char tmp[16] = {0};
    srsran_vec_sprint_hex(tmp, sizeof(tmp), subpdu->ul_grant, UL_GRANT_NBITS);
    p = oset_slprintf(p, last, "RAPID: %u, Temp C-RNTI: 0x%x, TA: %u, UL Grant: %s", subpdu->rapid, subpdu->temp_crnti, subpdu->ta, tmp);
  } else {
  	p = oset_slprintf(p, last, "Backoff Indicator: %u", subpdu->backoff_indicator);
  }
}


//configured through SIB1 for on-demand SI request (See 38.331 Sec 5.2.1)
void  mac_rar_pdu_nr_set_si_rapid(mac_rar_pdu_nr *rar_pdu, uint16_t si_rapid_)
{
	rar_pdu->si_rapid	 = si_rapid_;
	rar_pdu->si_rapid_set = true;
}

int mac_rar_pdu_nr_init_tx(mac_rar_pdu_nr *rar_pdu, byte_buffer_t* buffer_, uint32_t pdu_len_)
{
	if (buffer_ == NULL || buffer_->msg == NULL) {
		oset_error("Invalid buffer");
		return OSET_ERROR;
	}
	rar_pdu->buffer          = buffer_;
	rar_pdu->buffer->N_bytes = 0;
	cvector_clear(rar_pdu->subpdus);
	rar_pdu->pdu_len       = pdu_len_;
	rar_pdu->remaining_len = pdu_len_;
	return OSET_OK;
}


int mac_rar_pdu_nr_pack(mac_rar_pdu_nr *rar_pdu)
{
  int ret = OSET_ERROR;
  if (rar_pdu->buffer == NULL) {
    oset_error("Invalid buffer");
    return ret;
  }

  if (cvector_empty(rar_pdu->subpdus)) {
	  // If we reach root, the allocation failed
	  return ret;
  }

  // set E_bit for last subPDU
  mac_rar_subpdu_nr *last_subpdu = cvector_back(rar_pdu->subpdus);
  last_subpdu->E_bit = false; // last

  // write subPDUs one by one
  mac_rar_subpdu_nr *subpdu = NULL;
  cvector_for_each_in(subpdu, rar_pdu->subpdus){
    if (rar_pdu->remaining_len >= get_total_length(subpdu)) {
      write_subpdu(rar_pdu, subpdu, rar_pdu->buffer->msg + rar_pdu->buffer->N_bytes);
      rar_pdu->buffer->N_bytes += get_total_length(subpdu);
      rar_pdu->remaining_len -= get_total_length(subpdu);
    } else {
      oset_error("Not enough space in PDU to write subPDU");
      return ret;
    }
  }

  // fill up with padding, if any
  if (rar_pdu->remaining_len > 0) {
	memset(rar_pdu->buffer->msg + rar_pdu->buffer->N_bytes, 0, rar_pdu->remaining_len);
	rar_pdu->buffer->N_bytes += rar_pdu->remaining_len;
  }

  ret = OSET_OK;

  return ret;
}

void mac_rar_pdu_nr_to_string(mac_rar_pdu_nr *rar_pdu)
{
	char dumpstr[128] = {0};
	char *p = NULL, *last = NULL;

	last = dumpstr + 128;
	p = dumpstr;
	
  	p = oset_slprintf(p, last, "%s", "DL");

	mac_rar_subpdu_nr *subpdu = NULL;
	cvector_for_each_in(subpdu, rar_pdu->subpdus){
		subpdu_to_string(subpdu, p, last);
	}
	oset_debug("%s", dumpstr);
}

