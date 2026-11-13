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
  int64_t per_day = (int64_t)48000 * (int64_t)86400;
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
