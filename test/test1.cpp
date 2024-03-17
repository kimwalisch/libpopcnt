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

using namespace std;

/// Count 1 bits from &data[i] till &data[size].
/// @data: An array for testing
/// @i: Array start index
///
void test(vector<uint8_t>& data, size_t i)
{
  size_t size = data.size();

  uint64_t bits = popcnt(&data[i], size - i);
  uint64_t bits_verify = 0;

  for (; i < size; i++)
    bits_verify += popcnt64_bitwise(data[i]);

  if (bits != bits_verify)
  {
    cerr << endl;
    cerr << "libpopcnt test failed!" << endl;
    exit(1);
  }
}

int main(int argc, char* argv[])
{
  size_t size = 100000;

  if (argc > 1)
    size = atoi(argv[1]);

  // init array with only 1 bits
  vector<uint8_t> data(size, 0xff);

  if (!data.empty())
    test(data, 0);

  srand((unsigned) time(0));

  // generate array with random data
  for (size_t i = 0; i < size; i++)
    data[i] = (uint8_t) rand();

  for (size_t i = 0; i < size; i++)
  {
    test(data, i);
    double percent = (100.0 * i) / size;
    cout << "\rStatus: " << (int) percent << "%" << flush;
  }

  cout << "\rStatus: 100%" << endl;
  cout << "libpopcnt tested successfully!" << endl;

  return 0;
}
