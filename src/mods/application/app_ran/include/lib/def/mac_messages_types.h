/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef MAC_MESSAGES_TYPES_H_
#define MAC_MESSAGES_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/*prach<=======>mac*/
#define RACH_MAC_DETECTED_INFO(msgPtr)             (msgPtr)->msg_body.rach_mac_detected_info

/*pusch<=======>mac*/
#define PUSCH_MAC_PDU_INFO(msgPtr)                  (msgPtr)->msg_body.pusch_mac_pdu_info


typedef struct {
	uint16_t rnti;
	byte_buffer_t *pdu;
}pusch_mac_pdu_info_t;

#ifdef __cplusplus
}
#endif
#endif /* MAC_MESSAGES_TYPES_H_ */
