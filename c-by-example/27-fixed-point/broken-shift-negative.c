/* broken-shift-negative.c -- doubling a negative Q15 number with <<.
 *
 * Shifting left by one doubles a number, and for a positive one that is
 * fine. For a NEGATIVE one, C99 says the result is undefined: x << 1 is
 * only defined when x * 2 fits and x is not negative. Fixed-point code
 * reaches for shifts constantly; for scaling signed values, multiply
 * (x * 2), and let the compiler pick the instruction.
 */
#include <stdint.h>
#include <stdio.h>

int
main (void)
{
  int16_t x = -16384; /* -0.5 in Q15 */
  int     y = x << 1; /* -1.0, maybe: undefined behaviour */
  printf ("%d\n", y);
  return 0;
}
