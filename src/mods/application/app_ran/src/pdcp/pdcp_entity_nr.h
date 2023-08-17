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

#include "pdcp/pdcp_entity_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * NR PDCP Entity
 * PDCP entity for 5G NR
 ***************************************************************************/
typedef void (*_reordering_callback)(void *);

// 所有状态变量是非负整数，取值从0 到 [2^32 – 1]. 
// PDCP Data PDUs 的整数序列号 (SN) 循环范围:从 0到 2^pdcp-SN-Size – 1.
// Window_Size：该变量指示重排序窗口的大小，其值等于2^(pdcp-SN-Size – 1).

// COUNT值共32bit，其中低SN bit代表SN号；高（32减SN)bit代表HFN;
// RCVD_SN: 接收到的 PDCP Data PDU的SN号, 包含在PDU头部；
// RCVD_HFN: 接收到的 PDCP Data PDU的HFN号，由PDCP接收实体计算;

typedef struct {
	pdcp_entity_base   base;
	// State variables: 3GPP TS 38.323 v15.2.0, section 7.1
	// TX_NEXT:指示下一个将要被发送的PDCP SDU的COUNT值
	uint32_t tx_next; // COUNT value of next SDU to be transmitted.

	// RX_NEXT:指示下一个期待收到的PDCP PDU的COUNT值
	uint32_t rx_next; // COUNT value of next SDU expected to be received.

	// RX_DELIV[下边界]:指示的是还没有递交上层但是等待递交的first PDCP SDU的COUNT值
	uint32_t rx_deliv; // COUNT value of first SDU not delivered to upper layers, but still waited for.

	// RX_REORD:指示触发了重排序定时器的关联的PDCP 数据 PDU的COUNT值
	uint32_t rx_reord; // COUNT value following the COUNT value of PDCP Data PDU which triggered t-Reordering.

	// Constants: 3GPP TS 38.323 v15.2.0, section 7.2
	uint32_t      window_size;

	// Reordering Queue / Timers
	oset_stl_heap_t  reorder_queue;//std::map<uint32_t, byte_buffer_t>
	oset_hash_t      *reorder_queue_hash;
	// 接收测：重排序定时器，一个接收侧实体同时只能启动一个。用于检测丢包（序号不连续），
	// 超时后和RLC的UM模式的重组定时器超时类似，接收侧只是简单的向上层递交，不要求对端重传
	gnb_timer_t  *reordering_timer;

	// Discard callback (discardTimer)
	// 发送侧: DRB丢弃定时器，只有DRB才有，发送侧对每一个SDU都会启动一个定时器，超时后丢弃该SDU。用于防止发送缓冲拥塞
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
void pdcp_entity_nr_stop(pdcp_entity_nr *pdcp_nr);
bool pdcp_entity_nr_configure(pdcp_entity_nr* pdcp_nr, pdcp_config_t *cnfg_);

#ifdef __cplusplus
}
#endif

#endif