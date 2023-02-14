/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SCHED_COMMON_H
#define SCHED_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/// Error code of alloc attempt
typedef enum alloc_result {
  success,
  sch_collision,
  no_cch_space,
  no_sch_space,
  no_rnti_opportunity,
  invalid_grant_params,
  invalid_coderate,
  no_grant_space,
  other_cause
}alloc_result;
  
inline const char* to_string(alloc_result result)
{
  switch (result) {
    case success:
      return "success";
    case sch_collision:
      return "Collision with existing SCH allocations";
    case other_cause:
      return "error";
    case no_cch_space:
      return "No space available in PUCCH or PDCCH";
    case no_sch_space:
      return "Requested number of PRBs not available";
    case no_rnti_opportunity:
      return "rnti cannot be allocated (e.g. already allocated, no data, meas gap collision, carrier inactive, etc.)";
    case invalid_grant_params:
      return "invalid grant arguments (e.g. invalid prb mask)";
    case invalid_coderate:
      return "Effective coderate exceeds threshold";
    case no_grant_space:
      return "Max number of allocations reached";
    default:
      break;
  }
  return "unknown error";
}

#ifdef __cplusplus
}
#endif

#endif
