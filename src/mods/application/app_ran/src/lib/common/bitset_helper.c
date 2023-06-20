/************************************************************************
 *File name:
 *Description:
 *
 *Current Version:
 *Author: create by sunjiawei
 *Date: 2022.12
************************************************************************/
#include "gnb_common.h"
#include "lib/common/bitset_helper.h"

#undef  OSET_LOG2_DOMAIN
#define OSET_LOG2_DOMAIN   "app-gnb-bitset"

/**********************base**************************************/
#define numeric_limits(T) (8*sizeof(T))

static word_t mask_msb_zeros(size_t N)
{
  return (N == 0) ? ((word_t)-1) : (N == sizeof(word_t) * 8U) ? 0 : ((word_t)-1 >> (N));
}

static word_t mask_lsb_ones(size_t N)
{
  return mask_msb_zeros(sizeof(word_t) * 8U - N);
}

static word_t mask_msb_ones(size_t N)
{
  return ~(mask_msb_zeros(N));
}

static word_t mask_lsb_zeros(size_t N)
{
  return ~(mask_lsb_ones(N));
}

#ifdef __GNUC__ // clang and gcc
/// Specializations for unsigned long long
static word_t msb_count(word_t value)
{
  return (value) ? __builtin_clzll(value) : numeric_limits(word_t);
}
static word_t lsb_count(word_t value)
{
  return (value) ? __builtin_ctzll(value) : numeric_limits(word_t);
}

#else
static word_t msb_count(word_t value)
{
	if (value == 0) {
	  return numeric_limits(word_t);
	}
	word_t ret = 0;
	for (word_t shift = numeric_limits(word_t) >> 1; shift != 0; shift >>= 1) {//shift=shift>>1
	  word_t tmp = value >> shift;
	  if (tmp != 0) {
	    value = tmp;
	  } else {
	    ret |= shift;
	  }
	}
	return ret;
}

/***************************************************/
//stdint.h
//#define UINT64_MAX 0xffffffffffffffffULL /* 18446744073709551615ULL */
/***************************************************/
static word_t lsb_count(word_t value)
{
	if (value == 0) {
	  return numeric_limits(word_t);
	}
	word_t ret = 0;
	word_t mask = UINT64_MAX;//???
	for (word_t shift = numeric_limits(word_t) >> 1, mask >> shift;
	     shift != 0;
	     shift >>= 1, mask >>= shift) {
	  if ((value & mask) == 0) {
	    value >>= shift;
	    ret |= shift;
	  }
	}
	return ret;
}
#endif



/// uses msb as zero position
static word_t find_first_msb_one(word_t value)
{
  return (value) ? (sizeof(word_t) * 8U - 1 - msb_count(value))
                 : numeric_limits(word_t);
}

/// uses lsb as zero position
static word_t find_first_lsb_one(word_t value)
{
  return lsb_count(value);
}


/**********************private**********************************/
uint32_t ceil_div(uint32_t x, uint32_t y)
{
  return (x + y - 1) / y;
}

static size_t get_bitidx_(bounded_bitset *bit, size_t bitpos) { return bit->reversed ? bit_size(bit) - 1 - bitpos : bitpos; }

static word_t maskbit(size_t pos) { return ((word_t)1) << (pos % bits_per_word); }

static size_t nof_words_(bounded_bitset *bit) { return bit_size(bit) > 0 ? (bit_size(bit) - 1) / bits_per_word + 1 : 0; }
static word_t get_word_(bounded_bitset *bit, size_t bitidx) { return bit->buffer[bitidx / bits_per_word]; }
static size_t max_nof_words_(bounded_bitset *bit) { return (bit->N - 1) / bits_per_word + 1; }


static void assert_within_bounds_(bounded_bitset *bit, size_t pos, bool strict)
{
  ASSERT_IF_NOT(pos < bit_size(bit) || (!strict && pos == bit_size(bit)),
				"index=%zd is out-of-bounds for bitset of size=%zd",
				pos,
				bit_size(bit));
}

static void assert_range_bounds_(bounded_bitset *bit, size_t startpos, size_t endpos)
{
  ASSERT_IF_NOT(startpos <= endpos && endpos <= bit_size(bit),
				"range [%zd, %zd) out-of-bounds for bitsize of size=%zd",
				startpos,
				endpos,
				bit_size(bit));
}

static int find_last_(bounded_bitset *bit, size_t startpos, size_t endpos, bool value)
{
  size_t startword = startpos / bits_per_word;
  size_t lastword  = (endpos - 1) / bits_per_word;

  for (size_t i = lastword; i != startpos - 1; --i) {
	word_t w = bit->buffer[i];
	if (!value) {
	  w = ~w;
	}

	if (i == startword) {
	  size_t offset = startpos % bits_per_word;
	  w &= (bit->reversed) ? mask_msb_zeros(offset) : mask_lsb_zeros(offset);
	}
	if (i == lastword) {
	  size_t offset = (endpos - 1) % bits_per_word;
	  w &= (bit->reversed) ? mask_msb_ones(offset + 1) : mask_lsb_ones(offset + 1);
	}
	if (w != 0) {
	  return (int)(i * bits_per_word + find_first_msb_one(w));
	}
  }
  return -1;
}

//查找首个满足条件的bit位置
int find_first_(bounded_bitset *bit, size_t startpos, size_t endpos, bool value)
{
  size_t startword = startpos / bits_per_word;
  size_t lastword  = (endpos - 1) / bits_per_word;

  for (size_t i = startword; i <= lastword; ++i) {
	word_t w = bit->buffer[i];
	if (!value) {
	  w = ~w;
	}

	if (i == startword) {
	  size_t offset = startpos % bits_per_word;
	  w &= mask_lsb_zeros(offset);
	}

	if (i == lastword) {
	  size_t offset = (endpos - 1) % bits_per_word;
	  w &= mask_lsb_ones(offset + 1);
	}

	if (w != 0) {
	  return (int)(i * bits_per_word + find_first_lsb_one(w));
	}
  }
  return -1;
}

int find_first_reversed_(bounded_bitset *bit, size_t startpos, size_t endpos, bool value)
{
  size_t startbitpos = get_bitidx_(startpos), lastbitpos = get_bitidx_(endpos - 1);
  size_t startword = startbitpos / bits_per_word;
  size_t lastword  = lastbitpos / bits_per_word;

  for (size_t i = startword; i != lastword - 1; --i) {
	word_t w = bit->buffer[i];
	if (!value) {
	  w = ~w;
	}

	if (i == startword) {
	  size_t offset = startbitpos % bits_per_word;
	  w &= mask_lsb_ones(offset + 1);
	}
	if (i == lastword) {
	  size_t offset = lastbitpos % bits_per_word;
	  w &= mask_lsb_zeros(offset);
	}
	if (w != 0) {
	  word_t pos = find_first_msb_one(w);
	  return (int)(bit_size(bit) - 1 - (pos + i * bits_per_word));
	}
  }
  return -1;
}

static bool test_(bounded_bitset *bit, size_t bitpos)
{
  bitpos = get_bitidx_(bit, bitpos);
  return ((get_word_(bit, bitpos) & maskbit(bitpos)) != (word_t)0);
}

static void set_(bounded_bitset *bit, size_t bitpos)
{
  bitpos = get_bitidx_(bit, bitpos);
  get_word_(bit, bitpos) |= maskbit(bitpos);
}

static void reset_(bounded_bitset *bit, size_t bitpos)
{
  bitpos = get_bitidx_(bit, bitpos);
  get_word_(bit, bitpos) &= ~(maskbit(bitpos));
}

static void sanitize_(bounded_bitset *bit)
{
  size_t n		= bit_size(bit) % bits_per_word;
  size_t nwords = nof_words_(bit);
  if (n != 0 && nwords > 0) {
	bit->buffer[nwords - 1] &= ~((~((word_t)0)) << n);//~(1 << n)
  }
}

/**********************public**********************************/
void bit_init(bounded_bitset *bit, size_t N, size_t cur_size, bool reversed)
{
   oset_assert(bit);
   bit->N = N;
   bit->reversed = reversed;
   memset(bit->buffer, 0, sizeof(bit->buffer));
   //bit->buffer = oset_malloc(bits_buffer_len(N)*sizeof(word_t)); 
   bit->cur_size = cur_size;
}

size_t bit_max_size(bounded_bitset *bit) { return bit->N; }
size_t bit_size(bounded_bitset *bit) { return bit->cur_size; }
bool bit_reversed(bounded_bitset *bit) { return bit->reversed; }

void bit_resize(bounded_bitset *bit, size_t new_size)
{
  ERROR_IF_NOT_VOID(new_size <= bit_max_size(bit), "new size=%zd exceeds bitset capacity=%zd", new_size, max_size(bit));
  if (new_size == bit->cur_size) {
	return;
  }
  bit->cur_size = new_size;
  sanitize_(bit);//expand and not change old data
  for (size_t i = nof_words_(bit); i < max_nof_words_(bit); ++i) {
	bit->buffer[i] = (word_t)0;
  }
}

void bit_set(bounded_bitset *bit, size_t pos, bool val)
{
  assert_within_bounds_(pos, true);
  if (val) {
	set_(pos);
  } else {
	reset_(pos);
  }
}

void bit_set(bounded_bitset *bit, size_t pos)
{
  assert_within_bounds_(bit, pos, true);
  set_(bit, pos);
}

void bit_reset_all(bounded_bitset *bit)
{
  for (size_t i = 0; i < nof_words_(bit); ++i) {
		bit->buffer[i] = (word_t)0;
  }
}

void bit_reset(bounded_bitset *bit, size_t pos)
{
  assert_within_bounds_(bit, pos, true);
  reset_(bit, pos);
}

void bit_set_val(bounded_bitset *bit, size_t pos, bool val)
{
  assert_within_bounds_(bit, pos, true);
  if (val) {
	set_(bit, pos);
  } else {
	reset_(bit, pos);
  }
}

//判断是否为1
bool bit_test(bounded_bitset *bit, size_t pos)
{
  assert_within_bounds_(bit, pos, true);
  return test_(bit, pos);
}

bounded_bitset bit_flip(bounded_bitset *bit)
{
  for (size_t i = 0; i < nof_words_(bit); ++i) {
	bit->buffer[i] = ~(bit->buffer[i]);
  }
  sanitize_(bit);
  return *bit;
}

bounded_bitset bit_fill(bounded_bitset *bit, size_t startpos, size_t endpos, bool value)
{
  assert_range_bounds_(bit, startpos, endpos);
  // NOTE: can be optimized
  if (value) {
	for (size_t i = startpos; i < endpos; ++i) {
	  set_(bit, i);
	}
  } else {
	for (size_t i = startpos; i < endpos; ++i) {
	  reset_(bit, i);
	}
  }
  return *bit;
}

int bit_find_lowest(bounded_bitset *bit, size_t startpos, size_t endpos, bool value)
{
  assert_range_bounds_(bit, startpos, endpos);
  if (startpos == endpos) {
	return -1;
  }

  if (!bit->reversed) {
	return find_first_(bit, startpos, endpos, value);
  }
  return find_first_reversed_(bit, startpos, endpos, value);
}

bool bit_all(bounded_bitset *bit)
{
  const size_t nw = nof_words_(bit);
  if (nw == 0) {
	return true;
  }
  word_t allset = ~((word_t)0);
  for (size_t i = 0; i < nw - 1; i++) {
	if (bit->buffer[i] != allset) {
	  return false;
	}
  }
  return bit->buffer[nw - 1] == (allset >> (nw * bits_per_word - bit_size(bit)));
}

//交集
bool bit_any(bounded_bitset *bit)
{
  for (size_t i = 0; i < nof_words_(bit); ++i) {
	if (bit->buffer[i] != (word_t)0) {
	  return true;
	}
  }
  return false;
}

bool bit_any_range(bounded_bitset *bit, size_t start, size_t stop)
{
  assert_within_bounds_(bit, start, false);
  assert_within_bounds_(bit, stop, false);
	// NOTE: can be optimized
	for (size_t i = start; i < stop; ++i) {
		if (test_(bit, i)) {
			return true;
		}
	}
  return false;
}

bool bit_none(bounded_bitset *bit) { return !bit_any(bit); }

size_t bit_count(bounded_bitset *bit)
{
  size_t result = 0;
  for (size_t i = 0; i < nof_words_(bit); i++) {
	//		result += __builtin_popcountl(buffer[i]);
	// Note: use an "int" for count triggers popcount optimization if SSE instructions are enabled.
	int c = 0;
	for (word_t w = bit->buffer[i]; w > 0; c++) {
	  w &= w - 1;
	}
	result += c;
  }
  return result;
}


uint64_t bit_to_uint64(bounded_bitset *bit)
{
  ASSERT_IF_NOT(nof_words_(bit) == 1, "cannot convert bitset of size=%zd to uint64_t", bit_size(bit));
  return get_word_(0);
}

void bit_from_uint64(bounded_bitset *bit, uint64_t v)
{
  ASSERT_IF_NOT(nof_words_(bit) == 1, "cannot convert bitset of size=%zd to uint64_t", bit_size(bit));
  ASSERT_IF_NOT(v < (1U << bit_size(bit)), "ERROR: Provided mask=0x%" PRIx64 " does not fit in bitset of size=%zd", v, bit_size(bit));
  bit->buffer[0] = v;
}

char* bit_to_string(bounded_bitset *bit)
{
	char *p = NULL, *last = NULL;
	char out_buffer[128 + 1] = {0}; //max 128

	if (bit_size(bit) == 0) {
	    return NULL;
	}

	p = out_buffer;
	last = p + bit_size(bit) + 1;
	p[0] = 0;

	if (!bit->reversed) {
		for (size_t i = bit_size(bit); i > 0; --i) {
		    p = oset_slprintf(p, last, bit_test(bit, i - 1) ? '1' : '0');
		}
	} else {
		for (size_t i = 0; i < bit_size(bit); ++i) {
		    p = oset_slprintf(p, last, bit_test(bit, i) ? '1' : '0');
		}
	}
	return out_buffer;
}

uint8_t* bit_to_hex(bounded_bitset *bit)
{
	int len = max_nof_words_(bit) * sizeof(word_t);
	uint8_t out_buffer[len] = {0};
    char *str = NULL;

	if (bit_size(bit) == 0) {
	    return NULL;
	}
	
	for (int i = nof_words_(bit) - 1; i >= 0; --i) {
		str = oset_uint64_to_0string(bit->buffer[i]);
	    oset_hex_to_ascii(str, strlen(str), out_buffer, len);
		oset_free(str);
        str = NULL;
		out_buffer += sizeof(word_t);
		len = len - sizeof(word_t);
	}
	return out_buffer;
}

bounded_bitset bit_or_eq(bounded_bitset *bit, bounded_bitset *other)
{
	ASSERT_IF_NOT(bit_size(other) == bit_size(bit),
			"ERROR: operator|= called for bitsets of different sizes (%zd!=%zd)",
			bit_size(bit),
			bit_size(other));
	for (size_t i = 0; i < nof_words_(bit); ++i) {
		bit->buffer[i] |= other->buffer[i];
	}
	return *bit;
}

bounded_bitset bit_and_eq(bounded_bitset *bit, bounded_bitset *other)
{
	ASSERT_IF_NOT(bit_size(other) == bit_size(bit),
			"ERROR: operator&= called for bitsets of different sizes (%zd!=%zd)",
			bit_size(bit),
			bit_size(other));

	for (size_t i = 0; i < nof_words_(bit); ++i) {
		bit->buffer[i] &= other->buffer[i];
	}
	return *bit;
}


bounded_bitset bit_and(bounded_bitset *lhs, bounded_bitset *rhs)
{
	bounded_bitset res = {0}
	bit_init(&res, bit_max_size(lhs), bit_size(lhs), bit_reversed(lhs));//need free buffer
	return bit_and_eq(&res, rhs);
}

bounded_bitset bit_or(bounded_bitset *lhs, bounded_bitset *rhs)
{
	bounded_bitset res = {0}
	bit_init(&res, bit_max_size(lhs), bit_size(lhs), bit_reversed(lhs));//need free buffer
	return bit_or_eq(&res, rhs);
}

