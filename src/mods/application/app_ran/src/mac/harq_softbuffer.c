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

static void tx_harq_softbuffer_init(tx_harq_softbuffer *pool, uint32_t nof_prb_)
{
	// Note: for now we use same size regardless of nof_prb_
	srsran_softbuffer_tx_init_guru(&pool->buffer, SRSRAN_SCH_NR_MAX_NOF_CB_LDPC, SRSRAN_LDPC_MAX_LEN_ENCODED_CB);
}

static void tx_harq_softbuffer_destory(tx_harq_softbuffer *pool)
{
	srsran_softbuffer_tx_free(&pool->buffer);
}

static void rx_harq_softbuffer_init(rx_harq_softbuffer *pool, uint32_t nof_prb_)
{
	// Note: for now we use same size regardless of nof_prb_
	srsran_softbuffer_rx_init_guru(&pool->buffer, SRSRAN_SCH_NR_MAX_NOF_CB_LDPC, SRSRAN_LDPC_MAX_LEN_ENCODED_CB);
}

static void rx_harq_softbuffer_destory(rx_harq_softbuffer *pool)
{
	srsran_softbuffer_rx_free(&pool->buffer);
}


void rx_harq_softbuffer_reset(rx_harq_softbuffer *pool, uint32_t tbs_bits)
{
	srsran_softbuffer_rx_reset_tbs(&pool->buffer, tbs_bits);
}


void harq_softbuffer_pool_init(harq_softbuffer_pool *harq_pool, uint32_t nof_prb, uint32_t batch_size)
{
	ASSERT_IF_NOT(nof_prb <= SRSRAN_MAX_PRB_NR, "Invalid nof prb=%d", nof_prb);
	int i = 0;
	size_t idx = nof_prb - 1;
	if (harq_pool->tx_pool[idx] != NULL) {
		return;
	}

	harq_pool->batch_size = batch_size;


	//tx
	harq_pool->tx_buffer_idnex[idx] = 0;
	harq_pool->tx_pool[idx] = oset_malloc(sizeof(*harq_pool->tx_pool[idx]) * batch_size);
	for(i = 0, i < batch_size; ++i){
		tx_harq_softbuffer_init(&harq_pool->tx_pool[idx][i], nof_prb);//放入内存池
	}

	//rx
	harq_pool->rx_buffer_idnex[idx] = 0;
	harq_pool->rx_pool[idx] = oset_malloc(sizeof(*harq_pool->rx_pool[idx]) * batch_size);
	for(i = 0, i < batch_size; ++i){
		rx_harq_softbuffer_init(&harq_pool->rx_pool[idx][i], nof_prb);//放入内存池
	}
}

void harq_softbuffer_pool_destory(harq_softbuffer_pool *harq_pool, uint32_t nof_prb, uint32_t batch_size)
{
	ASSERT_IF_NOT(nof_prb <= SRSRAN_MAX_PRB_NR, "Invalid nof prb=%d", nof_prb);
	int i = 0;
	size_t idx = nof_prb - 1;

	//tx
	for(i = 0, i < batch_size; ++i){
		tx_harq_softbuffer_destory(&harq_pool->tx_pool[idx][i]);
	}
	oset_free(harq_pool->tx_pool[idx]);
	harq_pool->tx_buffer_idnex[idx] = 0;

	//rx
	for(i = 0, i < batch_size; ++i){
		rx_harq_softbuffer_destory(&harq_pool->rx_pool[idx][i]);
	}
	oset_free(harq_pool->rx_pool[idx]);
	harq_pool->rx_buffer_idnex[idx] = 0;

}

tx_harq_softbuffer *harq_softbuffer_pool_get_tx(harq_softbuffer_pool *harq_pool, uint32_t nof_prb)
{
	ASSERT_IF_NOT(nof_prb <= SRSRAN_MAX_PRB_NR, "Invalid Nprb=%d", nof_prb);
	tx_harq_softbuffer *tx_buffer = NULL;
	size_t idx = nof_prb - 1;

	ASSERT_IF_NOT(harq_pool->tx_buffer_idnex[idx] < harq_pool->batch_size, "Invalid tx_buffer_idnex[%ld]", idx);	

	if (harq_pool->tx_pool[idx] == NULL) {
		harq_softbuffer_pool_init(harq_pool, nof_prb, 4 * MAX_HARQ, 0);
	}
	
	tx_buffer = &harq_pool->tx_pool[idx][++harq_pool->tx_buffer_idnex[idx]];
	srsran_softbuffer_tx_reset(&tx_buffer->buffer);

	return tx_buffer;
}

rx_harq_softbuffer *harq_softbuffer_pool_get_rx(harq_softbuffer_pool *harq_pool, uint32_t nof_prb)
{
	ASSERT_IF_NOT(nof_prb <= SRSRAN_MAX_PRB_NR, "Invalid Nprb=%d", nof_prb);
	rx_harq_softbuffer *rx_buffer = NULL;
	size_t idx = nof_prb - 1;
	ASSERT_IF_NOT(harq_pool->rx_buffer_idnex[idx] < harq_pool->batch_size, "Invalid rx_buffer_idnex[%ld]", idx);	

	
	if (harq_pool->rx_pool[idx] == NULL) {
		harq_softbuffer_pool_init(harq_pool, nof_prb, 4 * MAX_HARQ, 0);
	}
	
	rx_buffer = &harq_pool->rx_pool[idx][++harq_pool->rx_buffer_idnex[idx]];
	srsran_softbuffer_rx_reset(&rx_buffer->buffer);

	return rx_buffer;
}


