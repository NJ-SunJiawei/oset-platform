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
#include "rrc/rrc_du_manager.h"
#include "rrc/rrc_ue.h"
#include "rrc/rrc_cell_asn_fill_inner.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UE_PSCELL_CC_IDX  0 // first NR cell is always Primary Secondary Cell for UE

typedef enum { Rx = 0, Tx } direction_t;

typedef struct {
	//cvector_vector_t(ASN_RRC_BCCH_DL_SCH_Message_t)  sibs;
	cvector_vector_t(struct sib_type_and_info_item_c_)	sibs;
	cvector_vector_t(byte_buffer_t *)   sib_buffer;//std::vector<srsran::unique_byte_buffer_t>
	//ASN_RRC_CellGroupConfig_t      *master_cell_group_out;
    struct cell_group_cfg_s        master_cell_group;
	phy_cfg_nr_t		           default_phy_ue_cfg_nr;
}cell_ctxt_t;

typedef struct rrc_manager_s{
	oset_apr_memory_pool_t    *app_pool;
	oset_apr_mutex_t          *mutex;
	oset_apr_thread_cond_t    *cond;

	rrc_nr_cfg_t			  *cfg;
	// interfaces
	enb_bearer_manager        bearer_mapper;
	// derived
	uint32_t			      slot_dur_ms;
	// vars
	du_config_manager         du_cfg;

	cell_ctxt_t	              *cell_ctxt;

	// UE Database
	oset_list_t 			  rrc_ue_list;
	oset_hash_t               *users;//<uint16_t, rrc_nr_ue, SRSENB_MAX_UES>

	bool					  running;
}rrc_manager_t;

rrc_manager_t *rrc_manager_self(void);

/*********************************************************************/
int rrc_init(void);
int rrc_destory(void);
int rrc_add_user(uint16_t rnti, uint32_t pcell_cc_idx, bool start_msg3_timer);
void rrc_rem_user(uint16_t rnti);
void rrc_rem_user_all(void);
void rrc_nr_ue_set_activity(rrc_nr_ue *ue, bool enabled);
void *gnb_rrc_task(oset_threadplus_t *thread, void *data);
/**********************mac api****************************************/
int API_rrc_mac_add_user(uint16_t rnti, uint32_t pcell_cc_idx);
int API_rrc_mac_read_pdu_bcch_dlsch(uint32_t sib_index, byte_buffer_t *buffer);
void API_rrc_mac_set_activity_user(uint16_t rnti);
int API_rrc_mac_update_user(uint16_t prev_rnti, uint16_t rnti);
/**********************rlc api****************************************/
void API_rrc_rlc_write_ul_pdu(uint16_t rnti, uint32_t lcid, byte_buffer_t *pdu);
/**********************pdcp api****************************************/
void API_rrc_pdcp_notify_integrity_error(uint16_t rnti, uint32_t lcid);

#ifdef __cplusplus
}
#endif

#endif
