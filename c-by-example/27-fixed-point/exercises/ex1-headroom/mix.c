/* mix.c -- mixing four signals, then turning the mix down.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       mix.c -o mix && ./mix
 *
 * Four signals at 0.4 add up to 1.6, which Q15 can't hold. Turned down
 * by a quarter afterwards, the right answer, 0.4, fits again. Whether
 * the program gets it depends on where the sum was kept on the way.
 */
#include <stdint.h>
#include <stdio.h>

typedef int16_t q15;

static q15
q15_sat (int32_t v)
{
  return v > INT16_MAX ? INT16_MAX : v < INT16_MIN ? INT16_MIN : (q15)v;
}

int
main (void)
{
  const q15 in[4]   = { 13107, 13107, 13107, 13107 }; /* 0.4 each */
  const q15 quarter = 8192;                           /* 0.25 */

  /* In Q15 all the way: each addition saturates. */
  q15 narrow = 0;
  for (int k = 0; k < 4; k++)
    narrow = q15_sat ((int32_t)narrow + (int32_t)in[k]);
  q15 narrow_out
      = q15_sat (((int32_t)narrow * (int32_t)quarter + (1 << 14)) >> 15);

  /* In an int32_t: room for sums up to 65536, saturated only at the end. */
  int32_t wide = 0;
  for (int k = 0; k < 4; k++)
    wide += in[k];
  q15 wide_out = q15_sat ((wide * (int32_t)quarter + (1 << 14)) >> 15);

  printf ("sum kept in Q15:   %.5f, then x 0.25 = %.5f\n",
          (double)narrow / 32768.0, (double)narrow_out / 32768.0);
  printf ("sum kept in int32: %.5f, then x 0.25 = %.5f\n",
          (double)wide / 32768.0, (double)wide_out / 32768.0);
  return 0;
}
