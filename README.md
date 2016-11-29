libpopcnt
=========
```libpopcnt.h is``` a header only C/C++ library for counting the
number of 1 bits (bit population count) in an array as quickly
as possible using specialized CPU instructions e.g. POPCNT,
AVX2.

The algorithms used in libpopcnt are described in the paper
[Faster Population Counts using AVX2 Instructions](https://arxiv.org/abs/1611.07612)
by Daniel Lemire, Nathan Kurz and Wojciech Mula (23 Nov 2016).
