# 18 · API design

Run this lesson's commands in `c-by-example/18-api-design/`.

A library's header is a promise. Once programs are written against it, every
name, every argument order and every "returns `NULL` when…" is something they
depend on, and changing it breaks them. The code behind the header can be
rewritten at will (lesson 14); the header is much harder to take back.

So the header deserves the most thought, and this lesson is a checklist for it,
applied to a small library called `sig`, the kind Part V builds. It has one
filter, lesson 14's low-pass, but every decision a bigger library would make is
made here, and written down at the top of the header:

```bash
cat sig.h
```

```c
/* sig.h -- the public API of a tiny signal library.
 *
 * Everything here is a decision a caller will live with, so each one is
 * written down:
 *
 *   - Every public name starts with sig_ (or SIG_): C has one namespace.
 *   - Objects are opaque, made by _create and freed by _destroy.
 *   - Anything that can fail returns a sig_status; results come back
 *     through an out-parameter. sig_strerror says what went wrong.
 *   - Parameters are in the caller's units (Hz), not the library's.
 *   - Inputs are const; nothing in _process allocates, prints or blocks.
 *   - No global state: separate objects may be used from separate threads.
 */
#ifndef SIG_H
#define SIG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{ /* C++ callers: these are C functions, with C names */
#endif

/* The version this header describes. sig_version() reports the version
   of the library actually linked, which may differ. */
#define SIG_VERSION_MAJOR 1
#define SIG_VERSION_MINOR 0
#define SIG_VERSION_PATCH 0

  typedef enum
  {
    SIG_OK     = 0,
    SIG_EINVAL = -1, /* an argument is out of range */
    SIG_ENOMEM = -2  /* out of memory */
  } sig_status;

  /* A short, constant description of s: never NULL, never to be freed. */
  const char *sig_strerror (sig_status s);

  /* The linked library's version, as "MAJOR.MINOR.PATCH". */
  const char *sig_version (void);

  /* A one-pole low-pass filter (lesson 14), set by its cutoff frequency. */
  typedef struct sig_lowpass sig_lowpass;

  /* Make a filter for a signal sampled at rate_hz, passing frequencies
     below cutoff_hz. Needs 0 < cutoff_hz < rate_hz / 2. On SIG_OK, *out is
     the new filter, owned by the caller; on any error *out is NULL. */
  sig_status sig_lowpass_create (double cutoff_hz, double rate_hz,
                                 sig_lowpass **out);

  /* Free f. Accepts NULL. */
  void sig_lowpass_destroy (sig_lowpass *f);

  /* Filter n samples from in into out. in and out may be the same buffer
     (in place), but must not otherwise overlap. Never fails. */
  void sig_lowpass_process (sig_lowpass *f, const float *in, float *out,
                            size_t n);

  /* Forget the past, as if just created. */
  void sig_lowpass_reset (sig_lowpass *f);

#ifdef __cplusplus
}
#endif

#endif /* SIG_H */
```

Behind it, `sig.c`:

```bash
cat sig.c
```

```c
/* sig.c -- the implementation behind sig.h. */
#include "sig.h"

#include <math.h>
#include <stdlib.h>

/* Strings are built from the numbers, so the two can't disagree. */
#define SIG_STR_(x) #x
#define SIG_STR(x) SIG_STR_ (x)

const char *
sig_version (void)
{
  /* Adjacent string literals are joined: "1" "." "0" is "1.0". */
  return SIG_STR (SIG_VERSION_MAJOR)   /* "1" */
      "." SIG_STR (SIG_VERSION_MINOR)  /* ".0" */
      "." SIG_STR (SIG_VERSION_PATCH); /* ".0" */
}

const char *
sig_strerror (sig_status s)
{
  switch (s)
    {
    case SIG_OK:
      return "success";
    case SIG_EINVAL:
      return "argument out of range";
    case SIG_ENOMEM:
      return "out of memory";
    }
  return "unknown status"; /* a value no sig_ function returns */
}

struct sig_lowpass
{
  float a; /* smoothing coefficient, from the cutoff */
  float y; /* the last output */
};

sig_status
sig_lowpass_create (double cutoff_hz, double rate_hz, sig_lowpass **out)
{
  *out = NULL;
  if (!(rate_hz > 0.0 && cutoff_hz > 0.0 && cutoff_hz < rate_hz / 2.0))
    return SIG_EINVAL;
  sig_lowpass *f = malloc (sizeof *f);
  if (f == NULL)
    return SIG_ENOMEM;
  /* The coefficient that puts the filter's corner at cutoff_hz. The
     caller thinks in Hz; converting is the library's job, done once. */
  const double pi = 3.14159265358979323846;
  f->a            = (float)(1.0 - exp (-2.0 * pi * cutoff_hz / rate_hz));
  f->y            = 0.0f;
  *out            = f;
  return SIG_OK;
}

void
sig_lowpass_destroy (sig_lowpass *f)
{
  free (f);
}

void
sig_lowpass_process (sig_lowpass *f, const float *in, float *out, size_t n)
{
  const float a = f->a;
  float       y = f->y;
  for (size_t i = 0; i < n; i++)
    {
      y += a * (in[i] - y); /* in[i] is read before out[i] is written, */
      out[i] = y;           /* so in == out is safe */
    }
  f->y = y;
}

void
sig_lowpass_reset (sig_lowpass *f)
{
  f->y = 0.0f;
}
```

And a program that knows only the header:

```bash
cat api.c
```

```c
/* api.c -- a program written against sig.h, and nothing else.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       api.c sig.c -o api -lm && ./api
 */
#include "sig.h"

#include <stdio.h>

enum
{
  N = 8
};

int
main (void)
{
  printf ("built against sig %d.%d.%d, running sig %s\n", SIG_VERSION_MAJOR,
          SIG_VERSION_MINOR, SIG_VERSION_PATCH, sig_version ());

  /* A mistake: 30 kHz is above half of 48 kHz. The status says so. */
  sig_lowpass *f;
  sig_status   s = sig_lowpass_create (30000.0, 48000.0, &f);
  printf ("30 kHz at 48 kHz: %s\n", sig_strerror (s));

  s = sig_lowpass_create (1000.0, 48000.0, &f);
  if (s != SIG_OK)
    {
      fprintf (stderr, "sig_lowpass_create: %s\n", sig_strerror (s));
      return 1;
    }

  const float in[N] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
  float       out[N];
  sig_lowpass_process (f, in, out, N); /* in is const: left as it was */
  printf (" 1 kHz at 48 kHz:");
  for (size_t i = 0; i < N; i++)
    printf (" %.4f", (double)out[i]);
  printf ("\n");

  sig_lowpass_destroy (f);
  return 0;
}
```

`sig.c` calls `exp`, so the build links the math library, `-lm` (lesson 03):

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined api.c sig.c -o api -lm
./api
```

```
built against sig 1.0.0, running sig 1.0.0
30 kHz at 48 kHz: argument out of range
 1 kHz at 48 kHz: 0.1227 0.2303 0.3248 0.4076 0.4803 0.5441 0.6000 0.6491
```

## The checklist

- **One prefix for everything.** C has a single namespace: two libraries that
  both define `filter` or `process` can't be used in one program (exercise 3).
  Every public name here starts with `sig_`, and every macro with `SIG_`.
  Private helpers in `sig.c` are `static`, which keeps them out of the
  namespace altogether.
- **Opaque types, made and freed in pairs.** `sig_lowpass` is lesson 14's
  opaque type, with lesson 10's create/destroy pair. The struct can change in
  any release without breaking a caller. The names have no `_t` ending: POSIX
  (lesson 03) reserves names ending in `_t` for itself, so a library shouldn't
  add more.
- **Say what went wrong.** Lesson 14's `lowpass_create` returned `NULL` whether
  the argument was bad or memory ran out. Here every call that can fail returns
  a `sig_status`, and the object comes back through an out-parameter,
  `sig_lowpass **out`. The caller can tell a bug in its own arguments from a
  machine out of memory, and `sig_strerror` turns either into words. `*out` is
  set to `NULL` on every failure, so it is never left holding garbage.
- **The caller's units.** A filter's coefficient means nothing to the person
  using it; its cutoff in Hz does. `sig_lowpass_create` takes Hz and the sample
  rate, checks them (the cutoff must be below half the rate), and works out the
  coefficient itself, once.
- **`const` is part of the contract.** `const float *in` promises that the
  filter won't change the input, and the compiler holds the library to it.
  `float *out` says where the result goes. Whether the two may be the same
  buffer is said in the comment, because nothing else can say it (exercise 2).
- **The processing path can't fail.** `sig_lowpass_process` returns nothing:
  every check happened at create time, and processing does no allocation, no
  printing and no locking (lesson 13). A function that can't fail is one the
  caller doesn't have to handle failing.
- **No global state.** Everything a filter needs is in its own struct, so two
  filters on two threads never touch the same memory. One `static` counter in
  `sig.c` would quietly end that.
- **A version, twice.** `SIG_VERSION_MAJOR` and friends are fixed when a
  program is compiled; `sig_version()` reports the library it is actually
  running with. When the major numbers differ, the API has changed under the
  program (exercise 1). `sig.c` builds the version string from the same macros,
  with the preprocessor's `#` operator, which turns a macro argument into a
  string, so the number and the string can't disagree.
- **`extern "C"`** tells a C++ compiler that these are C functions. Inside
  `#ifdef __cplusplus`, a C compiler never sees it.

## Pitfalls

- **Arguments the wrong way round.**
  [`broken-swapped-buffers.c`](broken-swapped-buffers.c) passes `out` where
  `in` goes, and the `const` in the API catches it:

  ```
  broken-swapped-buffers.c:19:32: error: passing argument 3 of ‘sig_lowpass_process’ discards ‘const’ qualifier from pointer target type [-Werror=discarded-qualifiers]
  ```

  It catches it here only because the caller's input is `const` too. Two plain
  `float` arrays swap without a word, which is a reason to declare data you
  won't change `const`, all the way out to the caller.

- **Booleans and bare numbers in arguments.** `make (48000, 1000, 1, 0)` says
  nothing at the call site. Order arguments the way they are read aloud, name
  the units, and prefer an `enum` to a flag.

- **A header that leaks its insides.** Every type and macro a public header
  defines is part of the API, including helper macros meant only for the
  implementation. `SIG_STR` is in `sig.c`, not `sig.h`, for that reason.

- **Changing behaviour without changing the version.** A function that used to
  accept a cutoff of 0 and now refuses it has broken its callers as surely as a
  renamed one. If a caller could notice, it is a change to the API.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. A program built against `sig` 1.0.0 might be run with a library from another
   release. Make it check, and refuse a library with a different major version.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-version/`:

   Built with the matching library:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined compat.c sig.c -o compat -lm
   ./compat
   ```

   ```
   header 1.0.0, library 1.0.0: compatible
   ```

   [`sig.c`](exercises/ex1-version/sig.c) here lets `-DSIG_BUILD_MAJOR=2`
   pretend to be the next major release:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -DSIG_BUILD_MAJOR=2 compat.c sig.c -o compat -lm
   ./compat
   ```

   ```
   header 1.0.0, library 2.0.0: incompatible, stopping
   ```

   The header's numbers were fixed when
   [`compat.c`](exercises/ex1-version/compat.c) was compiled; `sig_version()`
   is answered by whatever library it runs with. By convention (*semantic
   versioning*), a new minor or patch version only adds or fixes things, so a
   program built for 1.0 runs with 1.3; a new major version may have changed
   what the program depends on, so it stops before calling anything else. With
   a shared library (Part V), which is picked at run time, this is a real risk,
   not a contrived one.

   </details>

2. `sig.h` says `in` and `out` may be the same buffer. Test the promise. What
   would `float *restrict out` have changed?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-in-place/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined inplace.c sig.c -o inplace -lm
   ./inplace
   ```

   ```
   256 samples of a square wave: 0 differ
   ```

   [`inplace.c`](exercises/ex2-in-place/inplace.c) runs two identical filters,
   one into a second buffer and one in place, and every sample agrees. It works
   because `sig_lowpass_process` reads `in[i]` before it writes `out[i]`, and
   never reads that position again.

   `restrict` (C99) is a promise from the *caller* that two pointers never
   reach the same memory, which lets the compiler optimise more freely. With
   `restrict` on `in` and `out`, in place would break that promise, and the
   behaviour would be undefined. Allowing in place is convenient for callers;
   `restrict` can make the loop faster. Either is a fine API, but it has to be
   one or the other, and the header has to say which.

   </details>

3. [`fir.h`](exercises/ex3-prefix/fir.h) and
   [`smooth.h`](exercises/ex3-prefix/smooth.h) are two small libraries, each
   fine on its own. Include both in one program.

   <details><summary>Output and answer</summary>

   [`broken-clash.c`](exercises/ex3-prefix/broken-clash.c) does nothing but
   include them, and gcc refuses:

   ```
   smooth.h:10:3: error: conflicting types for ‘filter’; have ‘struct <anonymous>’
   smooth.h:12:6: error: conflicting types for ‘process’; have ‘void(filter *, float *, size_t)’ {aka ‘void(filter *, float *, long unsigned int)’}
   ```

   Both define `filter` and declare `process`, and C has one namespace for
   them. Neither library has a bug, but no program can use both, and only their
   authors can fix it, by renaming everything, which breaks every program
   already using them. A prefix from day one (`fir_filter`, `smooth_process`)
   costs a few letters per name.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                          |
| ------------------------------------------------------------ | ------------ | ----------------------------------------------------- |
| [`exp`](https://en.cppreference.com/c/numeric/math/exp)      | `<math.h>`   | *e* raised to the power x                             |
| [`fprintf`](https://en.cppreference.com/c/io/fprintf)        | `<stdio.h>`  | `printf` to a stream, such as `stderr`                |
| [`free`](https://en.cppreference.com/c/memory/free)          | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc` |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)      | `<stdlib.h>` | allocate memory, contents unset                       |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                          |
| [`strtol`](https://en.cppreference.com/c/string/byte/strtol) | `<stdlib.h>` | read a `long` from a string                           |

<!-- /libfuncs -->
