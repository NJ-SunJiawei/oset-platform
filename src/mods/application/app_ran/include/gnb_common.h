/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef GNB_COMMON_H_
#define GNB_COMMON_H_

#include "oset-core.h"

#include "bcd_helper.h"
#include "util_helper.h"
#include "band_helper_2c.h"
#include "lib/common/security.h"
#include "gnb_task_interface.h"
#include "rrc_interface_types.h"
#include "srsran/srsran.h"


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  srsran_carrier_nr_t	carrier;
  srsran_pdcch_cfg_nr_t pdcch;
  srsran_prach_cfg_t	prach;
  srsran_ssb_cfg_t		ssb;
  srsran_duplex_mode_t	duplex_mode;
}common_cfg_t;


typedef struct gnb_manager_s gnb_manager_t;
typedef struct rf_manager_s rf_manager_t;
typedef struct phy_manager_s phy_manager_t;
typedef struct mac_manager_s mac_manager_t;
typedef struct rrc_manager_s rrc_manager_t;

gnb_manager_t *gnb_manager_self(void);
rf_manager_t *rf_manager_self(void);
phy_manager_t *phy_manager_self(void);
mac_manager_t *mac_manager_self(void);
rrc_manager_t *rrc_manager_self(void);
prach_worker_manager_t *prach_work_manager_self(void);
task_map_t *task_map_self(task_id_t task_id);

const char *get_message_name(msg_ids_t message_id);
const char *get_task_name(task_id_t task_id);
#ifdef __cplusplus
}
#endif

#endif

