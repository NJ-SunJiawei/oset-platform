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

#include "rlc/rlc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rlc_um_nr_rx_s rlc_um_nr_rx;
typedef struct rlc_um_nr_tx_s rlc_um_nr_tx;

typedef struct rlc_um_base_tx_sdu {
	oset_lnode_t       lnode;
	byte_buffer_t      *buffer;
} rlc_um_base_tx_sdu_t;

// Transmitter sub-class base
typedef struct {
	char                  *rb_name;
	rlc_config_t          cfg;
	rlc_um_nr_tx          *tx;
	
	bsr_callback_t        bsr_callback;
	// TX SDU buffers
	oset_thread_mutex_t   unread_bytes_mutex;
	uint32_t              unread_bytes;
	oset_list_t           tx_sdu_queue;//rlc_um_base_tx_sdu_t
	byte_buffer_t         *tx_sdu;//存放分片报文
	// lte
	// rolling_average_t(double) mean_pdu_latency_us;
}rlc_um_base_tx;

typedef struct {
	char                  *rb_name;
	rlc_config_t          cfg;
	rlc_um_nr_rx          *rx;

	byte_buffer_t         *rx_sdu;
	uint32_t              lcid;
	rlc_bearer_metrics_t  *metrics;
}rlc_um_base_rx;


typedef struct rlc_um_base_s{
	rlc_common            common;
	rlc_config_t          cfg;
	bool                  tx_enabled;
	bool                  rx_enabled;
	oset_thread_mutex_t   metrics_mutex;
	rlc_bearer_metrics_t  metrics;
}rlc_um_base;



////////////////////////////////////////////////////////////////////
// Transmitter sub-class for NR
#define head_len_full   1		 // full SDU header size is always

typedef struct rlc_um_nr_tx_s{
	rlc_um_base_tx base_tx;
	uint32_t   TX_Next; // 该状态变量指示的是分配给下一个新产生的包含RLC SDU segment的UMD PDU的SN.
	                    // 该变量的初始值为0，在UM RLC向下层递交了包含RLC SDU最后一个片段的UMD PDU后按步长1更新;
	                    // send state as defined in TS 38.322 v15.3 Section 7
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


typedef struct rlc_um_nr_rx_s {
	rlc_um_base_rx  base_rx;
	uint32_t      RX_Next_Reassembly; // [下边界]该变量指示的是接收到的UMD PDU中最早的待组装的PDU的SN。该变量的初始值为0，是接收窗口的下边界
	                                  // the earliest SN that is still considered for reassembly
	uint32_t      RX_Timer_Trigger;   // 该变量指示的是触发t-Ressembly timer的UMD PDU的SN+1(用于RLC分段的重组)
									  // the SN following the SN which triggered t-Reassembly
	uint32_t      RX_Next_Highest; // [上边界]该变量指示的是的接收到的UMD PDU的最大SN+1。该变量的初始值为0，是接收窗口的上边界
								   // the SN following the SN of the UMD PDU with the highest SN among
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
void rlc_um_nr_stop(rlc_common *um_common);
bool rlc_um_nr_configure(rlc_common *um_common, rlc_config_t *cnfg_);
void rlc_um_nr_read_dl_pdu(rlc_common *um_common, uint8_t *payload, uint32_t nof_bytes);
void rlc_um_nr_write_dl_sdu(rlc_common *um_common, byte_buffer_t *sdu);
rlc_mode_t rlc_um_nr_get_mode(void);
bool rlc_um_nr_sdu_queue_is_full(rlc_common *um_common);

#ifdef __cplusplus
}
#endif

#endif
