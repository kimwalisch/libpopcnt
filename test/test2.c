/*
 * Simple C test program for libpopcnt.h.
 * Generates an array with random data and computes the bit population
 * count using 2 different algorithms and checks that the
 * results match.
 *
 * Usage: ./test2
 *
 * Copyright (C) 2017 Kim Walisch, <kim.walisch@gmail.com>
 *
 * This file is distributed under the BSD License. See the LICENSE
 * file in the top level directory.
 */

#include <libpopcnt.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

int main()
{
  int i, j;
  int size = 50000;

  uint64_t bits;
  uint64_t bits_verify;
  uint8_t* data = (uint8_t*) malloc(size);

  srand((unsigned) time(0));

  for (i = 0; i < size; i++)
    data[i] = (uint8_t) rand();

  for (i = 0; i < size; i++)
  {
    bits = popcnt(&data[i], size - i);
    bits_verify = 0;

    for (j = i; j < size; j++)
      bits_verify += popcount64(data[j]);

    if (bits != bits_verify)
    {
      printf("\nlibpopcnt test failed!\n");
      return 1;
    }
  }

  free(data);

  printf("\rStatus: 100%%\n");
  printf("libpopcnt tested successfully!\n");

  return 0;
}
