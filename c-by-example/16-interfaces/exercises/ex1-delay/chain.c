/* chain.c -- lesson 16's chain, with a delay on the end.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       chain.c gain.c lowpass.c delay.c -o chain && ./chain
 *
 * Two lines differ from the lesson's chain.c: STAGES, and the array.
 *
 * main holds an array of processor_t *, and treats every element the
 * same way: through the interface. Which function actually runs is
 * decided by each processor's own table, at run time.
 */
#include "processor.h"

#include <stdio.h>

enum
{
  STAGES = 3,
  N      = 8
};

static void
show (const char *label, const float *buf)
{
  printf ("%-8s", label);
  for (size_t i = 0; i < N; i++)
    printf (" %.4f", (double)buf[i]);
  printf ("\n");
}

/* Run a step through every stage in order, printing after each. */
static void
run (processor_t *const *chain)
{
  float buf[N];
  for (size_t i = 0; i < N; i++)
    buf[i] = 1.0f;
  show ("in", buf);
  for (size_t s = 0; s < STAGES; s++)
    {
      processor_process (chain[s], buf, N);
      show (processor_name (chain[s]), buf);
    }
}

int
main (void)
{
  processor_t *chain[STAGES]
      = { gain_create (0.5f), lowpass_create (0.5f), delay_create () };
  for (size_t s = 0; s < STAGES; s++)
    if (chain[s] == NULL)
      {
        for (size_t k = 0; k < STAGES; k++)
          processor_destroy (chain[k]); /* accepts NULL */
        return 1;
      }

  run (chain);

  /* Without a reset, the low-pass would start this run where the last
     one ended, near 0.5. Reset every stage, whatever its kind: */
  for (size_t s = 0; s < STAGES; s++)
    processor_reset (chain[s]);
  printf ("reset, and again:\n");
  run (chain);

  for (size_t s = 0; s < STAGES; s++)
    processor_destroy (chain[s]);
  return 0;
}
