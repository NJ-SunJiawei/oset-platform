#include "FSPLModel.h"
#include <math.h>
/*
功能：计算自由空间损耗
输入：double f,信号频率，单位Hz
double d，距离，单位 m
返回：double 自由空间损耗值 单位dB
*/
double calFSPLModel(double f, double d){
    return 32.447783 + 20*log10(d/1000) + 20*log10(f/1024/1024);
}