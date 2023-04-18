/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "mac/base_ue_buffer_manager.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-sched-baseUe"

static bool is_lcid_valid(uint32_t lcid) { return lcid <= MAX_LC_ID;}
static bool is_lcg_valid(uint32_t lcg) { return lcg <= MAX_LCG_ID;}

static const char* dir_to_string(direction_t dir)
{
  switch (dir) {
    case IDLE:
      return "idle";
    case BOTH:
      return "bi-dir";
    case DL:
      return "DL";
    case UL:
      return "UL";
    default:
      return "unrecognized direction";
  }
}


static bool config_lcid_internal(base_ue_buffer_manager *base_ue, uint32_t lcid, mac_lc_ch_cfg_t bearer_cfg)
{
  if (!is_lcid_valid(lcid)) {
    oset_error("SCHED: Configuring rnti=0x%x bearer with invalid lcid=%d", base_ue->rnti, lcid);
    return false;
  }
  if (!is_lcg_valid(bearer_cfg.group)) {
    oset_error("SCHED: Configuring rnti=0x%x bearer with invalid logical channel group id=%d", base_ue->rnti, bearer_cfg.group);
    return false;
  }

  // update bearer config
  if (bearer_cfg != base_ue->channels[lcid].cfg) {
    base_ue->channels[lcid].cfg = bearer_cfg;
	//无穷大
    if (base_ue->channels[lcid].cfg.pbr == pbr_infinity) {
      base_ue->channels[lcid].bucket_size = 0x7fffffff//std::numeric_limits<int>::max();
      base_ue->channels[lcid].Bj          = 0x7fffffff//std::numeric_limits<int>::max();
    } else {
      base_ue->channels[lcid].bucket_size = base_ue->channels[lcid].cfg.bsd * base_ue->channels[lcid].cfg.pbr;//最大容量
      base_ue->channels[lcid].Bj          = 0;//Bj代表token数，每个token=1Byte  ，每个tti注入的      pbr*tti(1ms)Byte
    }
    return true;
  }
  return false;
}


void base_ue_buffer_manager_init(base_ue_buffer_manager *base_ue, uint16_t rnti_)
{
	base_ue->rnti = rnti_;
	memset(base_ue->lcg_bsr, 0, MAX_NOF_LCGS*sizeof(int));
}

void base_ue_buffer_manager_config_lcids(base_ue_buffer_manager *base_ue, mac_lc_ch_cfg_t bearer_cfg_list[SCHED_NR_MAX_LCID])
{
	cvector_vector_t(uint32_t) changed_list;
	cvector_reserve(changed_list, MAX_NOF_LCIDS);

	for (uint32_t lcid = 0; is_lcid_valid(lcid); ++lcid) {
		if (config_lcid_internal(base_ue, lcid, bearer_cfg_list[lcid])) {
		  // add to the changed_list the lcids that have been updated with new parameters
		  cvector_push_back(changed_list, lcid);
		}
	}

	// Log configurations of the LCIDs for which there were param updates
	if (!cvector_empty(changed_list)) {
		char fmtbuf[1024] = {0};
		char *p, *last = NULL;
	    p = fmtbuf;
        last = fmtbuf + 1024;

		for (uint32_t i = 0; i < cvector_size(changed_list); ++i) {
		  uint32_t lcid = changed_list[i];
		  p = oset_slprintf(p, last,
							 "%s{lcid={%d}, mode={%s}, prio={%d}, lcg={%d}}",
							 i > 0 ? ", " : "",
							 lcid,
							 dir_to_string(base_ue->channels[lcid].cfg.direction),
							 base_ue->channels[lcid].cfg.priority,
							 base_ue->channels[lcid].cfg.group);
		}
		oset_info("SCHED: rnti=0x%x, new lcid configuration: [%s]", base_ue->rnti, p);
	}
}


