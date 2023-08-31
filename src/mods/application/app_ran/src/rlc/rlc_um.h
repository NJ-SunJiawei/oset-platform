/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/

#ifndef RLC_UM_H_
#define RLC_UM_H_

#include "rlc/rlc_um_base.h"


#ifdef __cplusplus
extern "C" {
#endif

// Transmitter sub-class for NR
#define head_len_full   1		 // full SDU header size is always

typedef struct {
	rlc_um_base_tx base_tx;
	uint32_t   TX_Next; // send state as defined in TS 38.322 v15.3 Section 7
						// It holds the value of the SN to be assigned for the next newly generated UMD PDU with
						// segment. It is initially set to 0, and is updated after the UM RLC entity submits a UMD PDU
						// including the last segment of an RLC SDU to lower layers.
	uint32_t   next_so; // The segment offset for the next generated PDU

	uint32_t   UM_Window_Size;
	uint32_t   mod;        // Rx counter modulus
	uint32_t   head_len_first;
	uint32_t   head_len_segment; // are computed during configure based on SN length
}rlc_um_nr_tx;

// Receiver sub-class for NR
typedef struct {
	rlc_um_nr_pdu_header_t header;
	byte_buffer_t          *buf;
} rlc_umd_pdu_nr_t;

// Rx window
typedef struct {
	oset_hash_t    *segments; //std::map<uint32_t, rlc_umd_pdu_nr_t> // Map of segments with SO as key
	byte_buffer_t  *sdu;
	uint32_t	   next_expected_so;
	uint32_t	   total_sdu_length;
} rlc_umd_pdu_segments_nr_t;


typedef struct {
	rlc_um_base_rx  base_rx;
	uint32_t      RX_Next_Reassembly; // the earliest SN that is still considered for reassembly
	uint32_t      RX_Timer_Trigger; // the SN following the SN which triggered t-Reassembly
	uint32_t      RX_Next_Highest; // the SN following the SN of the UMD PDU with the highest SN among
								 // received UMD PDUs. It serves as the higher edge of the reassembly window.
	uint32_t      UM_Window_Size;
	uint32_t      mod; // Rx counter modulus
	oset_hash_t   *rx_window;//std::map<uint32_t, rlc_umd_pdu_segments_nr_t>
	// TS 38.322 Sec. 7.3
	gnb_timer_t   *reassembly_timer; // to detect loss of RLC PDUs at lower layers
	oset_thread_mutex_t mutex;
}rlc_um_nr_rx;

typedef struct {
	rlc_um_base  base;
	rlc_um_nr_tx tx;
	rlc_um_nr_rx rx;
}rlc_um_nr;

rlc_um_nr *rlc_um_nr_init(uint32_t lcid_,	uint16_t rnti_);
void rlc_um_nr_stop(rlc_common *tm_common);

#ifdef __cplusplus
}
#endif

#endif
