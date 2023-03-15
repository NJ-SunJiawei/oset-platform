/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef PHY_CFG_NR_DEFAULT_H
#define PHY_CFG_NR_DEFAULT_H

#include "lib/common/phy_cfg_nr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  /**
   * @brief Carrier reference configuration for 10MHz serving cell bandwidth
   * - BW: 10 MHZ (52 PRB)
   * - PCI: 500
   * - SCS: 15 kHz
   * - SSB: 5ms
   */
  R_CARRIER_CUSTOM_10MHZ = 0,
  /**
   * @brief Carrier reference configuration for 10MHz serving cell bandwidth
   * - BW: 20 MHZ (106 PRB)
   * - PCI: 500
   * - SCS: 15 kHz
   * - SSB: 5ms
   */
  R_CARRIER_CUSTOM_20MHZ,
  R_CARRIER_COUNT,
}carrier_type_e;
char *R_CARRIER_STRING[R_CARRIER_COUNT] = {"10MHz", "20MHz"};

typedef enum {
  /**
   * @brief FDD, all slots for DL and UL
   */
  R_DUPLEX_FDD = 0,

  /**
   * @brief TDD custom reference 5 slot DL and 5 slot UL
   */
  R_DUPLEX_TDD_CUSTOM_6_4,

  /**
   * @brief TDD pattern FR1.15-1 defined in TS38.101-4 Table A.1.2-1
   */
  R_DUPLEX_TDD_FR1_15_1,
  R_DUPLEX_COUNT,
}duplex_type_e;
char *R_DUPLEX_STRING[R_DUPLEX_COUNT] = {"FDD", "6D+4U", "FR1.15-1"};

typedef enum {
  /**
   * @brief Carrier reference configuration for 10MHz serving cell bandwidth
   * - CORESET: all channel, 1 symbol
   * - Single common Search Space
   * - 2 possible candidate per aggregation level to allow DL and UL grants simultaneously
   */
  R_PDCCH_CUSTOM_COMMON_SS = 0,
}pdcch_type_e;

typedef enum {
  /**
   * @brief Custom fallback baseline configuration, designed for component testing
   * - Defined single common PDSCH time allocation starting at symbol index 1 and length 13
   * - No DMRS dedicated configuration
   */
  R_PDSCH_DEFAULT = 0,

  /**
   * @brief PDSCH parameters described in TS 38.101-4 Table 5.2.2.2.1-2 for the test described in table 5.2.2.2.1-3
   */
  R_PDSCH_TS38101_5_2_1,

  /**
   * @brief Invalid PDSCH reference channel
   */
  R_PDSCH_COUNT,

}pdsch_type_e;
char *R_PDSCH_STRING[R_PDSCH_COUNT] = {"default", "ts38101/5.2-1"};

typedef enum {
  /**
   * @brief Custom fallback baseline configuration, designed for component testing
   * - Single Time resource allocation
   * - transmission starts at symbol index 0 for 14 symbols
   * - k is 4 slots
   * - No DMRS dedicated configuration
   */
  R_PUSCH_DEFAULT = 0,
}pusch_type_e;

typedef enum {
  /**
   * @brief Custom single PUCCH resource per set
   * - Format 1 for 1 or 2 bits
   * - Format 2 for more than 2 bits
   */
  R_PUCCH_CUSTOM_ONE = 0,
}pucch_type_e;

typedef enum {
  /**
   * @brief Sets the delay between PDSCH and HARQ feedback timing automatically
   * - Dynamic HARQ ACK codebook
   * - Guarantees a minimum delay of 4ms
   * - Assume 15kHz SCS
   * - Assume TDD pattern2 is not enabled
   */
  R_HARQ_AUTO = 0,
}harq_type_e;

typedef enum {
  /**
   * @brief Sets the PRACH configuration to an LTE compatible configuration
   * - Configuration index 0
   * - Frequency offset 2 PRB
   * - Root sequence 2
   */
  R_PRACH_DEFAULT_LTE,
}prach_type_e;

typedef struct {
	carrier_type_e carrier;
	duplex_type_e duplex;
	pdcch_type_e pdcch;
	pdsch_type_e pdsch;
	pusch_type_e pusch;
	pucch_type_e pucch;
	harq_type_e harq;
	prach_type_e prach;
}reference_cfg_t;

void phy_cfg_nr_default_init(reference_cfg_t *reference_cfg, phy_cfg_nr_t *phy_cfg);


#ifdef __cplusplus
}
#endif

#endif
