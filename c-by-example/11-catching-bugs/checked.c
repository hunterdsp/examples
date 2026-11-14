/* checked.c -- a program that states its assumptions, so tools can check
 * them.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       checked.c -o checked && ./checked
 *
 * Built like this, four checkers watch every run: the compiler's warnings
 * (-Wall -Wextra), AddressSanitizer (bad memory accesses and leaks),
 * UndefinedBehaviorSanitizer (overflow, bad shifts, out-of-bounds
 * indexes) and the program's own assert()s. This one passes all four;
 * the broken-*.c files beside it each fail one.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* The mean of n samples. An empty buffer has no mean, so n == 0 is the
   CALLER's bug, not a case to handle: assert says so, in code, and stops
   the program with the file and line if it ever happens. */
static float
mean (const float *buf, size_t n)
{
  assert (buf != NULL);
  assert (n > 0);
  double sum = 0.0;
  for (size_t i = 0; i < n; i++)
    sum += (double)buf[i];
  return (float)(sum / (double)n);
}

int
main (void)
{
#ifdef NDEBUG
  printf ("asserts: off (built with -DNDEBUG)\n");
#else
  printf ("asserts: on\n");
#endif

  size_t n   = 8;
  float *buf = malloc (n * sizeof *buf);
  if (buf == NULL)
    return 1;
  for (size_t i = 0; i < n; i++)
    buf[i] = (float)i; /* 0 1 2 ... 7: every element written, none past */
  printf ("mean of 0..7 = %.2f\n", mean (buf, n));
  free (buf); /* freed once, and never touched again */
  printf ("no checker complained\n");
  return 0;
}
