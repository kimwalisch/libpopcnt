// libpopcnt.h - C++ library for counting the number of 1 bits (bit
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

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

// Clang & GCC >= 4.2
#if __has_builtin(__builtin_popcount) || \
    (defined(__GNUC__) && \
            (__GNUC__ > 4 || \
            (__GNUC__ == 4 && \
             __GNUC_MINOR__ >= 2)))
  #define HAS_BUILTIN_POPCOUNT
#endif

// GCC >= 4.2
#if defined(__GNUC__) && \
           (__GNUC__ > 4 || \
           (__GNUC__ == 4 && \
            __GNUC_MINOR__ >= 2))
  #define HAS_ASM_POPCNT
#endif

// Clang >= 3.0
#if defined(__clang__) && \
           (__clang_major__ > 3 || \
           (__clang_major__ == 3 && \
            __clang_minor__ >= 0))
  #define HAS_ASM_POPCNT
#endif

// GCC >= 4.9
#if (defined(__x86_64__) || \
     defined(__i386__)) && \
     defined(__GNUC__) && \
            (__GNUC__ > 4 || \
            (__GNUC__ == 4 && \
            __GNUC_MINOR__ >= 9))
  #define HAS_AVX2
#endif

// Clang >= 3.8
#if (defined(__x86_64__) || \
     defined(__i386__)) && \
     defined(__clang__) && \
    !defined(_MSC_VER) && \
    !defined(__apple_build_version__) && \
            (__clang_major__ > 3 || \
            (__clang_major__ == 3 && \
             __clang_minor__ >= 8))
  #define HAS_AVX2
#endif

// Apple Clang >= 8.0.0
#if (defined(__x86_64__) || \
     defined(__i386__)) && \
     defined(__clang__) && \
     defined(__apple_build_version__) && \
            (__apple_build_version__ >= 8000000)
  #define HAS_AVX2
#endif

#if defined(HAS_AVX2)
  #include <immintrin.h>
#endif

// MSVC, x86
#if defined(_MSC_VER) && \
   (defined(_M_X64) || \
    defined(_M_IX86))
  #include <intrin.h>
  #include <immintrin.h>
  #include <nmmintrin.h>
#endif

namespace {

/// This uses fewer arithmetic operations than any other known
/// implementation on machines with fast multiplication.
/// It uses 12 arithmetic operations, one of which is a multiply.
/// http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
///
inline uint64_t popcount64(uint64_t x)
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

#if defined(HAS_ASM_POPCNT) && \
    defined(__x86_64__)

inline uint64_t popcnt64(uint64_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

#elif defined(HAS_ASM_POPCNT) && \
      defined(__i386__)

inline uint32_t popcnt32(uint32_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

inline uint64_t popcnt64(uint64_t x)
{
  return popcnt32((uint32_t) x) +
         popcnt32((uint32_t)(x >> 32));
}

#elif defined(_MSC_VER) && \
      defined(_M_X64)

inline uint64_t popcnt64(uint64_t x)
{
  return _mm_popcnt_u64(x);
}

#elif defined(_MSC_VER) && \
      defined(_M_IX86)

inline uint64_t popcnt64(uint64_t x)
{
  return _mm_popcnt_u32((uint32_t) x) + 
         _mm_popcnt_u32((uint32_t)(x >> 32));
}

// non x86 CPUs
#elif defined(HAS_BUILTIN_POPCOUNT)

inline uint64_t popcnt64(uint64_t x)
{
  return __builtin_popcountll(x);
}

// no hardware POPCNT,
// use integer popcnt64 implementation
#else

inline uint64_t popcnt64(uint64_t x)
{
  return popcount64(x);
}

#endif

inline uint64_t popcnt64_unrolled(const uint64_t* data, uint64_t size)
{
  uint64_t i = 0;
  uint64_t limit = size - size % 4;
  uint64_t cnt = 0;

  for (; i < limit; i += 4)
  {
    cnt += popcnt64(data[i+0]);
    cnt += popcnt64(data[i+1]);
    cnt += popcnt64(data[i+2]);
    cnt += popcnt64(data[i+3]);
  }

  for (; i < size; i++)
    cnt += popcnt64(data[i]);

  return cnt;
}

/// Carry-save adder (CSA).
/// @see Chapter 5 in "Hacker's Delight".
///
inline void CSA(uint64_t& h, uint64_t& l, uint64_t a, uint64_t b, uint64_t c)
{
  uint64_t u = a ^ b; 
  h = (a & b) | (u & c);
  l = u ^ c;
}

/// Harley-Seal popcount (3rd iteration).
/// The Harley-Seal popcount algorithm is one of the fastest algorithms
/// for counting 1 bits in an array using only integer operations.
/// This implementation uses only 6.38 instructions per 64-bit word.
/// @see Chapter 5 in "Hacker's Delight" 2nd edition.
///
inline uint64_t popcnt64_hs(const uint64_t* data, uint64_t size)
{
  uint64_t cnt = 0;
  uint64_t ones = 0, twos = 0, fours = 0, eights = 0;
  uint64_t twosA, twosB, foursA, foursB;
  uint64_t limit = size - size % 8;
  uint64_t i = 0;

  for(; i < limit; i += 8)
  {
    CSA(twosA, ones, ones, data[i+0], data[i+1]);
    CSA(twosB, ones, ones, data[i+2], data[i+3]);
    CSA(foursA, twos, twos, twosA, twosB);
    CSA(twosA, ones, ones, data[i+4], data[i+5]);
    CSA(twosB, ones, ones, data[i+6], data[i+7]);
    CSA(foursB, twos, twos, twosA, twosB);
    CSA(eights, fours, fours, foursA, foursB);

    cnt += popcount64(eights);
  }

  cnt *= 8;
  cnt += 4 * popcount64(fours);
  cnt += 2 * popcount64(twos);
  cnt += 1 * popcount64(ones);

  for(; i < size; i++)
    cnt += popcount64(data[i]);

  return cnt;
}

// x86 cpuid
#if defined(__x86_64__) || \
    defined(__i386__) || \
    defined(_M_X64) || \
    defined(_M_IX86)

// %ecx bit flags
#define bit_POPCNT (1 << 23)

// %ebx bit flags
#define bit_AVX2 (1 << 5)

inline void run_cpuid(int eax, int ecx, int* abcd)
{
  int ebx = 0;
  int edx = 0;

#if defined(_MSC_VER)
  __cpuidex(abcd, eax, ecx);
#elif defined(__i386__) && \
      defined(__PIC__)
  // in case of PIC under 32-bit EBX cannot be clobbered
  __asm__ ("movl %%ebx, %%edi;"
           "cpuid;"
           "xchgl %%ebx, %%edi;"
           : "=D" (ebx),
             "+a" (eax),
             "+c" (ecx),
             "=d" (edx));
#else
  __asm__ ("cpuid;"
           : "+b" (ebx),
             "+a" (eax),
             "+c" (ecx),
             "=d" (edx));
#endif
  abcd[0] = eax;
  abcd[1] = ebx;
  abcd[2] = ecx;
  abcd[3] = edx;
}

inline int has_POPCNT()
{
  int abcd[4];

  run_cpuid(1, 0, abcd);
  if ((abcd[2] & bit_POPCNT) != bit_POPCNT)
    return 0;

  return bit_POPCNT;
}

inline int check_xcr0_ymm()
{
  int xcr0;
#if defined(_MSC_VER)
  xcr0 = (uint32_t) _xgetbv(0);
#else
  __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#endif
  return (xcr0 & 6) == 6;
}

inline int has_AVX2()
{
  int abcd[4];
  int osxsave_mask = (1 << 27);

  // must ensure OS supports extended processor state management
  run_cpuid(1, 0, abcd);
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

#endif /* cpuid */

#if defined(HAS_AVX2)

__attribute__ ((target ("avx2")))
inline void CSA256(__m256i& h, __m256i& l, __m256i a, __m256i b, __m256i c)
{
  __m256i u = a ^ b;
  h = (a & b) | (u & c);
  l = u ^ c;
}

__attribute__ ((target ("avx2")))
inline __m256i popcnt256(__m256i v)
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
inline uint64_t popcnt_avx2(const __m256i* data, uint64_t size)
{
  __m256i cnt = _mm256_setzero_si256();
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

    cnt = _mm256_add_epi64(cnt, popcnt256(sixteens));
  }

  cnt = _mm256_slli_epi64(cnt, 4);
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(eights), 3));
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(fours), 2));
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(twos), 1));
  cnt = _mm256_add_epi64(cnt, popcnt256(ones));

  for(; i < size; i++)
    cnt = _mm256_add_epi64(cnt, popcnt256(data[i]));

  uint64_t* cnt64 = (uint64_t*) &cnt;

  return cnt64[0] +
         cnt64[1] +
         cnt64[2] +
         cnt64[3];
}

/// Align memory to 32 bytes boundary
inline void align_avx2(const uint8_t*& p, uint64_t& size, uint64_t& cnt)
{
  for (; (uintptr_t) p % 8; p++)
  {
    cnt += popcnt64(*p);
    size -= 1;
  }
  for (; (uintptr_t) p % 32; p += 8)
  {
    cnt += popcnt64(
        *(const uint64_t*) p);
    size -= 8;
  }
}

#endif /* avx2 */

// non x86 CPUs
#if !defined(__x86_64__) && \
    !defined(__i386__) && \
    !defined(_M_X64) && \
    !defined(_M_IX86)

/// Align memory to 8 bytes boundary
inline void align8(const uint8_t*& p, uint64_t& size, uint64_t& cnt)
{
  for (; size > 0 && (uintptr_t) p % 8; p++)
  {
    cnt += popcnt64(*p);
    size -= 1;
  }
}

/// Count the number of 1 bits in the data array.
/// @param data  An array
/// @param size  Size of data in bytes
///
uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t cnt = 0;

  const uint8_t* data8 = (const uint8_t*) data;
  align8(data8, size, cnt);

  cnt += popcnt64_unrolled((const uint64_t*) data8, size / 8);
  data8 += size - size % 8;
  size = size % 8;

  for (uint64_t i = 0; i < size; i++)
    cnt += popcnt64(data8[i]);

  return cnt;
}

#endif

// x86 CPUs
#if defined(__x86_64__) || \
    defined(__i386__) || \
    defined(_M_X64) || \
    defined(_M_IX86)

/// Count the number of 1 bits in the data array.
/// @param data  An array
/// @param size  Size of data in bytes
///
uint64_t popcnt(const void* data, uint64_t size)
{
  static const int cpuid =
      has_POPCNT() | has_AVX2();

  uint64_t cnt = 0;
  const uint8_t* data8 = (const uint8_t*) data;

#if defined(HAS_AVX2)

  // AVX2 requires arrays >= 512 bytes
  if ((cpuid & bit_AVX2) &&
      size >= 512)
  {
    align_avx2(data8, size, cnt);
    const __m256i* data32 = (const __m256i*) data8;
    uint64_t size32 = size / 32;
    cnt += popcnt_avx2(data32, size32);
    data8 += size - size % 32;
    size = size % 32;
  }

#endif

  if (cpuid & bit_POPCNT)
  {
    const uint64_t* data64 = (const uint64_t*) data8;
    uint64_t size64 = size / 8;
    cnt += popcnt64_unrolled(data64, size64);
    data8 += size - size % 8;
    size = size % 8;
    for (uint64_t i = 0; i < size; i++)
      cnt += popcnt64(data8[i]);

    return cnt;
  }

  // pure integer algorithm
  const uint64_t* data64 = (const uint64_t*) data8;
  uint64_t size64 = size / 8;
  cnt += popcnt64_hs(data64, size64);
  data8 += size - size % 8;
  size = size % 8;
  for (uint64_t i = 0; i < size; i++)
    cnt += popcount64(data8[i]);

  return cnt;
}

#endif /* x86 */

} // namespace

#endif /* LIBPOPCNT_H */
