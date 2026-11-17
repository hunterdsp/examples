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
