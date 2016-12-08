// libpopcnt.h - C/C++ library for counting the number of 1 bits (bit
// population count) in an array as quickly as possible using
// specialized CPU instructions e.g. POPCNT, AVX2.
//
// Copyright (c) 2016, Kim Walisch
// Copyright (c) 2016, Wojciech Muła
//
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

#if defined(HAVE_AVX2)

#if defined(_MSC_VER) && \
   (defined(_WIN32) || defined(_WIN64))
  // __cpuidex()
  #include <intrin.h>
#endif

// %ebx bit flags
#define bit_AVX2 (1 << 5)

/// Portable cpuid implementation for x86 and x86-64 CPUs
/// (supports PIC and non-PIC code).
/// Returns 1 if the CPU supports cpuid else 0.
///
static int cpuid(unsigned int *eax,
                 unsigned int *ebx,
                 unsigned int *ecx,
                 unsigned int *edx)
{
#if defined(_MSC_VER) && \
   (defined(_WIN32) || defined(_WIN64))
  int regs[4];
  __cpuidex(regs, *eax, *ecx);
  *eax = regs[0];
  *ebx = regs[1];
  *ecx = regs[2];
  *edx = regs[3];
  return 1;
#elif defined(__i386__)
  #if defined(__PIC__)
    __asm__ __volatile__ (
     "mov %%ebx, %%esi;" // save %ebx PIC register
     "cpuid;"
     "xchg %%ebx, %%esi;"
     : "+a" (*eax), 
       "=S" (*ebx),
       "+c" (*ecx),
       "=d" (*edx));
  #else
    __asm__ __volatile__ (
     "cpuid;"
     : "+a" (*eax), 
       "=b" (*ebx),
       "+c" (*ecx),
       "=d" (*edx));
  #endif
  return 1;
#elif defined(__x86_64__)
  __asm__ __volatile__ (
   "cpuid;"
   : "+a" (*eax), 
     "=b" (*ebx),
     "+c" (*ecx),
     "=d" (*edx));
  return 1;
#else
  (void) eax;
  (void) ebx;
  (void) ecx;
  (void) edx;
  return 0;
#endif
}

static int has_avx2_cpuid()
{
  unsigned int eax = 7;
  unsigned int ebx;
  unsigned int ecx = 0;
  unsigned int edx;

  if (cpuid(&eax, &ebx, &ecx, &edx))
  {
    return (ebx & bit_AVX2) != 0;
  }

  return 0;
}

static int has_avx2()
{
  static int avx2 = has_avx2_cpuid();
  return avx2;
}

#endif /* HAVE_AVX2 */

/// This uses fewer arithmetic operations than any other known
/// implementation on machines with fast multiplication.
/// It uses 12 arithmetic operations, one of which is a multiply.
/// http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
///
static inline uint64_t popcount64c(uint64_t x)
{
  const uint64_t m1 = 0x5555555555555555ll;
  const uint64_t m2 = 0x3333333333333333ll;
  const uint64_t m4 = 0x0F0F0F0F0F0F0F0Fll;
  const uint64_t h01 = 0x0101010101010101ll;

  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2);
  x = (x + (x >> 4)) & m4;

  return (x * h01) >> 56;
}

#if defined(HAVE_POPCNT) && \
    defined(_MSC_VER) && \
    defined(_WIN64)

#include <nmmintrin.h>

static inline uint64_t popcnt_u64(uint64_t x)
{
  return _mm_popcnt_u64(x);
}

#elif defined(HAVE_POPCNT) && \
      defined(_MSC_VER) && \
      defined(_WIN32)

#include <nmmintrin.h>

static inline uint64_t popcnt_u64(uint64_t x)
{
  return _mm_popcnt_u32((uint32_t) x) + 
         _mm_popcnt_u32((uint32_t)(x >> 32));
}

#elif defined(HAVE_POPCNT) && \
      defined(__i386__) && \
      defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__> 1))

static inline uint64_t popcnt_u64(uint64_t x)
{
  return __builtin_popcount((uint32_t) x) +
         __builtin_popcount((uint32_t)(x >> 32));
}

#elif defined(HAVE_POPCNT) && \
      defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__> 1))

static inline uint64_t popcnt_u64(uint64_t x)
{
  return __builtin_popcountll(x);
}

#else

static inline uint64_t popcnt_u64(uint64_t x)
{
  // fallback popcount implementation if the POPCNT
  // instruction is not available
  return popcount64c(x);
}

#endif

#if defined(HAVE_POPCNT)

/// Count the number of 1 bits in an array using the POPCNT
/// instruction. On x86 CPUs this requires SSE4.2.
///
static inline uint64_t popcnt_u64_unrolled(const uint64_t* data, uint64_t size)
{
  uint64_t sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
  uint64_t limit = size - size % 4;
  uint64_t i = 0;

  for (; i < limit; i += 4)
  {
    sum0 += popcnt_u64(data[i+0]);
    sum1 += popcnt_u64(data[i+1]);
    sum2 += popcnt_u64(data[i+2]);
    sum3 += popcnt_u64(data[i+3]);
  }

  uint64_t total = sum0 + sum1 + sum2 + sum3;

  for (; i < size; i++)
    total += popcnt_u64(data[i]);

  return total;
}

#else

static inline void CSA(uint64_t& h, uint64_t& l, uint64_t a, uint64_t b, uint64_t c)
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

    total += popcount64c(sixteens);
  }

  total *= 16;
  total += 8 * popcount64c(eights);
  total += 4 * popcount64c(fours);
  total += 2 * popcount64c(twos);
  total += 1 * popcount64c(ones);

  for(; i < size; i++)
    total += popcount64c(data[i]);

  return total;
}

#endif /* popcnt_harley_seal */

#if defined(HAVE_AVX2)

#include <immintrin.h>

#if defined(_MSC_VER)

/// Define missing & operator overload for __m256i type on MSVC compiler
inline __m256i operator&(__m256i a, __m256i b)
{
  return _mm256_and_si256(a, b);
}

/// Define missing | operator overload for __m256i type on MSVC compiler
inline __m256i operator|(__m256i a, __m256i b)
{
  return _mm256_or_si256(a, b);
}

/// Define missing ^ operator overload for __m256i type on MSVC compiler
inline __m256i operator^(__m256i a, __m256i b)
{
  return _mm256_xor_si256(a, b);
}

#endif /* _MSC_VER */

static inline __m256i popcnt_m256i(const __m256i v)
{
  __m256i m1 = _mm256_set1_epi8(0x55);
  __m256i m2 = _mm256_set1_epi8(0x33);
  __m256i m4 = _mm256_set1_epi8(0x0F);

  __m256i t1 = _mm256_sub_epi8(v, (_mm256_srli_epi16(v,  1) & m1));
  __m256i t2 = _mm256_add_epi8(t1 & m2, (_mm256_srli_epi16(t1, 2) & m2));
  __m256i t3 = _mm256_add_epi8(t2, _mm256_srli_epi16(t2, 4)) & m4;

  return _mm256_sad_epu8(t3, _mm256_setzero_si256());
}

static inline void CSA_m256i(__m256i& h, __m256i& l, __m256i a, __m256i b, __m256i c)
{
  __m256i u = a ^ b;
  h = (a & b) | (u & c);
  l = u ^ c;
}

/// AVX2 Harley-Seal popcount (4th iteration).
/// The algorithm is based on the paper "Faster Population Counts
/// using AVX2 Instructions" by Daniel Lemire, Nathan Kurz and
/// Wojciech Mula (23 Nov 2016).
/// @see https://arxiv.org/abs/1611.07612
///
static uint64_t popcnt_harley_seal_avx2(const __m256i* data, uint64_t size)
{
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

  uint64_t* total64 = (uint64_t*) &total;

  return total64[0] +
         total64[1] +
         total64[2] +
         total64[3];
}

#endif /* HAVE_AVX2 */

/// Align memory to 8 bytes boundary
static inline void align8(const uint8_t*& data, uint64_t* size, uint64_t* total)
{
  for (; *size > 0 && (uintptr_t) data % 8 != 0; data++)
  {
    *total += popcnt_u64(*data);
    *size -= 1;
  }
}

/// Align memory to 32 bytes boundary
static inline void align32(const uint64_t*& data, uint64_t* size, uint64_t* total)
{
  for (; *size >= 8 && (uintptr_t) data % 32 != 0; data++)
  {
    *total += popcnt_u64(*data);
    *size -= 8;
  }
}

/// Count the number of 1 bits in the data array.
/// @param data  An array
/// @param size  Size of data in bytes
///
static uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t total = 0;

  const uint8_t* data8 = (const uint8_t*) data;
  align8(data8, &size, &total);
  const uint64_t* data64 = (const uint64_t*) data8;

#if defined(HAVE_AVX2)

  // AVX2 popcount is faster than POPCNT 
  // for array sizes >= 1 kilobyte
  if (size >= 1024 &&
      has_avx2())
  {
    align32(data64, &size, &total);
    total += popcnt_harley_seal_avx2((const __m256i*) data64, size / 32);
    data64 += (size / 32) * 4;
    size = size % 32;
  }

#endif /* HAVE_AVX2 */

#if defined(HAVE_POPCNT)
  total += popcnt_u64_unrolled(data64, size / 8);
#else
  total += popcnt_harley_seal(data64, size / 8);
#endif

  data64 += size / 8;
  size = size % 8;
  data8 = (const uint8_t*) data64;

  // process remaining bytes
  for (uint64_t i = 0; i < size; i++)
    total += popcnt_u64(data8[i]);

  return total;
}

#endif /* LIBPOPCNT_H */
