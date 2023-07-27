/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef MAC_H_
#define MAC_H_

#include "gnb_config_parser.h"
#include "lib/common/mac_pcap.h"
#include "mac/sched_nr.h"
#include "mac/ue_nr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NUMEROLOGY_IDX  0  /// only 15kHz supported at this stage
#define FIRST_RNTI      0x46

// BCH buffers
typedef struct sib_info_s {
  uint32_t			   index;
  uint32_t			   periodicity;
  byte_buffer_t        *payload;
}sib_info_t;


typedef struct mac_manager_s{
	oset_apr_memory_pool_t   *app_pool;
	oset_apr_mutex_t         *mutex;
	oset_apr_thread_cond_t   *cond;
	oset_apr_thread_rwlock_t *rwmutex;

	byte_buffer_t          *rar_pdu_buffer;

	mac_nr_args_t		   *args;
	
	mac_pcap               pcap;

	bool                   started;
	sched_nr               sched;
	cvector_vector_t(sched_nr_cell_cfg_t) cell_config;//std::vector<sched_nr_cell_cfg_t>
	// Map of active UEs
	oset_hash_t            *ue_db;//static_circular_map<uint16_t, std::unique_ptr<ue_nr>, SRSENB_MAX_UES>
	oset_list_t 		   mac_ue_list;
	uint16_t               ue_counter;
	cvector_vector_t(sib_info_t) bcch_dlsch_payload; //std::vector<sib_info_t>
	byte_buffer_t          *bcch_bch_payload;
	// Number of rach preambles detected for a CC
	cvector_vector_t(uint32_t)  detected_rachs;//SRSRAN_MAX_CARRIERS
	// Decoding of UL PDUs
	void                   *rx;
}mac_manager_t;
mac_manager_t *mac_manager_self(void);

///////////////////////////////////rrc/////////////////////////////////////////////////
int API_mac_rrc_cell_cfg(cvector_vector_t(sched_nr_cell_cfg_t) sched_cells);
int API_mac_rrc_api_ue_cfg(uint16_t rnti, sched_nr_ue_cfg_t *ue_cfg);
int API_mac_rrc_remove_user(uint16_t rnti);
///////////////////////////////////rlc/////////////////////////////////////////////////
int API_mac_rlc_buffer_state(uint16_t rnti, uint32_t lc_id, uint32_t tx_queue, uint32_t retx_queue);

/////////////////////////////////prach phy/////////////////////////////////////////////
//void mac_rach_detected(uint32_t tti, uint32_t enb_cc_idx, uint32_t preamble_idx, uint32_t time_adv);
ul_sched_t* API_mac_phy_get_ul_sched(srsran_slot_cfg_t* slot_cfg, uint32_t cc_idx);
dl_sched_t* API_mac_phy_get_dl_sched(srsran_slot_cfg_t *slot_cfg, uint32_t cc_idx);
int API_mac_phy_pucch_info(pucch_info_t *pucch_info, uint32_t cc_idx);
int API_mac_phy_pusch_info(srsran_slot_cfg_t *slot_cfg, pusch_info_t *pusch_info, uint32_t cc_idx);
///////////////////////////////////////////////////////////////////////////////////////
int mac_init(void);
int mac_destory(void);
void mac_get_metrics(mac_metrics_t *metrics);
void mac_store_msg3(uint16_t rnti, uint8 *sdu, int len);
void mac_remove_ue(uint16_t rnti);
void mac_remove_ue_all(void);
void *gnb_mac_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
