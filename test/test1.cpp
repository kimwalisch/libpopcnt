///
/// @file  test1.cpp
/// @brief Simple C++ test program for libpopcnt.h.
///        Generates an array with random data and computes the bit
///        population count using 2 different algorithms and checks
///        that the results match.
///
/// Usage: ./test1 [array bytes]
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
#include <cstdlib>
#include <stdint.h>

void test(std::vector<uint8_t>& data, int i)
{
  int size = data.size();
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
    std::exit(1);
  }
}

int main(int argc, char* argv[])
{
  int size = 100000;

  if (argc > 1)
    size = std::atoi(argv[1]);

  // init array with only 1 bits
  std::vector<uint8_t> data(size, 0xff);
  srand((unsigned) time(0));

  if (size > 0)
    test(data, 0);

  // generate array with random data
  for (int i = 0; i < size; i++)
    data[i] = (uint8_t) rand();

  for (int i = 1; i < size; i++)
    test(data, i);

  std::cout << "\rStatus: 100%" << std::endl;
  std::cout << "libpopcnt tested successfully!" << std::endl;

  return 0;
}
