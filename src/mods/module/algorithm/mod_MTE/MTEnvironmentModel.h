#ifndef  _MTENVIRONMENTMODEL_H
#define _MTENVIRONMENTMODEL_H

#include <iostream>
#ifdef __cplusplus
extern "C"{
#endif


struct CHANNELINDEX
{
	double m_CNR;          //载噪比值
	double m_ber;          //误码率
	double m_delay;         //时延
};

CHANNELINDEX  calWirelessChannelModel(double distance, double frequency = 20e9, double EIRP = 34, double G_T = 10, std::string encoding = "16QAM", double bandwidth = 200e6);

#ifdef __cplusplus
}
#endif

#endif


