/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef UE_NR_H_
#define UE_NR_H_

#include "mac/mac_metrics.h"
#include "mac/mac_sch_pdu_nr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN_RLC_PDU_LEN  5 ///< minimum bytes that need to be available in a MAC PDU for attempting to add another RLC SDU

typedef struct {
    oset_lnode_t         lnode;
	uint64_t             conres_id;
	uint16_t             rnti;
	uint32_t             last_tti;
	uint32_t             nof_failures;
	bool                 active_state;//true

	oset_apr_mutex_t     *metrics_mutex;
	// TODO: some counters are kept as members of class ue_nr, while some others (i.e., mcs) are kept in the ue_metrics
	// We should make these counters more uniform
	uint32_t		 phr_counter;
	uint32_t		 dl_cqi_valid_counter;
	uint32_t		 dl_ri_counter;
	uint32_t		 dl_pmi_counter;
	uint32_t		 pucch_sinr_counter;
	uint32_t		 pusch_sinr_counter;
	mac_ue_metrics_t ue_metrics;

	// UE-specific buffer for MAC PDU packing, unpacking and handling
	mac_sch_pdu_nr	mac_pdu_dl;
	mac_sch_pdu_nr	mac_pdu_ul;

	byte_buffer_t    *ue_rlc_buffer;//oset_pkbuf_t??
	byte_buffer_t    *last_msg3; ///< holds UE ID received in Msg3 for ConRes CE
}ue_nr;//user xontext

ue_nr *ue_nr_add(uint16_t rnti);
void ue_nr_remove(ue_nr *ue_nr_cxt);
int ue_nr_set_rnti(ue_nr *ue_nr_cxt, uint16_t rnti);
ue_nr *ue_nr_find_by_rnti(uint16_t rnti);

#ifdef __cplusplus
}
#endif

#endif
