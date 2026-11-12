/* broken-off-by-one.c -- one step past the end.
 *
 * An array of N elements has indices 0..N-1. `i <= n` visits N+1 of them,
 * and the last write lands on whatever happens to live after the array.
 * C does not check; AddressSanitizer does, and stops the program.
 */
#include <stdio.h>

static void
fill (float buf[], int n)
{
  for (int i = 0; i <= n; i++) /* should be i < n */
    buf[i] = 1.0f;
}

int
main (void)
{
  float buf[8];
  fill (buf, 8);
  printf ("%f\n", buf[0]);
  return 0;
}
