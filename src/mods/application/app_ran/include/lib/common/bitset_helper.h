/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/

#ifndef BITSET_HELPER_H_
#define BITSET_HELPER_H_

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t word_t;
#define bits_per_word (8*sizeof(word_t))  //64
#define bits_buffer_len(N) ((N - 1) / bits_per_word + 1)

typedef struct {
	bool     reversed;//true:msb   false:lsb (default)
	size_t   N;
	word_t  *buffer;
	size_t  cur_size;
}bounded_bitset;//= asn1c BIT_STRING_t

uint32_t ceil_div(uint32_t x, uint32_t y);
void bit_init(bounded_bitset *bit, size_t N, size_t cur_size, bool reversed = false);
size_t bit_max_size(bounded_bitset *bit);
size_t bit_size(bounded_bitset *bit);
void bit_resize(bounded_bitset *bit, size_t new_size);
void bit_set(bounded_bitset *bit, size_t pos, bool val);
void bit_set(bounded_bitset *bit, size_t pos);
void bit_reset(bounded_bitset *bit, size_t pos);
void bit_set_val(bounded_bitset *bit, size_t pos, bool val);
void bit_reset_all(bounded_bitset *bit);
bounded_bitset* bit_flip(bounded_bitset *bit);
bounded_bitset* bit_fill(bounded_bitset *bit, size_t startpos, size_t endpos, bool value = true);
int bit_find_lowest(bounded_bitset *bit, size_t startpos, size_t endpos, bool value = true);
bool bit_all(bounded_bitset *bit);
bool bit_any(bounded_bitset *bit);
bool bit_any_range(bounded_bitset *bit, size_t start, size_t stop);
bool bit_none(bounded_bitset *bit);
size_t bit_count(bounded_bitset *bit);
char* bit_to_string(bounded_bitset *bit);

#ifdef __cplusplus
}
#endif

#endif
