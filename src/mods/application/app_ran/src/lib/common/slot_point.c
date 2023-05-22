/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.02
************************************************************************/
#include "oset-core.h"
#include "lib/common/slot_point.h"

uint8_t nof_slots_per_subframe(slot_point *slot)
{ 
    return 1U << slot->numerology_; 
}

uint8_t nof_slots_per_frame(slot_point *slot)
{
	return nof_slots_per_subframe(slot) * NOF_SUBFRAMES_PER_FRAME;
}

uint32_t nof_slots_per_hf(slot_point *slot)
{ 
    return nof_slots_per_frame(slot) * NOF_SFNS; //1*10*1024
}

//valid()
bool slot_valid(slot_point *slot)
{ 
    return slot->numerology_ < NOF_NUMEROLOGIES;
}

uint16_t slot_sfn(slot_point *slot)
{
    return slot->count_ / nof_slots_per_frame(slot);
}

uint8_t  slot_idx(slot_point *slot)
{
	return slot->count_ % nof_slots_per_frame(slot);
}

uint16_t subframe_idx(slot_point *slot)
{
	return slot_idx(slot) / nof_slots_per_subframe(slot);
}

uint8_t  numerology_idx(slot_point *slot)
{ 
	return slot->numerology_;
}

uint32_t count_idx(slot_point *slot)
{
    return slot->count_;
}

void slot_clear(slot_point *slot)
{ 
	slot->numerology_ = NOF_NUMEROLOGIES;
}

void slot_point_init(slot_point *slot)
{
    slot->numerology_ = NOF_NUMEROLOGIES;
	slot->count_ = 0;
}

void slot_point_init(slot_point *slot, uint8_t numerology, uint32_t count)
{
    slot->numerology_ = numerology;
	slot->count_ = count;
    ASSERT_IF_NOT(numerology < NOF_NUMEROLOGIES, "Invalid numerology idx=%d passed", (int)numerology);
    ASSERT_IF_NOT(count < nof_slots_per_hf(slot), "Invalid slot count=%d passed", (int)count);
}


void slot_point_init(slot_point *s, uint8_t numerology, uint16_t sfn_val, uint8_t slot)
{
	s->numerology_ = numerology;
	s->count_ = slot + sfn_val * nof_slots_per_frame(s);
	ASSERT_IF_NOT(numerology < NOF_NUMEROLOGIES, "Invalid numerology idx=%d passed", (int)numerology);
	ASSERT_IF_NOT(sfn_val < NOF_SFNS, "Invalid SFN=%d provided", (int)sfn_val);
	ASSERT_IF_NOT(slot < nof_slots_per_frame(s),\
				  "Slot index=%d exceeds maximum number of slots=%d", (int)slot, (int)nof_slots_per_frame(s));
}


slot_point slot_point_sub_equal(slot_point *slot, uint32_t jump)
{
	int a = ((int)slot->count_ - (int)jump) % (int)(nof_slots_per_hf(slot));
	if (a < 0) {
		a += nof_slots_per_hf(slot);
	}
	slot->count_ = a;
	return *slot;
}


slot_point slot_point_add_equal(slot_point *slot, uint32_t jump)
{
  slot->count_ = (slot->count_ + jump) % nof_slots_per_hf(slot);
  return *slot;
}


slot_point slot_point_sub(slot_point slot, uint32_t jump)
{
	return slot_point_sub_equal(&slot, jump);
}


slot_point slot_point_add(slot_point slot, uint32_t jump)
{
	return slot_point_add_equal(&slot, jump);
}


void slot_point_plus_plus(slot_point *slot)
{
	slot->count_++;
	if (slot->count_ == nof_slots_per_hf(slot)) {
		slot->count_ = 0;
	}
}

//==
bool slot_point_equal(slot_point *slot, slot_point *other)
{
	return (other->count_ == slot->count_ && other->numerology_ == slot->numerology_);
}

//!=
bool slot_point_no_equal(slot_point *slot, slot_point *other) 
{
	return (!slot_point_equal(slot, other));
}

//<
bool slot_point_less(slot_point *slot, slot_point *other)
{
	ASSERT_IF_NOT(numerology_idx(slot) == numerology_idx(other), "Comparing slots of different numerologies");
	int a = (int)(other.count_) - (int)(slot->count_);
	if (a > 0) {
		return (a < (int)nof_slots_per_hf(slot) / 2);
	}
	return (a < -(int)nof_slots_per_hf(slot) / 2);
}

//<=
bool slot_point_less_equal(slot_point *slot, slot_point *other)
{
	return (slot_point_equal(slot, other) || slot_point_less(slot, other));
}

//>=
bool slot_point_greater_equal(slot_point *slot, slot_point *other)
{
	return (!slot_point_less(slot, other));
}

//>
bool slot_point_greater(slot_point *slot, slot_point *other)
{
	return (slot_point_greater_equal(slot, other) && slot_point_no_equal(slot, other));
}

slot_point slot_point_max(slot_point s1, slot_point s2)
{
  return slot_point_greater(&s1, &s2) ? s1 : s2;
}

slot_point slot_point_min(slot_point s1, slot_point s2)
{
  return slot_point_less(&s1, &s2) ? s1 : s2;
}

bool is_in_interval(slot_point *slot, slot_point begin, slot_point end)
{
	return (slot_point_greater_equal(slot, &begin) && slot_point_less(slot, &end));
}


