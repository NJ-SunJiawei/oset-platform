/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "gnb_common.h"
#include "mac/sched_nr_bwp.h"
	
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-bwp"

/// See TS 38.321, 5.1.3 - RAP transmission
int ra_sched_dl_rach_info(ra_sched *ra, rar_info_t *rar_info)
{
	// RA-RNTI = 1 + s_id + 14 × t_id + 14 × 80 × f_id + 14 × 80 × 8 × ul_carrier_id
	// s_id = index of the first OFDM symbol (0 <= s_id < 14)
	// t_id = index of first slot of the PRACH (0 <= t_id < 80)
	// f_id = index of the PRACH in the freq domain (0 <= f_id < 8) (for FDD, f_id=0)
	// ul_carrier_id = 0 for NUL and 1 for SUL carrier
	//根据时频信息gnb可以推算出ue的ra_rnti，用于msg2消息的加扰
	uint16_t ra_rnti = 1 + rar_info->ofdm_symbol_idx + 14 * slot_idx(rar_info->prach_slot) + 14 * 80 * rar_info->freq_idx;

	oset_info("SCHED: New PRACH slot=%d, preamble=%d, ra-rnti=0x%x, temp_crnti=0x%x, ta_cmd=%d, msg3_size=%d",
	          count_idx(&rar_info->prach_slot),
	          rar_info->preamble_idx,
	          ra_rnti,
	          rar_info->temp_crnti,
	          rar_info->ta_cmd,
	          rar_info->msg3_size);

	// find pending rar with same RA-RNTI
	// 时频相同，RA-RNTI相同的ra
	pending_rar_t *r = NULL;
	oset_stl_queue_foreach(&ra->pending_rars, r){
		if (r->prach_slot == rar_info->prach_slot && r->ra_rnti == ra_rnti) {
			if (MAX_GRANTS == cvector_size(r->msg3_grant)) {
				//PRACH被忽略，因为已达到每个tti的最大RAR授权数
				oset_error("PRACH ignored, as the the maximum number of RAR grants per tti has been reached");
				return OSET_ERROR;
			}
			cvector_push_back(r->msg3_grant, *rar_info);
			return OSET_OK;
		}
	}

	// create new RAR
	pending_rar_t p = {0};
	p.ra_rnti                            = ra_rnti;
	p.prach_slot                         = rar_info->prach_slot;
	const static uint32_t prach_duration = 1;
	for (slot_point t = rar_info->prach_slot + prach_duration; t < rar_info->prach_slot + cvector_size(ra->bwp_cfg->slots); ++t) {
		if (ra->bwp_cfg->slots[slot_idx(t)].is_dl) {
			slot_interval_init(p.rar_win, t, t + ra->bwp_cfg->cfg.rar_window_size);//rar windows
			break;
		}
	}
	cvector_push_back(p.msg3_grant, *rar_info);
	oset_stl_queue_add_last(&ra->pending_rars, p);
	return OSET_OK;
}


///////////////////////////////ra_sched///////////////////////////////////////////
static void ra_sched_destory(ra_sched *ra)
{
	pending_rar_t *elem = NULL;

	oset_stl_queue_foreach(&ra->pending_rars, elem){
		cvector_free(elem->msg3_grant);
	}
	oset_stl_queue_term(&ra->pending_rars);
}

static void ra_sched_init(ra_sched *ra, bwp_params_t *bwp_cfg_)
{
	  ra->bwp_cfg = bwp_cfg_;
	  oset_stl_queue_init(&ra->pending_rars);
}

///////////////////////////////bwp_manager///////////////////////////////////////
void bwp_manager_destory(bwp_manager *bwp)
{
	//ra
	ra_sched_destory(&bwp->ra);
	//si
	si_sched_destory(&bwp->si);
	//bwp_res_grid
	bwp_res_grid_destory(bwp->grid);
}

void bwp_manager_init(bwp_manager *bwp, bwp_params_t *bwp_cfg)
{
	bwp->cfg = bwp_cfg;
	ra_sched_init(&bwp->ra, bwp_cfg);
	si_sched_init(&bwp->si, bwp_cfg);
	bwp_res_grid_init(&bwp->grid, bwp_cfg);
}


