/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.07
************************************************************************/
#include "lib/rlc/rlc_common.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librlccom"

void rlc_common_init(rlc_common **common, char *rb_name, rlc_mode_t mode, oset_apr_memory_pool_t *usepool)
{
	rlc_common *entity = oset_core_alloc(usepool, sizeof(common));

	entity->rb_name = oset_core_strdup(usepool, rb_name);
	entity->suspended = false;
	entity->usepool = usepool;
	entity->mode = mode;
	entity->rx_pdu_resume_queue = oset_queue_create(static_blocking_queue_size);
	entity->tx_sdu_resume_queue = oset_queue_create(static_blocking_queue_size);
	entity->func = oset_core_alloc(usepool, sizeof(rlc_func_entity));
	*common = entity;
}

void rlc_common_destory(rlc_common *common)
{
	oset_queue_term(common->rx_pdu_resume_queue);
	oset_queue_destroy(common->rx_pdu_resume_queue);
	oset_queue_term(common->tx_sdu_resume_queue);
	oset_queue_destroy(common->tx_sdu_resume_queue);
}

// Enqueues the Rx PDU in the resume queue将Rx PDU排入恢复队列
void rlc_common_queue_rx_pdu(rlc_common *common, uint8_t* payload, uint32_t nof_bytes)
{
  byte_buffer_t *rx_pdu = byte_buffer_init();
  if (rx_pdu == nullptr) {
	srslog::fetch_basic_logger("RLC").warning("Couldn't allocate PDU in %s().", __FUNCTION__);
	return;
  }

  if (rx_pdu->get_tailroom() < nof_bytes) {
	srslog::fetch_basic_logger("RLC").warning("Not enough space to store PDU.");
	return;
  }

  memcpy(rx_pdu->msg, payload, nof_bytes);
  rx_pdu->N_bytes = nof_bytes;

  // Do not block ever
  if (!rx_pdu_resume_queue.try_push(std::move(rx_pdu))) {
	srslog::fetch_basic_logger("RLC").warning("Dropping SDUs while bearer suspended.");
	return;
  }
}



