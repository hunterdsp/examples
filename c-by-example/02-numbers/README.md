# 02 · Numbers

Run this lesson's commands in `c-by-example/02-numbers/`.

Signal processing is arithmetic on a lot of numbers, fast. So before anything
else: what C's number types actually hold, and the four ways they quietly give
you the wrong answer. Here is the program that shows them:

```bash
cat numbers.c
```

```c
/* numbers.c -- what the number types really hold.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       numbers.c -o numbers -lm && ./numbers
 *
 * -lm links the maths library (log10, pow); on Linux it is separate.
 */
#include <inttypes.h> /* PRId64: the printf format for int64_t */
#include <math.h>
#include <stdint.h>
#include <stdio.h>

int
main (void)
{
  /* Sizes. `int` is 32 bits on every machine this course runs on, but C
     only promises at least 16. When the size matters, say so: int16_t is
     exactly 16 bits everywhere. */
  printf ("int %zu, long %zu, float %zu, double %zu bytes\n", sizeof (int),
          sizeof (long), sizeof (float), sizeof (double));
  printf ("int16_t holds %d .. %d\n", INT16_MIN, INT16_MAX);

  /* Integer division throws the remainder away, toward zero. */
  printf ("7 / 2 = %d, 7 %% 2 = %d, -7 / 2 = %d, 7 / 2.0 = %.1f\n", 7 / 2,
          7 % 2, -7 / 2, 7 / 2.0);

  /* Promotion: arithmetic on small types happens in int. The sum is 300,
     and only storing it back into 8 bits wraps it (300 - 256 = 44). */
  uint8_t a = 200, b = 100;
  int sum = a + b;
  uint8_t wrapped = (uint8_t)(a + b);
  printf ("200 + 100 = %d as int, %d as uint8_t\n", sum, wrapped);

  /* Unsigned arithmetic wraps by definition... */
  uint8_t counter = 255;
  counter++;
  printf ("uint8_t 255 + 1 = %d\n", counter);
  /* ...signed arithmetic does NOT: see broken-overflow.c. A day of audio
     at 48 kHz needs 64 bits. */
  int64_t per_day = (int64_t)48000 * 86400;
  printf ("samples in a day at 48 kHz: %" PRId64 "\n", per_day);

  /* Floating point is binary, so 0.1 is not exactly 0.1. */
  double x = 0.1 + 0.2;
  printf ("0.1 + 0.2 = %.17f, == 0.3? %s\n", x, x == 0.3 ? "yes" : "no");
  /* A float has 24 bits of mantissa: past 2^24 it cannot count by one. */
  float big = 16777216.0f;
  printf ("16777216.0f + 1 = %.1f\n", big + 1.0f);

  /* Decibels: the unit signal processing lives in. Amplitude ratios use
     20 log10, power ratios 10 log10. Halving amplitude is about -6 dB. */
  printf ("half amplitude = %.4f dB\n", 20.0 * log10 (0.5));
  printf ("-6 dB = %.4f x amplitude\n", pow (10.0, -6.0 / 20.0));
  printf ("double the power = %.4f dB\n", 10.0 * log10 (2.0));
  return 0;
}
```

Build and run it. `-lm` links the maths library, which on Linux is separate
from the rest of the C library:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined numbers.c -o numbers -lm
./numbers
```

```
int 4, long 8, float 4, double 8 bytes
int16_t holds -32768 .. 32767
7 / 2 = 3, 7 % 2 = 1, -7 / 2 = -3, 7 / 2.0 = 3.5
200 + 100 = 300 as int, 44 as uint8_t
uint8_t 255 + 1 = 0
samples in a day at 48 kHz: 4147200000
0.1 + 0.2 = 0.30000000000000004, == 0.3? no
16777216.0f + 1 = 16777216.0
half amplitude = -6.0206 dB
-6 dB = 0.5012 x amplitude
double the power = 3.0103 dB
```

Each section below takes a few of those lines in turn.

## Integers

```
int 4, long 8, float 4, double 8 bytes
int16_t holds -32768 .. 32767
```

C only promises `int` is *at least* 16 bits, and `long` at least 32: it is 8
bytes here, but 4 on 64-bit Windows. The same program can hold different
numbers on different machines.

> [!TIP]
> **Use explicitly sized types to limit surprises.** `<stdint.h>` puts the
> width in the name, so it means the same on every machine:
>
> - `int8_t`, `int16_t`, `int32_t`, `int64_t`: signed, exactly 8, 16, 32 and 64
>   bits.
> - `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`: the same widths, unsigned.
>
> Each comes with its limits (`INT16_MIN`, `INT16_MAX`, `UINT8_MAX`), and
> `<inttypes.h>` gives its `printf` format (`PRId64` for `int64_t`, as in
> `numbers.c`). Reach for them whenever the size matters: a file format, a
> 16-bit audio sample, a hardware register, a count that grows with time.

```
7 / 2 = 3, 7 % 2 = 1, -7 / 2 = -3, 7 / 2.0 = 3.5
```

Integer division throws the remainder away, rounding toward zero. Make one side
a floating-point number (`2.0`) and you get the real quotient.

```
200 + 100 = 300 as int, 44 as uint8_t
uint8_t 255 + 1 = 0
```

Arithmetic on small types is done in `int` (*integer promotion*), so
`200 + 100` really is 300. Squeezing it back into 8 bits wraps it to 300 − 256,
which is 44. **Unsigned** types wrap like this by definition.

To keep every bit of a result, the type that holds it must be big enough:

| Operation                  | Result needs      | For example                     |
| -------------------------- | ----------------- | ------------------------------- |
| `a + b`, both N bits       | N + 1 bits        | two `int16_t` samples: 17 bits  |
| sum of K values of N bits  | N + ⌈log₂ K⌉ bits | 1024 `int16_t` samples: 26 bits |
| `a * b`, N bits and M bits | N + M bits        | `int16_t` × `int16_t`: 32 bits  |

Widen *before* the operation, not after. For two `int32_t`s, `(int64_t)a * b`
multiplies in 64 bits, but `(int64_t)(a * b)` multiplies in 32, overflows, and
then widens the wrong answer. Two `int16_t`s need no cast, because promotion
already does their multiply in a 32-bit `int`.

> [!TIP]
> **Unsigned overflow is a free modulus.** An N-bit unsigned result is always
> taken modulo 2ᴺ, and the hardware does it at no cost: the carry out of the
> top bit is simply dropped. A lot of code relies on that on purpose:
>
> - A `uint32_t` phase accumulator in an oscillator wraps exactly once per
>   cycle, with no `if` and no `fmod` (lesson 22).
> - A `uint8_t` index into a 256-entry buffer goes round by itself (lesson 12).
> - Hashes and checksums are defined modulo 2³² or 2⁶⁴, so they are written to
>   overflow.
>
> Only unsigned types promise this. A signed type that overflows is a bug (see
> Pitfalls), and a small unsigned type wraps only when the result is stored
> back into it, as `(uint8_t)(a + b)` does above.

## Floating point

```
0.1 + 0.2 = 0.30000000000000004, == 0.3? no
16777216.0f + 1 = 16777216.0
```

Floats are binary fractions, and 0.1 has no exact binary form, just as ⅓ has no
exact decimal one. So never compare floating-point results with `==`; compare
against a tolerance. A `float` carries about 7 significant digits (24 bits):
past 2²⁴ = 16,777,216 it cannot even count by one. That is why a long-running
phase accumulator or a running sum is kept in `double` (lesson 22).

## Decibels

```
half amplitude = -6.0206 dB
-6 dB = 0.5012 x amplitude
double the power = 3.0103 dB
```

Ears and radios both work on ratios, so signal levels are measured in decibels:
20·log₁₀ of an amplitude ratio, or 10·log₁₀ of a power ratio. Two numbers worth
remembering: **−6 dB is half the amplitude, +3 dB is double the power.**

## Pitfalls

- **Signed overflow is undefined behaviour.**
  [`broken-overflow.c`](broken-overflow.c) counts a day of audio at 48 kHz in
  an `int32_t`: 4.1 billion does not fit in 2.1 billion. For unsigned types
  that would wrap; for signed types the standard says *anything* may happen,
  and an optimising compiler is allowed to assume it never does. UBSan stops it
  on the spot:

  ```
  broken-overflow.c:14:15: runtime error: signed integer overflow:
  48000 * 86400 cannot be represented in type 'int'
  ```

  The fix is a type big enough for the answer, made big *before* the
  multiplication: `(int64_t)48000 * 86400`.

- **Mixing signed and unsigned.** In
  [`broken-sign-compare.c`](broken-sign-compare.c), `-1 < 1u` is false: the −1
  is converted to unsigned first and becomes 4294967295. `-Wextra` flags the
  comparison (`-Wsign-compare`).

- **`==` on floats.** See above.

- **Integer division where you meant real division.** `1 / 2` is 0.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Print `INT32_MAX` and `INT64_MAX`. How many days of 48 kHz audio fit in
   each?

   <details><summary>Output and answer</summary>

   In `exercises/ex1-limits/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined limits.c -o limits
   ./limits
   ```

   ```
   INT32_MAX = 2147483647
   INT64_MAX = 9223372036854775807
   int32_t lasts 12.4 hours
   int64_t lasts 6.1 million years
   ```

   Half a day for `int32_t`: a sample counter in one overflows after 12.4
   hours, so a program that passes every test can still fail on the first long
   run. `int64_t` lasts about 6 million years. Use 64 bits for any count that
   grows with time. (The answer prints hours and years because 0.5 days and 2
   billion days are hard to picture.)

   </details>

2. Convert −20 dB, −40 dB and −60 dB to amplitude ratios. What is the pattern?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-decibels/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined decibels.c -o decibels -lm
   ./decibels
   ```

   ```
   -20 dB = 0.1000 x amplitude
   -40 dB = 0.0100 x amplitude
   -60 dB = 0.0010 x amplitude
   ```

   Every −20 dB divides the amplitude by 10, because 20·log₁₀(0.1) = −20.
   Decibels turn multiplying into adding: a 1000-fold range becomes 60 dB,
   which is why audio levels, filter responses and radio signal strengths are
   all quoted in dB.

   </details>

3. Sum `0.1f` ten thousand times in a `float`, then in a `double`, and print
   both with `%.6f`. Which one is 1000?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-float-sum/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined sum.c -o sum
   ./sum
   ```

   ```
   float  sum of 0.1f: 999.902893
   double sum of 0.1f: 1000.000015
   double sum of 0.1 : 1000.000000
   ```

   Neither, at first. The `float` sum is off by a tenth: each addition rounds
   to about 7 digits, and ten thousand roundings pile up. The `double` sum of
   the same `0.1f` is off too, but by much less, and only because `0.1f` itself
   is not 0.1: it is the nearest `float`, slightly above it, and the `double`
   adds that up faithfully. Sum the `double` `0.1` and the error drops below
   the sixth decimal place. Accumulate in `double`, from `double` inputs,
   whenever a sum runs long.

   </details>
