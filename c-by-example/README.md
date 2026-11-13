# Learning C by Example

[![CI](https://github.com/hunterdsp/examples/actions/workflows/c-by-example.yml/badge.svg)](https://github.com/hunterdsp/examples/actions/workflows/c-by-example.yml)
[![Under construction](https://img.shields.io/badge/status-under%20construction-orange)](#learning-c-by-example)
[![C99](https://img.shields.io/badge/C-99-informational?logo=c&logoColor=white)](https://en.cppreference.com/w/c/99)
[![Linux | macOS](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS-informational?logo=linux&logoColor=white)](../.github/workflows/c-by-example.yml)
[![clang-format](https://img.shields.io/badge/clang--format-GNU-blue)](.clang-format)

> [!WARNING]
> **UNDER CONSTRUCTION.** Parts 0 and I (lessons 00–08) are written and tested;
> Parts II–V are still to come, and lessons may change as they do.

**Deceptively simple. Extremely powerful.**

C has a small grammar and almost no runtime, which is exactly why it is still
what fast code is written in. This course starts from an empty file and ends
with a C library ready for distribution, built one runnable example at a time.
Along the way it covers the pitfalls that bite, memory you manage yourself, and
how to build objects out of plain structs and function pointers.

The examples lean toward performance algorithms and signal processing, but you
need not be an expert in either to follow along.

## How it works

Each lesson is a folder: a README that walks through the idea, examples that
build with the one command at the top, and, where a lesson has a trap in it, a
`broken-*.c` that falls into it on purpose so you can see what the compiler or
a sanitizer says. Every example is built and checked by one command, on Linux
and macOS:

```bash
cd c-by-example         # from the repo root, like every command here
make test               # every lesson, with cc
make test CC=clang      # and again with clang
```

`make test` checks that each example prints exactly what its lesson says, and
that each broken one still fails, and for the reason given. It runs the
commands in every README, exercise answers included, and checks they print what
the README shows.

A lesson's commands run in its folder: `cd c-by-example/01-hello` to start
lesson 01, and so on. Each lesson ends with exercises. Every answer, folded
under the exercise, names the folder in `exercises/` it runs in, and shows the
commands to run and what they print.

You need a C compiler and `make`. To install them, and everything later lessons
use: `jbx install-deps -g dev` (lesson 00).

## Contents

**Part 0 · Setup**

- [00 Tools](00-tools/): what `cc` actually does, stage by stage

**Part I · The language, small and sharp**

- [01 Hello](01-hello/): the build command for the whole course, and exit
  status
- [02 Numbers](02-numbers/): integer sizes, promotion, float precision,
  decibels
- [03 Functions](03-functions/): a sine table, headers, pass by value, `static`
- [04 Arrays and loops](04-arrays/): a moving average, off-by-one, `sizeof`
- [05 Pointers](05-pointers/): addresses, in-place processing, `const`, `NULL`
- [06 Strings and files](06-strings-files/): raw samples on disk, `snprintf`,
  endianness
- [07 Structs](07-structs/): a biquad filter's state, designated initializers,
  padding
- [08 Multi-file builds](08-multi-file/): translation units, `make`, macros,
  linker errors

**Part II · Memory you own** *(coming next)*

09 Stack vs heap · 10 Ownership and lifecycle · 11 Catching bugs · 12 Ring
buffers · 13 Allocation for real time

**Part III · Objects without classes**

14 Opaque types · 15 Function pointers · 16 Interfaces (vtables) · 17
Composition and "inheritance" · 18 API design

**Part IV · Performance and signal processing**

19 Measuring first · 20 FIR filter · 21 IIR biquad · 22 Oscillator · 23 Complex
numbers · 24 FFT · 25 Caches and layout · 26 Vectorisation · 27 Fixed point ·
28 Concurrency

**Part V · Shipping it**

29 Library layout · 30 CMake · 31 Testing · 32 Docs · 33 Bundled tools
(`sig-tone | sig-filter | sig-fft`) · 34 Distribution · 35 Epilogue: the same
library, from Python
