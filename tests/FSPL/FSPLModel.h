#ifndef _FSPL_H
#define	_FSPL_H

#ifdef __cplusplus
extern "C" {
#endif
/*
功能：计算自由空间损耗
输入：double f,信号频率，单位Hz
double d，距离，单位 m
返回：double 自由空间损耗值 单位dB
*/

double calFSPLModel(double f, double d);
#ifdef __cplusplus
}
#endif

#endif