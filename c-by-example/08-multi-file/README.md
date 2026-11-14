# 08 · The preprocessor and multi-file builds

Run this lesson's commands in `c-by-example/08-multi-file/`.

A level meter in three files: a header that promises (`dsp.h`), the code that
keeps the promise (`dsp.c`), and a program that uses it (`meter.c`).

The header, `dsp.h`, declares what `dsp.c` offers:

```bash
cat dsp.h
```

```c
/* dsp.h -- level measurements on a buffer of samples.
 *
 * The include guard below means the body is seen once per translation
 * unit, however many times it is #included (meter.c does it twice, on
 * purpose, to show this).
 */
#ifndef DSP_H
#define DSP_H

#include <stddef.h>

/* A function-like macro, written defensively: every use of the argument
   and the whole expansion are in parentheses (see meter.c for why). */
#define DSP_SQUARE(x) ((x) * (x))

/* Root mean square: the "average loudness" of a buffer. n must be > 0. */
float dsp_rms (const float *buf, size_t n);

/* Largest absolute sample value. n must be > 0. */
float dsp_peak (const float *buf, size_t n);

/* An amplitude in decibels relative to full scale (1.0 = 0 dBFS). */
float dsp_dbfs (float amplitude);

#endif /* DSP_H */
```

`dsp.c` keeps the promise:

```bash
cat dsp.c
```

```c
/* dsp.c -- the implementation of dsp.h. */
#include "dsp.h"

#include <math.h>

float
dsp_rms (const float *buf, size_t n)
{
  double sum = 0.0; /* accumulate in double: lesson 02 */
  for (size_t i = 0; i < n; i++)
    sum += DSP_SQUARE ((double)buf[i]);
  return (float)sqrt (sum / (double)n);
}

float
dsp_peak (const float *buf, size_t n)
{
  float peak = 0.0f;
  for (size_t i = 0; i < n; i++)
    {
      float a = fabsf (buf[i]);
      if (a > peak)
        peak = a;
    }
  return peak;
}

float
dsp_dbfs (float amplitude)
{
  return 20.0f * log10f (amplitude);
}
```

`meter.c` uses it:

```bash
cat meter.c
```

```c
/* meter.c -- a level meter, built from more than one file.
 *
 *   make && ./meter                       (see the Makefile)
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       meter.c dsp.c -o meter -lm        (the same, by hand)
 */
#include <stdio.h>

#include "dsp.h"
#include "dsp.h" /* twice, on purpose: the include guard makes it harmless */

/* The naive version of DSP_SQUARE, without the parentheses. */
#define BAD_SQUARE(x) x *x

int
main (void)
{
  /* A full-scale square wave, then the same at half amplitude. */
  float  loud[]  = { 1, -1, 1, -1, 1, -1, 1, -1 };
  float  quiet[] = { 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f };
  size_t n       = sizeof loud / sizeof loud[0];

  printf ("loud : rms %.3f, peak %.3f = %6.2f dBFS\n", dsp_rms (loud, n),
          dsp_peak (loud, n), dsp_dbfs (dsp_peak (loud, n)));
  printf ("quiet: rms %.3f, peak %.3f = %6.2f dBFS\n", dsp_rms (quiet, n),
          dsp_peak (quiet, n), dsp_dbfs (dsp_peak (quiet, n)));

  /* Macros are text substitution, done before the compiler sees the code.
     BAD_SQUARE(1 + 2) becomes 1 + 2 * 1 + 2, which is 5. */
  printf ("BAD_SQUARE(1 + 2) = %d, DSP_SQUARE(1 + 2) = %d\n",
          BAD_SQUARE (1 + 2), DSP_SQUARE (1 + 2));
  return 0;
}
```

Build all three with `make` (the [`Makefile`](Makefile) is below), and run the
meter:

```bash
make && ./meter
```

```
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c meter.c
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c dsp.c
cc -fsanitize=address,undefined -o meter meter.o dsp.o -lm
loud : rms 1.000, peak 1.000 =   0.00 dBFS
quiet: rms 0.500, peak 0.500 =  -6.02 dBFS
BAD_SQUARE(1 + 2) = 5, DSP_SQUARE(1 + 2) = 9
```

Half amplitude reads −6 dB, as lesson 02 said it would.

## Translation units

The compiler never sees your *project*. It sees one `.c` file at a time, after
the preprocessor has pasted in its headers: a **translation unit**. `meter.c`
and `dsp.c` are compiled separately into `meter.o` and `dsp.o`, and only the
linker sees both. Two consequences:

- **Headers are how translation units agree.** Each `.c` that uses `dsp_rms`
  includes `dsp.h`, so each compiles its calls against the same declaration.
- **Change one file, recompile one file.** In a library of hundreds of files,
  that is the difference between a one-second and a ten-minute build.

## `make`

The [`Makefile`](Makefile) says what each file is built *from*:

```make
meter: meter.o dsp.o          # the program needs both objects
meter.o: meter.c dsp.h        # each object needs its source and the header
dsp.o: dsp.c dsp.h
```

`make` compares timestamps and runs only the recipes whose inputs changed. Edit
`dsp.c` and only `dsp.o` and `meter` rebuild; touch `dsp.h` and both objects
do. Recipe lines must start with a **tab**, not spaces.

## The preprocessor

- **Include guards.** `#ifndef DSP_H / #define DSP_H / … / #endif` makes a
  header's body appear once per translation unit, however often it's included.
  `meter.c` includes `dsp.h` twice to prove it. In a real project, headers
  include other headers, and without guards the same declaration would arrive
  twice.
- **Macros are text substitution**, done before compiling. That makes them fast
  and dangerous. `BAD_SQUARE(1 + 2)` expands to `1 + 2 * 1 + 2`, which is 5.
  `DSP_SQUARE` wraps every use of the argument, and the whole result, in
  parentheses: `((1 + 2) * (1 + 2))`, which is 9.

## Pitfalls

- **Linker errors.** [`broken-link.c`](broken-link.c) declares `dsp_rms` and
  calls it, but nothing in the build defines it. The compiler is satisfied (the
  declaration told it the shape); the linker is not:

  ```
  undefined reference to `dsp_rms'                     (Linux, GNU ld)
  Undefined symbols for architecture arm64: "_dsp_rms"  (macOS)
  ```

  Read it as "a `.c` file or a library is missing from the link line". Here,
  `dsp.c`.

- **Macro arguments evaluated twice.** `DSP_SQUARE(i++)` expands to
  `((i++) * (i++))`, which increments `i` twice and is undefined behaviour.
  Prefer a `static inline` function when you can: it has none of these
  problems, and costs nothing (Part III uses them).

- **Defining things in headers.** A header *declares*. A function or variable
  *defined* in a header is defined again in every `.c` that includes it, and
  the linker reports a duplicate. (`static inline` functions are the
  exception.)

- **Spaces instead of a tab in a Makefile**: `missing separator. Stop.`

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Run `make`, then `make` again. Then `touch dsp.c` and `make`. Which commands
   ran each time?

   <details><summary>Output and answer</summary>

   This one uses the lesson's own Makefile. `make clean` first, so the first
   `make` starts from nothing:

   ```bash
   make clean
   make
   ```

   ```
   rm -f meter meter.o dsp.o
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c meter.c
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c dsp.c
   cc -fsanitize=address,undefined -o meter meter.o dsp.o -lm
   ```

   It compiles both files and links. Again:

   ```bash
   make
   ```

   ```
   make: 'meter' is up to date.
   ```

   Nothing to do: every target is newer than what it is built from. Now
   `touch dsp.c`, which only updates the file's timestamp:

   ```bash
   sleep 1       # some makes (macOS's) only see whole seconds
   touch dsp.c
   make
   ```

   ```
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c dsp.c
   cc -fsanitize=address,undefined -o meter meter.o dsp.o -lm
   ```

   `make` recompiles `dsp.c` alone and relinks; `meter.o` is still up to date.
   That is the whole of `make`: compare timestamps, rebuild what is out of
   date, in the right order. (The `sleep 1` is for older versions of `make`,
   such as the one macOS ships, which compare timestamps to the whole second:
   touched in the same second it was built, `dsp.c` would look no newer than
   `dsp.o`. Typing by hand, you will never be that quick.)

   </details>

2. Add `float dsp_crest_db(const float *buf, size_t n)` (peak over rms, in dB)
   to `dsp.h` and `dsp.c`. A sine wave's crest factor is 3.01 dB; what is a
   square wave's?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-crest/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined crest.c dsp.c -o crest -lm
   ./crest
   ```

   ```
   sine  : crest 3.01 dB
   square: crest 0.00 dB
   ```

   0 dB: a square wave sits at its peak all the time, so its peak and rms are
   the same. A sine spends most of its time below its peak, so its rms is lower
   by √2, which is 3.01 dB. [`dsp_crest_db`](exercises/ex2-crest/dsp.c) is one
   line, built from the three functions already there. Crest factor says how
   much headroom a signal needs above its average level.

   </details>

3. Rewrite `DSP_SQUARE` as a `static inline float dsp_square(float x)` in
   `dsp.h`, and check the output is unchanged.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-inline/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined meter.c dsp.c -o meter -lm
   ./meter
   ```

   ```
   loud : rms 1.000, peak 1.000 =   0.00 dBFS
   quiet: rms 0.500, peak 0.500 =  -6.02 dBFS
   BAD_SQUARE(1 + 2) = 5, dsp_square(1 + 2) = 9
   ```

   The same numbers, and `dsp_square(1 + 2)` is 9 with no parentheses needed: a
   function's argument is a value, evaluated once, not text pasted in. It is
   `static inline` *in the header*, so each `.c` that includes it gets its own
   copy, which the compiler folds into the caller, as fast as the macro, and
   the linker sees no duplicates. One thing did change: the macro squared a
   `double` in `dsp_rms`, and `dsp_square` takes a `float`. The output is the
   same here, but a type is a decision a macro never made for you.

   </details>
