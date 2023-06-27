/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#include "gnb_common.h"
#include "gnb_timer.h"
#include "rf/radio.h"
#include "phy/phy.h"
#include "phy/prach_worker.h"
#include "phy/txrx.h"
#include "mac/mac.h"
#include "rlc/rlc.h"
#include "pdcp/pdcp.h"
#include "rrc/rrc.h"


#define NUM_OF_APP_TIMER      2
static gnb_manager_t gnb_manager = {0};
static gnb_metrics_t gnb_metrics = {0};

char const* const prefixes[2][9] = {
    {
        "",
        "m",
        "u",
        "n",
        "p",
        "f",
        "a",
        "z",
        "y",
    },
    {
        "",
        "k",
        "M",
        "G",
        "T",
        "P",
        "E",
        "Z",
        "Y",
    },
};

static float clamp_sinr(float sinr)
{
  if (sinr > 99.9f) {
	return 99.9f;
  }
  if (sinr < -99.9f) {
	return -99.9f;
  }
  return sinr;
};


char* float_to_string(float f)
{
  const int degree = (f == 0.0) ? 0 : lrint(floor(log10f(fabs(f)) / 3));

  char factor[2] = {0};
  char result[10] = {0};

  if (abs(degree) < 9) {
    if (degree < 0)
      factor = prefixes[0][abs(degree)];
    else
      factor = prefixes[1][abs(degree)];
  } else {
    return "failed";
  }

  const double scaled = f * pow(1000.0, -degree);
  oset_snprintf(result, sizeof(result), "%5.2f%s", scaled, factor);
  return result;
}

static void set_metrics_helper(uint32_t        num_ue, mac_metrics_t *mac)
{
	char dumpstr[512] = {0};
	char *p = NULL, *last = NULL;

	for (size_t i = 0; i < num_ue; i++) {
		if (mac->ues[i].tx_errors > mac->ues[i].tx_pkts) {
			oset_log_print(OSET_LOG2_INFO,"tx caution errors %d > %d", mac->ues[i].tx_errors, mac->ues[i].tx_pkts);
		}
		if (mac->ues[i].rx_errors > mac->ues[i].rx_pkts) {
			oset_log_print(OSET_LOG2_INFO,"rx caution errors %d > %d", mac->ues[i].rx_errors, mac->ues[i].rx_pkts);
		}

		last = dumpstr + 512;
		p = dumpstr;

		p = oset_slprintf(p, last, "%4s", "nr");
		p = oset_slprintf(p, last, "%4u", mac->ues[i].pci);
		p = oset_slprintf(p, last, "%4u", mac->ues[i].rnti);
		if (!iszero(mac->ues[i].dl_cqi)) {
			p = oset_slprintf(p, last, "%4d", int(mac->ues[i].dl_cqi));
		} else {
			p = oset_slprintf(p, last, "%4s", "n/a");
		}
		p = oset_slprintf(p, last, "%4d", int(mac->ues[i].dl_ri));
		float dl_mcs = mac->ues[i].dl_mcs;
		if (!isnan(dl_mcs)) {
			p = oset_slprintf(p, last, "%4d", int(dl_mcs));
		} else {
			p = oset_slprintf(p, last, "%4d", 0);
		}
		if (mac->ues[i].tx_brate > 0) {
			p = oset_slprintf(p, last, "%4s", float_to_string((float)mac->ues[i].tx_brate / (mac->ues[i].nof_tti * 1e-3)));
		} else {
			p = oset_slprintf(p, last, "%4d", 0);
		}
		p = oset_slprintf(p, last, "%4d", mac->ues[i].tx_pkts - mac->ues[i].tx_errors);
		p = oset_slprintf(p, last, "%4d", mac->ues[i].tx_errors);		
		if (mac->ues[i].tx_pkts > 0 && mac->ues[i].tx_errors) {
			p = oset_slprintf(p, last, "%4d%", int((float)100 * mac->ues[i].tx_errors / mac->ues[i].tx_pkts));
		} else {
			p = oset_slprintf(p, last, "%4d", 0);	
		}

		p = oset_slprintf(p, last, "%4s", " |");


		float pusch_sinr = mac->ues[i].pusch_sinr;
		if (!isnan(pusch_sinr) && !iszero(pusch_sinr)) {
			p = oset_slprintf(p, last, "%5.2f", clamp_sinr(pusch_sinr));
		} else {
			p = oset_slprintf(p, last, "%4s", "n/a");
		}
		float pucch_sinr = mac->ues[i].pucch_sinr;
		if (!isnan(pucch_sinr) && ! iszero(pucch_sinr)) {
			p = oset_slprintf(p, last, "%5.2f", clamp_sinr(pucch_sinr));
		} else {
			p = oset_slprintf(p, last, "%4s", "n/a");
		}
		int phr = mac->ues[i].phr;
		if (!isnan(phr)) {
			p = oset_slprintf(p, last, "%4d", phr);
		} else {
			p = oset_slprintf(p, last, "%4d", 0);
		}
		float ul_mcs = mac->ues[i].ul_mcs;
		if (!isnan(ul_mcs)) {
			p = oset_slprintf(p, last, "%4d", int(ul_mcs));
		} else {
			p = oset_slprintf(p, last, "%4d", 0);
		}
		if (mac->ues[i].rx_brate > 0) {
			p = oset_slprintf(p, last, "%4s", float_to_string((float)mac->ues[i].rx_brate / (mac->ues[i].nof_tti * 1e-3)));
		} else {
			p = oset_slprintf(p, last, "%4d", 0);
		}
		p = oset_slprintf(p, last, "%4d", mac->ues[i].rx_pkts - mac->ues[i].rx_errors);
		p = oset_slprintf(p, last, "%4d", mac->ues[i].rx_errors);		
		if (mac->ues[i].rx_pkts > 0 && mac->ues[i].rx_errors) {
			p = oset_slprintf(p, last, "%4d%", int((float)100 * mac->ues[i].rx_errors / mac->ues[i].rx_pkts));
		} else {
			p = oset_slprintf(p, last, "%4d", 0);	
		}
		p = oset_slprintf(p, last, "%4s", float_to_string(mac->ues[i].ul_buffer));

		oset_log_print(OSET_LOG2_INFO, "%s", dumpstr);
	}
}


static void gnb_metrics_stdout(gnb_metrics_t *metrics)
{
	static int n_reports = 0;

	if (metrics->rf.rf_error) {
		oset_log_print(OSET_LOG2_INFO, "RF status: O=%u, U=%u, L=%u\n", metrics->rf.rf_o, metrics->rf.rf_u, metrics->rf.rf_l);
	}

	if (cvector_empty(metrics->nr_stack.mac.ues) {
		return;
	}

	if (++n_reports > 10) {
		n_reports = 0;
		oset_log_print(OSET_LOG2_INFO, "\n");
		oset_log_print(OSET_LOG2_INFO, "               -----------------DL----------------|-------------------------UL-------------------------");
		oset_log_print(OSET_LOG2_INFO, "rat  pci rnti  cqi  ri  mcs  brate   ok  nok  (%) | pusch  pucch  phr  mcs  brate   ok  nok  (%)    bsr");
	}

	//mac stdout print
	set_metrics_helper(cvector_size(metrics->nr_stack.mac.ues), metrics->nr_stack.mac);
}


static bool stack_get_metrics(stack_metrics_t *metrics)
{
  bool metrics_ready = false;

  rrc_get_metrics(metrics->rrc);
  // obtain MAC metrics 
  mac_get_metrics(&metrics->mac);

  return true;
}


static bool gnb_get_metrics(gnb_metrics_t* m)
{
  if (!gnb_manager.metrics_running) {
	return false;
  }
  rf_get_metrics(&m->rf);
  phy_get_metrics(m->phy);
  stack_get_metrics(&m->nr_stack);
  return true;
}


void gnb_metrics_handle(void)
{
	gnb_metrics = {0};
	gnb_get_metrics(&gnb_metrics);
	gnb_metrics_stdout(&gnb_metrics);
	
	cvector_free(gnb_metrics.phy);
	cvector_free(gnb_metrics.nr_stack.mac.cc_info);
	cvector_free(gnb_metrics.nr_stack.mac.ues);
	cvector_free(gnb_metrics.nr_stack.rlc.ues);
	cvector_free(gnb_metrics.nr_stack.pdcp.ues);
	cvector_free(gnb_metrics.nr_stack.rrc.ues);
}

static void *gnb_metrics_task(oset_threadplus_t *thread, void *data)
{
	msg_def_t *received_msg = NULL;
	uint32_t length = 0;
	task_map_t *task = taskmap[TASK_METRICS];
	int rv = 0;
	oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_NOTICE, "Starting metrics thread");

	while(gnb_manager_self()->running){
			gnb_metrics_handle();
			oset_msleep(oset_time_from_msec(gnb_manager.args.general.metrics_period_secs));
		}
	}
}

static void gnb_metrics_init(void)
{
	gnb_manager.metrics_running = 1;
}

static void gnb_metrics_destory(void)
{
	gnb_manager.metrics_running = 0;

	cvector_free(gnb_metrics.phy);
	cvector_free(gnb_metrics.nr_stack.mac.cc_info);
	cvector_free(gnb_metrics.nr_stack.mac.ues);
	cvector_free(gnb_metrics.nr_stack.rlc.ues);
	cvector_free(gnb_metrics.nr_stack.pdcp.ues);
	cvector_free(gnb_metrics.nr_stack.rrc.ues);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
gnb_manager_t *gnb_manager_self(void)
{
    return &gnb_manager;
}

void gnb_manager_init(void)
{
	gnb_manager.running = true;
	gnb_manager.app_timer = gnb_timer_mgr_create(SRSENB_MAX_UES * NUM_OF_APP_TIMER);
	oset_assert(gnb_manager.app_timer);
	gnb_manager.band_helper = band_helper_create();
	gnb_arg_default(&gnb_manager.args);
	gnb_arg_second(&gnb_manager.args);
	parse_cfg_files(&gnb_manager.args, &gnb_manager.rrc_cfg, &gnb_manager.rrc_nr_cfg, &gnb_manager.phy_cfg);
	gnb_metrics_init();
}

void gnb_manager_destory(void)
{
	gnb_manager.running = false;
	rrc_cell_cfg_nr_t *cell = NULL;
	cvector_for_each_in(cell, gnb_manager.rrc_nr_cfg.cell_list){
		cvector_free(cell->pdcch_cfg_common.common_search_space_list);
		cvector_free(cell->pdcch_cfg_ded.ctrl_res_set_to_add_mod_list);
		cvector_free(cell->pdcch_cfg_ded.search_spaces_to_add_mod_list);
	}
	cvector_free(gnb_manager.rrc_nr_cfg.cell_list);

	band_helper_destory(gnb_manager.band_helper);//???

	oset_list2_free(gnb_manager.rrc_nr_cfg.five_qi_cfg);

	cvector_free(gnb_manager.phy_cfg->phy_cell_cfg_nr);

	gnb_timer_mgr_destroy(gnb_manager.app_timer);

	if (gnb_manager.args.phy.dl_channel_args.enable) {
		channel_destory(gnb_manager.dl_channel);
	}

	if (gnb_manager.args.phy.ul_channel_args.enable) {
		channel_destory(gnb_manager.ul_channel);
	}
	gnb_manager.app_pool = NULL; /*app_pool release by openset process*/
}

void gnb_layer_tasks_args_init(void)
{
	task_map_self(TASK_TIMER)->info.func = gnb_timer_task;
	task_map_self(TASK_METRICS)->info.func = gnb_metrics_task;
	task_map_self(TASK_TXRX)->info.func = gnb_txrx_task;
	task_map_self(TASK_PRACH)->info.func = gnb_prach_task;
	task_map_self(TASK_MAC)->info.func = gnb_mac_task;
	task_map_self(TASK_RRC)->info.func = gnb_rrc_task;
}

int gnb_layer_tasks_create(void)
{
	/*todo bind CPU*/

	if (OSET_ERROR == task_thread_create(TASK_TIMER, NULL)) {
	  oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Create task for gNB Timer failed");
	  return OSET_ERROR;
	}

	if (OSET_ERROR == task_thread_create(TASK_METRICS, NULL)) {
	  oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Create task for gNB metrics failed");
	  return OSET_ERROR;
	}

	//rrc_init must start before phy, rrc_config_phy()
	//rrc_init must start before mac, rrc_config_mac()
	if (OSET_ERROR == task_thread_create(TASK_RRC, NULL)) {
	  oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Create task for gNB RRC failed");
	  return OSET_ERROR;
	}

	pdcp_init();
	rlc_init();

	if (OSET_ERROR == task_thread_create(TASK_MAC, NULL)) {
	  oset_log2_printf(OSET_CHANNEL_LOG, OSET_LOG2_ERROR, "Create task for gNB MAC failed");
	  return OSET_ERROR;
	}

	rf_init();
	phy_init();
    return OSET_OK;
}

void gnb_layer_tasks_destory(void)
{
	phy_destory();
	rf_destory();
	oset_threadplus_destroy(task_map_self(TASK_MAC)->thread, 1);
	pdcp_destory();
	rlc_destory();
	oset_threadplus_destroy(task_map_self(TASK_RRC)->thread, 1);
	oset_threadplus_destroy(task_map_self(TASK_METRICS)->thread, 1);
	oset_threadplus_destroy(task_map_self(TASK_TIMER)->thread, 1);
}

