libpopcnt
=========

[![Build Status](https://travis-ci.org/kimwalisch/libpopcnt.svg)](https://travis-ci.org/kimwalisch/libpopcnt)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/kimwalisch/libpopcnt?branch=master&svg=true)](https://ci.appveyor.com/project/kimwalisch/libpopcnt)
[![Github Releases](https://img.shields.io/github/release/kimwalisch/libpopcnt.svg)](https://github.com/kimwalisch/libpopcnt/releases)

```libpopcnt.h``` is a header only C/C++ library for counting the
number of 1 bits (bit population count) in an array as quickly as
possible using specialized CPU instructions e.g.
[POPCNT](https://en.wikipedia.org/wiki/SSE4#POPCNT_and_LZCNT),
[AVX2](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions).
```libpopcnt.h``` has been tested successfully using the GCC,
Clang and MSVC compilers.

The algorithms used in ```libpopcnt.h``` are described in the paper
[Faster Population Counts using AVX2 Instructions](https://arxiv.org/abs/1611.07612)
by Daniel Lemire, Nathan Kurz and Wojciech Mula (23 Nov 2016).

How it works
------------

```libpopcnt.h``` uses a combination of 3 different bit population
count algorithms based on the CPU architecture and the input array
size:

* For array sizes < 1 kilobyte an unrolled ```POPCNT``` algorithm
is used.
* For array sizes ≥ 1 kilobyte an ```AVX2``` algorithm is used.
* For CPUs without ```POPCNT``` instruction a portable 
integer algorithm is used.

The GitHub repository
[WojciechMula/sse-popcount](https://github.com/WojciechMula/sse-popcount/tree/master/results)
contains extensive benchmarks for the 3 algorithms used in
```libpopcnt.h```. The algorithms are named
```harley-seal```, ```avx2-harley-seal```, ```builtin-popcnt-unrolled```.

C/C++ API
---------

```C++
#include "libpopcnt.h"

/// Count the number of 1 bits in the data array.
/// @param data  An array
/// @param size  Size of data in bytes
uint64_t popcnt(const void* data, uint64_t size);
```

How to compile
--------------

Compilation does not require any special compiler flags (like
```-mpopcnt```, ```-mavx2```)! Also on x86 CPUs ```libpopcnt.h```
checks using ```cpuid``` if the CPU supports POPCNT/AVX2
before using it.


```bash
# How to compile using GCC
gcc -O2 program.c

# How to compile using Clang
clang -O2 program.c

# How to compile using Microsoft Visual C++
cl /O2 program.cpp
```

Speedup
-------

The benchmark below shows the speedup of the 3 algorithms
used in ```libpopcnt.h``` compared to a basic ```lookup-8```
popcount algorithm. ```libpopcnt.h``` automatically **picks**
the fastest algorithm for the given array size.

<table>
  <tr align="center">
    <td><b>procedure</b></td>
    <td><b>32 B</b></td>
    <td><b>64 B</b></td>
    <td><b>128 B</b></td>
    <td><b>256 B</b></td>
    <td><b>512 B</b></td>
    <td><b>1024 B</b></td>
    <td><b>2048 B</b></td>
    <td><b>4096 B</b></td>
  </tr>
  <tr>
    <td>lookup-8</td> 
    <td>1.00</td>
    <td>1.00</td>
    <td>1.00</td>
    <td>1.00</td>
    <td>1.00</td>
    <td>1.00</td>
    <td>1.00</td>
    <td>1.00</td>
  </tr>
  <tr>
    <td>harley-seal</td> 
    <td>1.27</td>
    <td>1.43</td>
    <td>2.41</td>
    <td>2.82</td>
    <td>3.12</td>
    <td>3.31</td>
    <td>3.41</td>
    <td>3.47</td>
  </tr>
  <tr>
    <td>builtin-popcnt-unrolled</td> 
    <td><b>4.75</b></td>
    <td><b>6.36</b></td>
    <td><b>8.58</b></td>
    <td><b>8.55</b></td>
    <td><b>6.72</b></td>
    <td>7.60</td>
    <td>7.88</td>
    <td>7.94</td>
  </tr>
  <tr>
    <td>avx2-harley-seal</td> 
    <td>1.15</td>
    <td>1.85</td>
    <td>3.22</td>
    <td>4.17</td>
    <td>8.46</td>
    <td><b>10.74</b></td>
    <td><b>12.52</b></td>
    <td><b>13.66</b></td>
  </tr>
</table>

This benchmark was run on an Intel Skylake i7-6700 CPU with GCC 5.3.

More benchmarks are available at
[WojciechMula/sse-popcount](https://github.com/WojciechMula/sse-popcount/tree/master/results).
