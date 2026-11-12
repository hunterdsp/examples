# 02 · Numbers

```bash
cd c-by-example/02-numbers
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

Signal processing is arithmetic on a lot of numbers, fast. So before
anything else: what C's number types actually hold, and the four ways they
quietly give you the wrong answer. `-lm` links the maths library, which on
Linux is separate from the rest of the C library.

## Integers

```
int 4, long 8, float 4, double 8 bytes
int16_t holds -32768 .. 32767
```

C only promises `int` is *at least* 16 bits. When the size matters (a file
format, a 16-bit audio sample, a hardware register) use `<stdint.h>`:
`int16_t` is exactly 16 bits on every machine, `uint32_t` exactly 32 and
unsigned.

```
7 / 2 = 3, 7 % 2 = 1, -7 / 2 = -3, 7 / 2.0 = 3.5
```

Integer division throws the remainder away, rounding toward zero. Make one
side a floating-point number (`2.0`) and you get the real quotient.

```
200 + 100 = 300 as int, 44 as uint8_t
uint8_t 255 + 1 = 0
```

Arithmetic on small types is done in `int` (*integer promotion*), so
`200 + 100` really is 300. Squeezing it back into 8 bits wraps it:
300 − 256 = 44. **Unsigned** types wrap like this by definition.

## Floating point

```
0.1 + 0.2 = 0.30000000000000004, == 0.3? no
16777216.0f + 1 = 16777216.0
```

Floats are binary fractions, and 0.1 has no exact binary form, just as ⅓
has no exact decimal one. So never compare floating-point results with
`==`; compare against a tolerance. A `float` carries about 7 significant
digits (24 bits): past 2²⁴ = 16,777,216 it cannot even count by one. That
is why a long-running phase accumulator or a running sum is kept in
`double` (lesson 22).

## Decibels

```
half amplitude = -6.0206 dB
-6 dB = 0.5012 x amplitude
double the power = 3.0103 dB
```

Ears and radios both work on ratios, so signal levels are measured in
decibels: 20·log₁₀ of an amplitude ratio, or 10·log₁₀ of a power ratio.
Two numbers worth remembering: **−6 dB is half the amplitude, +3 dB is
double the power.**

## Pitfalls

- **Signed overflow is undefined behaviour.** [`broken-overflow.c`](broken-overflow.c)
  counts a day of audio at 48 kHz in an `int32_t`: 4.1 billion does not fit
  in 2.1 billion. For unsigned types that would wrap; for signed types the
  standard says *anything* may happen, and an optimising compiler is
  allowed to assume it never does. UBSan stops it on the spot:

  ```
  broken-overflow.c:14:15: runtime error: signed integer overflow:
  48000 * 86400 cannot be represented in type 'int'
  ```

  The fix is a type big enough for the answer, made big *before* the
  multiplication: `(int64_t)48000 * 86400`.
- **Mixing signed and unsigned.** In [`broken-sign-compare.c`](broken-sign-compare.c),
  `-1 < 1u` is false: the −1 is converted to unsigned first and becomes
  4294967295. `-Wextra` flags the comparison (`-Wsign-compare`).
- **`==` on floats.** See above.
- **Integer division where you meant real division.** `1 / 2` is 0.

## Exercises

Try each one before opening its answer. Every answer's commands run in
this lesson's folder, where the commands above left you, and end back
there. Its code is in [`exercises/`](exercises/), one folder per exercise.

1. Print `INT32_MAX` and `INT64_MAX`. How many days of 48 kHz audio fit in
   each?

   <details><summary>Output and answer</summary>

   ```bash
   cd exercises/ex1-limits
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined limits.c -o limits
   ./limits
   cd ../..
   ```

   ```
   INT32_MAX = 2147483647
   INT64_MAX = 9223372036854775807
   int32_t lasts 12.4 hours
   int64_t lasts 6.1 million years
   ```

   Half a day for `int32_t`: a sample counter in one overflows after 12.4
   hours, so a program that passes every test can still fail on the first
   long run. `int64_t` lasts about 6 million years. Use 64 bits for any
   count that grows with time. (The answer prints hours and years because
   0.5 days and 2 billion days are hard to picture.)

   </details>

2. Convert −20 dB, −40 dB and −60 dB to amplitude ratios. What is the
   pattern?

   <details><summary>Output and answer</summary>

   ```bash
   cd exercises/ex2-decibels
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined decibels.c -o decibels -lm
   ./decibels
   cd ../..
   ```

   ```
   -20 dB = 0.1000 x amplitude
   -40 dB = 0.0100 x amplitude
   -60 dB = 0.0010 x amplitude
   ```

   Every −20 dB divides the amplitude by 10, because 20·log₁₀(0.1) = −20.
   Decibels turn multiplying into adding: a 1000-fold range becomes 60 dB,
   which is why audio levels, filter responses and radio signal strengths
   are all quoted in dB.

   </details>

3. Sum `0.1f` ten thousand times in a `float`, then in a `double`, and print
   both with `%.6f`. Which one is 1000?

   <details><summary>Output and answer</summary>

   ```bash
   cd exercises/ex3-float-sum
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined sum.c -o sum
   ./sum
   cd ../..
   ```

   ```
   float  sum of 0.1f: 999.902893
   double sum of 0.1f: 1000.000015
   double sum of 0.1 : 1000.000000
   ```

   Neither, at first. The `float` sum is off by a tenth: each addition
   rounds to about 7 digits, and ten thousand roundings pile up. The
   `double` sum of the same `0.1f` is off too, but by much less, and only
   because `0.1f` itself is not 0.1: it is the nearest `float`, slightly
   above it, and the `double` adds that up faithfully. Sum the `double`
   `0.1` and the error drops below the sixth decimal place. Accumulate
   in `double`, from `double` inputs, whenever a sum runs long.

   </details>
