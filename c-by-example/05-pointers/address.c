/* address.c -- a pointer, and writing through it.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       address.c -o address && ./address
 */
#include <stdio.h>

int
main (void)
{
  float  x = 1.0f;
  float *p = &x; /* p holds the address of x */

  printf ("x is %.1f\n", x);
  printf ("*p is %.1f\n", *p); /* the thing p points at: x */

  *p = 2.0f; /* write through p, and x changes */
  printf ("after *p = 2.0f, x is %.1f\n", x);
  return 0;
}
