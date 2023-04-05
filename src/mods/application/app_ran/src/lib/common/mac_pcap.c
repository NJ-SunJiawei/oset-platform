/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "lib/common/mac_pcap.h"
	
	
#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-mac-pcap"

void mac_pcap_base_enable(mac_pcap *pcap, bool enable_)
{
	oset_apr_mutex_lock(pcap->base.mutex);
	pcap->base.running = enable_;
	oset_apr_mutex_unlock(pcap->base.mutex);
}

void mac_pcap_base_set_ue_id(mac_pcap *pcap, uint16_t ue_id_)
{
	oset_apr_mutex_lock(pcap->base.mutex);
	pcap->base.ue_id = ue_id_;
	oset_apr_mutex_unlock(pcap->base.mutex);
}

void mac_pcap_write_pdu(srsran::mac_pcap_base::pcap_pdu_t& pdu)
{
  if (pdu.pdu != nullptr) {
    switch (pdu.rat) {
      case srsran_rat_t::lte:
        LTE_PCAP_MAC_UDP_WritePDU(pcap_file, &pdu.context, pdu.pdu->msg, pdu.pdu->N_bytes);
        break;
      case srsran_rat_t::nr:
        NR_PCAP_MAC_UDP_WritePDU(pcap_file, &pdu.context_nr, pdu.pdu->msg, pdu.pdu->N_bytes);
        break;
      default:
        logger.error("Error writing PDU to PCAP. Unsupported RAT selected.");
    }
  }
}

static void* mac_pcap_base_run_thread(oset_threadplus_t *t, void *param)
{
	mac_pcap_base *base = (mac_pcap_base *)param;
	pcap_pdu_t *pdu = NULL;
	uint32_t length = 0;

	// blocking write until stopped
	while (base->running) {
		int rv = oset_ring_queue_get(base->queue, &pdu, &length);
		if(rv != OSET_OK)
		{
			if (rv == OSET_DONE)
				break;

			if (rv == OSET_RETRY){
				continue;
			}
		}
		oset_apr_mutex_lock(base->mutex);
		mac_pcap_write_pdu(pdu);
		oset_apr_mutex_unlock(base->mutex);
		oset_ring_buf_ret(pdu);
		pdu = NULL;
		length = 0;
	}

	// write remainder of queue
	pcap_pdu_t pdu = {0};
	while (OSET_OK == oset_ring_queue_try_get(base->queue, &pdu, &length)) {
		oset_apr_mutex_lock(base->mutex);
		mac_pcap_write_pdu(pdu);
		oset_apr_mutex_unlock(base->mutex);
	}
}


uint32_t mac_pcap_open(mac_pcap *pcap, char *filename_, uint32_t ue_id_)
{
	if (pcap->pcap_file != NULL) {
		oset_error("PCAP writer for %s already running. Close first.", filename_);
		return OSET_ERROR;
	}

	oset_apr_mutex_lock(pcap->base.mutex);
	// set UDP DLT
	pcap->dlt       = UDP_DLT;
	pcap->pcap_file = DLT_PCAP_Open(pcap->dlt, filename_);
	if (pcap->pcap_file == NULL) {
		logger.error("Couldn't open %s to write PCAP", filename_);
		return OSET_ERROR;
	}

	pcap->filename      = filename_;
	pcap->base.ue_id    = ue_id_;
	pcap->base.running  = true;
	oset_apr_mutex_unlock(pcap->base.mutex);

	// start writer thread
	oset_threadattr_t *attr = oset_threadattr_create();
	oset_threadattr_schedparam_set(attr, TASK_PRIORITY_MIN_PLUS);
	oset_threadplus_t *thread = oset_threadplus_create(attr, mac_pcap_base_run_thread, &pcap->base);
	oset_assert(thread);

  return OSET_OK;
}

