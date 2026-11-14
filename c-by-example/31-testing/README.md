# 31 · Testing

Run this lesson's commands in `c-by-example/31-testing/`.

Every lesson in this course is tested: `make test` builds each example, runs
it, and compares what it prints with what its README says. That works for a
course. A library needs tests of its own that travel with it: run by anyone who
builds it, on their machine and their compiler, every time, and that check what
the library *promises*, not what it happened to print once.

`sig`'s tests need no framework. A test is a program that checks things and
exits with status 0 if they all held, and anything else if not; that exit
status is all CMake's test runner looks at. A dozen lines help it report well:

```bash
cat ../sig/tests/check.h
```

```c
/* check.h -- the whole test framework: one macro and one function.
 *
 * CHECK(cond) records a check, and if cond is false prints where and
 * what, and carries on, so one run reports every failure, not just the
 * first. A test program ends with `return check_report (argv[0]);`,
 * which prints a summary and exits non-zero if anything failed: that
 * exit status is all CTest looks at.
 *
 * Not assert(): a release build defines NDEBUG, which deletes every
 * assert, and the tests along with them (lesson 31).
 */
#ifndef SIG_TESTS_CHECK_H
#define SIG_TESTS_CHECK_H

#include <stdio.h>
#include <stdlib.h>

static int check_count, check_failed;

#define CHECK(cond)                                                           \
  do                                                                          \
    {                                                                         \
      check_count++;                                                          \
      if (!(cond))                                                            \
        {                                                                     \
          check_failed++;                                                     \
          printf ("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond);          \
        }                                                                     \
    }                                                                         \
  while (0)

/* The test's name: its path, without the directories. */
static const char *
check_name (const char *path)
{
  const char *name = path;
  for (const char *p = path; *p != '\0'; p++)
    if (*p == '/' || *p == '\\')
      name = p + 1;
  return name;
}

static int
check_report (const char *path)
{
  printf ("%s: %d of %d checks passed\n", check_name (path),
          check_count - check_failed, check_count);
  return check_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif /* SIG_TESTS_CHECK_H */
```

One test program per part of the library. The biquad's:

```bash
cat ../sig/tests/test_biquad.c
```

```c
/* test_biquad.c -- the filters pass and stop what they should. */
#include "check.h"

#include <sig/biquad.h>
#include <sig/osc.h>

#include <math.h>

enum
{
  RATE = 48000,
  N    = 4800 /* a tenth of a second */
};

/* The amplitude of a sine at freq_hz after filter f: the largest output
   in the second half, once the filter has settled. */
static double
gain (sig_biquad *f, double freq_hz)
{
  static float buf[N];
  sig_osc     *o;
  if (sig_osc_create (freq_hz, RATE, &o) != SIG_OK)
    return -1.0;
  sig_osc_process (o, buf, N);
  sig_osc_destroy (o);
  sig_biquad_reset (f);
  sig_biquad_process (f, buf, buf, N); /* in place */
  double peak = 0.0;
  for (size_t i = N / 2; i < N; i++)
    peak = fabs ((double)buf[i]) > peak ? fabs ((double)buf[i]) : peak;
  return peak;
}

int
main (int argc, char *argv[])
{
  (void)argc;
  sig_biquad *f = (sig_biquad *)1;

  CHECK (sig_biquad_create (SIG_LOWPASS, 0.0, 0.7071, RATE, &f) == SIG_EINVAL
         && f == NULL);
  CHECK (sig_biquad_create (SIG_LOWPASS, 1000.0, 0.0, RATE, &f) == SIG_EINVAL
         && f == NULL);
  CHECK (sig_biquad_create ((sig_biquad_type)7, 1000.0, 0.7071, RATE, &f)
             == SIG_EINVAL
         && f == NULL);

  /* A Butterworth low-pass at 1000 Hz: flat well below the corner, 3 dB
     down (0.7071) at it, and falling 12 dB per octave above it. */
  CHECK (sig_biquad_create (SIG_LOWPASS, 1000.0, 0.7071, RATE, &f) == SIG_OK);
  if (f == NULL)
    return check_report (argv[0]);
  CHECK (fabs (gain (f, 100.0) - 1.0) < 0.01);
  CHECK (fabs (gain (f, 1000.0) - 0.7071) < 0.01);
  CHECK (gain (f, 8000.0) < 0.02);
  sig_biquad_destroy (f);

  /* The high-pass, the other way round. */
  CHECK (sig_biquad_create (SIG_HIGHPASS, 1000.0, 0.7071, RATE, &f) == SIG_OK);
  if (f == NULL)
    return check_report (argv[0]);
  CHECK (gain (f, 100.0) < 0.02);
  CHECK (fabs (gain (f, 1000.0) - 0.7071) < 0.01);
  CHECK (fabs (gain (f, 10000.0) - 1.0) < 0.02);

  /* reset forgets the past: an impulse gives the same response twice. */
  float a[8] = { 1.0f }, b[8] = { 1.0f };
  sig_biquad_reset (f);
  sig_biquad_process (f, a, a, 8);
  sig_biquad_reset (f);
  sig_biquad_process (f, b, b, 8);
  int same = 1;
  for (size_t i = 0; i < 8; i++)
    same = same && a[i] == b[i];
  CHECK (same);

  sig_biquad_destroy (f);
  return check_report (argv[0]);
}
```

## Running them

CMake's `enable_testing()` and one `add_test` per program (in `CMakeLists.txt`,
lesson 30) register them, and **CTest** runs them. Build with the sanitizers,
so the tests check memory as well as answers:

```bash
cmake -S ../sig -B build -DSIG_SANITIZE=ON
cmake --build build
```

```bash
ctest --test-dir build
```

CTest runs each test, and prints a line for each and a summary. The times vary
from run to run (trimmed):

```
Test project …/31-testing/build
    Start 1: common
1/5 Test #1: common ...........................   Passed    0.01 sec
    Start 2: osc
2/5 Test #2: osc ..............................   Passed    0.01 sec
    Start 3: biquad
3/5 Test #3: biquad ...........................   Passed    0.01 sec
    Start 4: fft
4/5 Test #4: fft ..............................   Passed    0.01 sec
    Start 5: pipeline
5/5 Test #5: pipeline .........................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 5

Total Test time (real) =   0.06 sec
```

Each test is also a program you can run yourself, to see its checks:

```bash
build/test_biquad
```

```
test_biquad: 12 of 12 checks passed
```

## What it shows

- **Promises, not printouts.** `test_biquad` doesn't compare samples with a
  list someone saved. It checks what a low-pass filter *is*: flat well below
  the corner, 3 dB down (0.7071) at it, and far down an octave and more above.
  Any correct biquad passes, on any machine; a wrong one fails.
- **Tolerances.** No check asks for an exact float. Two compilers, or one
  compiler at two optimisation levels, may round differently (lesson 26), and a
  test that wants the last bit fails on a machine where the code is right. Each
  tolerance is much bigger than rounding and much smaller than any real
  mistake.
- **The error paths too.** The first checks make filters that must be refused:
  a 0 Hz corner, a `q` of 0, a type that isn't one. They check the status *and*
  that `*out` was set to `NULL`, which `sig_biquad_create` promises, by
  starting `f` at an address that isn't `NULL`. Code that handles errors is the
  code that runs least, so it is where bugs live.
- **Every failure, not just the first.** `CHECK` records a failure and carries
  on, so one run lists everything that broke, with the file, the line and the
  condition, all from the preprocessor: `__FILE__`, `__LINE__` and `#cond`,
  which turns the condition into a string (lesson 18's `#`).
- **End to end.** The `pipeline` test runs `sig-tone | sig-filter | sig-fft`
  through `sh` and requires the output to match a pattern
  (`PASS_REGULAR_EXPRESSION`): the tools, the library and the file format
  between them, all at once.

## Pitfalls

- **A test that can't fail.** A test that passes proves something only if it
  would fail when the code is wrong. The way to find out is to break the code
  on purpose and watch it (exercise 1). A test that never failed has never been
  shown to work.

- **`assert` in tests.** `assert` disappears when `NDEBUG` is defined, and
  every release build defines it (lesson 30, exercise 3). Tests written with
  `assert` check nothing in exactly the build that ships (exercise 2). That is
  why `check.h` is its own macro.

- **Tests that need each other.** Each test program makes its own objects and
  checks its own results, and none relies on another having run first, so CTest
  can run them in any order, or all at once (`ctest -j 4`).

- **Randomness.** A test that uses `rand()` gets different numbers on different
  C libraries. When a test needs noise, it makes its own, with a generator
  written into the test (exercise 3).

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Break the low-pass filter: make `b0` a quarter of what it should be, instead
   of a half, and run `test_biquad` against it.

   <details><summary>Output and answer</summary>

   This one uses the lesson's own folder. `sed` writes a changed copy of
   `biquad.c` into the build folder, so `sig` itself is left alone, and `diff`
   shows what changed:

   ```bash
   mkdir -p build/sabotage
   sed 's|(1.0 - c) / 2.0 :|(1.0 - c) / 4.0 :|' ../sig/src/biquad.c > build/sabotage/biquad.c
   diff ../sig/src/biquad.c build/sabotage/biquad.c
   ```

   ```
   34c34
   <   double b0    = type == SIG_LOWPASS ? (1.0 - c) / 2.0 : (1.0 + c) / 2.0;
   ---
   >   double b0    = type == SIG_LOWPASS ? (1.0 - c) / 4.0 : (1.0 + c) / 2.0;
   ```

   The test, built with the broken copy instead of the real one
   (`-I ../sig/src` finds `internal.h` for it):

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../sig/include -I ../sig/src ../sig/tests/test_biquad.c build/sabotage/biquad.c ../sig/src/osc.c ../sig/src/common.c -o build/sabotage/test_biquad -lm
   build/sabotage/test_biquad
   ```

   ```
   ../sig/tests/test_biquad.c:53: FAILED: fabs (gain (f, 100.0) - 1.0) < 0.01
   ../sig/tests/test_biquad.c:54: FAILED: fabs (gain (f, 1000.0) - 0.7071) < 0.01
   test_biquad: 10 of 12 checks passed
   ```

   Two checks caught it, and the test exits with status 1, so CTest would
   report it. The third low-pass check, that 8000 Hz is far down, still passed:
   halving the filter's gain makes it *more* true. That check can't catch this
   bug, and only breaking the code showed it. If no check had failed, the tests
   would have had a hole exactly where the bug was.

   </details>

2. [`assert_test.c`](exercises/ex2-assert/assert_test.c) checks an FFT's size
   with `assert`, and expects the wrong size. Build it as a release build
   would, with `-DNDEBUG`.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-assert/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -DNDEBUG -I ../../../sig/include assert_test.c ../../../sig/src/*.c -o assert_test -lm
   ./assert_test
   ```

   ```
   all tests passed
   ```

   A wrong expectation, and it passed. `assert (x)` is defined as nothing at
   all when `NDEBUG` is defined: the check isn't skipped, it was never
   compiled. Without `-DNDEBUG` the same test stops, as it should. With glibc,
   it prints:

   <!-- no-run -->

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../../../sig/include assert_test.c ../../../sig/src/*.c -o assert_test -lm
   ./assert_test
   ```

   ```
   assert_test: assert_test.c:15: main: Assertion `sig_fft_size (p) == 1000' failed.
   ```

   `assert` is for checking a program's own assumptions while it is being
   developed. A test's checks must hold in every build, which is what `CHECK`
   does. Worse, anything with a side effect inside an `assert`, such as
   `assert (sig_fft_create (n, &p) == SIG_OK)`, vanishes with it: the release
   build never makes the FFT.

   </details>

3. `sig_biquad_process` promises that `in` and `out` may be the same buffer.
   Write a test for that promise, with `check.h`.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-in-place/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../../../sig/include -I ../../../sig/tests test_in_place.c ../../../sig/src/*.c -o test_in_place -lm
   ./test_in_place
   ```

   ```
   test_in_place: 4 of 4 checks passed
   ```

   [`test_in_place.c`](exercises/ex3-in-place/test_in_place.c) filters the same
   noise twice, with two identical filters, once into a separate buffer and
   once in place, and requires every sample to be equal. Equal exactly, for
   once: it is the same arithmetic on the same machine, so any difference is a
   bug, not rounding. The last check makes sure the filter changed something,
   so that a `sig_biquad_process` that did nothing couldn't pass. The noise
   comes from a generator written into the test, so it is the same on every
   machine. In `sig` it would be one more `add_test`.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                               | header       | what it does                                                    |
| ------------------------------------------------------ | ------------ | --------------------------------------------------------------- |
| [`assert`](https://en.cppreference.com/c/error/assert) | `<assert.h>` | stop the program if a condition is false; removed by `-DNDEBUG` |
| [`printf`](https://en.cppreference.com/c/io/fprintf)   | `<stdio.h>`  | formatted output to `stdout`                                    |

<!-- /libfuncs -->
