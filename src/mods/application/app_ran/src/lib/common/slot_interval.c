/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "oset-core.h"
#include "lib/common/slot_interval.h"

void slot_interval_init(slot_interval *p)
{
    p->start_ = 0;
    p->stop_ = 0;
}

void slot_interval_init(slot_interval *p, slot_point start_point, slot_point stop_point)
{ 
    oset_assert(slot_point_less_equal(&start_point, &stop_point));
    p->start_ = start_point;
    p->stop_ = stop_point;
}

slot_point slot_interval_start(slot_interval *p)   { return p->start_; }
slot_point slot_interval_stop(slot_interval *p)   { return p->stop_; }

bool slot_interval_empty(slot_interval *p) { return slot_point_equal(&p->stop_, &p->start_); }
int32_t slot_interval_length(slot_interval *p) { return abs(slot_point_sub(&p->stop_, &p->start_)); }

void slot_interval_set(slot_interval *p, slot_point start_point, slot_point stop_point)
{
	ERROR_IF_NOT_VOID(stop_point >= start_point, "interval::set called for invalid range points");
	p->start_ = start_point;
	p->stop_  = stop_point;
}

bool slot_interval_overlaps(slot_interval *p, slot_interval *other) { return slot_point_less(&p->start_, &other->stop_) && slot_point_greater(&p->stop_, &other->start_); }

bool slot_interval_contains(slot_interval *p, slot_point point) { return slot_point_greater_equal(&point, &p->start_) && slot_point_less(&point, &p->stop_); }

slot_interval* slot_interval_intersect(slot_interval *p, slot_interval *other)
{
  if (!overlaps(other)) {
	slot_interval_init(p);
  } else {
	p->start_ = slot_point_min(slot_interval_start(p), slot_interval_start(other));
	p->stop_  = slot_point_min(slot_interval_stop(p), slot_interval_stop(other));
  }
  return p;
}


