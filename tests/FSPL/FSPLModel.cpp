#include "FSPLModel.h"
#include <math.h>
/*
���ܣ��������ɿռ����
���룺double f,�ź�Ƶ�ʣ���λHz
double d�����룬��λ m
���أ�double ���ɿռ����ֵ ��λdB
*/
double calFSPLModel(double f, double d){
    return 32.447783 + 20*log10(d/1000) + 20*log10(f/1024/1024);
}