/************************************************************************
 *File name:
 *Description:  stole from srsran
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/


#ifndef COMMON_H
#define COMMON_H

/*******************************************************************************
                              INCLUDES
*******************************************************************************/
#include <memory.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

/*******************************************************************************
                              DEFINES
*******************************************************************************/
#ifdef __cplusplus
	extern "C" {
#endif

#define SRSRAN_UE_CATEGORY 4

#define SRSRAN_N_SRB 3
#define SRSRAN_N_DRB 8
#define SRSRAN_N_RADIO_BEARERS 11

#define SRSRAN_N_MCH_LCIDS 32

#define FDD_HARQ_DELAY_DL_MS 4
#define FDD_HARQ_DELAY_UL_MS 4
#define MSG3_DELAY_MS 2 // Delay added to FDD_HARQ_DELAY_DL_MS

#define subcarrier_spacing_15kHz  0

#define TTI_LOOP(scs) (1024 * ((1U << (NUM)) * 10))
#define TTI_SUB(a, b) ((((a) + TTI_LOOP(1)) - (b)) % TTI_LOOP(subcarrier_spacing_15kHz))
#define TTI_ADD(a, b) (((a) + (b)) % TTI_LOOP(subcarrier_spacing_15kHz))

#define TTI_TX(tti) TTI_ADD(tti, FDD_HARQ_DELAY_DL_MS)

#define TTI_RX(tti) (TTI_SUB(tti, FDD_HARQ_DELAY_UL_MS))
#define TTI_RX_ACK(tti) (TTI_ADD(tti, FDD_HARQ_DELAY_UL_MS + FDD_HARQ_DELAY_DL_MS))

#define TTIMOD_SZ 20
#define TTIMOD(tti) (tti % TTIMOD_SZ)

#define INVALID_TTI 10241
#define TX_ENB_DELAY FDD_HARQ_DELAY_UL_MS

#define PHICH_MAX_SF 6 // Maximum PHICH in a subframe (1 in FDD, > 1 in TDD, see table 9.1.2-1 36.213)

#define ASYNC_DL_SCHED (FDD_HARQ_DELAY_UL_MS <= 4)

// Cat 4 UE - Max number of DL-SCH transport block bits received within a TTI
// 3GPP 36.306 v15.4.0 Table 4.1.1 for Category 11 with 2 layers and 256QAM
#define SRSRAN_MAX_TBSIZE_BITS 97896
#define SRSRAN_BUFFER_HEADER_OFFSET 1020
#define SRSRAN_MAX_BUFFER_SIZE_BITS (SRSRAN_MAX_TBSIZE_BITS + SRSRAN_BUFFER_HEADER_OFFSET)
#define SRSRAN_MAX_BUFFER_SIZE_BYTES (SRSRAN_MAX_TBSIZE_BITS / 8 + SRSRAN_BUFFER_HEADER_OFFSET)

/*******************************************************************************
                              TYPEDEFS
*******************************************************************************/
typedef enum srsran_rat_e { lte, nr, nulltype } srsran_rat_t;
// helper functions
inline const char* enum_to_text(const char* const array[], uint32_t nof_types, uint32_t enum_val)
{
  return enum_val >= nof_types ? "" : array[enum_val];
}

inline uint16_t enum_to_number(uint16_t* array, uint32_t nof_types, uint32_t enum_val)
{
  return enum_val >= nof_types ? -1 : array[enum_val];
}


#ifdef __cplusplus
}
#endif

#endif
