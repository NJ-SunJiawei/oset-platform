/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/

#ifndef SLOT_INTERVAL_H_
#define SLOT_INTERVAL_H_

#include "lib/common/slot_point.h"

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#ifdef __cplusplus
extern "C" {
#endif

/****************************slot_interval***********************************/
typedef struct
{
	slot_point start_;
	slot_point stop_;
}slot_interval;

void slot_interval_init(slot_interval *p);
void slot_interval_init(slot_interval *p, slot_interval start_point, slot_interval stop_point);
slot_point slot_interval_start(slot_interval *p);
slot_point slot_interval_stop(slot_interval *p);
bool slot_interval_empty(slot_interval *p);
slot_point slot_interval_length(slot_interval *p);
void slot_interval_set(slot_interval *p, slot_point start_point, slot_point stop_point);
bool slot_interval_overlaps(slot_interval *p, slot_interval *other);
bool slot_interval_contains(slot_interval *p, slot_point point);
slot_interval* slot_interval_intersect(slot_interval *p, slot_interval *other);

#ifdef __cplusplus
}
#endif

#endif
