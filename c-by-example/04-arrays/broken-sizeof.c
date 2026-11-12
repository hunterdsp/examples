/* broken-sizeof.c -- an array parameter is a pointer.
 *
 * `float buf[8]` in a parameter list means `float *buf`. sizeof buf is
 * then the size of a pointer (8), not of 8 floats (32), so this "length"
 * is 2. The compiler knows, and says so.
 */
#include <stdio.h>

static int
length (float buf[8])
{
  return (int)(sizeof buf / sizeof buf[0]);
}

int
main (void)
{
  float buf[8] = { 0 };
  printf ("%d\n", length (buf));
  return 0;
}
