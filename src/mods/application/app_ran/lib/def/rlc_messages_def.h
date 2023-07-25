/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#if defined(DISABLE_ITTI_XER_PRINT)

#else
MESSAGE_DEF(RLC_AM_DATA_PDU_IND,      msg_text_t,         rlc_am_data_pdu_ind)
MESSAGE_DEF(RLC_AM_DATA_PDU_REQ,      msg_text_t,         rlc_am_data_pdu_req)
MESSAGE_DEF(RLC_AM_STATUS_PDU_IND,    msg_text_t,         rlc_am_status_pdu_ind)
MESSAGE_DEF(RLC_AM_STATUS_PDU_REQ,    msg_text_t,         rlc_am_status_pdu_req)
MESSAGE_DEF(RLC_AM_SDU_REQ,           msg_text_t,         rlc_am_sdu_req)
MESSAGE_DEF(RLC_AM_SDU_IND,           msg_text_t,         rlc_am_sdu_ind)

MESSAGE_DEF(RLC_UM_DATA_PDU_IND,      msg_text_t,         rlc_um_data_pdu_ind)
MESSAGE_DEF(RLC_UM_DATA_PDU_REQ,      msg_text_t,         rlc_um_data_pdu_req)
MESSAGE_DEF(RLC_UM_SDU_REQ,           msg_text_t,         rlc_um_sdu_req)
MESSAGE_DEF(RLC_UM_SDU_IND,           msg_text_t,         rlc_um_sdu_ind)
#endif
