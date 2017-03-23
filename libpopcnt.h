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

#define HAVE__attribute__target \
    (defined(__GNUC__) && \
            (__GNUC__ > 4 || \
            (__GNUC__ == 4 && __GNUC_MINOR__ >= 9))) || \
    (defined(__clang__) && \
            (__clang_major__ > 3 || \
            (__clang_major__ == 3 && __clang_minor__ >= 7)))

// non x64 CPUs or
// compiler does not support __attribute__((target))
#if !defined(HAVE__attribute__target) || \
    (!defined(__x86_64__) && \
     !defined(_M_X64))

static inline uint64_t popcnt64_unrolled(const uint64_t* data, uint64_t size)
{
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
}

/// Align memory to 8 bytes boundary
static inline void align8(const uint8_t*& data, uint64_t* size, uint64_t* total)
{
  for (; *size > 0 && (uintptr_t) data % 8 != 0; data++)
  {
    *total += popcnt64(*data);
    *size -= 1;
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

  total += popcnt64_unrolled((const uint64_t*) data8, size / 8);
  data8 += size - size % 8;
  size = size % 8;

  for (uint64_t i = 0; i < size; i++)
    total += popcnt64(data8[i]);

  return total;
}

#endif

#endif /* LIBPOPCNT_H */
