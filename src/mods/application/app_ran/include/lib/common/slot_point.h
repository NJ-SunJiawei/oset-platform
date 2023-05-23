/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2023.01
************************************************************************/

#ifndef SLOT_POINT_H_
#define SLOT_POINT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************slot*****************************************/

#define  NOF_NUMEROLOGIES		  5
#define  NOF_SFNS				  1024
#define  NOF_SUBFRAMES_PER_FRAME  10

typedef struct
{
  uint32_t numerology_ : 3;
  uint32_t count_ : 29;
}slot_point;

uint8_t nof_slots_per_subframe(slot_point *slot);
uint8_t nof_slots_per_frame(slot_point *slot);
uint32_t nof_slots_per_hf(slot_point *slot);
bool slot_valid(slot_point *slot);
uint16_t slot_sfn(slot_point *slot);
uint8_t  slot_idx(slot_point *slot);
uint16_t subframe_idx(slot_point *slot);
uint8_t  numerology_idx(slot_point *slot);
uint32_t count_idx(slot_point *slot);
void slot_clear(slot_point *slot);
void slot_point_init(slot_point *slot);
void slot_point_init(slot_point *slot, uint8_t numerology, uint32_t count);
void slot_point_init(slot_point *s, uint8_t numerology, uint16_t sfn_val, uint8_t slot);
slot_point slot_point_sub_equal(slot_point *slot, uint32_t jump);
slot_point slot_point_add_equal(slot_point *slot, uint32_t jump);
slot_point slot_point_sub_jump(slot_point slot, uint32_t jump);
slot_point slot_point_add_jump(slot_point slot, uint32_t jump);
int32_t slot_point_sub(slot_point *slot, slot_point *other);
void slot_point_plus_plus(slot_point *slot);
bool slot_point_equal(slot_point *slot, slot_point *other);
bool slot_point_no_equal(slot_point *slot, slot_point *other);
bool slot_point_less(slot_point *slot, slot_point *other);
bool slot_point_less_equal(slot_point *slot, slot_point *other);
bool slot_point_greater_equal(slot_point *slot, slot_point *other);
bool slot_point_greater(slot_point *slot, slot_point *other);
slot_point slot_point_max(slot_point s1, slot_point s2);
slot_point slot_point_min(slot_point s1, slot_point s2);
bool is_in_interval(slot_point *slot, slot_point begin, slot_point end);


#ifdef __cplusplus
}
#endif

#endif
