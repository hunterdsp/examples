/* broken-null.c -- reading through a NULL pointer.
 *
 * find_peak returns NULL for an empty buffer, as documented, and the
 * caller forgot to check. Address 0 is never valid: the program crashes,
 * and the sanitizer says exactly where.
 */
#include <stddef.h>
#include <stdio.h>

static const float *
find_peak (const float *buf, size_t n)
{
  if (n == 0)
    return NULL;
  return buf;
}

int
main (int argc, char *argv[])
{
  (void)argv;
  float sig[4] = { 1, 2, 3, 4 };
  size_t n = (size_t)argc - 1; /* 0 when run with no arguments */
  const float *peak = find_peak (sig, n);
  printf ("peak %.2f\n", *peak); /* forgot: if (peak == NULL) ... */
  return 0;
}
