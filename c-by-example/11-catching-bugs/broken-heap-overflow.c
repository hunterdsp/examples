/* broken-heap-overflow.c -- one sample past the end of a heap block.
 *
 * Lesson 04's off-by-one, on the heap: `i <= n` runs once too often and
 * writes buf[n], which is past the n samples malloc gave. The allocator
 * keeps its own records next to the block, so this write can corrupt them
 * and crash a later, innocent malloc. AddressSanitizer surrounds every
 * block with no-go bytes, and stops the program at the bad write.
 */
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
  (void)argv;
  size_t n   = (size_t)argc + (size_t)7; /* 8, but decided at run time */
  float *buf = malloc (n * sizeof *buf);
  if (buf == NULL)
    return 1;
  for (size_t i = 0; i <= n; i++) /* should be i < n */
    buf[i] = 0.0f;
  printf ("cleared %zu samples\n", n);
  free (buf);
  return 0;
}
