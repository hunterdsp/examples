/* crest.c (exercise 2) -- how spiky is a sine, and a square?
 *
 * A sine spends most of its time below its peak, so its rms is lower:
 * peak / rms = sqrt(2), which is 3.01 dB. A square wave sits at its peak
 * all the time, so peak and rms are equal: 0 dB.
 */
#include <math.h>
#include <stdio.h>

#include "dsp.h"

int
main (void)
{
  const double two_pi = 6.283185307179586;
  float        sine[48], square[48];
  for (int i = 0; i < 48; i++)
    {
      sine[i]   = (float)sin (two_pi * i / 48.0); /* one cycle */
      square[i] = i < 24 ? 1.0f : -1.0f;
    }
  printf ("sine  : crest %.2f dB\n", dsp_crest_db (sine, 48));
  printf ("square: crest %.2f dB\n", dsp_crest_db (square, 48));
  return 0;
}
