/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.08
************************************************************************/

#ifndef PDCP_ENTITY_NR_H
#define PDCP_ENTITY_NR_H

#include "lib/pdcp/pdcp_entity_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * NR PDCP Entity
 * PDCP entity for 5G NR
 ***************************************************************************/
typedef void (*_reordering_callback)(void *);

typedef struct {
	pdcp_entity_base   base;
	// State variables: 3GPP TS 38.323 v15.2.0, section 7.1
	uint32_t tx_next; // COUNT value of next SDU to be transmitted.
	uint32_t rx_next; // COUNT value of next SDU expected to be received.
	uint32_t rx_deliv; // COUNT value of first SDU not delivered to upper layers, but still waited for.
	uint32_t rx_reord; // COUNT value following the COUNT value of PDCP Data PDU which triggered t-Reordering.

	// Constants: 3GPP TS 38.323 v15.2.0, section 7.2
	uint32_t      window_size;

	// Reordering Queue / Timers
	oset_hash_t  *reorder_queue;//std::map<uint32_t, byte_buffer_t>
	gnb_timer_t  *reordering_timer;

	// Discard callback (discardTimer)
	oset_hash_t  *discard_timers_map;//std::map<uint32_t, timer_handler::unique_timer>

	// COUNT overflow protection
	bool tx_overflow;
	bool rx_overflow;
	enum rlc_mode_t {
		UM,
		AM,
	} rlc_mode;
}pdcp_entity_nr;

pdcp_entity_nr* pdcp_entity_nr_init(uint32_t lcid_, uint16_t rnti_, oset_apr_memory_pool_t	*usepool);

#ifdef __cplusplus
}
#endif

#endif
