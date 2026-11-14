# 27 · Fixed point

Run this lesson's commands in `c-by-example/27-fixed-point/`.

Floating point keeps track of where the binary point is for you. **Fixed
point** makes you do it: a number is an ordinary integer, and the program
agrees, by convention, how many of its bits come after the point. In exchange,
the arithmetic is integer arithmetic, which is exact, gives the same result on
every machine, and on processors without floating-point hardware (many small
DSP chips and microcontrollers) it is often the fastest arithmetic the chip
has. The reference code of speech codecs such as ITU-T's G.729 is written in
it.

The workhorse format is **Q15**: an `int16_t` holding `x × 32768`. Fifteen bits
after the point and a sign bit cover −1 to just under +1, in steps of 1/32768.
Getting it right comes down to three habits: round instead of truncating,
saturate instead of wrapping, and keep intermediate results wide.

```bash
cat q15.c
```

```c
/* q15.c -- numbers between -1 and 1, in a 16-bit integer.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       q15.c -o q15 -lm && ./q15
 *
 * Q15 stores x as the int16_t nearest to x * 32768: 15 bits after the
 * binary point, and a sign. It covers -1 to 0.99997, in steps of
 * 1/32768. Integer arithmetic is exact and the same on every machine,
 * and on a processor without floating-point hardware (many small DSPs
 * and microcontrollers) it is often the fastest arithmetic there is.
 * The price: the programmer, not the hardware, keeps track of the
 * scale.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>

typedef int16_t q15;

#define Q15_ONE 32768 /* 1.0, which itself doesn't fit: max is 32767 */

/* Clamp to the int16_t range: SATURATE, rather than wrap around. */
static q15
q15_sat (int32_t v)
{
  return v > INT16_MAX ? INT16_MAX : v < INT16_MIN ? INT16_MIN : (q15)v;
}

static q15
q15_from_float (double x)
{
  return q15_sat ((int32_t)lround (x * Q15_ONE));
}

static double
q15_to_float (q15 a)
{
  return (double)a / Q15_ONE;
}

/* a * b: the 32-bit product has 30 bits after the point; add half of
   the last place kept, to round, and shift 15 of them away. (Shifting a
   negative number right is implementation-defined in C; every compiler
   in this course shifts in copies of the sign bit, which divides by 2
   rounding down, as wanted here.) */
static q15
q15_mul (q15 a, q15 b)
{
  int32_t p = (int32_t)a * (int32_t)b; /* exact: fits in 31 bits */
  return q15_sat ((p + (1 << 14)) >> 15);
}

/* a + b, saturating. In plain int16_t, 0.75 + 0.5 would wrap to -0.75. */
static q15
q15_add (q15 a, q15 b)
{
  return q15_sat ((int32_t)a + (int32_t)b);
}

int
main (void)
{
  q15 half = q15_from_float (0.5), quarter = q15_from_float (0.25);
  printf ("0.5 is %d, 0.25 is %d, -1 is %d\n", half, quarter,
          q15_from_float (-1.0));
  printf ("1.0 saturates to %d = %.5f\n", q15_from_float (1.0),
          q15_to_float (q15_from_float (1.0)));
  printf ("0.5 * 0.25 = %.5f\n", q15_to_float (q15_mul (half, quarter)));
  printf ("-1 * -1 = %.5f (saturated: 1 doesn't fit)\n",
          q15_to_float (q15_mul (INT16_MIN, INT16_MIN)));
  q15 a = q15_from_float (0.75);
  /* Converting 40960 to int16_t is implementation-defined too; every
     compiler here wraps it around, to 40960 - 65536. */
  printf ("0.75 + 0.5 = %.5f saturated, %.5f wrapped\n",
          q15_to_float (q15_add (a, half)),
          q15_to_float ((q15)((int32_t)a + (int32_t)half)));

  /* A gain of 0.3 applied to a sine, in Q15 and in double: how big is
     the difference, compared with the signal? */
  q15    g   = q15_from_float (0.3);
  double sig = 0.0, err = 0.0;
  for (int i = 0; i < 48000; i++)
    {
      double x = 0.9 * sin (6.283185307179586 * 440.0 * (double)i / 48000.0);
      double exact = 0.3 * x;
      double fixed = q15_to_float (q15_mul (q15_from_float (x), g));
      sig += exact * exact;
      err += (fixed - exact) * (fixed - exact);
    }
  printf ("gain in Q15: error %.0f dB below the signal\n",
          -10.0 * log10 (err / sig));
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined q15.c -o q15 -lm
./q15
```

```
0.5 is 16384, 0.25 is 8192, -1 is -32768
1.0 saturates to 32767 = 0.99997
0.5 * 0.25 = 0.12500
-1 * -1 = 0.99997 (saturated: 1 doesn't fit)
0.75 + 0.5 = 0.99997 saturated, -0.75000 wrapped
gain in Q15: error 84 dB below the signal
```

## What it shows

- **The scale is a convention.** `16384` means 0.5 because everyone agreed to
  divide by 32768. Nothing in the type says so: the comments and the function
  names carry it.
- **+1 doesn't fit.** A 16-bit signed integer goes from −32768 to +32767, so
  Q15 covers −1 exactly but stops one step short of +1. Converting 1.0, or
  multiplying −1 by −1, saturates to 0.99997.
- **Multiply wide, then scale back.** Two Q15 numbers multiply to a Q30 number,
  30 bits after the point, which needs 32 bits. `q15_mul` widens both to
  `int32_t` first (an `int16_t` product would overflow), adds half of the last
  bit it will keep, which rounds instead of truncating (exercise 3), and shifts
  the extra 15 bits away.
- **Saturate, don't wrap.** 0.75 + 0.5 doesn't fit. Wrapped, as integer
  overflow does, it becomes −0.75: a full-scale jump to the other extreme,
  heard as a loud crack. Saturated, it stays at the largest value, heard as
  mild distortion. DSP processors saturate in hardware for exactly this reason.
- **84 dB of quality.** Q15 rounding adds noise about 84 dB below a loud
  signal: comparable to a CD, which is also 16 bits. Exercise 2 shows where
  that number comes from.

## Pitfalls

- **Shifting a negative number left.**
  [`broken-shift-negative.c`](broken-shift-negative.c) doubles −0.5 with
  `x << 1`. Shifting a negative value left is undefined behaviour in C:

  ```
  broken-shift-negative.c:16:17: runtime error: left shift of negative value -16384
  ```

  Fixed-point code reaches for shifts all the time. For signed values, multiply
  or divide by the power of two, and let the compiler choose the instruction;
  it will pick the shift when it is safe.

- **Shifting a negative number right** is only *implementation-defined*: it
  gives some answer, and the compiler's documentation says which. Every
  compiler this course uses shifts in copies of the sign bit, which divides by
  2 rounding down, and `q15_mul` relies on it, with a comment saying so.

- **Narrow intermediate results.** Sums of Q15 numbers easily pass 1 on the way
  to an answer that doesn't. Keep them in 32 bits and saturate once, at the end
  (exercise 1).

- **The `int16_t` product.** `a * b` with two `int16_t`s is computed as `int`
  (lesson 02's promotion), which happens to be 32 bits here. Writing the casts
  to `int32_t` says what is needed, instead of relying on `int`'s size.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Mix four Q15 signals of 0.4 each, then turn the mix down to a quarter. Keep
   the sum in Q15, then in an `int32_t`.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-headroom/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined mix.c -o mix
   ./mix
   ```

   ```
   sum kept in Q15:   0.99997, then x 0.25 = 0.25000
   sum kept in int32: 1.59998, then x 0.25 = 0.39999
   ```

   The true sum is 1.6, which Q15 can't hold, so the saturating Q15 sum stops
   at 0.99997, and a quarter of it is 0.25: wrong by a third, though the final
   answer, 0.4, would have fitted. An `int32_t` has room for sums up to 2 (and
   far beyond), so it carries 1.6 through, and only the final result is
   saturated. That spare room is **headroom**, and DSP processors build it into
   their accumulators: Texas Instruments' C55x DSPs, for one, add 40-bit
   accumulators to their 16-bit samples.

   </details>

2. Apply a gain of 0.3 to a sine with everything rounded to 7, 11, 15 and 23
   bits after the point. How far below the signal is the error each time?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-bits/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined bits.c -o bits -lm
   ./bits
   ```

   ```
   bits  format   error below signal
      7  Q7             36 dB
     11  Q11            60 dB
     15  Q15            84 dB
     23  Q23           132 dB
   ```

   Six decibels for every bit. Each bit halves the size of a step, and so of
   the rounding error, and half is −6.02 dB (lesson 02). The rule of thumb "6
   dB per bit", counting every bit, sign included, tells you how many bits a
   job needs: 8-bit audio has about 48 dB of range, 16-bit about 96, 24-bit
   about 144. The table sits about 12 dB below the rule at every width, because
   its signal peaks at 0.9 × 0.3 = 0.27, 11 dB below full scale, and three
   roundings add up to more error than one.

   </details>

3. After a Q15 multiply, 15 bits must go. Compare dropping them (`p >> 15`)
   with rounding first (`(p + (1 << 14)) >> 15`), averaged over a sine wave.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-rounding/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined rounding.c -o rounding -lm
   ./rounding
   ```

   ```
   average error, truncated: -0.499 of the last bit
   average error, rounded:   +0.000 of the last bit
   ```

   Dropping bits always rounds down, so the error is always negative, half a
   step on average: a constant offset, a DC bias, added to every sample. A
   filter that feeds its output back (lesson 21) adds that bias again on every
   pass, and it builds up. Rounding errs both ways equally, and the average is
   zero. It costs one addition.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                     | header      | what it does                                                       |
| ------------------------------------------------------------ | ----------- | ------------------------------------------------------------------ |
| [`fabs`](https://en.cppreference.com/c/numeric/math/fabs)    | `<math.h>`  | absolute value of a `double`                                       |
| [`ldexp`](https://en.cppreference.com/c/numeric/math/ldexp)  | `<math.h>`  | x times 2 to the power e                                           |
| [`log10`](https://en.cppreference.com/c/numeric/math/log10)  | `<math.h>`  | base-10 logarithm of a `double`                                    |
| [`lround`](https://en.cppreference.com/c/numeric/math/round) | `<math.h>`  | round to the nearest whole number, halves away from 0, as a `long` |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>` | formatted output to `stdout`                                       |
| [`round`](https://en.cppreference.com/c/numeric/math/round)  | `<math.h>`  | round to the nearest whole number, halves away from 0              |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)      | `<math.h>`  | sine of an angle in radians                                        |

<!-- /libfuncs -->
