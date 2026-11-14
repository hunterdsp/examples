# 29 · Library layout

Run this lesson's commands in `c-by-example/29-library-layout/`.

Every lesson so far has been a folder of `.c` files, compiled together into one
program. Code that other people's programs use is shipped differently, as a
**library**: compiled once, with a header that says what it offers, and linked
into programs its author never sees.

Part V turns Part IV's oscillator, biquad and FFT into one, called `sig`,
following lesson 18's rules, and then ships it: a build (lesson 30), tests
(31), documentation (32), command-line tools (33), an install that other
projects can find (34), and a Python interface (35). The library lives in
[`c-by-example/sig/`](../sig/), and each of these lessons works on it from its
own folder, as `../sig`. (`LC_ALL=C` makes `ls` sort by byte value, capitals
first, as it does on macOS; Linux's usual setting ignores case.)

```bash
LC_ALL=C ls ../sig ../sig/include/sig ../sig/src
```

```
../sig:
CMakeLists.txt
README.md
cmake
compile_flags.txt
docs
include
python
sig.pc.in
src
tests
tools

../sig/include/sig:
biquad.h
common.h
fft.h
osc.h
sig.h

../sig/src:
biquad.c
common.c
fft.c
internal.h
osc.c
```

The files the later lessons are about are already there; this one is about two
folders, `include/` and `src/`.

## Public and private

**`include/sig/` is the library's interface**: the headers a program using it
includes, and the only part of the source that gets installed (lesson 34).
Everything in them is a promise (lesson 18). Here is one:

```bash
cat ../sig/include/sig/osc.h
```

```c
/**
 * @file osc.h
 * @brief A sine oscillator whose phase never drifts.
 *
 * Lesson 22's oscillator: a 32-bit phase accumulator, so the phase wraps
 * at the end of every cycle by itself, and a 1024-entry sine table read
 * with linear interpolation. Integer adds are exact, so the phase gathers
 * no rounding error, however long it runs. The frequency is the one asked
 * for, rounded to a multiple of rate_hz / 2^32 (1.1e-5 Hz at 48 kHz),
 * and each sample is within 1e-5 of the sine of its phase. Against an
 * ideal sine at exactly the frequency asked for, that rounding adds an
 * error that grows with time: up to 3.6e-5 after one second at 48 kHz.
 */
#ifndef SIG_OSC_H
#define SIG_OSC_H

#include <sig/common.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** A sine oscillator. Opaque: made by sig_osc_create(). */
  typedef struct sig_osc sig_osc;

  /**
   * Make an oscillator.
   *
   * @param freq_hz The frequency to play. Needs 0 < freq_hz < rate_hz / 2.
   * @param rate_hz The sample rate, in samples per second.
   * @param[out] out On SIG_OK, the new oscillator, owned by the caller;
   *                 on any error, NULL.
   * @return SIG_OK, SIG_EINVAL or SIG_ENOMEM.
   */
  SIG_API sig_status sig_osc_create (double freq_hz, double rate_hz,
                                     sig_osc **out);

  /**
   * Free an oscillator.
   *
   * @param o The oscillator, or NULL, which does nothing.
   */
  SIG_API void sig_osc_destroy (sig_osc *o);

  /**
   * Write the next n samples of a sine of amplitude 1, starting at phase 0
   * on the first call and carrying on where the last call stopped.
   *
   * @param o The oscillator.
   * @param[out] out Where the n samples go.
   * @param n How many samples.
   */
  SIG_API void sig_osc_process (sig_osc *o, float *out, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* SIG_OSC_H */
```

The comments starting `/**` are for Doxygen, which turns them into the
library's documentation (lesson 32). `SIG_API`, from `common.h`, marks each
function as part of the interface; lesson 34 says what it does.

**`src/` is the implementation**: the `.c` files, and a header that only they
include:

```bash
cat ../sig/src/internal.h
```

```c
/* internal.h -- what the library's own .c files share, and nobody else.
 *
 * It lives in src/, not include/, so it is never installed: a program
 * using sig can't include it, and the library may change it at will.
 * Its functions still have external linkage, since several .c files call
 * them, so they are named sig__ (two underscores), meaning "internal",
 * and the shared library hides them (lesson 34).
 */
#ifndef SIG_INTERNAL_H
#define SIG_INTERNAL_H

#define SIG_PI 3.14159265358979323846

/* 1 if a signal sampled at rate_hz can carry freq_hz: 0 < freq_hz <
   rate_hz / 2. Written so that a NaN fails every test. */
int sig__freq_ok (double freq_hz, double rate_hz);

#endif /* SIG_INTERNAL_H */
```

`sig__freq_ok` checks a frequency, and both the oscillator and the biquad call
it, so it can't be `static` (lesson 08): it has to be visible to the other
files of the library, and so it is visible to every program linked with it too.
Its name says it is not for them. Nothing in `include/` mentions it, so no
program can declare it without copying it, and the library is free to change
it.

## A folder named after the library

A program includes the headers as `<sig/osc.h>`, not `"osc.h"`, and the
compiler is told where `include/` is with `-I`. The `sig/` folder is a
namespace for header names, the way the `sig_` prefix is for function names:
lesson 24 had an `fft.h`, and so do half the signal-processing libraries in the
world. Two libraries that both install an `fft.h` into the same place can't
both be used, but `sig/fft.h` and `fftw3.h` can.

Angle brackets mean "search the `-I` folders and the system's", which is right
for a library's headers: the program doesn't say where they are, the build
does.

## Building it by hand

A library is compiled, but never run on its own: it has no `main`. Compile each
file of `src/` to an object file (lesson 08's `-c`), here into this folder:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../sig/include -c ../sig/src/*.c
```

`osc.c` includes `"internal.h"` in quotes, which is looked for first in the
folder of the file doing the including, `src/`, so only `include/` needs a
`-I`.

A **static library** is those object files, bundled into one *archive* by `ar`:
`r` puts the files in, `c` creates the archive, and `s` adds an index of the
functions each file defines. By convention its name is `lib`, the library's
name, and `.a`:

```bash
ar rcs libsig.a biquad.o common.o fft.o osc.o
```

A program that uses it:

```bash
cat tone.c
```

```c
/* tone.c -- a program that uses sig, the library, not a copy of its code.
 *
 * It includes the header from include/sig/ and links with libsig.a; no
 * line of the oscillator is compiled here.
 */
#include <sig/osc.h>

#include <stdio.h>

int
main (void)
{
  sig_osc   *osc;
  sig_status s = sig_osc_create (30000.0, 48000.0, &osc);
  printf ("30000 Hz at 48000 samples/s: %s\n", sig_strerror (s));

  s = sig_osc_create (750.0, 48000.0, &osc);
  if (s != SIG_OK)
    {
      fprintf (stderr, "tone: %s\n", sig_strerror (s));
      return 1;
    }
  float buf[8];
  sig_osc_process (osc, buf, 8);
  sig_osc_destroy (osc);

  printf ("750 Hz:");
  for (size_t i = 0; i < 8; i++)
    printf (" %.3f", (double)buf[i]);
  printf ("\n");
  return 0;
}
```

`-L .` adds this folder to where the linker looks for libraries, and `-lsig`
links `libsig.a`: the linker adds the `lib` and the `.a` itself, as it does for
`-lm`, the maths library, which `sig` uses.

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../sig/include tone.c -L . -lsig -lm -o tone
./tone
```

```
30000 Hz at 48000 samples/s: argument out of range
750 Hz: 0.000 0.098 0.195 0.290 0.383 0.471 0.556 0.634
```

## What it shows

- **One copy of the code.** No line of the oscillator is in this folder. Every
  program that uses `sig` uses the same, tested code, and a fix to the library
  reaches all of them when they are next built.

- **The header is all a program sees.** `tone.c` makes an oscillator and never
  learns what is in one (lesson 14): `sig_osc` is opaque, so the library can
  change its insides without breaking `tone.c`.

- **Errors come back as values.** 30000 Hz can't be played at 48000 samples per
  second, since it is over half the rate (lesson 22), and `sig_osc_create` says
  so with a status, not by printing, or exiting.

- **What's in the archive.** `nm` lists the functions each object file defines
  (`T`) and needs from elsewhere (`U`). With GNU binutils on Linux (macOS's
  `nm` puts an `_` in front of every name):

  <!-- no-run -->

  ```bash
  nm libsig.a
  ```

  ```
  biquad.o:
  0000000000000000 T sig_biquad_create
  00000000000006b9 T sig_biquad_destroy
  00000000000006d8 T sig_biquad_process
  0000000000000dc5 T sig_biquad_reset
                   U sig__freq_ok
  ...
  common.o:
  000000000000005a T sig__freq_ok
  0000000000000011 T sig_strerror
  0000000000000000 T sig_version
  ...
  ```

  (trimmed: the sanitizers add many more). `sig__freq_ok` is in there as a `T`,
  like every public function, because to the linker it is just one more
  function. Lesson 34's shared library hides it.

## Pitfalls

- **A header that isn't self-contained.**
  [`broken-not-self-contained.c`](broken-not-self-contained.c) includes
  [`meter.h`](meter.h) first, before anything else, and `meter.h` uses `size_t`
  without including `<stddef.h>`:

  ```
  meter.h:9:37: error: unknown type name 'size_t'
  ```

  The header worked in the program it was written for, which happened to
  include `<stdio.h>` first. Every header must include what it uses itself.
  That is why each of `sig`'s `.c` files includes its own header first: if the
  header were missing an include, its own `.c` file would be the first to fail
  to compile (exercise 1 checks all of them).

- **Two copies of a header.** A program built against one version of
  `include/sig/` and linked with a library built from another can compile, link
  and still be wrong, if a struct or an enum changed between them. Only
  `include/` and the library, built together, installed together, are a
  matching pair (lesson 34).

- **The order of `-l`.** The GNU linker reads its inputs left to right, and
  takes from an archive only the functions that something before it needed
  (exercise 3).

## Exercises

Try each one before opening its answer. All three use the lesson's own folder,
and the library built in it.

1. Check that every one of `sig`'s public headers compiles on its own, first in
   a file, with nothing before it.

   <details><summary>Output and answer</summary>

   For each header, `printf` writes a one-line C file that includes it, and
   `cc` compiles it from its standard input (`-x c -` says the input is C, and
   comes from `-`, standard input), only checking it: `-fsyntax-only` writes
   nothing.

   ```bash
   for h in ../sig/include/sig/*.h; do
     printf '#include <sig/%s>\n' "${h##*/}" |
       cc -std=c99 -Wall -Wextra -I ../sig/include -fsyntax-only -x c - &&
       echo "${h##*/}: compiles on its own"
   done
   ```

   ```
   biquad.h: compiles on its own
   common.h: compiles on its own
   fft.h: compiles on its own
   osc.h: compiles on its own
   sig.h: compiles on its own
   ```

   `${h##*/}` is the shell removing everything up to the last `/`, leaving the
   file's name. Run as part of a library's tests, this loop finds the next
   `meter.h` before anyone else does.

   </details>

2. `tone` uses only the oscillator. How much of `libsig.a` ended up inside it?

   <details><summary>Output and answer</summary>

   The first `grep` keeps the functions (`T`), and `grep -o` prints only the
   part of each line that matches, so the names come out the same with or
   without macOS's `_`:

   ```bash
   nm tone | grep ' T ' | grep -o 'sig_[a-z_]*' | sort -u
   ```

   ```
   sig__freq_ok
   sig_osc_create
   sig_osc_destroy
   sig_osc_process
   sig_strerror
   sig_version
   ```

   No biquad and no FFT: the linker copies from an archive only the object
   files that define something the program needs. `tone` calls
   `sig_osc_create`, so `osc.o` comes in, and `osc.o` calls `sig__freq_ok`, so
   `common.o` comes in, whole, `sig_version` with it, though nothing calls it.
   The unit is the object file, not the function, which is why C's own library
   is built from hundreds of small files.

   </details>

3. Put `-lsig` before `tone.c` on the command line.

   <details><summary>Output and answer</summary>

   It fails with the GNU linker, which Linux uses (trimmed):

   <!-- no-run -->

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../sig/include -L . -lsig -lm tone.c -o tone
   ```

   ```
   /usr/bin/x86_64-linux-gnu-ld.bfd: /tmp/ccdekD6J.o: in function `main':
   …/29-library-layout/tone.c:14:(.text+0xbf): undefined reference to `sig_osc_create'
   …/29-library-layout/tone.c:15:(.text+0xd2): undefined reference to `sig_strerror'
   ```

   The GNU linker goes through its inputs once, left to right. When it reaches
   `libsig.a`, nothing has asked for `sig_osc_create` yet, so it takes nothing,
   and when `tone.c` asks, the archive is behind it. Libraries go after the
   files that use them, and a library after the libraries it uses: `-lsig -lm`,
   since `sig` uses `sin`. macOS's linker searches every archive for every name
   and accepts either order, so a build written on a Mac can fail on Linux. In
   the right order, it works everywhere:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../sig/include tone.c -L . -lsig -lm -o tone
   ./tone
   ```

   ```
   30000 Hz at 48000 samples/s: argument out of range
   750 Hz: 0.000 0.098 0.195 0.290 0.383 0.471 0.556 0.634
   ```

   The flags matter here too: `libsig.a` was compiled with the sanitizers, so
   anything linked with it must be linked with `-fsanitize` as well, or the
   sanitizers' own functions are the undefined references. A library and the
   programs that use it are built with the same options.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                              | header      | what it does                           |
| ----------------------------------------------------- | ----------- | -------------------------------------- |
| [`fprintf`](https://en.cppreference.com/c/io/fprintf) | `<stdio.h>` | `printf` to a stream, such as `stderr` |
| [`printf`](https://en.cppreference.com/c/io/fprintf)  | `<stdio.h>` | formatted output to `stdout`           |

<!-- /libfuncs -->
