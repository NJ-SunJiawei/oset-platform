/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "bitset_helper.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-bitset"


/**********************private**********************************/
static size_t get_bitidx_(bounded_bitset *bit, size_t bitpos) { return bit->reversed ? size(bit) - 1 - bitpos : bitpos; }

static word_t maskbit(size_t pos) { return ((word_t)1) << (pos % bits_per_word); }

static size_t nof_words_(bounded_bitset *bit) { return size(bit) > 0 ? (size(bit) - 1) / bits_per_word + 1 : 0; }
static word_t get_word_(bounded_bitset *bit, size_t bitidx) { return bit->buffer[bitidx / bits_per_word]; }
static size_t max_nof_words_(bounded_bitset *bit) { return (bit->N - 1) / bits_per_word + 1; }

static void sanitize_(bounded_bitset *bit)
{
  size_t n		= size(bit) % bits_per_word;
  size_t nwords = nof_words_(bit);
  if (n != 0 && nwords > 0) {
	bit->buffer[nwords - 1] &= ~((~((word_t)0)) << n);//~(1 << n)
  }
}

/**********************public**********************************/
void bitset_init(bounded_bitset *bit, size_t N, bool reversed)
{
   oset_assert(bit);
   bit->N = N;
   bit->reversed = reversed;
   bit->buffer = oset_malloc(bits_buffer_len(N)*sizeof(word_t)); 
   bit->cur_size = 0;
}

size_t max_size(bounded_bitset *bit) { return bit->N; }
size_t size(bounded_bitset *bit) { return bit->cur_size; }

void resize(bounded_bitset *bit, size_t new_size)
{
  ERROR_IF_NOT(new_size <= max_size(bit), "new size=%zd exceeds bitset capacity=%zd", new_size, max_size(bit));
  if (new_size == bit->cur_size) {
	return;
  }
  bit->cur_size = new_size;
  sanitize_();
  for (size_t i = nof_words_(); i < max_nof_words_(); ++i) {
	bit->buffer[i] = (word_t)0;
  }
}



