/* fixed.c (exercise 1) -- leaky.c, with the free it was missing.
 *
 * block_new's comment says the caller must free what it returns, so the
 * caller does, before the last copy of the address goes out of scope.
 * LeakSanitizer then has nothing to report.
 */
#include <stdio.h>
#include <stdlib.h>

/* Returns n zeroed floats the caller must free, or NULL. */
static float *
block_new (size_t n)
{
  return calloc (n, sizeof (float));
}

int
main (void)
{
  for (int i = 0; i < 4; i++)
    {
      float *block = block_new (256);
      if (block == NULL)
        return 1;
      printf ("block %d: %zu bytes\n", i, (size_t)256 * sizeof *block);
      free (block); /* while its address is still in hand */
    }
  return 0;
}
