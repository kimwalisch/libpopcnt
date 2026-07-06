# libpopcnt

[![Build status](https://github.com/kimwalisch/libpopcnt/actions/workflows/ci.yml/badge.svg)](https://github.com/kimwalisch/libpopcnt/actions/workflows/ci.yml)
[![benchmark](https://github.com/kimwalisch/libpopcnt/actions/workflows/benchmark.yml/badge.svg)](https://github.com/kimwalisch/libpopcnt/actions/workflows/benchmark.yml)
[![Github Releases](https://img.shields.io/github/release/kimwalisch/libpopcnt.svg)](https://github.com/kimwalisch/libpopcnt/releases)

```libpopcnt.h``` is a header-only C/C++ library for counting the
number of 1 bits (bit population count) in an array as quickly as
possible using specialized CPU instructions i.e.
[POPCNT](https://en.wikipedia.org/wiki/SSE4#POPCNT_and_LZCNT),
[AVX2](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions),
[AVX512](https://en.wikipedia.org/wiki/Advanced_Vector_Extensions),
[NEON](https://en.wikipedia.org/wiki/ARM_architecture_family#Advanced_SIMD_(Neon)),
[SVE](https://en.wikipedia.org/wiki/AArch64#Scalable_Vector_Extension_(SVE)).
```libpopcnt.h``` has been tested successfully using the GCC,
Clang and MSVC compilers.

A Rust port is available as the
[```simd-popcnt```](https://github.com/kimwalisch/simd-popcnt) crate.

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

## How to compile

```libpopcnt.h``` does not require any special compiler flags like ```-mavx2```!
To get the best performance we only recommend to compile with
optimizations enabled e.g. ```-O3``` or ```-O2```.

```bash
cc  -O3 program.c
c++ -O3 program.cpp
```

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
    <td><code>NEON</code>, <code>SVE</code></td> 
  </tr>
  <tr>
    <td><b>PPC64</b></td>
    <td><code>POPCNTD</code></td>
  </tr>
</table>

For other CPU architectures a fast integer popcount algorithm is used.

## How it works

On x86 CPUs, ```libpopcnt.h``` first queries your CPU's supported
instruction sets using the ```CPUID``` instruction (this is done only once).
Then ```libpopcnt.h``` chooses the fastest bit population count algorithm
supported by your CPU:

* If the CPU supports ```AVX512``` the ```AVX512 VPOPCNT``` algorithm is used.
* Else if the CPU supports ```AVX2``` the ```AVX2 Harley Seal``` algorithm is used.
* Else if the CPU supports ```POPCNT``` the ```POPCNT``` algorithm is used.
* For CPUs without ```POPCNT``` instruction a portable integer algorithm is used.

Note that ```libpopcnt.h``` works on all CPUs (x86, ARM, PPC, WebAssembly, ...).
It is portable by default and hardware acceleration is only enabled if the CPU
supports it. ```libpopcnt.h``` it is also thread-safe.

We take performance seriously, if you compile using e.g. ```-march=native```
on an x86 CPU with AVX512 support then all runtime ```CPUID``` checks are removed!

## ARM SVE (Scalable Vector Extension)

ARM SVE is a vector instruction set for ARM CPUs that was first released in
2020. Unlike ARM NEON's fixed 128-bit vectors, ARM SVE's vector length is
implementation-defined (128 up to 2048 bits). libpopcnt's ARM SVE popcount
algorithm is faster than its ARM NEON algorithm, especially on CPUs whose SVE
vector width is larger than NEON's 128 bits.

On Linux and Windows libpopcnt automatically detects at runtime whether the CPU
supports ARM SVE and, if so, dispatches to the ARM SVE popcount algorithm (much
like the ```CPUID``` checks used on x86 CPUs). Otherwise it transparently falls
back to the portable ARM NEON popcount algorithm. This works out of the box, no
special compiler flags are required.

## Development

```bash
cmake .
make -j
make test
```

The above commands also build the ```benchmark``` program which is
useful for benchmarking ```libpopcnt.h```. Below is a
usage example run on an AMD EPYC 9R14 CPU from 2023:

```bash
# Usage: ./benchmark [array bytes] [iters]
./benchmark
Iters: 10000000
Array size: 16.00 KB
Algorithm: AVX512
Status: 100%
Seconds: 1.23
133.5 GB/s
```

## Acknowledgments

Some of the algorithms used in ```libpopcnt.h``` are described in the paper
[Faster Population Counts using AVX2 Instructions](https://arxiv.org/abs/1611.07612)
by Daniel Lemire, Nathan Kurz and Wojciech Mula (23 Nov 2016). The AVX2 Harley Seal
popcount algorithm used in ```libpopcnt.h``` has been copied from Wojciech Muła's
[sse-popcount](https://github.com/WojciechMula/sse-popcount) GitHub repo.
