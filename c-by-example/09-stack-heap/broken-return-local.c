/* broken-return-local.c -- returning the address of a local array.
 *
 * buf lives in tone_on_stack's stack frame, and the frame is reused the
 * moment the function returns. The caller would get the address of memory
 * that no longer belongs to it. The compiler sees it, and -Werror stops
 * the build.
 */
#include <stdio.h>

static float *
tone_on_stack (void)
{
  float buf[4] = { 0.0f, 0.5f, 1.0f, 0.5f };
  return buf; /* buf dies here */
}

int
main (void)
{
  float *p = tone_on_stack ();
  printf ("%.1f\n", p[1]);
  return 0;
}
