/* effects.c -- a chain inside a chain, and a ramp that is a gain.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       effects.c gain.c ramp.c lowpass.c chain.c -o effects && ./effects
 *
 * The whole effect is ONE processor_t *: a chain whose first stage is
 * itself a chain. main processes it, resets it and destroys it with the
 * same three calls as any other processor, and destroying it destroys
 * everything inside.
 */
#include "gain.h"
#include "processor.h"

#include <stdio.h>

enum
{
  N = 8
};

static void
show (const char *label, const float *buf)
{
  printf ("%-9s", label);
  for (size_t i = 0; i < N; i++)
    printf (" %.4f", (double)buf[i]);
  printf ("\n");
}

int
main (void)
{
  /* A fade-in over 4 samples, then a low-pass: the "voice". */
  processor_t *ramp            = ramp_create (1.0f, 4);
  processor_t *voice_stages[2] = { ramp, lowpass_create (0.5f) };
  processor_t *voice           = chain_create (voice_stages, 2);
  /* The voice, then a master gain: the whole effect. */
  processor_t *effect_stages[2] = { voice, gain_create (2.0f) };
  processor_t *effect           = chain_create (effect_stages, 2);
  if (effect == NULL)
    return 1; /* chain_create already destroyed every stage */

  float buf[N] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
  show ("in", buf);
  processor_process (effect, buf, N);
  show ("effect", buf);

  /* A ramp IS a gain: the same memory, read through the parent's type. */
  printf ("the ramp, as a gain: g = %.2f\n", (double)((gain_t *)ramp)->g);

  /* A method only ramps have checks the kind before using it. */
  printf ("ramp_set on the ramp:  %d\n", ramp_set (ramp, 0.5f, 2));
  printf ("ramp_set on the chain: %d\n", ramp_set (effect, 0.5f, 2));

  processor_destroy (effect); /* and with it, all five processors */
  return 0;
}
