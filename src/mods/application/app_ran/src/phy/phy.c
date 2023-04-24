/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "rf/channel_2c.h"
#include "phy/phy.h"
#include "phy/prach_worker.h"
#include "lib/srsran/phy/utils/phy_logger.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-hphy"

#ifdef DEBUG_WRITE_FILE
	FILE*           f;
	static uint32_t num_slots     = 0;
	static uint32_t slots_to_dump = 10;
#endif


static phy_manager_t phy_manager = {
	.worker_args.nof_phy_threads = 1,
	.worker_args.nof_prach_workers = 0,
	.slot_args.cell_index = 0,
	.slot_args.nof_max_prb = SRSRAN_MAX_PRB_NR,
	.slot_args.nof_tx_ports = 1,
	.slot_args.nof_rx_ports = 1,
	.slot_args.rf_port = 0,
	.slot_args.scs = srsran_subcarrier_spacing_15kHz,
	.slot_args.pusch_max_its = 10,
	.slot_args.pusch_min_snr_dB = -10.0f,
	.slot_args.srate_hz = 0.0,
};
	
phy_manager_t *phy_manager_self(void)
{
	return &phy_manager;
}

static void phy_manager_init(void)
{
	phy_manager.app_pool = gnb_manager_self()->app_pool;
	oset_apr_mutex_init(&phy_manager.mutex, OSET_MUTEX_NESTED, phy_manager.app_pool);
	oset_apr_thread_cond_create(&phy_manager.cond, phy_manager.app_pool);
}

static void phy_manager_destory(void)
{
	oset_apr_mutex_destroy(phy_manager.mutex);
	oset_apr_thread_cond_destroy(phy_manager.cond);

    //pool
	phy_manager.app_pool = NULL; /*app_pool release by openset process*/
}

bool dl_channel_emulator(const phy_args_t *params, const phy_cfg_t *cfg)
{
	phy_manager.workers_common.cell_list_nr = cfg->phy_cell_cfg_nr;
	struct phy_cell_cfg_nr_t *cell_nr = &cfg->phy_cell_cfg_nr[0];

	// Instantiate DL channel emulator
	if (params->dl_channel_args.enable) {
	int channel_prbs = cell_nr->carrier.nof_prb;
		gnb_manager_self()->dl_channel = channel_create(params->dl_channel_args);
		channel_set_srate(gnb_manager_self()->dl_channel, (uint32_t)srsran_sampling_freq_hz(channel_prbs));
		channel_set_signal_power_dBfs(gnb_manager_self()->dl_channel, srsran_enb_dl_get_maximum_signal_power_dBfs(channel_prbs));
	}

	return true;
}


static void phy_parse_common_config(const phy_cfg_t *cfg)
{
	// PRACH configuration
	//phy_manager.prach_cfg.config_idx       = cfg->prach_cnfg.prach_cfg_info.prach_cfg_idx;
	//phy_manager.prach_cfg.hs_flag          = cfg->prach_cnfg.prach_cfg_info.high_speed_flag;
	//phy_manager.prach_cfg.root_seq_idx     = cfg->prach_cnfg.root_seq_idx;
	//phy_manager.prach_cfg.zero_corr_zone   = cfg->prach_cnfg.prach_cfg_info.zero_correlation_zone_cfg;
	//phy_manager.prach_cfg.freq_offset      = cfg->prach_cnfg.prach_cfg_info.prach_freq_offset;
	//phy_manager.prach_cfg.num_ra_preambles = 0;//???lte
	// DMRS
	//phy_manager.workers_common.dmrs_pusch_cfg.cyclic_shift        = cfg->pusch_cnfg.ul_ref_sigs_pusch.cyclic_shift;
	//phy_manager.workers_common.dmrs_pusch_cfg.delta_ss            = cfg->pusch_cnfg.ul_ref_sigs_pusch.group_assign_pusch;
	//phy_manager.workers_common.dmrs_pusch_cfg.group_hopping_en    = cfg->pusch_cnfg.ul_ref_sigs_pusch.group_hop_enabled;
	//phy_manager.workers_common.dmrs_pusch_cfg.sequence_hopping_en = cfg->pusch_cnfg.ul_ref_sigs_pusch.seq_hop_enabled;
}


static int init_nr(const phy_args_t& args, const phy_cfg_t& cfg)
{
	int rv = OSET_ERROR;

	if (cvector_empty(cfg->phy_cell_cfg_nr)) {
		return OSET_ERROR;
	}
	struct phy_cell_cfg_nr_t *cell_nr = cfg->phy_cell_cfg_nr[0];
	//todo one cell

	phy_manager.worker_args.nof_prach_workers = args->nof_prach_threads;//0 or 1 allow
	phy_manager.worker_args.nof_phy_threads = args->nof_phy_threads;

	rv = oset_threadpool_create(&phy_manager.th_pools, phy_manager.worker_args.nof_phy_threads, phy_manager.worker_args.nof_phy_threads);
	oset_assert(OSET_OK == rv);

	phy_manager.slot_args.cell_index = 0;
	phy_manager.slot_args.nof_max_prb	= cell_nr->carrier.nof_prb;
	phy_manager.slot_args.nof_tx_ports = cell_nr->carrier.max_mimo_layers;
	phy_manager.slot_args.nof_rx_ports = cell_nr->carrier.max_mimo_layers;
	phy_manager.slot_args.rf_port = cell_nr->rf_port;
	if(args->nr_pusch_max_its)   phy_manager.slot_args.pusch_max_its = args->nr_pusch_max_its;

	if (0 == phy_manager.slot_args.srate_hz) {
	 phy_manager.slot_args.srate_hz = SRSRAN_SUBC_SPACING_NR(cell_nr->carrier.scs) * srsran_min_symbol_sz_rb(cell_nr->carrier.nof_prb);//15k*2048=30.72MHz
	}

	if(!slot_worker_init(&phy_manager.slot_args)) return OSET_ERROR;

	/*phy_manager.common_cfg receive from rrc config  rrc_config_phy()*/
	if (set_common_cfg_from_rrc(&phy_manager.common_cfg)) {
		oset_error("Couldn't set common PHY config");
		return OSET_ERROR;
	}

	return OSET_OK;
}

bool slot_worker_set_common_cfg(const srsran_carrier_nr_t *carrier,
                                 const srsran_pdcch_cfg_nr_t *pdcch_cfg_,
                                 const srsran_ssb_cfg_t      *ssb_cfg_)
{
	// Set gNb DL carrier
	if (srsran_gnb_dl_set_carrier(&slot_manager.slot_worker.gnb_dl, carrier) < SRSRAN_SUCCESS) {
	oset_error("Error setting DL carrier");
	return false;
	}

	// Configure SSB
	if (srsran_gnb_dl_set_ssb_config(&slot_manager.slot_worker.gnb_dl, ssb_cfg_) < SRSRAN_SUCCESS) {
	oset_error("Error setting SSB");
	return false;
	}

	// Set gNb UL carrier
	if (srsran_gnb_ul_set_carrier(&slot_manager.slot_worker.gnb_ul, carrier) < SRSRAN_SUCCESS) {
	oset_error("Error setting UL carrier (pci=%d, nof_prb=%d, max_mimo_layers=%d)",
	             carrier.pci,
	             carrier.nof_prb,
	             carrier.max_mimo_layers);
	return false;
	}

	slot_manager.slot_worker.pdcch_cfg = *pdcch_cfg_;

	// Update subframe length
	slot_manager.slot_worker.sf_len = SRSRAN_SF_LEN_PRB_NR(carrier->nof_prb);

	return true;
}


static int set_common_cfg_from_rrc(common_cfg_t *common_cfg)
{
	// Best effort to convert NR carrier into LTE cell
	srsran_cell_t cell = {0};
	int           ret  = srsran_carrier_to_cell(&common_cfg->carrier, &cell);
	if (ret < OSET_OK) {
		oset_error("Converting carrier to cell for PRACH (%d)", ret);
		return OSET_ERROR;
	}

	// Best effort to set up NR-PRACH config reused for NR
	srsran_prach_cfg_t *prach_cfg           = &common_cfg->prach;
	uint32_t           lte_nr_prach_offset = (common_cfg->carrier.nof_prb - cell.nof_prb) / 2;
	if (prach_cfg->freq_offset < lte_nr_prach_offset) {
		oset_error("prach_cfg.freq_offset=%d is not compatible with LTE", prach_cfg->freq_offset);
		return OSET_ERROR;
	}
	prach_cfg->freq_offset -= lte_nr_prach_offset;
	prach_cfg->is_nr                 = true;
	prach_cfg->tdd_config.configured = (common_cfg->duplex_mode == SRSRAN_DUPLEX_MODE_TDD);

	// Set the PRACH configuration
	prach_worker_init(0, &cell, prach_cfg, phy_manager.worker_args.nof_prach_workers)
	prach_worker_manager_self(0)->max_prach_offset_us = 1000;

	// Setup SSB sampling rate and scaling
	srsran_ssb_cfg_t *ssb_cfg = &common_cfg->ssb;
	ssb_cfg->srate_hz         = phy_manager.slot_args.srate_hz;
	ssb_cfg->scaling = srsran_convert_dB_to_amplitude(srsran_gnb_dl_get_maximum_signal_power_dBfs(common_cfg->carrier.nof_prb));

	// Print SSB configuration, helps debugging gNb and UE
	char ssb_cfg_str[512] = {};
	srsran_ssb_cfg_to_str(ssb_cfg, ssb_cfg_str, sizeof(ssb_cfg_str));
	oset_debug("Setting SSB configuration %s", ssb_cfg_str);

	// For slot worker set configuration
	if(!slot_worker_set_common_cfg(&common_cfg->carrier, &common_cfg->pdcch, ssb_cfg)) return OSET_ERROR;

	return OSET_OK;
}

int phy_init(void)
{
	phy_manager_init();

	srsran_phy_log_register_handler(true);

	phy_args_t *args = &gnb_manager_self()->args.phy;
	phy_cfg_t *cfg = &gnb_manager_self()->phy_cfg;

	phy_manager.workers_common.params = *args;

	// Instantiate DL channel emulator
	dl_channel_emulator(args, cfg);

	if (cfg->cfr_config.cfr_enable) {
	  phy_manager.workers_common.cfr_config = cfg->cfr_config;
	}

	phy_parse_common_config(cfg);

	if (init_nr(args, cfg) != OSET_OK) {
	  oset_error("Couldn't initialize NR PHY");
	  return OSET_ERROR;
	}

	txrx_init();
	return OSET_OK;
}



int phy_destory(void)
{
	for (int i = 0; i < get_nof_carriers(); i++) {
		prach_worker_stop(i);
	}

	txrx_stop();
	//slot_worker
	slot_worker_destory();

	phy_manager_destory();
	return OSET_OK;
}


