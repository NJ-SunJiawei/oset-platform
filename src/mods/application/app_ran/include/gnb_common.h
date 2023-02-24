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
#include "lib/srsran/srsran.h"
#include "lib/common/buffer_interface.h"
#include "lib/common/bcd_interface.h"
#include "lib/common/asn_helper.h"
#include "lib/common/bcd_helper.h"
#include "lib/common/util_helper.h"
#include "lib/common/band_helper_2c.h"

#include "gnb_manager.h"
#include "gnb_task_interface.h"


#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb"

#ifdef __cplusplus
extern "C" {
#endif

#define SRSENB_MAX_UES   64

gnb_manager_t *gnb_manager_self(void);
task_map_t *task_map_self(task_id_t task_id);

const char *get_message_name(msg_ids_t message_id);
const char *get_task_name(task_id_t task_id);

#ifdef __cplusplus
}
#endif

#endif

