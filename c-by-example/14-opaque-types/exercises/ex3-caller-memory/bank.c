/* bank.c -- four filters in one block: one malloc, one free.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       bank.c lowpass.c -o bank && ./bank
 */
#include "lowpass.h"

#include <stdio.h>
#include <stdlib.h>

enum
{
  FILTERS = 4,
  N       = 4
};

int
main (void)
{
  /* Setup: the only allocation. Each filter gets lowpass_size() bytes. */
  size_t         each  = lowpass_size ();
  unsigned char *block = malloc ((size_t)FILTERS * each);
  if (block == NULL)
    return 1;
  printf ("%zu bytes per filter, %zu in one block\n", each,
          (size_t)FILTERS * each);

  const float coef[FILTERS] = { 0.1f, 0.2f, 0.4f, 0.8f };
  lowpass_t  *bank[FILTERS];
  for (size_t k = 0; k < FILTERS; k++)
    bank[k] = lowpass_init (block + k * each, coef[k]);

  /* Processing: a step through each, and not one malloc. */
  for (size_t k = 0; k < FILTERS; k++)
    {
      float buf[N] = { 1.0f, 1.0f, 1.0f, 1.0f };
      lowpass_process (bank[k], buf, N);
      printf ("a = %.1f:", (double)coef[k]);
      for (size_t i = 0; i < N; i++)
        printf (" %.4f", (double)buf[i]);
      printf ("\n");
    }

  free (block); /* teardown: every filter at once */
  return 0;
}
