/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_NR_SCH_H_
#define SCHED_NR_SCH_H_

#include "mac/sched_common.h"
#include "mac/sched_nr_cfg.h"
#include "mac/ue_cfg_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  bwp_params_t  *bwp_cfg;
  uint32_t      slot_idx;//转换后的slotid
  cvector_vector_t(pdsch_t *) pdschs;//bounded_vector<pdsch_t, MAX_GRANTS>;
  bwp_rb_bitmap dl_prbs;
}pdsch_allocator;


typedef struct {
  bwp_params_t  *bwp_cfg;
  uint32_t      slot_idx;

  cvector_vector_t(pusch_t *) puschs;//bounded_vector<pusch_t, MAX_GRANTS>
  bwp_rb_bitmap ul_prbs;
}pusch_allocator;

typedef expected(pdsch_t*, alloc_result) pdsch_alloc_result;

inline pdcch_dl_alloc_result pdsch_alloc_result_succ(pdsch_t* pdsch)
{
	pdsch_alloc_result result = {0};
	result.has_val = true;
	result.res.val = pdsch;
	return result;
}

inline pdcch_dl_alloc_result pdsch_alloc_result_fail(alloc_result alloc_res)
{
	pdsch_alloc_result result = {0};
	result.has_val = false;
	result.res.unexpected = alloc_res;
	return result;
}


pdsch_t* pdsch_allocator_alloc_pdsch_unchecked(pdsch_allocator *pdsch_alloc,
														uint32_t                   coreset_id,
														srsran_search_space_type_t ss_type,
														srsran_dci_format_nr_t     dci_fmt,
														prb_grant                  *grant,
														srsran_dci_dl_nr_t         *out_dci);
pdsch_t* pdsch_allocator_alloc_ue_pdsch_unchecked(pdsch_allocator *pdsch_alloc,
															uint32_t 				  ss_id,
															srsran_dci_format_nr_t	  dci_fmt,
															prb_grant 		          *grant,
															ue_carrier_params_t       *ue,
															srsran_dci_dl_nr_t		  *dci);
//pdsch_alloc_result pdsch_allocator_alloc_si_pdsch(pdsch_allocator *pdsch_alloc, uint32_t ss_id, prb_grant *grant, srsran_dci_dl_nr_t *dci);
pdsch_t* pdsch_allocator_alloc_rar_pdsch_unchecked(pdsch_allocator *pdsch_alloc, prb_grant *grant, srsran_dci_dl_nr_t *dci);
pdsch_t* pdsch_allocator_alloc_si_pdsch_unchecked(pdsch_allocator *pdsch_alloc, uint32_t ss_id, prb_grant *grant, srsran_dci_dl_nr_t *dci);
alloc_result pdsch_allocator_is_si_grant_valid(pdsch_allocator *pdsch_alloc, uint32_t ss_id, prb_grant *grant);
alloc_result pdsch_allocator_is_rar_grant_valid(pdsch_allocator *pdsch_alloc, prb_grant *grant);
alloc_result pdsch_allocator_is_ue_grant_valid(pdsch_allocator *pdsch_alloc,
														ue_carrier_params_t        *ue,
														uint32_t                   ss_id,
														srsran_dci_format_nr_t     dci_fmt,
														prb_grant                  *grant);
void pdsch_allocator_reserve_prbs(pdsch_allocator *pdsch_alloc, prb_grant *grant);
prb_bitmap* pdsch_allocator_occupied_prbs(pdsch_allocator *pdsch_alloc, uint32_t ss_id, srsran_dci_format_nr_t dci_fmt);
rbg_bitmap* pdsch_allocator_occupied_rbgs(pdsch_allocator *pdsch_alloc);
void pdsch_allocator_destory(pdsch_allocator *pdsch_alloc);
void pdsch_allocator_reset(pdsch_allocator *pdsch_alloc);
void pdsch_allocator_init(pdsch_allocator *pdsch_alloc, bwp_params_t *cfg_, uint32_t slot_index, cvector_vector_t(pdsch_t) pdsch_lst);
////////////////////////////////////////////////////////////////////////////////////////////////////////
pusch_t* pusch_allocator_alloc_pusch_unchecked(pusch_allocator *pusch_alloc, prb_grant *grant, srsran_dci_ul_nr_t *out_dci);
prb_bitmap* pusch_allocator_occupied_prbs(pusch_allocator *pusch_alloc);
alloc_result pusch_allocator_is_grant_valid(pusch_allocator *pusch_alloc, srsran_search_space_type_t ss_type, prb_grant *grant, bool verbose);
void pusch_allocator_reserve_prbs(pusch_allocator *pdsch_alloc, prb_grant *grant);
alloc_result pusch_allocator_has_grant_space(pusch_allocator *pusch_alloc, uint32_t nof_grants, bool verbose);
void pusch_allocator_destory(pusch_allocator *pusch_alloc);
void pusch_allocator_reset(pusch_allocator *pusch_alloc);
void pusch_allocator_init(pusch_allocator *pusch_alloc, bwp_params_t *cfg_, uint32_t slot_index,  cvector_vector_t(pusch_t) pusch_lst);

#ifdef __cplusplus
}
#endif

#endif
