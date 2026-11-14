/* resolution.c -- how close to 440 Hz can an N-bit phase get?
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       resolution.c -o resolution -lm && ./resolution
 *
 * An N-bit phase accumulator can only step by whole numbers out of 2^N
 * per sample, so it plays the frequency nearest to the one asked for
 * that is a multiple of rate / 2^N. A cent is a hundredth of a semitone;
 * trained ears notice a few.
 */
#include <math.h>
#include <stdio.h>

int
main (void)
{
  const double rate = 48000.0, want = 440.0;
  const int    bits[] = { 8, 12, 16, 24, 32 };
  printf ("bits  step (Hz)    plays (Hz)  off by (cents)\n");
  for (size_t i = 0; i < sizeof bits / sizeof bits[0]; i++)
    {
      double cycle = pow (2.0, (double)bits[i]);
      double step  = rate / cycle;              /* smallest change */
      double inc   = floor (want / step + 0.5); /* nearest whole step */
      double plays = inc * step;
      double cents = 1200.0 * log2 (plays / want); /* 1200 per octave */
      printf ("%4d  %10.6f  %12.6f  %+13.4f\n", bits[i], step, plays, cents);
    }
  return 0;
}
