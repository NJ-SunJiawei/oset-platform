/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef COMMON_NR_H_
#define COMMON_NR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Radio Bearers
#define  MAX_NR_SRB_ID      3
#define  MAX_NR_DRB_ID      29
#define  MAX_NR_NOF_BEARERS  (MAX_NR_DRB_ID + MAX_NR_SRB_ID) // 32

typedef enum{ 
  srb0,
  srb1,
  srb2,
  srb3,
  count,
} nr_srb;

typedef enum{
  drb1 = 1,
  drb2,
  drb3,
  drb4,
  drb5,
  drb6,
  drb7,
  drb8,
  drb9,
  drb10,
  drb11,
  drb12,
  drb13,
  drb14,
  drb15,
  drb16,
  drb17,
  drb18,
  drb19,
  drb20,
  drb21,
  drb22,
  drb23,
  drb24,
  drb25,
  drb26,
  drb27,
  drb28,
  drb29,
  invalid,
}nr_drb;

inline bool is_nr_lcid(uint32_t lcid)
{
  return lcid < MAX_NR_NOF_BEARERS;
}

inline bool is_nr_srb(uint32_t srib)
{
  return srib <= MAX_NR_SRB_ID;
}

inline const char* get_srb_name(nr_srb srb_id)
{
  static const char* names[] = {"SRB0", "SRB1", "SRB2", "SRB3", "invalid SRB id"};
  return names[(uint32_t)(srb_id < count ? srb_id : count)];
}

inline uint32_t srb_to_lcid(nr_srb srb_id)
{
  return (uint32_t)srb_id;
}

inline nr_srb nr_lcid_to_srb(uint32_t lcid)
{
  return (nr_srb)lcid;
}

inline nr_drb nr_drb_id_to_drb(uint32_t drb_id)
{
  return (nr_drb)drb_id;
}

inline bool is_nr_drb(uint32_t drib)
{
  return drib > MAX_NR_SRB_ID && is_nr_lcid(drib);
}

inline const char* get_drb_name(nr_drb drb_id)
{
  static const char* names[] = {"DRB1",  "DRB2",  "DRB3",  "DRB4",  "DRB5",  "DRB6",          "DRB7",  "DRB8",
                                "DRB9",  "DRB10", "DRB11", "DRB12", "DRB13", "DRB14",         "DRB15", "DRB16",
                                "DRB17", "DRB18", "DRB19", "DRB20", "DRB21", "DRB22",         "DRB23", "DRB24",
                                "DRB25", "DRB26", "DRB27", "DRB28", "DRB29", "invalid DRB id"};
  return names[(uint32_t)(drb_id < invalid ? drb_id : invalid) - 1];
}

inline const char* get_rb_name(uint32_t lcid)
{
  return (lcid <= MAX_NR_SRB_ID) ? get_srb_name(nr_lcid_to_srb(lcid))
                                : get_drb_name((lcid - MAX_NR_SRB_ID));
}


#ifdef __cplusplus
}
#endif

#endif
