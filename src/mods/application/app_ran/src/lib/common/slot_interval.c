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
    oset_assert(start_point <= stop_point);
    p->start_ = start_point;
    p->stop_ = stop_point;
}

slot_point slot_interval_start(slot_interval *p)   { return p->start_; }
slot_point slot_interval_stop(slot_interval *p)   { return p->stop_; }

bool slot_interval_empty(slot_interval *p) { return p->stop_ == p->start_; }
slot_point slot_interval_length(slot_interval *p) { return p->stop_ - p->start_; }

void slot_interval_set(slot_interval *p, slot_point start_point, slot_point stop_point)
{
	ERROR_IF_NOT_VOID(stop_point >= start_point, "interval::set called for invalid range points");
	p->start_ = start_point;
	p->stop_  = stop_point;
}

void slot_interval_resize_by(slot_interval *p, slot_point len)
{
  // Detect length overflows
	ERROR_IF_NOT_VOID(p!=NULL ||(len >= 0 || slot_interval_length(p) >= -len), "Resulting interval would be invalid");
	p->stop_ += len;
}

void slot_interval_resize_to(slot_interval *p, slot_point len)
{
	ERROR_IF_NOT_VOID(p!=NULL || len >= 0, "Interval width must be positive");
	p->stop_ = p->start_ + len;
}

void slot_interval_displace_by(slot_interval *p, int offset)
{
	p->start_ += offset;
	p->stop_ += offset;
}

void slot_interval_displace_to(slot_interval *p, slot_point start_point)
{
	p->stop_  = start_point + slot_interval_length(p);
	p->start_ = start_point;
}

bool overlaps(slot_interval *p, slot_interval *other) { return p->start_ < other->stop_ && other->start_ < p->stop_; }

bool contains(slot_interval *p, slot_point point) { return p->start_ <= point && point < p->stop_; }

slot_interval* intersect(slot_interval *p, slot_interval *other)
{
  if (! overlaps(other)) {
	slot_interval_init(p);
  } else {
	p->start_ = MAX(slot_interval_start(p), slot_interval_start(other));
	p->stop_  = MIN(slot_interval_stop(p), slot_interval_stop(other));
  }
  return p;
}


