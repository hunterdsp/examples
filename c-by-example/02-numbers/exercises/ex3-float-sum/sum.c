/* sum.c (exercise 3) -- adding 0.1 ten thousand times.
 *
 * Every addition rounds to the nearest value the type can hold. In a
 * float the error of each step is around a millionth, and ten thousand of
 * them add up. And 0.1f itself is not 0.1: it is the nearest float, a
 * little above it, which even a double sum faithfully adds up.
 */
#include <stdio.h>

int
main (void)
{
  float  f          = 0.0f;
  double d_of_float = 0.0, d = 0.0;
  for (int i = 0; i < 10000; i++)
    {
      f += 0.1f;          /* float sum of the float 0.1f */
      d_of_float += 0.1f; /* double sum of the same float 0.1f */
      d += 0.1;           /* double sum of the double 0.1 */
    }
  printf ("float  sum of 0.1f: %.6f\n", f);
  printf ("double sum of 0.1f: %.6f\n", d_of_float);
  printf ("double sum of 0.1 : %.6f\n", d);
  return 0;
}
