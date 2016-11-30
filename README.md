libpopcnt
=========
[![Build Status](https://travis-ci.org/kimwalisch/libpopcnt.svg)](https://travis-ci.org/kimwalisch/libpopcnt)
[![GitHub license](https://img.shields.io/badge/license-BSD%202-blue.svg)](https://github.com/kimwalisch/libpopcnt/blob/master/LICENSE)

```libpopcnt.h``` is a C/C++ library for counting the number of 1 bits
(bit population count) in an array as quickly as possible using
specialized CPU instructions e.g. POPCNT, AVX2.

The algorithms used in ```libpopcnt.h``` are described in the paper
[Faster Population Counts using AVX2 Instructions](https://arxiv.org/abs/1611.07612)
by Daniel Lemire, Nathan Kurz and Wojciech Mula (23 Nov 2016).

C/C++ API
=========
```C++
#include "libpopcnt.h"

/// Count the number of 1 bits in the data array.
/// @param data  An array
/// @param size  Size of data in bytes
uint64_t popcnt(const void* data, uint64_t size);
```

How to use it
=============
At compile time you need to specify if your compiler supports the
```POPCNT``` & ```AVX2``` instructions. At runtime ```libpopcnt.h``` will then
choose the fastest popcount algorithm by checking (using ```cpuid```)
whether the current CPU supports ```POPCNT``` & ```AVX2```. Hence
```libpopcnt.h``` is fast and portable, it also runs on CPUs that do
not support the ```POPCNT``` & ```AVX2``` instructions.

```bash
# How to compile on x86 & x86_64 CPUs
gcc -mpopcnt -DHAVE_POPCNT -mavx2 -DHAVE_AVX2 program.c
```
