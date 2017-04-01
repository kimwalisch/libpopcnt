///
/// @file  test1.cpp
/// @brief Simple C++ test program for libpopcnt.h.
///        Generates an array with random data and computes the bit
///        population count using 2 different algorithms and checks
///        that the results match.
///
/// Usage: ./test1
///
/// Copyright (C) 2017 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the LICENSE
/// file in the top level directory.
///

#include <libpopcnt.h>

#include <iostream>
#include <vector>
#include <ctime>
#include <stdint.h>

int main()
{
  int size = 100000;
  std::vector<uint8_t> data(size);
  srand((unsigned) time(0));

  for (int i = 0; i < size; i++)
    data[i] = (uint8_t) rand();

  for (int i = 0; i < size; i++)
  {
    double percent = (100.0 * i) / size;
    std::cout << "\rStatus: " << (int) percent << "%" << std::flush;

    uint64_t bits = popcnt(&data[i], size - i);
    uint64_t bits_verify = 0;

    for (int j = i; j < size; j++)
      bits_verify += popcount64(data[j]);

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
