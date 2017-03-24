// libpopcnt.h - C/C++ library for counting the number of 1 bits (bit
// population count) in an array as quickly as possible using
// specialized CPU instructions e.g. POPCNT, AVX2.
//
// Copyright (c) 2016 - 2017, Kim Walisch
// Copyright (c) 2016 - 2017, Wojciech Muła
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

// GCC >= 4.9 & Clang >= 3.8 have __attribute__((target))
#define HAVE__attribute__target \
  ((defined(__GNUC__) && \
           (__GNUC__ > 4 || \
           (__GNUC__ == 4 && \
            __GNUC_MINOR__ >= 9))) || \
   (defined(__clang__) && \
    defined(__apple_build_version__) && \
    __apple_build_version__ >= 8000000) || \
   (defined(__clang__) && \
   !defined(__apple_build_version__) && \
           (__clang_major__ > 3 || \
           (__clang_major__ == 3 && \
            __clang_minor__ >= 8))))

/// This uses fewer arithmetic operations than any other known
/// implementation on machines with fast multiplication.
/// It uses 12 arithmetic operations, one of which is a multiply.
/// http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
///
static inline uint64_t popcount64c(uint64_t x)
{
  uint64_t m1 = 0x5555555555555555ll;
  uint64_t m2 = 0x3333333333333333ll;
  uint64_t m4 = 0x0F0F0F0F0F0F0F0Fll;
  uint64_t h01 = 0x0101010101010101ll;

  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2);
  x = (x + (x >> 4)) & m4;

  return (x * h01) >> 56;
}

#if defined(_MSC_VER) && \
    defined(_M_X64)

#include <nmmintrin.h>

static inline uint64_t popcnt64(uint64_t x)
{
  return _mm_popcnt_u64(x);
}

#elif defined(_MSC_VER) && \
      defined(_M_IX86)

#include <nmmintrin.h>

static inline uint64_t popcnt64(uint64_t x)
{
  return _mm_popcnt_u32((uint32_t) x) + 
         _mm_popcnt_u32((uint32_t)(x >> 32));
}

// GCC, Clang, icpc: x64 CPUs
#elif defined(__x86_64__) && \
      defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))

static inline uint64_t popcnt64(uint64_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

// GCC, Clang, icpc: x86 CPUs
#elif defined(__i386__) && \
      defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))

static inline uint32_t popcnt32(uint32_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

static inline uint64_t popcnt64(uint64_t x)
{
  return popcnt32((uint32_t) x) +
         popcnt32((uint32_t)(x >> 32));
}

// GCC & Clang: non x86 CPUs
#elif defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))

static inline uint64_t popcnt64(uint64_t x)
{
  return __builtin_popcountll(x);
}

// no hardware POPCNT,
// use integer popcnt64 implementation
#else

static inline uint64_t popcnt64(uint64_t x)
{
  return popcount64c(x);
}

#endif

// x86 cpuid
#if defined(__x86_64__) || \
    defined(__i386__) || \
    defined(_M_X64) || \
    defined(_M_IX86)

#if defined(_MSC_VER)
  #include <intrin.h>
#endif

// %ecx bit flags
#define bit_POPCNT (1 << 23)

// %ebx bit flags
#define bit_AVX2   (1 << 5)

static inline void run_cpuid(uint32_t eax, uint32_t ecx, uint32_t* abcd)
{
  uint32_t ebx = 0;
  uint32_t edx = 0;

#if defined(_MSC_VER)
  __cpuidex(abcd, eax, ecx);
#else

#if defined(__i386__) &&  \
    defined(__PIC__)
  // in case of PIC under 32-bit EBX cannot be clobbered
  __asm__ ("movl %%ebx, %%edi;"
           "cpuid;"
           "xchgl %%ebx, %%edi;"
           : "=D" (ebx),
#else
  __asm__ ("cpuid;"
           : "+b" (ebx),
#endif
             "+a" (eax),
             "+c" (ecx),
             "=d" (edx));
#endif

  abcd[0] = eax;
  abcd[1] = ebx;
  abcd[2] = ecx;
  abcd[3] = edx;
}

#if !defined(_MSC_VER)

static inline int check_xcr0_ymm()
{
  uint32_t xcr0;
  __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
  return ((xcr0 & 6) == 6);
}

static inline int has_AVX2()
{
  uint32_t abcd[4];
  uint32_t osxsave_mask = (1 << 27);

  // must ensure OS supports extended processor state management
  run_cpuid( 1, 0, abcd );
  if ((abcd[2] & osxsave_mask) != osxsave_mask)
    return 0;

  // must ensure OS supports ZMM registers (and YMM, and XMM)
  if (!check_xcr0_ymm())
    return 0;

  run_cpuid(7, 0, abcd);
  if ((abcd[1] & bit_AVX2) != bit_AVX2)
    return 0;

  return bit_AVX2;
}

#endif /* !_MSC_VER */

static inline int has_POPCNT()
{
  uint32_t abcd[4];

  run_cpuid(1, 0, abcd);
  if ((abcd[2] & bit_POPCNT) != bit_POPCNT)
    return 0;

  return bit_POPCNT;
}

#endif /* cpuid */

static inline uint64_t popcnt64_unrolled(const uint64_t* data, uint64_t size)
{
  uint64_t sum0 = 0;
  uint64_t sum1 = 0;
  uint64_t sum2 = 0;
  uint64_t sum3 = 0;

  uint64_t i = 0;
  uint64_t limit = size - size % 4;

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
}

/// Carry-save adder (CSA).
/// @see Chapter 5 in "Hacker's Delight".
///
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
static inline uint64_t popcnt64_hs(const uint64_t* data, uint64_t size)
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

// non x86 CPUs
#if !defined(__x86_64__) && \
    !defined(__i386__) && \
    !defined(_M_X64) && \
    !defined(_M_IX86)

/// Align memory to 8 bytes boundary
static inline void align8(const uint8_t*& p, uint64_t& size, uint64_t& total)
{
  for (; size > 0 && (uintptr_t) p % 8 != 0; p++)
  {
    total += popcnt64(*p);
    size -= 1;
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
  align8(data8, size, total);

  total += popcnt64_unrolled((const uint64_t*) data8, size / 8);
  data8 += size - size % 8;
  size = size % 8;

  for (uint64_t i = 0; i < size; i++)
    total += popcnt64(data8[i]);

  return total;
}

#endif

// x86 CPUs, no avx2
#if !HAVE__attribute__target && \
    (defined(__x86_64__) || \
     defined(__i386__) || \
     defined(_M_X64) || \
     defined(_M_IX86))

/// Count the number of 1 bits in the data array.
/// @param data  An array
/// @param size  Size of data in bytes
///
static uint64_t popcnt(const void* data, uint64_t size)
{
  static const int cpuid = has_POPCNT();

  uint64_t total = 0;
  const uint8_t* data8 = (const uint8_t*) data;

  if (cpuid & bit_POPCNT)
  {
    const uint64_t* data64 = (const uint64_t*) data8;
    uint64_t size64 = size / 8;
    total += popcnt64_unrolled(data64, size64);
    data8 += size - size % 8;
    size = size % 8;
    for (uint64_t i = 0; i < size; i++)
      total += popcnt64(data8[i]);

    return total;
  }

  // pure integer algorithm
  const uint64_t* data64 = (const uint64_t*) data8;
  uint64_t size64 = size / 8;
  total += popcnt64_hs(data64, size64);
  data8 += size - size % 8;
  size = size % 8;
  for (uint64_t i = 0; i < size; i++)
    total += popcount64c(data8[i]);

  return total;
}

#endif

// x86 CPUs, compiler supports (target ("avx2"))
#if HAVE__attribute__target && \
    (defined(__x86_64__) || \
     defined(__i386__))

#include <immintrin.h>

__attribute__ ((target ("avx2")))
static inline void CSA256(__m256i& h, __m256i& l, __m256i a, __m256i b, __m256i c)
{
  __m256i u = a ^ b;
  h = (a & b) | (u & c);
  l = u ^ c;
}

__attribute__ ((target ("avx2")))
static inline __m256i popcnt256(__m256i v)
{
    __m256i lookup1 = _mm256_setr_epi8(
        4, 5, 5, 6, 5, 6, 6, 7,
        5, 6, 6, 7, 6, 7, 7, 8,
        4, 5, 5, 6, 5, 6, 6, 7,
        5, 6, 6, 7, 6, 7, 7, 8
    );

    __m256i lookup2 = _mm256_setr_epi8(
        4, 3, 3, 2, 3, 2, 2, 1,
        3, 2, 2, 1, 2, 1, 1, 0,
        4, 3, 3, 2, 3, 2, 2, 1,
        3, 2, 2, 1, 2, 1, 1, 0
    );

    __m256i low_mask = _mm256_set1_epi8(0x0f);
    __m256i lo = v & low_mask;
    __m256i hi = _mm256_srli_epi16(v, 4) & low_mask;
    __m256i popcnt1 = _mm256_shuffle_epi8(lookup1, lo);
    __m256i popcnt2 = _mm256_shuffle_epi8(lookup2, hi);

    return _mm256_sad_epu8(popcnt1, popcnt2);
}

/// AVX2 Harley-Seal popcount (4th iteration).
/// The algorithm is based on the paper "Faster Population Counts
/// using AVX2 Instructions" by Daniel Lemire, Nathan Kurz and
/// Wojciech Mula (23 Nov 2016).
/// @see https://arxiv.org/abs/1611.07612
///
__attribute__ ((target ("avx2")))
static inline uint64_t popcnt_avx2(const __m256i* data, uint64_t size)
{
  __m256i total = _mm256_setzero_si256();
  __m256i ones = _mm256_setzero_si256();
  __m256i twos = _mm256_setzero_si256();
  __m256i fours = _mm256_setzero_si256();
  __m256i eights = _mm256_setzero_si256();
  __m256i sixteens = _mm256_setzero_si256();
  __m256i twosA, twosB, foursA, foursB, eightsA, eightsB;

  uint64_t i = 0;
  uint64_t limit = size - size % 16;

  for(; i < limit; i += 16)
  {
    CSA256(twosA, ones, ones, data[i+0], data[i+1]);
    CSA256(twosB, ones, ones, data[i+2], data[i+3]);
    CSA256(foursA, twos, twos, twosA, twosB);
    CSA256(twosA, ones, ones, data[i+4], data[i+5]);
    CSA256(twosB, ones, ones, data[i+6], data[i+7]);
    CSA256(foursB, twos, twos, twosA, twosB);
    CSA256(eightsA,fours, fours, foursA, foursB);
    CSA256(twosA, ones, ones, data[i+8], data[i+9]);
    CSA256(twosB, ones, ones, data[i+10], data[i+11]);
    CSA256(foursA, twos, twos, twosA, twosB);
    CSA256(twosA, ones, ones, data[i+12], data[i+13]);
    CSA256(twosB, ones, ones, data[i+14], data[i+15]);
    CSA256(foursB, twos, twos, twosA, twosB);
    CSA256(eightsB, fours, fours, foursA, foursB);
    CSA256(sixteens, eights, eights, eightsA, eightsB);

    total = _mm256_add_epi64(total, popcnt256(sixteens));
  }

  total = _mm256_slli_epi64(total, 4);
  total = _mm256_add_epi64(total, _mm256_slli_epi64(popcnt256(eights), 3));
  total = _mm256_add_epi64(total, _mm256_slli_epi64(popcnt256(fours), 2));
  total = _mm256_add_epi64(total, _mm256_slli_epi64(popcnt256(twos), 1));
  total = _mm256_add_epi64(total, popcnt256(ones));

  for(; i < size; i++)
    total = _mm256_add_epi64(total, popcnt256(data[i]));

  uint64_t* total64 = (uint64_t*) &total;

  return total64[0] +
         total64[1] +
         total64[2] +
         total64[3];
}

/// Align memory to 32 bytes boundary
static inline void align_avx2(const uint8_t*& p, uint64_t& size, uint64_t& total)
{
  for (; (uintptr_t) p % 8; p++)
  {
    total += popcnt64(*p);
    size -= 1;
  }
  for (; (uintptr_t) p % 32; p += 8)
  {
    total += popcnt64(
        *(const uint64_t*) p);
    size -= 8;
  }
}

/// Count the number of 1 bits in the data array.
/// @param data  An array
/// @param size  Size of data in bytes
///
static uint64_t popcnt(const void* data, uint64_t size)
{
  static const int cpuid =
      has_POPCNT() | has_AVX2();

  uint64_t total = 0;
  const uint8_t* data8 = (const uint8_t*) data;

  // AVX2 is faster than POPCNT for
  // array sizes >= 1 KB
  if ((cpuid & bit_AVX2) &&
      size >= 1024)
  {
    align_avx2(data8, size, total);
    const __m256i* data32 = (const __m256i*) data8;
    uint64_t size32 = size / 32;
    total += popcnt_avx2(data32, size32);
    data8 += size - size % 32;
    size = size % 32;
  }

  if (cpuid & bit_POPCNT)
  {
    const uint64_t* data64 = (const uint64_t*) data8;
    uint64_t size64 = size / 8;
    total += popcnt64_unrolled(data64, size64);
    data8 += size - size % 8;
    size = size % 8;
    for (uint64_t i = 0; i < size; i++)
      total += popcnt64(data8[i]);

    return total;
  }

  // pure integer algorithm
  const uint64_t* data64 = (const uint64_t*) data8;
  uint64_t size64 = size / 8;
  total += popcnt64_hs(data64, size64);
  data8 += size - size % 8;
  size = size % 8;
  for (uint64_t i = 0; i < size; i++)
    total += popcount64c(data8[i]);

  return total;
}

#endif /* avx2 */

#endif /* LIBPOPCNT_H */
