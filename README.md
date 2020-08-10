# libpopcnt

[![Build Status](https://travis-ci.org/kimwalisch/libpopcnt.svg)](https://travis-ci.org/kimwalisch/libpopcnt)
[![Build Status](https://ci.appveyor.com/api/projects/status/github/kimwalisch/libpopcnt?branch=master&svg=true)](https://ci.appveyor.com/project/kimwalisch/libpopcnt)
[![Github Releases](https://img.shields.io/github/release/kimwalisch/libpopcnt.svg)](https://github.com/kimwalisch/libpopcnt/releases)

```libpopcnt.h``` is a header-only C/C++ library for counting the
number of 1 bits (bit population count) in an array as quickly as
possible using specialized CPU instructions i.e.
[POPCNT](https://en.wikipedia.org/wiki/SSE4#POPCNT_and_LZCNT),
[AVX2](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions),
[AVX512](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions),
[NEON](https://en.wikipedia.org/wiki/ARM_architecture#Advanced_SIMD_.28NEON.29).
```libpopcnt.h``` has been tested successfully using the GCC,
Clang and MSVC compilers.

The algorithms used in ```libpopcnt.h``` are described in the paper
[Faster Population Counts using AVX2 Instructions](https://arxiv.org/abs/1611.07612)
by Daniel Lemire, Nathan Kurz and Wojciech Mula (23 Nov 2016).

## How it works

On x86 CPUs ```libpopcnt.h``` uses a combination of 4 different bit
population count algorithms:

* For array sizes < 512 bytes a ```POPCNT``` algorithm is used.
* For array sizes ≥ 512 bytes an ```AVX2``` algorithm is used.
* For array sizes ≥ 1024 bytes an ```AVX512``` algorithm is used.
* For CPUs without ```POPCNT``` instruction a portable 
integer algorithm is used.

Note that ```libpopcnt.h``` works on all CPUs, it checks at run-time
whether your CPU supports POPCNT, AVX2, AVX512 before using it
and it is also thread-safe.

## C/C++ API

```C
#include "libpopcnt.h"

/*
 * Count the number of 1 bits in the data array
 * @data: An array
 * @size: Size of data in bytes
 */
uint64_t popcnt(const void* data, uint64_t size);
```

## Speedup

This benchmark shows the speedup of the 4 popcount algorithms
used on x86 CPUs compared to the basic [lookup-8](https://github.com/WojciechMula/sse-popcount/blob/master/popcnt-lookup.cpp#L139)
popcount algorithm for different array sizes (in bytes).

<table>
  <tr align="center">
    <td><b>Algorithm</b></td>
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
    <td>bit-parallel-mul</td>
    <td>1.41</td>
    <td>1.54</td>
    <td>1.63</td>
    <td>1.78</td>
    <td>1.60</td>
    <td>1.62</td>
    <td>1.63</td>
    <td>1.64</td>
  </tr>
  <tr>
    <td>builtin-popcnt</td> 
    <td><b>4.75</b></td>
    <td><b>6.36</b></td>
    <td><b>8.58</b></td>
    <td><b>8.55</b></td>
    <td>6.72</td>
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
    <td><b>8.46</b></td>
    <td>10.74</td>
    <td>12.52</td>
    <td>13.66</td>
  </tr>
  <tr>
    <td>avx512-harley-seal</td> 
    <td>0.35</td>
    <td>1.49</td>
    <td>2.54</td>
    <td>3.83</td>
    <td>5.63</td>
    <td><b>15.12</b></td>
    <td><b>22.18</b></td>
    <td><b>25.60</b></td>
  </tr>
</table>

```libpopcnt.h``` automatically **picks** the fastest algorithm for
the given array size. This benchmark was run on an Intel Xeon
Platinum 8168 CPU with GCC 5.4.

## CPU architectures

```libpopcnt.h``` has hardware accelerated popcount algorithms for
the following CPU architectures:

<table>
  <tr>
    <td><b>x86</b></td>
    <td><code>POPCNT</code>, <code>AVX2</code>, <code>AVX512</code></td> 
  </tr>
  <tr>
    <td><b>x86-64</b></td>
    <td><code>POPCNT</code>, <code>AVX2</code>, <code>AVX512</code></td>
  </tr>
  <tr>
    <td><b>ARM</b></td>
    <td><code>NEON</code></td> 
  </tr>
  <tr>
    <td><b>PPC64</b></td>
    <td><code>POPCNTD</code></td>
  </tr>
</table>

For other CPU architectures a fast integer popcount algorithm is used.

## How to compile

```libpopcnt.h``` does not require any special compiler flags like ```-mavx2```!
In order to get the best performance we recommend however to compile with
optimizations enabled e.g. ```-O3``` or ```-O2```.

```bash
cc  -O3 program.c
c++ -O3 program.cpp
```

## Development

```bash
cmake .
make -j
make test
```

The above commands also build the ```benchmark``` program which is
useful for benchmarking ```libpopcnt.h```. Below is a
usage example run on an Intel Xeon Platinum 8168 CPU from 2017:

```bash
# Usage: ./benchmark [array bytes] [iters]
./benchmark
Iters: 10000000
Array size: 16.00 KB
Algorithm: AVX512
Status: 100%
Seconds: 1.59
103.4 GB/s
```

## Acknowledgments

The vectorized popcount algorithms used in ```libpopcnt.h``` have
originally been written by [Wojciech Muła](https://github.com/WojciechMula/sse-popcount),
I just made a convenient and portable C/C++ library using these algorithms.
