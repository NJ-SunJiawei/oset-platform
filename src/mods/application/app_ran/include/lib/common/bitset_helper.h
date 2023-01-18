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
#define bits_per_word (8*sizeof(word_t))
#define bits_buffer_len(N) ((N - 1) / bits_per_word + 1)

typedef struct {
	bool     reversed;
	size_t   N;
	word_t  *buffer;
	size_t  cur_size;
}bounded_bitset;


void bitset_init(bounded_bitset *bit, size_t N, bool reversed = false);
size_t max_size(bounded_bitset *bit);
size_t size(bounded_bitset *bit);

#ifdef __cplusplus
}
#endif

#endif
