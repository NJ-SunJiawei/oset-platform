/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef GNB_INTERFACE_H_
#define GNB_INTERFACE_H_

#include "oset-core.h"
#include "lib/srsran/srsran.h"

#ifdef __cplusplus
extern "C" {
#endif

/****phy_interface_rrc_nr****/
typedef struct {
  srsran_carrier_nr_t	carrier;
  srsran_pdcch_cfg_nr_t pdcch;
  srsran_prach_cfg_t	prach;
  srsran_ssb_cfg_t		ssb;
  srsran_duplex_mode_t	duplex_mode;
}common_cfg_t;


/****mac_interface_phy_nr****/
#define MAX_SSB			       4
#define MAX_GRANTS 		       4
#define MAX_PUCCH_MSG		   64
#define MAX_PUCCH_CANDIDATES   2
#define MAX_NZP_CSI_RS 	       4

typedef struct pdcch_dl_s {
  srsran_dci_cfg_nr_t dci_cfg;
  srsran_dci_dl_nr_t  dci;
}pdcch_dl_t;

typedef struct pdcch_ul_s {
  srsran_dci_cfg_nr_t dci_cfg;
  srsran_dci_ul_nr_t  dci;
}pdcch_ul_t;

typedef struct pdsch_s {
  srsran_sch_cfg_nr_t	sch; ///< PDSCH configuration
  byte_buffer_t         *data[SRSRAN_MAX_TB]; ///< Data pointer //实际传输tb数据指针
}pdsch_t;

typedef struct ssb_s {
  srsran_pbch_msg_nr_t pbch_msg;
}ssb_t;

typedef struct  dl_sched_s{
  cvector_vector_t(ssb_t)      ssb;//bounded_vector<ssb_t, MAX_SSB>
  cvector_vector_t(pdcch_dl_t *) pdcch_dl;//bounded_vector<pdcch_dl_t, MAX_GRANTS>
  cvector_vector_t(pdcch_ul_t *) pdcch_ul;//bounded_vector<pdcch_ul_t, MAX_GRANTS>
  cvector_vector_t(pdsch_t)    pdsch;//bounded_vector<pdsch_t, MAX_GRANTS>
  cvector_vector_t(srsran_csi_rs_nzp_resource_t) nzp_csi_rs;//bounded_vector<srsran_csi_rs_nzp_resource_t, MAX_NZP_CSI_RS>
}dl_sched_t;


typedef struct pusch_t {
  uint32_t			  pid;	///< HARQ process ID
  srsran_sch_cfg_nr_t sch; ///< PUSCH configuration
};

/**
 * @brief Describes a possible PUCCH candidate transmission
 * @note The physical layer shall try decoding all the possible PUCCH candidates and report back to the stack the
 * strongest of the candidates. This is thought to be used in the case of SR opportunities in which the UE could
 * transmit HARQ-ACK in two possible resources.
 */
typedef struct pucch_candidate_s {
  srsran_uci_cfg_nr_t		 uci_cfg;  ///< UCI configuration for the opportunity
  srsran_pucch_nr_resource_t resource; ///< PUCCH resource to use
}pucch_candidate_t;

typedef struct pucch_s {
  srsran_pucch_nr_common_cfg_t	pucch_cfg;  ///< UE dedicated PUCCH configuration
  cvector_vector_t(pucch_candidate_t) candidates; ///< PUCCH candidates to decode要解码的PUCCH候选//bounded_vector<pucch_candidate_t, MAX_PUCCH_CANDIDATES>
}pucch_t;

typedef struct ul_sched_s {
   cvector_vector_t(pusch_t) pusch;//bounded_vector<pusch_t, MAX_GRANTS>
   cvector_vector_t(pucch_t) pucch;//bounded_vector<pucch_t, MAX_GRANTS>
}ul_sched_t;

typedef struct pucch_info_s {
  srsran_uci_data_nr_t			uci_data; ///< RNTI is available under cfg->pucch->rnti
  srsran_csi_trs_measurements_t csi;	  ///< DMRS based signal Channel State Information (CSI)
}pucch_info_t;

typedef struct pusch_info_s {
  // Context
  uint16_t rnti;	///< UE temporal identifier
  uint32_t pid; ///< HARQ process ID

  // SCH and UCI payload information
  srsran_pusch_res_nr_t pusch_data;
  srsran_uci_cfg_nr_t	uci_cfg; ///< Provides UCI configuration, so stack does not need to keep the pending state

  // Actual SCH PDU
  byte_buffer_t         *pdu;

  // PUSCH signal measurements
  srsran_csi_trs_measurements_t csi; ///< DMRS based signal Channel State Information (CSI)
}pusch_info_t;

typedef struct rach_info_s {
  uint32_t enb_cc_idx;
  uint32_t slot_index;
  uint32_t preamble;
  uint32_t time_adv;
}rach_info_t;

/*******************phy_cell_cfg_nr_t************************/
typedef struct phy_cell_cfg_nr_s {
  srsran_carrier_nr_t carrier;//phy cell
  uint32_t            rf_port;
  uint32_t            cell_id;
  float               gain_db;
  bool                dl_measure;
}phy_cell_cfg_nr_t;

#ifdef __cplusplus
}
#endif

#endif
