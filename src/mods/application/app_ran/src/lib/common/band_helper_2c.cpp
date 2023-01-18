/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "oset-core.h"
#include "lib/common/band_helper_2c.h"

using namespace srsran;

band_helper_t *band_helper_create(void)
{
	srsran_band_helper *band_helper = new srsran_band_helper();
    return (band_helper_t *) band_helper;
}


uint32_t get_ul_arfcn_from_dl_arfcn_2c(band_helper_t *band_helper, uint32_t dl_arfcn)
{
	uint32_t ret = 0;

    if (band_helper == NULL) {
        return OSET_ERROR;
    }
    ret = ((srsran_band_helper *) band_helper)->get_ul_arfcn_from_dl_arfcn(dl_arfcn);
	return ret;
}


srsran_duplex_mode_t get_duplex_mode_2c(band_helper_t *band_helper, uint16_t band)
{
    if (band_helper == NULL) {
        return OSET_ERROR;
    }

    return ((srsran_band_helper *) band_helper)->get_duplex_mode(band);
}

uint32_t get_abs_freq_point_a_arfcn_2c(band_helper_t *band_helper, uint32_t nof_prb, uint32_t arfcn)
{
    if (band_helper == NULL) {
        return OSET_ERROR;
    }

    return ((srsran_band_helper *) band_helper)->get_abs_freq_point_a_arfcn(nof_prb, arfcn);
}


double nr_arfcn_to_freq_2c(band_helper_t *band_helper, uint32_t nr_arfcn)
{
    if (band_helper == NULL) {
        return OSET_ERROR;
    }
    return ((srsran_band_helper *) band_helper)->nr_arfcn_to_freq(nr_arfcn);
}

uint32_t freq_to_nr_arfcn_2c(band_helper_t *band_helper, double freq)
{
    if (band_helper == NULL) {
        return OSET_ERROR;
    }
    return ((srsran_band_helper *) band_helper)->freq_to_nr_arfcn(freq);
}

srsran_ssb_pattern_t get_ssb_pattern_2c(band_helper_t *band_helper, uint16_t band, srsran_subcarrier_spacing_t scs)
{
    if (band_helper == NULL) {
        return OSET_ERROR;
    }
    return ((srsran_band_helper *) band_helper)->get_ssb_pattern(band, scs);
}


uint32_t get_abs_freq_ssb_arfcn_2c(band_helper_t *band_helper, 
                                                    uint16_t                    band,
                                                    srsran_subcarrier_spacing_t scs,
                                                    uint32_t                    freq_point_a_arfcn,
                                                    uint32_t                    coreset0_offset_rb)
{
	if (band_helper == NULL) {
		return OSET_ERROR;
	}
	return ((srsran_band_helper *) band_helper)->get_abs_freq_ssb_arfcn(band, scs, freq_point_a_arfcn, coreset0_offset_rb);
}



int band_helper_destory(band_helper_t *band_helper)
{
    if (band_helper == NULL) {
        return OSET_ERROR;
    }
    delete reinterpret_cast<srsran_band_helper * >(band_helper);
    return OSET_OK;
}


