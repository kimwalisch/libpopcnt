#ifndef POPCNT_H
#define POPCNT_H

#include <stdint.h>

#if defined(_MSC_VER) && defined(_WIN64)
#define HAVE_POPCNT

#include <nmmintrin.h>

inline uint64_t popcnt_u64(uint64_t x)
{
  return _mm_popcnt_u64(x);
}

#elif defined(_MSC_VER) && defined(_WIN32)
#define HAVE_POPCNT

#include <nmmintrin.h>

inline uint64_t popcnt_u64(uint64_t x)
{
  return _mm_popcnt_u32((uint32_t) x) + 
         _mm_popcnt_u32((uint32_t)(x >> 32));
}

#elif defined(__i386__) && \
      defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__> 1)

#define HAVE_POPCNT

inline uint64_t popcnt_u64(uint64_t x)
{
  return __builtin_popcount((uint32_t) x) +
         __builtin_popcount((uint32_t)(x >> 32));
}

#elif defined(__GNUC__) && \
             (__GNUC__ > 4 || \
             (__GNUC__ == 4 && __GNUC_MINOR__> 1)

#define HAVE_POPCNT

inline uint64_t popcnt_u64(uint64_t x)
{
  return __builtin_popcountll(x);
}

#else

/// This uses fewer arithmetic operations than any other known  
/// implementation on machines with fast multiplication.
/// It uses 12 arithmetic operations, one of which is a multiply.
/// http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
///
inline uint64_t popcnt_u64(uint64_t x)
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
uint64_t popcnt_array_u64(const uint64_t* data, const uint64_t size)
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

    total += popcnt_u64(sixteens);
  }

  total *= 16;
  total += 8 * popcnt_u64(eights);
  total += 4 * popcnt_u64(fours);
  total += 2 * popcnt_u64(twos);
  total += 1 * popcnt_u64(ones);

  for(; i < size; i++)
    total += popcnt_u64(data[i]);

  return total;
}

#endif

#if defined(HAVE_POPCNT)

/// Count the number of 1 bits in an array using the POPCNT
/// instruction. On x86_64 CPUs this requires SSE4.2.
///
inline uint64_t popcnt_array_u64(const uint64_t* array, uint64_t size)
{
  uint64_t sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;
  uint64_t limit = size - size % 4;
  uint64_t i = 0;

  for (; i < limit; i += 4)
  {
    sum0 += popcnt_u64(array[i+0]);
    sum1 += popcnt_u64(array[i+1]);
    sum2 += popcnt_u64(array[i+2]);
    sum3 += popcnt_u64(array[i+3]);
  }

  uint64_t total = sum0 + sum1 + sum2 + sum3;

  for (; i < size; i++)
    total += popcnt_u64(array[i]);

  return total;
}

#endif

#endif /* POPCNT_H */
