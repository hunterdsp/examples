/* shapes.c -- saw, square and triangle from one phase accumulator.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       shapes.c -o shapes && ./shapes
 *
 * The phase is a number that climbs steadily and wraps: plotted, it IS a
 * sawtooth. Every other basic waveform is a little arithmetic on it. At
 * 6 kHz and 48 kHz sampling, one cycle is exactly 8 samples.
 */
#include <stdint.h>
#include <stdio.h>

static const double CYCLE = 4294967296.0; /* 2^32 */

int
main (void)
{
  uint32_t phase = 0;
  uint32_t inc   = (uint32_t)(6000.0 / 48000.0 * CYCLE); /* 2^32 / 8 */

  printf ("phase     saw  square  triangle\n");
  for (int i = 0; i < 8; i++)
    {
      double p      = (double)phase / CYCLE; /* 0 .. 1 */
      double saw    = 2.0 * p - 1.0;         /* -1 .. 1, rising */
      double square = p < 0.5 ? 1.0 : -1.0;  /* top half, bottom half */
      double tri    = 1.0 - 2.0 * (saw < 0.0 ? -saw : saw); /* fold saw */
      printf ("%.3f  %6.2f  %6.2f  %8.2f\n", p, saw, square, tri);
      phase += inc;
    }
  return 0;
}
