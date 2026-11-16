/* broken-complex-printf.c -- printing a complex number with %f.
 *
 * printf has no conversion for complex numbers. %f takes ONE double, and
 * a double complex is two, so the call reads the wrong bytes, or only
 * some of them. With -Wall the compiler checks every format against its
 * arguments and warns; -Werror makes that an error. Print the parts:
 * creal (z) and cimag (z).
 */
#include <complex.h>
#include <stdio.h>

int
main (void)
{
  double complex z = 3.0 + 4.0 * I;
  printf ("z = %f\n", z);
  return 0;
}
