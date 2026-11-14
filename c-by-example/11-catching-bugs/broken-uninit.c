/* broken-uninit.c -- a sum that never started at zero.
 *
 * A local variable holds whatever its memory held before (lesson 07), so
 * `sum` starts at garbage, and so does the result. The sanitizers in this
 * course don't check this, but the compiler does: -Wall warns that `sum`
 * is used uninitialised, and -Werror makes that stop the build.
 */
#include <stdio.h>

int
main (void)
{
  float buf[4] = { 0.0f, 1.0f, 2.0f, 3.0f };
  float sum; /* should be: float sum = 0.0f; */
  for (int i = 0; i < 4; i++)
    sum += buf[i];
  printf ("sum %.1f\n", sum);
  return 0;
}
