/* fades.c -- one fade, wrapped around two different kinds.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       fades.c fade.c gain.c lowpass.c chain.c -o fades && ./fades
 */
#include "processor.h"

#include <stdio.h>

enum
{
  N = 8
};

static void
run (const char *label, processor_t *p)
{
  float buf[N] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
  processor_process (p, buf, N);
  printf ("%-15s", label);
  for (size_t i = 0; i < N; i++)
    printf (" %.4f", (double)buf[i]);
  printf ("\n");
}

int
main (void)
{
  processor_t *gain      = fade_create (gain_create (0.5f), 4);
  processor_t *stages[2] = { gain_create (0.5f), lowpass_create (0.5f) };
  processor_t *chain     = fade_create (chain_create (stages, 2), 4);
  if (gain != NULL && chain != NULL)
    {
      run ("fade(gain):", gain);
      run ("fade(chain):", chain);
    }
  processor_destroy (gain);
  processor_destroy (chain);
  return 0;
}
