#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <stdint.h>

#include "libpopcnt.h"

int main(int argc, char** argv)
{
  srand(time(0));
  int max_size = 100000;

  if (argv[1])
    max_size = std::atoi(argv[1]);

  // Generate vectors with random data and compute the bit
  // population count using 2 different algorithms and
  // check that the results match
  //
  for (int size = 0; size < max_size; size++)
  {
    double percent = (100.0 * size) / max_size;

    std::cout << "\rStatus: " << (int) percent << "%" << std::flush;
    std::vector<uint8_t> data(size);

    for (int i = 0; i < size; i++)
      data[i] = (uint8_t) rand();

    uint64_t bits = popcnt(&data[0], size);
    uint64_t bits_verify = 0;

    for (int i = 0; i < size; i++)
      bits_verify += popcount64c(data[i]);

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
