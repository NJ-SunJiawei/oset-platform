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

// Function called from PHY worker context, locking not needed as PDU queue is thread-safe
static void pack_and_queue_nr(mac_pcap *pcap,
									  uint8_t* payload,
                                      uint32_t payload_len,
                                      uint32_t tti,
                                      uint16_t crnti,
                                      uint16_t ue_id,
                                      uint8_t  harqid,
                                      uint8_t  direction,
                                      uint8_t  rnti_type)
{
  if (pcap->base.running && payload != NULL) {
    pcap_pdu_t *pdu = oset_ring_buf_get(pcap->base.buf);
	*pdu = {0};
    pdu->rat                            = (srsran_rat_t)nr;
    pdu->context_nr.radioType           = FDD_RADIO;
    pdu->context_nr.direction           = direction;
    pdu->context_nr.rntiType            = rnti_type;
    pdu->context_nr.rnti                = crnti;
    pdu->context_nr.ueid                = ue_id;
    pdu->context_nr.harqid              = harqid;
    pdu->context_nr.system_frame_number = tti / 10;
    pdu->context_nr.sub_frame_number    = tti % 10;

    // try to allocate PDU buffer
	pdu->pdu = oset_pkbuf_alloc(NULL, SRSRAN_MAX_BUFFER_SIZE_BYTES);
    oset_pkbuf_reserve(pdu->pdu, SRSRAN_BUFFER_HEADER_OFFSET);
    if (pdu->pdu != NULL && oset_pkbuf_tailroom(pdu->pdu) >= payload_len) {
      // copy payload into PDU buffer
		oset_pkbuf_put_data(pdu->pdu, payload, payload_len);
      	if (OSET_OK != oset_ring_queue_try_put(pcap->base.queue, pdu->pdu->data, pdu->pdu->len)) {
        	oset_warn("Dropping PDU (%d B) in NR PCAP. Write queue full.", payload_len);
      	}
    } else {
    	oset_warn("Dropping PDU in NR PCAP. No buffer available or not enough space (pdu_len=%d).", payload_len);
    }
  }
}

static void mac_pcap_write_pdu(mac_pcap *pcap, pcap_pdu_t *pdu)
{
	if (pdu->pdu != NULL) {
	  switch (pdu->rat) {
		case (srsran_rat_t)lte:
		  LTE_PCAP_MAC_UDP_WritePDU(pcap->pcap_file, &pdu->context, pdu->pdu->data, pdu->pdu->len);
		  break;
		case (srsran_rat_t)nr:
		  NR_PCAP_MAC_UDP_WritePDU(pcap->pcap_file, &pdu->context_nr, pdu->pdu->data, pdu->pdu->len);
		  break;
		default:
		  oset_error("Error writing PDU to PCAP. Unsupported RAT selected.");
	  }
	}
}

static void* mac_pcap_base_run_thread(oset_threadplus_t *t, void *param)
{
  mac_pcap *pcap = (mac_pcap_base *)param;
  pcap_pdu_t *pdu = NULL;
  uint32_t length = 0;

  // blocking write until stopped
  while (pcap->base.running) {
	  int rv = oset_ring_queue_try_get(pcap->base.queue, &pdu, &length);
	  if(rv != OSET_OK)
	  {
		  if (rv == OSET_DONE)
			  break;

		  if (rv == OSET_RETRY){
			  continue;
		  }
	  }
	  oset_apr_mutex_lock(pcap->base.mutex);
	  mac_pcap_write_pdu(pcap, pdu);
	  oset_apr_mutex_unlock(pcap->base.mutex);
	  oset_pkbuf_free(pdu->pdu);
	  oset_ring_buf_ret(pdu);
	  pdu = NULL;
	  length = 0;
  }

  // write remainder of queue ???
  //no blocking
  while (OSET_OK == oset_ring_queue_try_get(pcap->base.queue, &pdu, &length)) {
	  oset_apr_mutex_lock(pcap->base.mutex);
	  mac_pcap_write_pdu(pcap, pdu);
	  oset_apr_mutex_unlock(pcap->base.mutex);
	  oset_pkbuf_free(pdu->pdu);
	  oset_ring_buf_ret(pdu);
	  pdu = NULL;
	  length = 0;
  }
}


void mac_pcap_enable(mac_pcap *pcap, bool enable_)
{
	oset_apr_mutex_lock(pcap->base.mutex);
	pcap->base.running = enable_;
	oset_apr_mutex_unlock(pcap->base.mutex);
}

void mac_pcap_set_ue_id(mac_pcap *pcap, uint16_t ue_id_)
{
	oset_apr_mutex_lock(pcap->base.mutex);
	pcap->base.ue_id = ue_id_;
	oset_apr_mutex_unlock(pcap->base.mutex);
}

void mac_pcap_write_dl_crnti_nr(mac_pcap *pcap, uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
	pack_and_queue_nr(pcap, pdu, pdu_len_bytes, tti, rnti, pcap->base.ue_id, harqid, DIRECTION_DOWNLINK, C_RNTI);
}

void mac_pcap_write_dl_crnti_nr(mac_pcap *pcap,
									uint8_t* pdu,
									uint32_t pdu_len_bytes,
									uint16_t crnti,
									uint16_t ue_id,
									uint8_t  harqid,
									uint32_t tti)
{
	pack_and_queue_nr(pcap, pdu, pdu_len_bytes, tti, crnti, ue_id, harqid, DIRECTION_DOWNLINK, C_RNTI);
}

void mac_pcap_write_ul_crnti_nr(mac_pcap *pcap, uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
	pack_and_queue_nr(pcap, pdu, pdu_len_bytes, tti, rnti, pcap->base.ue_id, harqid, DIRECTION_UPLINK, C_RNTI);
}

void mac_pcap_write_ul_crnti_nr(mac_pcap *pcap,
									uint8_t* pdu,
									uint32_t pdu_len_bytes,
									uint16_t rnti,
									uint16_t ue_id,
									uint8_t  harqid,
									uint32_t tti)
{
	pack_and_queue_nr(pcap, pdu, pdu_len_bytes, tti, rnti, ue_id, harqid, DIRECTION_UPLINK, C_RNTI);
}

void mac_pcap_write_dl_ra_rnti_nr(mac_pcap *pcap,
										uint8_t* pdu,
									  uint32_t pdu_len_bytes,
									  uint16_t rnti,
									  uint8_t  harqid,
									  uint32_t tti)
{
	pack_and_queue_nr(pcap, pdu, pdu_len_bytes, tti, rnti, pcap->base.ue_id, harqid, DIRECTION_DOWNLINK, RA_RNTI);
}

void mac_pcap_write_dl_bch_nr(mac_pcap *pcap, uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
	pack_and_queue_nr(pcap, pdu, pdu_len_bytes, tti, rnti, pcap->base.ue_id, harqid, DIRECTION_DOWNLINK, NO_RNTI);
}

void mac_pcap_write_dl_pch_nr(mac_pcap *pcap, uint8_t* pdu, uint32_t pdu_len_bytes, uint16_t rnti, uint8_t harqid, uint32_t tti)
{
	pack_and_queue_nr(pcap, pdu, pdu_len_bytes, tti, rnti, pcap->base.ue_id, harqid, DIRECTION_DOWNLINK, P_RNTI);
}

void mac_pcap_write_dl_si_rnti_nr(mac_pcap *pcap,
									  uint8_t* pdu,
									  uint32_t pdu_len_bytes,
									  uint16_t rnti,
									  uint8_t  harqid,
									  uint32_t tti)
{
	pack_and_queue_nr(pcap, pdu, pdu_len_bytes, tti, rnti, pcap->base.ue_id, harqid, DIRECTION_DOWNLINK, SI_RNTI);
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

	// start writer thread
	oset_threadattr_t *attr = oset_threadattr_create();
	oset_threadattr_schedparam_set(attr, TASK_PRIORITY_MIN_PLUS);
	pcap->base.thread = oset_threadplus_create(attr, mac_pcap_base_run_thread, &pcap);
	oset_assert(pcap->base.thread);
	oset_apr_mutex_unlock(pcap->base.mutex);

	return OSET_OK;
}

uint32_t mac_pcap_close(mac_pcap *pcap)
{
	if (pcap->running == false || pcap->pcap_file == NULL) {
		return OSET_ERROR;
	}

	{
		oset_apr_mutex_lock(pcap->base.mutex);
	    // tell writer thread to stop
	    pcap->base.running   = false;
		oset_ring_queue_term(pcap->base.queue);//todo???
		oset_apr_mutex_unlock(pcap->base.mutex);
	}

	oset_threadplus_destroy(pcap->base.thread, 5);

	// close file handle
	{
		oset_apr_mutex_lock(pcap->base.mutex);
		oset_info("Saving MAC PCAP (DLT=%d) to %s\n", pcap->dlt, pcap->filename);
		DLT_PCAP_Close(pcap->pcap_file);
		pcap->pcap_file = NULL;
		oset_apr_mutex_unlock(pcap->base.mutex);
	}

  return OSET_OK;
}


