/* broken-assert.c -- calling mean() with nothing to average.
 *
 * mean() asserts n > 0. Here the caller asks for the mean of an empty
 * buffer, so the assert fires: the program stops at once, naming the
 * file, the line and the condition that was false. Without the assert,
 * mean would divide 0 by 0 and carry on with NaN.
 */
#include <assert.h>
#include <stdio.h>

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
main (int argc, char *argv[])
{
  (void)argv;
  float  buf[4] = { 0.0f, 1.0f, 2.0f, 3.0f };
  size_t n      = (size_t)argc - (size_t)1; /* 0: no arguments given */
  printf ("mean %.2f\n", mean (buf, n));
  return 0;
}
