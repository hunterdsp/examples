/* broken-sizeof.c -- an array parameter is only an address.
 *
 * `float buf[8]` in a parameter list receives only the address of the
 * first element. sizeof buf is then the size of an address (8), not of 8
 * floats (32), so this "length" is 2. The compiler knows, and says so.
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
