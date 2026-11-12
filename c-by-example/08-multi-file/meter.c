/* meter.c -- a level meter, built from more than one file.
 *
 *   make && ./meter                       (see the Makefile)
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       meter.c dsp.c -o meter -lm        (the same, by hand)
 */
#include <stdio.h>

#include "dsp.h"
#include "dsp.h" /* twice, on purpose: the include guard makes it harmless */

/* The naive version of DSP_SQUARE, without the parentheses. */
#define BAD_SQUARE(x) x * x

int
main (void)
{
  /* A full-scale square wave, then the same at half amplitude. */
  float loud[] = { 1, -1, 1, -1, 1, -1, 1, -1 };
  float quiet[] = { 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f };
  size_t n = sizeof loud / sizeof loud[0];

  printf ("loud : rms %.3f, peak %.3f = %6.2f dBFS\n", dsp_rms (loud, n),
          dsp_peak (loud, n), dsp_dbfs (dsp_peak (loud, n)));
  printf ("quiet: rms %.3f, peak %.3f = %6.2f dBFS\n", dsp_rms (quiet, n),
          dsp_peak (quiet, n), dsp_dbfs (dsp_peak (quiet, n)));

  /* Macros are text substitution, done before the compiler sees the code.
     BAD_SQUARE(1 + 2) becomes 1 + 2 * 1 + 2, which is 5. */
  printf ("BAD_SQUARE(1 + 2) = %d, DSP_SQUARE(1 + 2) = %d\n",
          BAD_SQUARE (1 + 2), DSP_SQUARE (1 + 2));
  return 0;
}
