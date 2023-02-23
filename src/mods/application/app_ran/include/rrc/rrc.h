/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef RRC_H_
#define RRC_H_

#include "lib/common/phy_cfg_nr.h"
#include "lib/common/bearer_manager.h"
#include "rrc/rrc_nr_ue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UE_PSCELL_CC_IDX  0 // first NR cell is always Primary Secondary Cell for UE

typedef enum { Rx = 0, Tx } direction_t;

typedef struct {
  A_DYN_ARRAY_OF(struct sib_type_and_info_item_c_)	sibs;
  A_DYN_ARRAY_OF(byte_buffer_t) *sib_buffer;//std::vector<srsran::unique_byte_buffer_t>
  struct cell_group_cfg_s       master_cell_group;
  phy_cfg_nr_t		            default_phy_ue_cfg_nr;
}cell_ctxt_t;

typedef struct rrc_manager_s{
	oset_apr_memory_pool_t    *app_pool;
	oset_apr_mutex_t          *mutex;
	oset_apr_thread_cond_t    *cond;

	rrc_nr_cfg_t			  cfg;
	// interfaces
	enb_bearer_manager        *bearer_mapper;
	// derived
	uint32_t			      slot_dur_ms;
	// vars
	du_config_manager         *du_cfg;

	cell_ctxt_t	              *cell_ctxt;
	oset_hash_t               *users;//srsran::static_circular_map<uint16_t, rrc_nr_ue, SRSENB_MAX_UES>
	bool					  running;
}rrc_manager_t;

void *gnb_rrc_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
