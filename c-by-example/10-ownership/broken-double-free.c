/* broken-double-free.c -- two names for one block, and both free it.
 *
 * `copy = buf` copies the ADDRESS, not the samples (lesson 05), so both
 * names point at the same block. Each free looks reasonable on its own;
 * together they give the block back twice, which corrupts the allocator's
 * bookkeeping. gcc sees it while compiling; where it doesn't,
 * AddressSanitizer stops the program at the second free.
 */
#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
  float *buf = malloc (8 * sizeof *buf);
  if (buf == NULL)
    return 1;
  buf[0]      = 1.0f;
  float *copy = buf; /* a second name, not a second block */
  printf ("copy[0] = %.1f, the same sample\n", copy[0]);
  free (buf);
  free (copy);
  return 0;
}
