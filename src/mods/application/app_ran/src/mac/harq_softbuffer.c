/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/harq_softbuffer.h"
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-harq"

static void tx_harq_softbuffer_init(srsran_softbuffer_tx_t *buffer, uint32_t nof_prb_)
{
	// Note: for now we use same size regardless of nof_prb_
	srsran_softbuffer_tx_init_guru(buffer, SRSRAN_SCH_NR_MAX_NOF_CB_LDPC, SRSRAN_LDPC_MAX_LEN_ENCODED_CB);
}

static void tx_harq_softbuffer_destory(srsran_softbuffer_tx_t *buffer)
{
	srsran_softbuffer_tx_free(buffer);
}

static void rx_harq_softbuffer_init(srsran_softbuffer_rx_t *buffer, uint32_t nof_prb_)
{
	// Note: for now we use same size regardless of nof_prb_
	srsran_softbuffer_rx_init_guru(buffer, SRSRAN_SCH_NR_MAX_NOF_CB_LDPC, SRSRAN_LDPC_MAX_LEN_ENCODED_CB);
}

static void rx_harq_softbuffer_destory(srsran_softbuffer_rx_t *buffer)
{
	srsran_softbuffer_rx_free(buffer);
}


void harq_softbuffer_pool_init(harq_softbuffer_pool *harq_pool, uint32_t nof_prb, uint32_t batch_size, uint32_t init_size)
{
	ASSERT_IF_NOT(nof_prb <= SRSRAN_MAX_PRB_NR, "Invalid nof prb=%d", nof_prb);
	int i = 0;
	size_t idx = nof_prb - 1;
	if (harq_pool->tx_pool[idx] != NULL) {
		return;
	}

	if (init_size == 0) {
		init_size = batch_size;
	}

	//tx
	oset_pool_init(&harq_pool->tx_id_pool[idx], init_size);
	harq_pool->tx_pool[idx] = oset_malloc(sizeof(tx_harq_softbuffer));
	harq_pool->tx_pool[idx]->buffer = oset_malloc(init_size * sizeof(srsran_softbuffer_tx_t));
	for(i = 0, i < init_size; ++i){
		tx_harq_softbuffer_init(&harq_pool->tx_pool[idx]->buffer[i], nof_prb);//放入内存池
	}

	//rx
	oset_pool_init(&harq_pool->rx_id_pool[idx], init_size);
	harq_pool->rx_pool[idx] = oset_malloc(sizeof(rx_harq_softbuffer));
	harq_pool->rx_pool[idx]->buffer = oset_malloc(init_size * sizeof(srsran_softbuffer_rx_t));
	for(i = 0, i < init_size; ++i){
		rx_harq_softbuffer_init(&harq_pool->rx_pool[idx]->buffer[i], nof_prb);//放入内存池
	}
}

void harq_softbuffer_pool_destory(harq_softbuffer_pool *harq_pool, uint32_t nof_prb, uint32_t batch_size, uint32_t init_size)
{
	ASSERT_IF_NOT(nof_prb <= SRSRAN_MAX_PRB_NR, "Invalid nof prb=%d", nof_prb);
	int i = 0;
	size_t idx = nof_prb - 1;
	if (init_size == 0) {
		init_size = batch_size;
	}

	//tx
	for(i = 0, i < init_size; ++i){
		tx_harq_softbuffer_destory(&harq_pool->tx_pool[idx]->buffer[i]);
	}
	oset_free(harq_pool->tx_pool[idx]->buffer);
	oset_free(harq_pool->tx_pool[idx]);
	oset_pool_final(&harq_pool->tx_id_pool[idx]);

	//rx
	for(i = 0, i < init_size; ++i){
		rx_harq_softbuffer_destory(&harq_pool->rx_pool[idx]->buffer[i]);
	}
	oset_free(harq_pool->rx_pool[idx]->buffer);
	oset_free(harq_pool->rx_pool[idx]);
	oset_pool_final(&harq_pool->rx_id_pool[idx]);

}

srsran_softbuffer_tx_t *harq_softbuffer_pool_get_tx(harq_softbuffer_pool *harq_pool, uint32_t nof_prb)
{
	ASSERT_IF_NOT(nof_prb <= SRSRAN_MAX_PRB_NR, "Invalid Nprb=%d", nof_prb);

	uint32_t *buffer_id = NULL;
	srsran_softbuffer_tx_t *tx_buffer = NULL;

	size_t idx = nof_prb - 1;
	if (harq_pool->tx_pool[idx] == NULL) {
		harq_softbuffer_pool_init(harq_pool, nof_prb, 4 * MAX_HARQ, 0);
	}
	
	oset_pool_alloc(&harq_pool->tx_id_pool[idx], &buffer_id);
    oset_assert(buffer_id);
	
	tx_buffer = &harq_pool->tx_pool[idx]->buffer[oset_pool_index(&harq_pool->tx_id_pool[idx], buffer_id)];
	srsran_softbuffer_tx_reset(tx_buffer);

	return tx_buffer;
}

srsran_softbuffer_rx_t *harq_softbuffer_pool_get_rx(harq_softbuffer_pool *harq_pool, uint32_t nof_prb)
{
	ASSERT_IF_NOT(nof_prb <= SRSRAN_MAX_PRB_NR, "Invalid Nprb=%d", nof_prb);

	uint32_t *buffer_id = NULL;
	srsran_softbuffer_rx_t *rx_buffer = NULL;

	size_t idx = nof_prb - 1;
	if (harq_pool->rx_pool[idx] == NULL) {
		harq_softbuffer_pool_init(harq_pool, nof_prb, 4 * MAX_HARQ, 0);
	}
	
	oset_pool_alloc(&harq_pool->rx_id_pool[idx], &buffer_id);
    oset_assert(buffer_id);
	
	rx_buffer = &harq_pool->rx_pool[idx]->buffer[oset_pool_index(&harq_pool->rx_id_pool[idx], buffer_id)];
	srsran_softbuffer_rx_reset(rx_buffer);

	return rx_buffer;
}


