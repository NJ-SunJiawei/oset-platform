/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.09
************************************************************************/

#ifndef RLC_AM_H_
#define RLC_AM_H_

#include "rlc/rlc_common.h"

#ifdef __cplusplus
extern "C" {
#endif
#define  INVALID_RLC_SN           0xFFFFFFFF
#define  RETX_COUNT_NOT_STARTED   0xFFFFFFFF

#define  poll_periodicity         8 // After how many data PDUs a status PDU shall be requested
#define  invalid_rlc_sn           0xFFFFFFFF
#define  max_tx_queue_size        256

#define  rlc_am_nr_status_pdu_sizeof_header_ack_sn         3 ///< header fixed part and ACK SN
#define  rlc_am_nr_status_pdu_sizeof_nack_sn_ext_12bit_sn  2 ///< NACK SN and extension fields (12 bit SN)
#define  rlc_am_nr_status_pdu_sizeof_nack_sn_ext_18bit_sn  3 ///< NACK SN and extension fields (18 bit SN)
#define  rlc_am_nr_status_pdu_sizeof_nack_so               4 ///< NACK segment offsets (start and end)
#define  rlc_am_nr_status_pdu_sizeof_nack_range            1 ///< NACK range (nof consecutively lost SDUs)

///< AM NR PDU header
typedef struct  {
  rlc_dc_field_t      dc; ///< Data/Control (D/C) field
  uint8_t             p; ///< Polling bit
  rlc_nr_si_field_t   si; ///< Segmentation info
  rlc_am_nr_sn_size_t sn_size; ///< Sequence number size (12 or 18 bits)
  uint32_t            sn; ///< Sequence number
  uint16_t            so; ///< Sequence offset
}rlc_am_nr_pdu_header_t;

typedef struct {
  rlc_am_nr_pdu_header_t header;
  byte_buffer_t          *buf;
}rlc_amd_pdu_nr_t;

typedef struct {
  oset_lnode_t           lnode;
  rlc_am_nr_pdu_header_t header;
  byte_buffer_t          *buf;
  uint32_t               rlc_sn;
}rlc_amd_rx_pdu_nr;

typedef struct {
  uint32_t             rlc_sn;
  bool                 fully_received;
  bool                 has_gap;
  byte_buffer_t        *buf;
  oset_list_t          segments;// 按so从小到大排序 // std::set<rlc_amd_rx_pdu_nr, rlc_amd_rx_pdu_nr_cmp>;
}rlc_amd_rx_sdu_nr_t;

typedef struct {
  uint32_t             rlc_sn;
  byte_buffer_t        *buf;
}rlc_amd_tx_sdu_nr_t;

/// AM NR Status PDU header
typedef struct {
	/// Stored SN size required to compute the packed size
	rlc_am_nr_sn_size_t sn_size; // = rlc_am_nr_sn_size_t::nulltype
	/// Stored modulus to determine continuous sequences across SN overflows
	uint32_t mod_nr ; // = cardinality(rlc_am_nr_sn_size_t::nulltype)
	/// Internal NACK container; keep in sync with packed_size_
	cvector_vector_t(rlc_status_nack_t) nacks;
	/// Stores the current packed size; sync on each change of nacks_
	uint32_t packed_size; //= rlc_am_nr_status_pdu_sizeof_header_ack_sn
	/// CPT header
	rlc_am_nr_control_pdu_type_t cpt ;// = rlc_am_nr_control_pdu_type_t::status_pdu
	/// SN of the next not received RLC Data PDU
	uint32_t ack_sn ;//= INVALID_RLC_SN// = rx_highest_status
}rlc_am_nr_status_pdu_t;
///////////////////////////////////////////////////////////////////////////////
typedef struct rlc_am_base_s  rlc_am_base;
typedef struct rlc_am_nr_rx_s rlc_am_nr_rx;
typedef struct rlc_am_nr_tx_s rlc_am_nr_tx;


typedef struct rlc_am_base_s{
	rlc_common            common;
	rlc_config_t          cfg;
	bool                  tx_enabled;
	bool                  rx_enabled;
	oset_thread_mutex_t   metrics_mutex;
	rlc_bearer_metrics_t  metrics;
}rlc_am_base;

typedef struct {
	char				*rb_name;
	/****************************************************************************
	* Configurable parameters
	* Ref: 3GPP TS 38.322 version 16.2.0 Section 7.4
	***************************************************************************/
	rlc_am_nr_config_t  cfg;
	rlc_am_base         *base;
	rlc_am_nr_tx        *tx;
	rlc_am_nr_rx        *rx;

	bool                do_status; // light-weight access from Tx entity
	oset_thread_mutex_t mutex;
}rlc_am_base_rx;


typedef struct rlc_am_base_tx_sdu {
	oset_lnode_t       lnode;
	byte_buffer_t      *buffer;
} rlc_am_base_tx_sdu_t;

typedef struct {
	char			   *rb_name;
	/****************************************************************************
	* Configurable parameters
	* Ref: 3GPP TS 38.322 version 16.2.0 Section 7.4
	***************************************************************************/
	rlc_am_nr_config_t cfg;
	rlc_am_base        *base;
	rlc_am_nr_tx       *tx;
	rlc_am_nr_rx       *rx;
	bsr_callback_t      bsr_callback;

	// Tx SDU buffers
	oset_thread_mutex_t  unread_bytes_mutex;
	uint32_t             unread_bytes;
	oset_list_t          tx_sdu_queue;//rlc_am_base_tx_sdu_t
	// Mutexes
	oset_thread_mutex_t mutex;
}rlc_am_base_tx;

///////////////////////////////////////////////////////////////////////////////
typedef struct  {
	oset_lnode_t lnode;
	uint32_t so;
	uint32_t payload_len;
}pdu_segment;

typedef struct {
	uint32_t               rlc_sn;
	uint32_t               pdcp_sn;
	rlc_am_nr_pdu_header_t header;
	byte_buffer_t          *sdu_buf;
	uint32_t               retx_count;
	oset_list_t            segment_list;//pdu_segment
}rlc_amd_tx_pdu_nr;

/****************************************************************************
 * Tx state variables
 * Ref: 3GPP TS 38.322 version 16.2.0 Section 7.1
 ***************************************************************************/
typedef struct {
  /*
   * TX_Next_Ack: This state variable holds the value of the SN of the next RLC SDU for which a positive
   * acknowledgment is to be received in-sequence, and it serves as the lower edge of the transmitting window. It is
   * initially set to 0, and is updated whenever the AM RLC entity receives a positive acknowledgment for an RLC SDU
   * with SN = TX_Next_Ack.
   */
  uint32_t tx_next_ack;//该状态变量指示的是等待ACK的first AMD PDU的SN。该变量的初始值为0；当接收到SN等于TX_Next_Ack的AMD PDU的ACK指示时更新；(传输窗口的下边界)
  /*
   * TX_Next: This state variable holds the value of the SN to be assigned for the next newly generated AMD PDU. It is
   * initially set to 0, and is updated whenever the AM RLC entity constructs an AMD PDU with SN = TX_Next and
   * contains an RLC SDU or the last segment of a RLC SDU.
   */
  uint32_t tx_next;//该状态变量指示的是分配给下一个新产生的包含RLC SDU segment的AMD PDU的SN
  /*
   * POLL_SN: This state variable holds the value of the highest SN of the AMD PDU among the AMD PDUs submitted to
   * lower layer when POLL_SN is set according to sub clause 5.3.3.2. It is initially set to 0.
   */
  uint32_t poll_sn;
  /*
   * PDU_WITHOUT_POLL: This counter is initially set to 0. It counts the number of AMD PDUs sent since the most recent
   * poll bit was transmitted.
   */
  uint32_t pdu_without_poll;
  /*
   * BYTE_WITHOUT_POLL: This counter is initially set to 0. It counts the number of data bytes sent since the most
   * recent poll bit was transmitted.
   */
  uint32_t byte_without_poll;
}rlc_am_nr_tx_state_t;

typedef struct {
	oset_lnode_t lnode;
	uint32_t     sn;         ///< sequence number
	bool         is_segment; ///< flag whether this is a segment or not
	uint32_t     so_start;   ///< offset to first byte of this segment
	// so_end or segment_length are different for LTE and NR, hence are defined in subclasses
	uint32_t     current_so; ///< stores progressing SO during segmentation of this object
}rlc_amd_retx_base_t;

typedef struct {
	rlc_amd_retx_base_t  retx_base;
	uint32_t             segment_length; ///< number of bytes contained in this segment
}rlc_amd_retx_nr_t;

typedef struct rlc_am_nr_tx_s{
  rlc_am_base_tx  base_tx;

  /****************************************************************************
   * Tx state variables
   * Ref: 3GPP TS 38.322 version 16.2.0 Section 7.1
   ***************************************************************************/
  rlc_am_nr_tx_state_t   st;

  oset_hash_t      *tx_window;//rlc_amd_tx_pdu_nr//记录重传
  // Queues, buffers and container
  oset_list_t      retx_queue;//rlc_amd_retx_nr_t
  uint32_t         sdu_under_segmentation_sn; //= INVALID_RLC_SN // SN of the SDU currently being segmented.
  pdcp_sn_vector_t notify_info_vec;

  uint32_t		  mod_nr;
  uint32_t		  AM_Window_Size;
  // Helper constants
  uint32_t        min_hdr_size;  //= 2 // Pre-initialized for 12 bit SN, updated by configure()
  uint32_t        so_size;       //= 2
  uint32_t        max_hdr_size;  //= 4 // Pre-initialized for 12 bit SN, updated by configure()

  /****************************************************************************
   * TX timers
   * Ref: 3GPP TS 38.322 version 16.2.0 Section 7.3
   ***************************************************************************/
  gnb_timer_t *poll_retransmit_timer;// ARQ-轮询定时器；
                                     // polling的目的：发送端获取接收端的RLC PDU接收状态，从而进行数据重传；
}rlc_am_nr_tx;

/****************************************************************************
 * State Variables
 * Ref: 3GPP TS 38.322 version 16.2.0 Section 7.1
 ***************************************************************************/
typedef struct {
	/*
	* RX_Next: This state variable holds the value of the SN following the last in-sequence completely received RLC
	* SDU, and it serves as the lower edge of the receiving window. It is initially set to 0, and is updated whenever
	* the AM RLC entity receives an RLC SDU with SN = RX_Next.
	*/
	uint32_t rx_next;// [下边界]该变量指示的是last按序完全接收到的AMD PDU的SN+1。该变量初始值为0。是接收窗口的下边界,此变量之前的报文，接收端都已经全部成功接收。
	/*
	* RX_Next_Status_Trigger: This state variable holds the value of the SN following the SN of the RLC SDU which
	* triggered t-Reassembly.
	*/
	uint32_t rx_next_status_trigger;//该变量指示的是触发t-Ressembly timer的AMD PDU的SN+1。
	/*
	* RX_Next_Highest: This state variable holds the highest possible value of the SN which can be indicated by
	*"ACK_SN" when a STATUS PDU needs to be constructed. It is initially set to 0.
	*/
	uint32_t rx_highest_status;//该变量指示的是在构建STATUS PDU时可以通过ACK_SN指示的最高的可能的SN。该变量初始值为0。(考虑HARQ重传完成后，RLC实体可以判断接收状态的RLC SN.)

	/*
	* RX_Next_Highest: This state variable holds the value of the SN following the SN of the RLC SDU with the
	* highest SN among received RLC SDUs. It is initially set to 0.
	*/
	uint32_t rx_next_highest;//该变量指示的是接收到的AMD PDU中具有最大SN的AMD PDU的SN+1
}rlc_am_nr_rx_state_t;

typedef struct rlc_am_nr_rx_s{
	rlc_am_base_rx    base_rx;

	// RX Window
	oset_hash_t      *rx_window;//rlc_amd_rx_sdu_nr_t

	// Mutexes
	//oset_thread_mutex_t mutex;

	uint32_t		mod_nr;
	uint32_t		AM_Window_Size;
	/****************************************************************************
	* Rx timers
	* Ref: 3GPP TS 38.322 version 16.2.0 Section 7.3
	***************************************************************************/
	// t-StatusProhibit ARQ-状态报告定时器(防止频繁上报):
	//	  状态报告触发后，如果t-StatusProhibit 定时器没有运行，则在新传时（MAC 指示的），将构建的STATUS PDU递交底层，同时开启定时器；
	//	  状态报告触发后，如果t-StatusProhibit 定时器在运行，状态报告是不能发送的。需要等到定时器超时后，在第一个传输机会到来时，将构建的STATUS PDU递交给底层，同时开启定时器；
	//	构建STATUS PDU：
	//	  需要注意的是，t-Reassembly定时器超时会触发状态变量（RX_Highest_Status）更新以及状态报告。因此，应该以更新状态变量后的包接收状态来构建STATUS PDU(可理解为状态报告的触发点在状态变量更新之后)
	gnb_timer_t *status_prohibit_timer;

	gnb_timer_t *reassembly_timer;// 重组定时器 // to detect loss of RLC PDUs at lower layers

	/****************************************************************************
	* Rx state variables
	* Ref: 3GPP TS 38.322 version 16.2.0 Section 7.1
	***************************************************************************/
	rlc_am_nr_rx_state_t st;
}rlc_am_nr_rx;

typedef struct {
	rlc_am_base  base;
	rlc_am_nr_tx tx;
	rlc_am_nr_rx rx;
}rlc_am_nr;

rlc_am_nr *rlc_am_nr_init(uint32_t lcid_,	uint16_t rnti_);
void rlc_am_nr_stop(rlc_common *am_common);
void rlc_am_nr_get_buffer_state(rlc_common *am_common, uint32_t *n_bytes_newtx, uint32_t *n_bytes_prio);
bool rlc_am_nr_configure(rlc_common *am_common, rlc_config_t *cfg_);
void rlc_am_nr_set_bsr_callback(rlc_common *am_common, bsr_callback_t callback);
rlc_bearer_metrics_t rlc_am_nr_get_metrics(rlc_common *am_common);
void rlc_am_nr_reset_metrics(rlc_common *am_common);
void rlc_am_nr_reestablish(rlc_common *am_common);
void rlc_am_nr_write_ul_pdu(rlc_common *am_common, uint8_t *payload, uint32_t nof_bytes);
void rlc_am_nr_read_dl_pdu(rlc_common *am_common, uint8_t *payload, uint32_t nof_bytes);
void rlc_am_nr_write_dl_sdu(rlc_common *am_common, byte_buffer_t *sdu);
rlc_mode_t rlc_am_nr_get_mode(void);
bool rlc_am_nr_sdu_queue_is_full(rlc_common *am_common);
void rlc_am_nr_discard_sdu(rlc_common *am_common, uint32_t discard_sn);



#ifdef __cplusplus
}
#endif

#endif
