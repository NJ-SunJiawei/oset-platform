/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef BAND_HELPER_2C_H_
#define BAND_HELPER_2C_H_

#include "band_helper.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct band_helper_t band_helper_t;

band_helper_t *band_helper_create(void);
uint32_t get_ul_arfcn_from_dl_arfcn_2c(band_helper_t *band_helper, uint32_t dl_arfcn);
srsran_duplex_mode_t get_duplex_mode_2c(band_helper_t *band_helper, uint16_t band);
uint32_t get_abs_freq_point_a_arfcn_2c(band_helper_t *band_helper, uint32_t nof_prb, uint32_t arfcn);
double nr_arfcn_to_freq_2c(band_helper_t *band_helper, uint32_t nr_arfcn);
uint32_t freq_to_nr_arfcn_2c(band_helper_t *band_helper, double freq);
srsran_ssb_pattern_t get_ssb_pattern_2c(band_helper_t *band_helper, uint16_t band, srsran_subcarrier_spacing_t scs);
uint32_t get_abs_freq_ssb_arfcn_2c(band_helper_t *band_helper, 
                                                    uint16_t                    band,
                                                    srsran_subcarrier_spacing_t scs,
                                                    uint32_t                    freq_point_a_arfcn,
                                                    uint32_t                    coreset0_offset_rb);

int band_helper_destory(band_helper_t *band_helper);

#ifdef __cplusplus
}
#endif

#endif
