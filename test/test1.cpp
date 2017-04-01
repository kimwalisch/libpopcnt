///
/// @file  test.cpp
/// @brief Simple test program for libpopcnt.h.
///        Generates arrays with random data and computes the bit
///        population count using 2 different algorithms and checks
///        that the results match.
///
/// Usage: ./test [max size]
///
/// Copyright (C) 2017 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the LICENSE
/// file in the top level directory.
///

#include <libpopcnt.h>

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <stdint.h>

int main(int argc, char* argv[])
{
  int max = 100000;
  srand((unsigned) time(0));

  if (argc > 1)
    max = std::atoi(argv[1]);

  for (int size = 1; size < max; size++)
  {
    double percent = (100.0 * size) / max;

    std::cout << "\rStatus: " << (int) percent << "%" << std::flush;
    std::vector<uint8_t> data(size);

    for (int i = 0; i < size; i++)
      data[i] = (uint8_t) rand();

    uint64_t bits = popcnt(&data[0], size);
    uint64_t bits_verify = 0;

    for (int i = 0; i < size; i++)
      bits_verify += popcount64(data[i]);

    if (bits != bits_verify)
    {
      std::cerr << std::endl;
      std::cerr << "libpopcnt test failed!" << std::endl;
      return 1;
    }
  }

  std::cout << "\rStatus: 100%" << std::endl;
  std::cout << "libpopcnt tested successfully!" << std::endl;

  return 0;
}
