# sig

A small signal-processing library in C99: a sine oscillator, low-pass and
high-pass filters, and an amplitude spectrum, with three command-line tools
that pipe into each other. It is the library Part V of
[Learning C by Example](https://github.com/hunterdsp/examples/tree/main/c-by-example)
builds and ships.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Use

```c
#include <sig/sig.h>

#include <stdio.h>

int
main (void)
{
  sig_osc *osc;
  if (sig_osc_create (440.0, 48000.0, &osc) != SIG_OK)
    return 1;
  float buf[4];
  sig_osc_process (osc, buf, 4);
  sig_osc_destroy (osc);
  printf ("sig %s: %.4f %.4f %.4f %.4f\n", sig_version (), buf[0], buf[1],
          buf[2], buf[3]);
  return 0;
}
```

Every function that can fail returns a `sig_status`, and `sig_strerror` says
what it means. The headers in [`include/sig/`](include/sig/) document every
function, and `cmake --build build --target docs` turns them into HTML, if
Doxygen is installed.

## Tools

Samples travel between the tools as raw `float`s at 48000 per second:

```bash
build/sig-tone 1 750 6000 | build/sig-filter lowpass 1500 | build/sig-fft
```

```
  750.00 Hz  0.485
 6000.00 Hz  0.028
```

| tool                                   | does                                           |
| -------------------------------------- | ---------------------------------------------- |
| `sig-tone SECONDS FREQ_HZ...`          | writes sines of amplitude 0.5                  |
| `sig-filter lowpass\|highpass FREQ_HZ` | filters with a Butterworth biquad              |
| `sig-fft`                              | lists the frequencies in the last 4096 samples |

## Layout

| path                  | holds                                      |
| --------------------- | ------------------------------------------ |
| `include/sig/`        | the public headers: the API                |
| `src/`                | the implementation, and its private header |
| `tests/`              | one test program per part, run by CTest    |
| `tools/`              | the command-line tools                     |
| `docs/`               | Doxygen's settings                         |
| `cmake/`, `sig.pc.in` | how other projects find an installed sig   |
| `python/`             | a Python wrapper, using `ctypes`           |

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every C library function sig's code calls, linked to its definition.

| function                                                       | header        | what it does                                                        |
| -------------------------------------------------------------- | ------------- | ------------------------------------------------------------------- |
| [`UINT32_C`](https://en.cppreference.com/c/types/integer)      | `<stdint.h>`  | a constant of at least `uint32_t`'s range: `UINT32_C (1)` (a macro) |
| [`cabs`](https://en.cppreference.com/c/numeric/complex/cabs)   | `<complex.h>` | size (magnitude) of a `double complex`                              |
| [`cexp`](https://en.cppreference.com/c/numeric/complex/cexp)   | `<complex.h>` | *e* raised to a complex power: `cexp (I * t)` is a phasor           |
| [`cos`](https://en.cppreference.com/c/numeric/math/cos)        | `<math.h>`    | cosine of an angle in radians                                       |
| [`exit`](https://en.cppreference.com/c/program/exit)           | `<stdlib.h>`  | end the program, with a status                                      |
| [`fabs`](https://en.cppreference.com/c/numeric/math/fabs)      | `<math.h>`    | absolute value of a `double`                                        |
| [`ferror`](https://en.cppreference.com/c/io/ferror)            | `<stdio.h>`   | did a read or write on this stream fail?                            |
| [`fprintf`](https://en.cppreference.com/c/io/fprintf)          | `<stdio.h>`   | `printf` to a stream, such as `stderr`                              |
| [`fread`](https://en.cppreference.com/c/io/fread)              | `<stdio.h>`   | read raw bytes from a file                                          |
| [`free`](https://en.cppreference.com/c/memory/free)            | `<stdlib.h>`  | give back memory from `malloc`, `calloc` or `realloc`               |
| [`fwrite`](https://en.cppreference.com/c/io/fwrite)            | `<stdio.h>`   | write raw bytes to a file                                           |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)        | `<stdlib.h>`  | allocate memory, contents unset                                     |
| [`memmove`](https://en.cppreference.com/c/string/byte/memmove) | `<string.h>`  | copy bytes, where the two regions may overlap                       |
| [`perror`](https://en.cppreference.com/c/io/perror)            | `<stdio.h>`   | print a message, and why the last call failed                       |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)        | `<math.h>`    | sine of an angle in radians                                         |
| [`snprintf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`   | `printf` into a buffer, never past its size                         |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp)   | `<string.h>`  | compare two strings                                                 |
| [`strtod`](https://en.cppreference.com/c/string/byte/strtof)   | `<stdlib.h>`  | read a `double` from a string                                       |

<!-- /libfuncs -->
