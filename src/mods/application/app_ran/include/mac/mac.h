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
  oset_pkbuf_t         *payload;
}sib_info_t;


typedef struct mac_manager_s{
	oset_apr_memory_pool_t   *app_pool;
	oset_apr_mutex_t         *mutex;
	oset_apr_thread_cond_t   *cond;
	oset_apr_thread_rwlock_t *rwmutex;

	byte_buffer_t          *rar_pdu_buffer;

	mac_nr_args_t		   *args;
	// initial UE config, before RRC setup (without UE-dedicated)
    phy_cfg_nr_t           default_ue_phy_cfg;
	mac_pcap               pcap;

	bool                   started;
	sched_nr               sched;
	cvector_vector_t(sched_nr_cell_cfg_t) cell_config;//std::vector<sched_nr_cell_cfg_t>
	// Map of active UEs
	OSET_POOL(ue_pool, ue_nr); //mac rnti user context
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
int mac_cell_cfg(cvector_vector_t(sched_nr_cell_cfg_t) sched_cells);
void mac_remove_ue(uint16_t rnti);
int mac_ue_cfg(uint16_t rnti, sched_nr_ue_cfg_t ue_cfg);//rrc_nr::ue::update_mac
/////////////////////////////////prach phy/////////////////////////////////////////////
void mac_rach_detected(uint32_t tti, uint32_t enb_cc_idx, uint32_t preamble_idx, uint32_t time_adv);

///////////////////////////////////sched///////////////////////////////////////////////
int mac_slot_indication(srsran_slot_cfg_t *slot_cfg);
dl_sched_t* mac_get_dl_sched(srsran_slot_cfg_t *slot_cfg);
///////////////////////////////////////////////////////////////////////////////////////
void mac_get_metrics(mac_metrics_t *metrics);
void mac_remove_ue_all(void);
void *gnb_mac_task(oset_threadplus_t *thread, void *data);

#ifdef __cplusplus
}
#endif

#endif
