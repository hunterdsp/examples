/* broken-wrong-signature.c -- a function that doesn't fit the pointer.
 *
 * shaper_fn points at functions taking and returning float. half takes
 * and returns double: a different type of function. Called through a
 * shaper_fn it would be handed a float where it reads a double, and its
 * double result read back as a float. The compiler refuses the
 * assignment; a cast would silence it, and the call would then be
 * undefined behaviour.
 */
#include <stdio.h>

typedef float (*shaper_fn) (float x);

static double
half (double x)
{
  return x * 0.5;
}

int
main (void)
{
  shaper_fn f = half; /* wrong type: double (*)(double) */
  printf ("%f\n", (double)f (1.0f));
  return 0;
}
