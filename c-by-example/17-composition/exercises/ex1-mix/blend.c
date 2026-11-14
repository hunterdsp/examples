/* blend.c -- half the dry signal, plus a smoothed copy.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       blend.c mix.c gain.c lowpass.c -o blend && ./blend
 */
#include "processor.h"

#include <stdio.h>

enum
{
  N = 8
};

int
main (void)
{
  processor_t *blend = mix_create (gain_create (0.5f), lowpass_create (0.1f));
  if (blend == NULL)
    return 1;
  float buf[N] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
  processor_process (blend, buf, N);
  printf ("%s:", processor_name (blend));
  for (size_t i = 0; i < N; i++)
    printf (" %.4f", (double)buf[i]);
  printf ("\n");
  processor_destroy (blend);
  return 0;
}
