/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/

#ifndef RLC_LIB_H_
#define RLC_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rlc/rlc_tm.h"
#include "rlc/rlc_um.h"
#include "rlc/rlc_am.h"


typedef struct {
	oset_apr_memory_pool_t   *usepool;
	uint16_t		         rnti;
	//OSET_POOL(pool, byte_buffer_t);// 256
	oset_hash_t              *rlc_array; //std::map<uint16_t lcid, std::unique_ptr<rlc_common>>
	cvector_vector_t(uint32_t) valid_lcids_cached;
	//oset_hash_t              *rlc_array_mrb;// mrb 组播/多播技术
	//oset_apr_thread_rwlock_t *rwlock;
	uint32_t                 default_lcid;
	bsr_callback_t           bsr_callback;
	// Timer needed for metrics calculation
	oset_time_t              metrics_tp;
}rlc_lib_t;

rlc_common * rlc_valid_lcid(rlc_lib_t *rlc, uint32_t lcid);
void rlc_reset_metrics(rlc_lib_t *rlc);
void rlc_lib_init(rlc_lib_t *rlc, uint32_t lcid_, bsr_callback_t bsr_callback_);
void rlc_lib_stop(rlc_lib_t *rlc);
rlc_common *rlc_array_find_by_lcid(rlc_lib_t *rlc, uint32_t lcid);
void rlc_lib_del_bearer(rlc_lib_t *rlc, uint32_t lcid);
int rlc_lib_add_bearer(rlc_lib_t *rlc, uint32_t lcid, rlc_config_t *cnfg);
void rlc_lib_write_ul_pdu(rlc_lib_t *rlc, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes);
uint32_t rlc_lib_read_dl_pdu(rlc_lib_t *rlc, uint32_t lcid, uint8_t* payload, uint32_t nof_bytes);
void rlc_lib_write_dl_sdu(rlc_lib_t *rlc, uint32_t lcid, byte_buffer_t *sdu);
bool rlc_lib_rb_is_um(rlc_lib_t *rlc, uint32_t lcid);
bool rlc_lib_sdu_queue_is_full(rlc_lib_t *rlc, uint32_t lcid);


#ifdef __cplusplus
}
#endif

#endif
