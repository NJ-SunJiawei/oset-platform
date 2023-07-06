/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef RRC_MESSAGES_TYPES_H_
#define RRC_MESSAGES_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif


/*rrc<=======>rrc*/
#define RRC_SELF_REM_USER_INFO(msgPtr)                  (msgPtr)->msg_body.rrc_self_rem_user_info


typedef struct {
	uint16_t rnti;
}rrc_self_rem_user_info_t;



#ifdef __cplusplus
}
#endif

#endif /* RRC_MESSAGES_TYPES_H_ */
