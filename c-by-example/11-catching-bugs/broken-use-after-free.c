/* broken-use-after-free.c -- reading a block after giving it back.
 *
 * free() returns the block to the allocator, which may hand it to the
 * next malloc. Reading it afterwards may still show the old samples, or
 * someone else's data: the program is wrong either way, and usually only
 * sometimes. AddressSanitizer keeps freed memory off limits for a while,
 * so the read is caught every time.
 */
#include <stdio.h>
#include <stdlib.h>

/* Returns the peak of buf, and (by mistake) frees it first. */
static float
peak_and_free (float *buf, size_t n)
{
  free (buf); /* too early: the loop below still reads buf */
  float peak = 0.0f;
  for (size_t i = 0; i < n; i++)
    if (buf[i] > peak)
      peak = buf[i];
  return peak;
}

int
main (void)
{
  size_t n   = 4;
  float *buf = malloc (n * sizeof *buf);
  if (buf == NULL)
    return 1;
  for (size_t i = 0; i < n; i++)
    buf[i] = (float)i;
  printf ("peak %.1f\n", peak_and_free (buf, n));
  return 0;
}
