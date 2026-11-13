/* sizes.c (exercise 3) -- every pointer is the same size.
 *
 * A pointer holds an address, and an address does not depend on what is
 * stored there. The pointed-to TYPE only tells the compiler how to read
 * what it finds, and how far p + 1 steps.
 */
#include <stdio.h>

int
main (void)
{
  printf ("sizeof(float *)  = %zu\n", sizeof (float *));
  printf ("sizeof(char *)   = %zu\n", sizeof (char *));
  printf ("sizeof(double *) = %zu\n", sizeof (double *));
  printf ("but sizeof(float) = %zu and sizeof(char) = %zu\n", sizeof (float),
          sizeof (char));
  return 0;
}
