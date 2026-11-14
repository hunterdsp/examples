# Learning C by Example

[![CI](https://github.com/hunterdsp/examples/actions/workflows/c-by-example.yml/badge.svg)](https://github.com/hunterdsp/examples/actions/workflows/c-by-example.yml)
[![Under construction](https://img.shields.io/badge/status-under%20construction-orange)](#learning-c-by-example)
[![C99](https://img.shields.io/badge/C-99-informational?logo=c&logoColor=white)](https://en.cppreference.com/w/c/99)
[![Linux | macOS](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS-informational?logo=linux&logoColor=white)](../.github/workflows/c-by-example.yml)
[![clang-format](https://img.shields.io/badge/clang--format-GNU-blue)](.clang-format)

> [!WARNING]
> **UNDER CONSTRUCTION.**

**Deceptively simple. Extremely powerful.**

C has a small grammar and almost no runtime, which is why it is still around
and what a lot of (fast) code is written in. This introduction to the language
starts from an empty file and ends with a C library ready for distribution,
built one runnable example at a time. Along the way it covers some common
pitfalls, memory management, and how to build objects out of plain structs and
function pointers.

The examples lean toward performance algorithms and signal processing, but you
need not be an expert in either to follow along.

## How it works

The course is five *Parts*, each a series of *lessons*, and each lesson is a
*folder*: a README that walks through the idea, examples that build with the
one command at the top, and, where a lesson has a trap in it, a `broken-*.c`
that falls into it on purpose so you can see what the compiler or a sanitizer
says. Every example is built and checked by one command, on Linux and macOS,
using `make`, a program that runs build commands (lesson 08 explains it):

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

Following a lesson leaves its programs, and the `.i`, `.s` and `.o` files of
lesson 00, in the lesson's folder (and Part V's builds in `build/` folders).
Git ignores them, and any of them can be deleted at any time: the commands make
them again. `git clean -fdX .`, in a lesson's folder, deletes everything git
ignores there.

You need a C compiler and `make`, and for Part V, CMake, Doxygen, pkg-config
and Python 3. To install them, and everything later lessons use:
`jbx install-deps -g dev` (lesson 00).

## Contents

Keep the [quick reference](REFERENCE.md) open beside the lessons: `printf`
conversions, data types and their sizes, escape sequences, operator precedence,
and which header holds what.

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

**Part II · Memory you own**

- [09 Stack vs heap](09-stack-heap/): `malloc`, `calloc`, `free`, and memory
  that outlives its function
- [10 Ownership and lifecycle](10-ownership/): create and destroy, `realloc`,
  who frees what
- [11 Catching bugs](11-catching-bugs/): the sanitizers, `assert`, leaks, and
  what no tool sees
- [12 Ring buffers](12-ring-buffers/): a queue that never ends, power-of-two
  masks, a delay line
- [13 Allocation for real time](13-realtime/): allocate at setup, arenas, and a
  deadline

**Part III · Objects without classes**

- [14 Opaque types](14-opaque-types/): a struct nobody else can see inside, and
  why that is freedom
- [15 Function pointers](15-function-pointers/): tables of functions,
  callbacks, `void *`, `qsort`
- [16 Interfaces (vtables)](16-interfaces/): many kinds of processor, one loop
- [17 Composition and "inheritance"](17-composition/): a chain of chains, and a
  ramp that is a gain
- [18 API design](18-api-design/): prefixes, status codes, units, `const`, and
  versions

**Part IV · Performance and signal processing**

- [19 Measuring first](19-measuring/): timing honestly, and what the compiler
  already did
- [20 FIR filters](20-fir/): taps, a windowed-sinc low-pass, flexible array
  members
- [21 IIR filters](21-biquad/): the biquad, stability, and subnormal numbers
- [22 Oscillators](22-oscillator/): a 32-bit phase accumulator, and why not
  `float` time
- [23 Complex numbers](23-complex/): `<complex.h>`, phasors, mixing
- [24 The FFT](24-fft/): radix 2, plans, windows and leakage
- [25 Caches and memory layout](25-caches/): rows before columns, structs of
  arrays
- [26 Vectorisation](26-vectorisation/): why a float sum won't vectorise, and
  how to let it
- [27 Fixed point](27-fixed-point/): Q15, rounding, saturation, headroom
- [28 Concurrency](28-concurrency/): threads, races, false sharing, a blocking
  queue

**Part V · Shipping it**

Part V builds and ships one library, [`sig`](sig/): Part IV's oscillator,
biquad and FFT, with everything a library needs around them.

- [29 Library layout](29-library-layout/): public headers and private source, a
  static library by hand
- [30 CMake](30-cmake/): targets, usage requirements, options, build types
- [31 Testing](31-testing/): tests that check promises, CTest, and breaking the
  code to test the tests
- [32 Documentation](32-docs/): Doxygen, warnings as errors, examples that run
- [33 Bundled tools](33-tools/): `sig-tone | sig-filter | sig-fft`, raw samples
  through pipes
- [34 Distribution](34-distribution/): shared libraries, installing, pkg-config
  and `find_package`
- [35 Epilogue: the same library, from Python](35-python/): `ctypes`, and where
  next
