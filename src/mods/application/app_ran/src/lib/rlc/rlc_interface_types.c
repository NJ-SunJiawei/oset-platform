/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
//#include "oset-core.h"
#include "lib/rlc/rlc_interface_types.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-librlc"

rlc_config_t default_rlc_config(void)
{
	rlc_config_t cfg          = {0};

	cfg.rat                   = (srsran_rat_t)nr;
	cfg.rlc_mode              = (rlc_mode_t)tm;
	cfg.tx_queue_length       = RLC_TX_QUEUE_LEN;
	return cfg;
}


rlc_config_t mch_config(void)
{
	rlc_config_t cfg          = {0};

	cfg.rat                   = (srsran_rat_t)lte;
	cfg.rlc_mode              = (rlc_mode_t)um;
	cfg.um.t_reordering       = 45;
	cfg.um.rx_sn_field_length = (rlc_umd_sn_size_t)size5bits;
	cfg.um.rx_window_size     = 16;
	cfg.um.rx_mod             = 32;
	cfg.um.tx_sn_field_length = (rlc_umd_sn_size_t)size5bits;
	cfg.um.tx_mod             = 32;
	cfg.um.is_mrb             = true;
	cfg.tx_queue_length       = 1024;
	return cfg;
}

rlc_config_t srb_config(uint32_t idx)
{
	rlc_config_t rlc_cfg         = {0};

	if (idx == 0 && idx > 2) {
	  return rlc_cfg;
	}
	// SRB1 and SRB2 are AM
	rlc_cfg.rat                  = (srsran_rat_t)lte;
	rlc_cfg.rlc_mode             = (rlc_mode_t)am;
	rlc_cfg.am.t_poll_retx       = 45;
	rlc_cfg.am.poll_pdu          = -1;
	rlc_cfg.am.poll_byte         = -1;
	rlc_cfg.am.max_retx_thresh   = 4;
	rlc_cfg.am.t_reordering      = 35;
	rlc_cfg.am.t_status_prohibit = 0;
	rlc_cfg.tx_queue_length      = RLC_TX_QUEUE_LEN;
	return rlc_cfg;
}

rlc_config_t default_rlc_um_config(uint32_t sn_size = 10)
{
	rlc_config_t cnfg    = {0};

	if (sn_size == 10) {
		cnfg.um.rx_sn_field_length = (rlc_umd_sn_size_t)size10bits;
		cnfg.um.rx_window_size     = 512;
		cnfg.um.rx_mod             = 1024;
		cnfg.um.tx_sn_field_length = (rlc_umd_sn_size_t)size10bits;
		cnfg.um.tx_mod             = 1024;
	} else if (sn_size == 5) {
		cnfg.um.rx_sn_field_length = (rlc_umd_sn_size_t)size5bits;
		cnfg.um.rx_window_size     = 16;
		cnfg.um.rx_mod             = 32;
		cnfg.um.tx_sn_field_length = (rlc_umd_sn_size_t)size5bits;
		cnfg.um.tx_mod             = 32;
	}else{
		return cnfg;		
	}

	cnfg.rat				   = (srsran_rat_t)lte;
	cnfg.rlc_mode			   = (rlc_mode_t)um;
	cnfg.um.t_reordering	   = 5;
	cnfg.tx_queue_length	   = RLC_TX_QUEUE_LEN;
	return cnfg;
}

rlc_config_t default_rlc_am_config()
{
	rlc_config_t rlc_cnfg         = {0};

	rlc_cnfg.rat                  = (srsran_rat_t)lte;
	rlc_cnfg.rlc_mode             = (rlc_mode_t)am;
	rlc_cnfg.am.t_reordering      = 5;
	rlc_cnfg.am.t_status_prohibit = 5;
	rlc_cnfg.am.max_retx_thresh   = 4;
	rlc_cnfg.am.poll_byte         = 25;
	rlc_cnfg.am.poll_pdu          = 4;
	rlc_cnfg.am.t_poll_retx       = 5;
	rlc_cnfg.tx_queue_length 	 = RLC_TX_QUEUE_LEN;
	return rlc_cnfg;
}

rlc_config_t default_rlc_am_nr_config(uint32_t sn_size = 12)
{
	rlc_config_t rlc_cnfg = {0};

	if (sn_size == 12) {
		rlc_cnfg.am_nr.tx_sn_field_length = (rlc_am_nr_sn_size_t)size12bits;
		rlc_cnfg.am_nr.rx_sn_field_length = (rlc_am_nr_sn_size_t)size12bits;
	} else if (sn_size == 18) {
		rlc_cnfg.am_nr.tx_sn_field_length = (rlc_am_nr_sn_size_t)size18bits;
		rlc_cnfg.am_nr.rx_sn_field_length = (rlc_am_nr_sn_size_t)size18bits;
	} else {
		return rlc_cnfg;
	}
	rlc_cnfg.rat		             = (srsran_rat_t)nr;
	rlc_cnfg.rlc_mode	             = (rlc_mode_t)am;
	rlc_cnfg.am_nr.t_status_prohibit = 8;
	rlc_cnfg.am_nr.max_retx_thresh   = 4;
	rlc_cnfg.am_nr.t_reassembly      = 35;
	rlc_cnfg.am_nr.poll_pdu          = 4;
	rlc_cnfg.am_nr.t_poll_retx       = 45;
	rlc_cnfg.tx_queue_length	     = RLC_TX_QUEUE_LEN;
	return rlc_cnfg;
}

rlc_config_t default_rlc_um_nr_config(uint32_t sn_size = 6)
{
	rlc_config_t cnfg = {0};

	if (sn_size == 6) {
	  cnfg.um_nr.sn_field_length = (rlc_um_nr_sn_size_t)size6bits;
	} else if (sn_size == 12) {
	  cnfg.um_nr.sn_field_length = (rlc_um_nr_sn_size_t)size12bits;
	} else {
	  return cnfg;
	}
	cnfg.rat          = (srsran_rat_t)nr;
	cnfg.rlc_mode     = (rlc_mode_t)um;

	cnfg.um_nr.t_reassembly_ms = 5; // lowest non-zero value
	cnfg.tx_queue_length	   = RLC_TX_QUEUE_LEN;
	return cnfg;
}

