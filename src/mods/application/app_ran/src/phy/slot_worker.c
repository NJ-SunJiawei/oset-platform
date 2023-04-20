/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.04
************************************************************************/
#include "gnb_common.h"
#include "phy/phy.h"
#include "phy/slot_worker.h"
#include "mac/mac.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-slot-worker"


void slot_worker_work_imp(void)
{
  // Inform Scheduler about new slot
  mac_slot_indication(&phy_manager_self()->slot_worker.dl_slot_cfg);//do nothing

  // Get Transmission buffer获取发送buf
  uint32_t            nof_ant      = (uint32_t)tx_buffer.size();//rf 发送buffer
  srsran::rf_buffer_t tx_rf_buffer = {};
  tx_rf_buffer.set_nof_samples(sf_len);//发送1slot（15khz）
  for (uint32_t a = 0; a < nof_ant; a++) {
    tx_rf_buffer.set(rf_port, a, nof_ant, tx_buffer[a]);//tx_buffer和tx_rf_buffer地址绑定
  }

  //(优先处理上行消息)，需要通过slot task同步确保下行消息顺序

  // Process uplink
  if (not work_ul()) {//phy up
    // Wait and release synchronization
    sync.wait(this);
    sync.release();
    common.worker_end(context, false, tx_rf_buffer);
    return;
  }

  // Process downlink
  if (not work_dl()) {//phy down
    common.worker_end(context, false, tx_rf_buffer);
    return;
  }

  common.worker_end(context, true, tx_rf_buffer);

#ifdef DEBUG_WRITE_FILE
  if (num_slots++ < slots_to_dump) {
    printf("Writing slot %d\n", dl_slot_cfg.idx);
    fwrite(tx_rf_buffer.get(0), tx_rf_buffer.get_nof_samples() * sizeof(cf_t), 1, f);
  } else if (num_slots == slots_to_dump) {
    printf("Baseband signaled dump finished. Please close app.\n");
    fclose(f);
  }
#endif
}

