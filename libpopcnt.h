// libpopcnt.h - C/C++ library for counting the number of 1 bits (bit
// population count) in an array as quickly as possible using
// specialized CPU instructions e.g. POPCNT, AVX2.
//
// Copyright (c) 2016, Kim Walisch
// Copyright (c) 2016, Wojciech Muła
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef LIBPOPCNT_H
#define LIBPOPCNT_H

#include <stdint.h>

#if defined(_MSC_VER) && defined(_WIN64)
#include <nmmintrin.h>

#define HAVE_POPCNT64

inline uint64_t popcnt64(uint64_t x)
{
  return _mm_popcnt_u64(x);
}

#elif defined(_MSC_VER) && defined(_WIN32)
#include <nmmintrin.h>

#define HAVE_POPCNT64

inline uint64_t popcnt64(uint64_t x)
{
  return _mm_popcnt_u32((uint32_t) x) + 
         _mm_popcnt_u32((uint32_t)(x >> 32));
}

#elif defined(__i386__) && \
      defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__> 1))

#define HAVE_POPCNT64

inline uint64_t popcnt64(uint64_t x)
{
  return __builtin_popcount((uint32_t) x) +
         __builtin_popcount((uint32_t)(x >> 32));
}

#elif defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__> 1))

#define HAVE_POPCNT64

inline uint64_t popcnt64(uint64_t x)
{
  return __builtin_popcountll(x);
}

#else

/// This uses fewer arithmetic operations than any other known  
/// implementation on machines with fast multiplication.
/// It uses 12 arithmetic operations, one of which is a multiply.
/// http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
///
inline uint64_t popcnt64(uint64_t x)
{
  const uint64_t m1 = 0x5555555555555555ll;
  const uint64_t m2 = 0x3333333333333333ll;
  const uint64_t m = 0x0F0F0F0F0F0F0F0Fll;
  const uint64_t h01 = 0x0101010101010101ll;

  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2);
  x = (x + (x >> 4)) & m4;

  return (x * h01) >> 56;
}

inline void CSA(uint64_t& h, uint64_t& l, uint64_t a, uint64_t b, uint64_t c)
{
  uint64_t u = a ^ b; 
  h = (a & b) | (u & c);
  l = u ^ c;
}

/// Harley-Seal popcount (4th iteration).
/// The Harley-Seal popcount algorithm is one of the fastest algorithms
/// for counting 1 bits in an array using only integer operations.
/// This implementation uses only 5.69 instructions per 64-bit word.
/// @see Chapter 5 in "Hacker's Delight" 2nd edition.
///
static uint64_t popcnt_harley_seal(const uint64_t* data, uint64_t size)
{
  if (size == 0)
    return 0;

  uint64_t total = 0;
  uint64_t ones = 0, twos = 0, fours = 0, eights = 0, sixteens = 0;
  uint64_t twosA, twosB, foursA, foursB, eightsA, eightsB;
  uint64_t limit = size - size % 16;
  uint64_t i = 0;

  for(; i < limit; i += 16)
  {
    CSA(twosA, ones, ones, data[i+0], data[i+1]);
    CSA(twosB, ones, ones, data[i+2], data[i+3]);
    CSA(foursA, twos, twos, twosA, twosB);
    CSA(twosA, ones, ones, data[i+4], data[i+5]);
    CSA(twosB, ones, ones, data[i+6], data[i+7]);
    CSA(foursB, twos, twos, twosA, twosB);
    CSA(eightsA,fours, fours, foursA, foursB);
    CSA(twosA, ones, ones, data[i+8], data[i+9]);
    CSA(twosB, ones, ones, data[i+10], data[i+11]);
    CSA(foursA, twos, twos, twosA, twosB);
    CSA(twosA, ones, ones, data[i+12], data[i+13]);
    CSA(twosB, ones, ones, data[i+14], data[i+15]);
    CSA(foursB, twos, twos, twosA, twosB);
    CSA(eightsB, fours, fours, foursA, foursB);
    CSA(sixteens, eights, eights, eightsA, eightsB);

    total += popcnt64(sixteens);
  }

  total *= 16;
  total += 8 * popcnt64(eights);
  total += 4 * popcnt64(fours);
  total += 2 * popcnt64(twos);
  total += 1 * popcnt64(ones);

  for(; i < size; i++)
    total += popcnt64(data[i]);

  return total;
}

#endif

/// Count the number of 1 bits in an array using the POPCNT
/// instruction. On x86 CPUs this requires SSE4.2.
///
static uint64_t popcnt64_unrolled(const uint64_t* data, uint64_t size)
{
#if !defined(HAVE_POPCNT64)
  return popcnt_harley_seal(data, size);
#else
  if (size == 0)
    return 0;

  uint64_t sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
  uint64_t limit = size - size % 4;
  uint64_t i = 0;

  for (; i < limit; i += 4)
  {
    sum0 += popcnt64(data[i+0]);
    sum1 += popcnt64(data[i+1]);
    sum2 += popcnt64(data[i+2]);
    sum3 += popcnt64(data[i+3]);
  }

  uint64_t total = sum0 + sum1 + sum2 + sum3;

  for (; i < size; i++)
    total += popcnt64(data[i]);

  return total;
#endif
}

#if defined(__AVX2__)
#include <immintrin.h>

inline __m256i popcnt_m256i(const __m256i v)
{
  __m256i m1 = _mm256_set1_epi8(0x55);
  __m256i m2 = _mm256_set1_epi8(0x33);
  __m256i m4 = _mm256_set1_epi8(0x0F);

  __m256i t1 = _mm256_sub_epi8(v, (_mm256_srli_epi16(v,  1) & m1));
  __m256i t2 = _mm256_add_epi8(t1 & m2, (_mm256_srli_epi16(t1, 2) & m2));
  __m256i t3 = _mm256_add_epi8(t2, _mm256_srli_epi16(t2, 4)) & m4;

  return _mm256_sad_epu8(t3, _mm256_setzero_si256());
}

inline void CSA_m256i(__m256i& h, __m256i& l, __m256i a, __m256i b, __m256i c)
{
  __m256i u = a ^ b;
  h = (a & b) | (u & c);
  l = u ^ c;
}

static uint64_t popcnt_harley_seal_avx2(const __m256i* data, uint64_t size)
{
  if (size == 0)
    return 0;

  __m256i total = _mm256_setzero_si256();
  __m256i ones = _mm256_setzero_si256();
  __m256i twos = _mm256_setzero_si256();
  __m256i fours = _mm256_setzero_si256();
  __m256i eights = _mm256_setzero_si256();
  __m256i sixteens = _mm256_setzero_si256();
  __m256i twosA, twosB, foursA, foursB, eightsA, eightsB;

  uint64_t limit = size - size % 16;
  uint64_t i = 0;

  for(; i < limit; i += 16)
  {
    CSA_m256i(twosA, ones, ones, data[i+0], data[i+1]);
    CSA_m256i(twosB, ones, ones, data[i+2], data[i+3]);
    CSA_m256i(foursA, twos, twos, twosA, twosB);
    CSA_m256i(twosA, ones, ones, data[i+4], data[i+5]);
    CSA_m256i(twosB, ones, ones, data[i+6], data[i+7]);
    CSA_m256i(foursB, twos, twos, twosA, twosB);
    CSA_m256i(eightsA,fours, fours, foursA, foursB);
    CSA_m256i(twosA, ones, ones, data[i+8], data[i+9]);
    CSA_m256i(twosB, ones, ones, data[i+10], data[i+11]);
    CSA_m256i(foursA, twos, twos, twosA, twosB);
    CSA_m256i(twosA, ones, ones, data[i+12], data[i+13]);
    CSA_m256i(twosB, ones, ones, data[i+14], data[i+15]);
    CSA_m256i(foursB, twos, twos, twosA, twosB);
    CSA_m256i(eightsB, fours, fours, foursA, foursB);
    CSA_m256i(sixteens, eights, eights, eightsA, eightsB);

    total = _mm256_add_epi64(total, popcnt_m256i(sixteens));
  }

  total = _mm256_slli_epi64(total, 4);
  total = _mm256_add_epi64(total, _mm256_slli_epi64(popcnt_m256i(eights), 3));
  total = _mm256_add_epi64(total, _mm256_slli_epi64(popcnt_m256i(fours), 2));
  total = _mm256_add_epi64(total, _mm256_slli_epi64(popcnt_m256i(twos), 1));
  total = _mm256_add_epi64(total, popcnt_m256i(ones));

  for(; i < size; i++)
    total = _mm256_add_epi64(total, popcnt_m256i(data[i]));

  return (uint64_t) _mm256_extract_epi64(total, 0) +
         (uint64_t) _mm256_extract_epi64(total, 1) +
         (uint64_t) _mm256_extract_epi64(total, 2) +
         (uint64_t) _mm256_extract_epi64(total, 3);
}

#endif /* __AVX2__ */

static uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t total = 0;
 
  const uint8_t* data8 = (const uint8_t*) data;
  uintptr_t align8 = (uintptr_t) data8 % 8;
  if (align8 > size)
    align8 = size;

  // align memory to 8 bytes boundary for uint64_t type
  for (uint64_t i = 0; i < align8; i++)
  {
    total += popcnt64(*data8++);
    size -= 1;
  }

  const uint64_t* data64 = (const uint64_t*) data8;

#if defined(__AVX2__)

  // AVX2 popcount is faster than SSE4.2 POPCNT
  // for array sizes >= 1 kilobyte
  if (size >= 1024)
  {
    uintptr_t align32 = (uintptr_t) data64 % 32;
    if (align32 > size)
      align32 = size;

    // align memory to 32 bytes boundary for __m256i type
    for (uint64_t i = 0; i < align32 / 8; i++)
    {
      total += popcnt64(*data64++);
      size -= 8;
    }

    // process remaining 256-bit words
    total += popcnt_harley_seal_avx2((const __m256i*) data64, size / 32);
    data64 += (size / 32) * 4;
    size = size % 32;
  }

#endif /* __AVX2__ */

  // process remaining 64-bit words
  total += popcnt64_unrolled(data64, size / 8);
  data64 += size / 8;
  size = size % 8;
  data8 = (const uint8_t*) data64;

  // process remaining bytes
  for (uint64_t i = 0; i < size; i++)
    total += popcnt64(data8[i]);

  return total;
}

#endif /* LIBPOPCNT_H */
