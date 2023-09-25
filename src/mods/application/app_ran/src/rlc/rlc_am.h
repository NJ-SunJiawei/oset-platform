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
  rlc_am_nr_pdu_header_t header;
  byte_buffer_t          *buf;
  uint32_t               rlc_sn;
}rlc_amd_rx_pdu_nr;

typedef struct {
  uint32_t             rlc_sn;
  bool                 fully_received;
  bool                 has_gap;
  byte_buffer_t        *buf;
  using segment_list_t = std::set<rlc_amd_rx_pdu_nr, rlc_amd_rx_pdu_nr_cmp>;
  segment_list_t segments;
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
	uint32_t ack_sn ;//= INVALID_RLC_SN
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
	oset_list_t          tx_sdu_queue;
	// Mutexes
	oset_thread_mutex_t mutex;
}rlc_am_base_tx;

///////////////////////////////////////////////////////////////////////////////
typedef struct  {
	oset_lnode_t lnode;
	uint32_t so;
	uint32_t payload_len;
}pdu_segment;

typedef struct rlc_amd_tx_pdu_nr {
	uint32_t               rlc_sn;
	uint32_t               pdcp_sn;
	rlc_am_nr_pdu_header_t header;
	byte_buffer_t          *sdu_buf;
	uint32_t               retx_count;
	oset_list_t            segment_list;//pdu_segment
};

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
  uint32_t tx_next_ack;
  /*
   * TX_Next: This state variable holds the value of the SN to be assigned for the next newly generated AMD PDU. It is
   * initially set to 0, and is updated whenever the AM RLC entity constructs an AMD PDU with SN = TX_Next and
   * contains an RLC SDU or the last segment of a RLC SDU.
   */
  uint32_t tx_next;
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

  oset_hash_t      *tx_window;//rlc_amd_tx_pdu_nr
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
  gnb_timer_t *poll_retransmit_timer;
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
	uint32_t rx_next;
	/*
	* RX_Next_Status_Trigger: This state variable holds the value of the SN following the SN of the RLC SDU which
	* triggered t-Reassembly.
	*/
	uint32_t rx_next_status_trigger;
	/*
	* RX_Next_Highest: This state variable holds the highest possible value of the SN which can be indicated by
	*"ACK_SN" when a STATUS PDU needs to be constructed. It is initially set to 0.
	*/
	uint32_t rx_highest_status;
	/*
	* RX_Next_Highest: This state variable holds the value of the SN following the SN of the RLC SDU with the
	* highest SN among received RLC SDUs. It is initially set to 0.
	*/
	uint32_t rx_next_highest;
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
	gnb_timer_t *status_prohibit_timer;
	gnb_timer_t *reassembly_timer;

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

bool rlc_am_nr_configure(rlc_common *am_common, rlc_config_t *cfg_);


#ifdef __cplusplus
}
#endif

#endif
