/*
 * libpopcnt.h - C/C++ library for counting the number of 1 bits (bit
 * population count) in an array as quickly as possible using
 * specialized CPU instructions i.e. POPCNT, AVX2, AVX512, NEON.
 *
 * Copyright (c) 2016 - 2024, Kim Walisch
 * Copyright (c) 2016 - 2018, Wojciech Muła
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LIBPOPCNT_H
#define LIBPOPCNT_H

#include <stdint.h>
#include <string.h>

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif

#ifndef __has_include
  #define __has_include(x) 0
#endif

#ifdef __GNUC__
  #define GNUC_PREREQ(x, y) \
      (__GNUC__ > x || (__GNUC__ == x && __GNUC_MINOR__ >= y))
#else
  #define GNUC_PREREQ(x, y) 0
#endif

#ifdef __clang__
  #define CLANG_PREREQ(x, y) \
      (__clang_major__ > x || (__clang_major__ == x && __clang_minor__ >= y))
#else
  #define CLANG_PREREQ(x, y) 0
#endif

#if (_MSC_VER < 1900) && \
    !defined(__cplusplus)
  #define inline __inline
#endif

#if (defined(__i386__) || \
     defined(__x86_64__) || \
     defined(_M_IX86) || \
     defined(_M_X64))
  #define X86_OR_X64
#endif

#if GNUC_PREREQ(4, 2) || \
    __has_builtin(__builtin_popcount)
  #define HAVE_BUILTIN_POPCOUNT
#endif

#if GNUC_PREREQ(4, 2) || \
    CLANG_PREREQ(3, 0)
  #define HAVE_ASM_POPCNT
#endif

#if defined(X86_OR_X64) && \
   (defined(HAVE_ASM_POPCNT) || \
    defined(_MSC_VER))
  #define HAVE_POPCNT
#endif

/* GCC compiler */
#if defined(X86_OR_X64) && \
    GNUC_PREREQ(4, 9)
  #define HAVE_AVX2
#endif

/* GCC compiler */
#if defined(X86_OR_X64) && \
    GNUC_PREREQ(11, 0)
  #define HAVE_AVX512
#endif

/* MSVC compatible compilers (Windows) */
#if defined(X86_OR_X64) && \
    defined(_MSC_VER)
  /* clang-cl (LLVM 10 from 2020) requires /arch:AVX2 or
  * /arch:AVX512 to enable vector instructions */
  #if defined(__clang__)
    #if defined(__AVX2__)
      #define HAVE_AVX2
    #endif
    #if defined(__AVX512__)
      #define HAVE_AVX2
      #define HAVE_AVX512
    #endif
  /* MSVC 2017 or later does not require
  * /arch:AVX2 or /arch:AVX512 */
  #elif _MSC_VER >= 1910
    #define HAVE_AVX2
    #define HAVE_AVX512
  #endif
#endif

/* Clang (Unix-like OSes) */
#if defined(X86_OR_X64) && !defined(_MSC_VER)
  #if CLANG_PREREQ(3, 8) && \
      __has_attribute(target) && \
      (!defined(__apple_build_version__) || __apple_build_version__ >= 8000000)
    #define HAVE_AVX2
  #endif
  #if CLANG_PREREQ(9, 0) && \
      __has_attribute(target) && \
      (!defined(__apple_build_version__) || __apple_build_version__ >= 8000000)
    #define HAVE_AVX512
  #endif
#endif

/*
 * Only enable CPUID runtime checks if this is really
 * needed. E.g. do not enable if user has compiled
 * using -march=native on a CPU that supports AVX512.
 */
#if defined(X86_OR_X64) && \
   (defined(__cplusplus) || \
    defined(_MSC_VER) || \
   (GNUC_PREREQ(4, 2) || \
    __has_builtin(__sync_val_compare_and_swap))) && \
   ((defined(HAVE_AVX512) && !(defined(__AVX512__) || (defined(__AVX512F__) && defined(__AVX512VPOPCNTDQ__)))) || \
    (defined(HAVE_AVX2) && !defined(__AVX2__)) || \
    (defined(HAVE_POPCNT) && !defined(__POPCNT__)))
  #define HAVE_CPUID
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This uses fewer arithmetic operations than any other known
 * implementation on machines with fast multiplication.
 * It uses 12 arithmetic operations, one of which is a multiply.
 * http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
 */
static inline uint64_t popcnt64_bitwise(uint64_t x)
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

#if defined(HAVE_ASM_POPCNT) && \
    defined(__x86_64__)

static inline uint64_t popcnt64(uint64_t x)
{
  __asm__ ("popcnt %1, %0" : "=r" (x) : "0" (x));
  return x;
}

#elif defined(HAVE_ASM_POPCNT) && \
      defined(__i386__)

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

#elif defined(_MSC_VER) && \
      defined(_M_X64)

#include <intrin.h>

static inline uint64_t popcnt64(uint64_t x)
{
  return __popcnt64(x);
}

#elif defined(_MSC_VER) && \
      defined(_M_IX86)

#include <intrin.h>

static inline uint64_t popcnt64(uint64_t x)
{
  return __popcnt((uint32_t) x) + 
         __popcnt((uint32_t)(x >> 32));
}

/* non x86 CPUs */
#elif defined(HAVE_BUILTIN_POPCOUNT)

static inline uint64_t popcnt64(uint64_t x)
{
  return __builtin_popcountll(x);
}

/* no hardware POPCNT,
 * use pure integer algorithm */
#else

static inline uint64_t popcnt64(uint64_t x)
{
  return popcnt64_bitwise(x);
}

#endif

#if defined(HAVE_CPUID)

#if defined(_MSC_VER)
  #include <intrin.h>
  #include <immintrin.h>
#endif

/* CPUID bits documentation: */
/* https://en.wikipedia.org/wiki/CPUID */

/* %ebx bit flags */
#define bit_AVX2    (1 << 5)
#define bit_AVX512F (1 << 16)

/* %ecx bit flags */
#define bit_POPCNT (1 << 23)
#define bit_AVX512_VPOPCNTDQ (1 << 14)

/* xgetbv bit flags */
#define XSTATE_SSE (1 << 1)
#define XSTATE_YMM (1 << 2)
#define XSTATE_ZMM (7 << 5)

static inline void run_cpuid(int eax, int ecx, int* abcd)
{
#if defined(_MSC_VER)
  __cpuidex(abcd, eax, ecx);
#else
  int ebx = 0;
  int edx = 0;

  #if defined(__i386__) && \
      defined(__PIC__)
    /* In case of PIC under 32-bit EBX cannot be clobbered */
    asm volatile("movl %%ebx, %%edi;"
                 "cpuid;"
                 "xchgl %%ebx, %%edi;"
                 : "+a" (eax),
                   "=D" (ebx),
                   "+c" (ecx),
                   "=d" (edx));
  #else
    asm volatile("cpuid"
                 : "+a" (eax),
                   "+b" (ebx),
                   "+c" (ecx),
                   "=d" (edx));
  #endif

  abcd[0] = eax;
  abcd[1] = ebx;
  abcd[2] = ecx;
  abcd[3] = edx;
#endif
}

#if defined(HAVE_AVX2) || \
    defined(HAVE_AVX512)

static inline uint64_t get_xcr0()
{
#if defined(_MSC_VER)
  return _xgetbv(0);
#else
  uint32_t eax;
  uint32_t edx;

  asm volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
  return eax | (((uint64_t) edx) << 32);
#endif
}

#endif

static inline int get_cpuid()
{
  int flags = 0;
  int abcd[4];

  run_cpuid(1, 0, abcd);

  if ((abcd[2] & bit_POPCNT) == bit_POPCNT)
    flags |= bit_POPCNT;

#if defined(HAVE_AVX2) || \
    defined(HAVE_AVX512)

  int osxsave_mask = (1 << 27);

  /* ensure OS supports extended processor state management */
  if ((abcd[2] & osxsave_mask) != osxsave_mask)
    return 0;

  uint64_t ymm_mask = XSTATE_SSE | XSTATE_YMM;
  uint64_t zmm_mask = XSTATE_SSE | XSTATE_YMM | XSTATE_ZMM;
  uint64_t xcr0 = get_xcr0();

  if ((xcr0 & ymm_mask) == ymm_mask)
  {
    run_cpuid(7, 0, abcd);

    if ((abcd[1] & bit_AVX2) == bit_AVX2)
      flags |= bit_AVX2;

    if ((xcr0 & zmm_mask) == zmm_mask)
    {
      if ((abcd[1] & bit_AVX512F) == bit_AVX512F &&
          (abcd[2] & bit_AVX512_VPOPCNTDQ) == bit_AVX512_VPOPCNTDQ)
        flags |= bit_AVX512_VPOPCNTDQ;
    }
  }

#endif

  return flags;
}

#endif /* cpuid */

#if defined(HAVE_AVX2) && \
    __has_include(<immintrin.h>)

#include <immintrin.h>

#if !defined(_MSC_VER)
  __attribute__ ((target ("avx2")))
#endif
static inline void CSA256(__m256i* h, __m256i* l, __m256i a, __m256i b, __m256i c)
{
  __m256i u = _mm256_xor_si256(a, b);
  *h = _mm256_or_si256(_mm256_and_si256(a, b), _mm256_and_si256(u, c));
  *l = _mm256_xor_si256(u, c);
}

#if !defined(_MSC_VER)
  __attribute__ ((target ("avx2")))
#endif
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
  __m256i lo = _mm256_and_si256(v, low_mask);
  __m256i hi = _mm256_and_si256(_mm256_srli_epi16(v, 4), low_mask);
  __m256i popcnt1 = _mm256_shuffle_epi8(lookup1, lo);
  __m256i popcnt2 = _mm256_shuffle_epi8(lookup2, hi);

  return _mm256_sad_epu8(popcnt1, popcnt2);
}

/*
 * AVX2 Harley-Seal popcount (4th iteration).
 * The algorithm is based on the paper "Faster Population Counts
 * using AVX2 Instructions" by Daniel Lemire, Nathan Kurz and
 * Wojciech Mula (23 Nov 2016).
 * @see https://arxiv.org/abs/1611.07612
 */
#if !defined(_MSC_VER)
  __attribute__ ((target ("avx2")))
#endif
static inline uint64_t popcnt_avx2(const __m256i* ptr, uint64_t size)
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
  uint64_t* cnt64;

  for(; i < limit; i += 16)
  {
    CSA256(&twosA, &ones, ones, _mm256_loadu_si256(ptr + i + 0), _mm256_loadu_si256(ptr + i + 1));
    CSA256(&twosB, &ones, ones, _mm256_loadu_si256(ptr + i + 2), _mm256_loadu_si256(ptr + i + 3));
    CSA256(&foursA, &twos, twos, twosA, twosB);
    CSA256(&twosA, &ones, ones, _mm256_loadu_si256(ptr + i + 4), _mm256_loadu_si256(ptr + i + 5));
    CSA256(&twosB, &ones, ones, _mm256_loadu_si256(ptr + i + 6), _mm256_loadu_si256(ptr + i + 7));
    CSA256(&foursB, &twos, twos, twosA, twosB);
    CSA256(&eightsA, &fours, fours, foursA, foursB);
    CSA256(&twosA, &ones, ones, _mm256_loadu_si256(ptr + i + 8), _mm256_loadu_si256(ptr + i + 9));
    CSA256(&twosB, &ones, ones, _mm256_loadu_si256(ptr + i + 10), _mm256_loadu_si256(ptr + i + 11));
    CSA256(&foursA, &twos, twos, twosA, twosB);
    CSA256(&twosA, &ones, ones, _mm256_loadu_si256(ptr + i + 12), _mm256_loadu_si256(ptr + i + 13));
    CSA256(&twosB, &ones, ones, _mm256_loadu_si256(ptr + i + 14), _mm256_loadu_si256(ptr + i + 15));
    CSA256(&foursB, &twos, twos, twosA, twosB);
    CSA256(&eightsB, &fours, fours, foursA, foursB);
    CSA256(&sixteens, &eights, eights, eightsA, eightsB);

    cnt = _mm256_add_epi64(cnt, popcnt256(sixteens));
  }

  cnt = _mm256_slli_epi64(cnt, 4);
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(eights), 3));
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(fours), 2));
  cnt = _mm256_add_epi64(cnt, _mm256_slli_epi64(popcnt256(twos), 1));
  cnt = _mm256_add_epi64(cnt, popcnt256(ones));

  for(; i < size; i++)
    cnt = _mm256_add_epi64(cnt, popcnt256(_mm256_loadu_si256(ptr + i)));

  cnt64 = (uint64_t*) &cnt;

  return cnt64[0] +
         cnt64[1] +
         cnt64[2] +
         cnt64[3];
}

#endif

#if defined(HAVE_AVX512) && \
    __has_include(<immintrin.h>)

#include <immintrin.h>

#if !defined(_MSC_VER)
  __attribute__ ((target ("avx512f,avx512vpopcntdq")))
#endif
static inline uint64_t popcnt_avx512(const uint64_t* ptr, const uint64_t size)
{
    __m512i cnt = _mm512_setzero_si512();
    uint64_t i = 0;

    for (; i + 8 < size; i += 8)
    {
      __m512i vec = _mm512_loadu_epi64(&ptr[i]);
      vec = _mm512_popcnt_epi64(vec);
      cnt = _mm512_add_epi64(cnt, vec);
    }

    __mmask8 mask = (__mmask8) (0xff >> (i + 8 - size));
    __m512i vec = _mm512_maskz_loadu_epi64(mask , &ptr[i]);
    vec = _mm512_popcnt_epi64(vec);
    cnt = _mm512_add_epi64(cnt, vec);

    return _mm512_reduce_add_epi64(cnt);
}

#endif

/* x86 CPUs */
#if defined(X86_OR_X64)

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i = 0;
  uint64_t cnt = 0;
  const uint8_t* ptr = (const uint8_t*) data;

/*
 * CPUID runtime checks are only enabled if this is needed.
 * E.g. CPUID is disabled when a user compiles his
 * code using -march=native on a CPU with AVX512.
 */
#if defined(HAVE_CPUID)
  #if defined(__cplusplus)
    /* C++11 thread-safe singleton */
    static const int cpuid = get_cpuid();
  #else
    static int cpuid_ = -1;
    int cpuid = cpuid_;
    if (cpuid == -1)
    {
      cpuid = get_cpuid();

      #if defined(_MSC_VER)
        _InterlockedCompareExchange(&cpuid_, cpuid, -1);
      #else
        __sync_val_compare_and_swap(&cpuid_, -1, cpuid);
      #endif
    }
  #endif
#endif

#if defined(HAVE_AVX512)
  #if defined(__AVX512__) || \
     (defined(__AVX512F__) && defined(__AVX512VPOPCNTDQ__))
    /* For tiny arrays AVX512 is not worth it */
    if (i + 32 <= size)
  #else
    if ((cpuid & bit_AVX512_VPOPCNTDQ) &&
        i + 32 <= size)
  #endif
    {
      const uint64_t* ptr64 = (const uint64_t*)(ptr + i);
      cnt += popcnt_avx512(ptr64, (size - i) / 8);
      i = size - size % 8;
    }
#endif

#if defined(HAVE_AVX2)
  #if defined(__AVX2__)
    /* AVX2 requires arrays >= 512 bytes */
    if (i + 512 <= size)
  #else
    if ((cpuid & bit_AVX2) &&
        i + 512 <= size)
  #endif
    {
      const __m256i* ptr256 = (const __m256i*)(ptr + i);
      cnt += popcnt_avx2(ptr256, (size - i) / 32);
      i = size - size % 32;
    }
#endif

#if defined(HAVE_POPCNT)
  /* 
   * The user has compiled without -mpopcnt.
   * Unfortunately the MSVC compiler does not have
   * a POPCNT macro so we cannot get rid of the
   * runtime check for MSVC.
   */
  #if !defined(__POPCNT__)
    if (cpuid & bit_POPCNT)
  #endif
    {
      /* We use unaligned memory accesses here to improve performance */
      for (; i < size - size % 8; i += 8)
        cnt += popcnt64(*(const uint64_t*)(ptr + i));
      for (; i < size; i++)
        cnt += popcnt64(ptr[i]);

      return cnt;
    }
#endif

/*
 * This code is used for:
 * 1) Compiler does not support POPCNT.
 * 2) x86 CPU does not support POPCNT (cpuid != POPCNT).
 */
#if !defined(HAVE_POPCNT) || \
    !defined(__POPCNT__)
  /*
   * Pure integer popcount algorithm.
   * We use unaligned memory accesses here to improve performance.
   */
  for (; i < size - size % 8; i += 8)
    cnt += popcnt64_bitwise(*(const uint64_t*)(ptr + i));

  if (i < size)
  {
    uint64_t val = 0;
    size_t bytes = (size_t)(size - i);
    memcpy(&val, &ptr[i], bytes);
    cnt += popcnt64_bitwise(val);
  }

  return cnt;
#endif
}

#elif defined(__ARM_FEATURE_SVE) && \
      __has_include(<arm_sve.h>)

#include <arm_sve.h>

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i = 0;
  const uint64_t* ptr64 = (const uint64_t*) data;
  uint64_t size64 = size / sizeof(uint64_t);
  svbool_t pg = svwhilelt_b64(i, size64);
  svuint64_t vcnt = svdup_u64(0);

  do {
    svuint64_t vec = svld1_u64(pg, &ptr64[i]);
    vec = svcnt_u64_z(pg, vec);
    vcnt = svadd_u64_z(svptrue_b64(), vcnt, vec);
    i += svcntd();
    pg = svwhilelt_b64(i, size64);
  }
  while (svptest_any(svptrue_b64(), pg));

  uint64_t cnt = svaddv_u64(svptrue_b64(), vcnt);
  size %= sizeof(uint64_t);

  if (size > 0)
  {
    uint64_t val = 0;
    memcpy(&val, &ptr64[i], size);
    cnt += popcnt64(val);
  }

  return cnt;
}

#elif (defined(__ARM_NEON) || \
       defined(__aarch64__)) && \
      __has_include(<arm_neon.h>)

#include <arm_neon.h>

static inline uint64x2_t vpadalq(uint64x2_t sum, uint8x16_t t)
{
  return vpadalq_u32(sum, vpaddlq_u16(vpaddlq_u8(t)));
}

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i = 0;
  uint64_t cnt = 0;
  uint64_t chunk_size = 64;
  const uint8_t* ptr = (const uint8_t*) data;

  if (size >= chunk_size)
  {
    uint64_t iters = size / chunk_size;
    uint64x2_t sum = vcombine_u64(vcreate_u64(0), vcreate_u64(0));
    uint8x16_t zero = vcombine_u8(vcreate_u8(0), vcreate_u8(0));

    do
    {
      uint8x16_t t0 = zero;
      uint8x16_t t1 = zero;
      uint8x16_t t2 = zero;
      uint8x16_t t3 = zero;

      /*
       * After every 31 iterations we need to add the
       * temporary sums (t0, t1, t2, t3) to the total sum.
       * We must ensure that the temporary sums <= 255
       * and 31 * 8 bits = 248 which is OK.
       */
      uint64_t limit = (i + 31 < iters) ? i + 31 : iters;
  
      /* Each iteration processes 64 bytes */
      for (; i < limit; i++)
      {
        uint8x16x4_t input = vld4q_u8(ptr);
        ptr += chunk_size;

        t0 = vaddq_u8(t0, vcntq_u8(input.val[0]));
        t1 = vaddq_u8(t1, vcntq_u8(input.val[1]));
        t2 = vaddq_u8(t2, vcntq_u8(input.val[2]));
        t3 = vaddq_u8(t3, vcntq_u8(input.val[3]));
      }

      sum = vpadalq(sum, t0);
      sum = vpadalq(sum, t1);
      sum = vpadalq(sum, t2);
      sum = vpadalq(sum, t3);
    }
    while (i < iters);

    i = 0;
    size %= chunk_size;

    uint64_t tmp[2];
    vst1q_u64(tmp, sum);
    cnt += tmp[0];
    cnt += tmp[1];
  }

#if defined(__ARM_FEATURE_UNALIGNED)
  /* We use unaligned memory accesses here to improve performance */
  for (; i < size - size % 8; i += 8)
    cnt += popcnt64(*(const uint64_t*)(ptr + i));
#else
  if (i + 8 <= size)
  {
    /* Align memory to an 8 byte boundary */
    for (; (uintptr_t)(ptr + i) % 8; i++)
      cnt += popcnt64(ptr[i]);
    for (; i < size - size % 8; i += 8)
      cnt += popcnt64(*(const uint64_t*)(ptr + i));
  }
#endif

  if (i < size)
  {
    uint64_t val = 0;
    size_t bytes = (size_t)(size - i);
    memcpy(&val, &ptr[i], bytes);
    cnt += popcnt64(val);
  }

  return cnt;
}

/* all other CPUs */
#else

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
static inline uint64_t popcnt(const void* data, uint64_t size)
{
  uint64_t i = 0;
  uint64_t cnt = 0;
  const uint8_t* ptr = (const uint8_t*) data;

  if (size >= 8)
  {
    /*
     * Since we don't know whether this CPU architecture
     * supports unaligned memory accesses we align
     * memory to an 8 byte boundary.
     */
    for (; (uintptr_t)(ptr + i) % 8; i++)
      cnt += popcnt64(ptr[i]);
    for (; i < size - size % 8; i += 8)
      cnt += popcnt64(*(const uint64_t*)(ptr + i));
  }

  for (; i < size; i++)
    cnt += popcnt64(ptr[i]);

  return cnt;
}

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIBPOPCNT_H */
