/* broken-not-self-contained.c -- meter.h included first, as it should be
 * possible to: the compiler meets size_t before anything has defined it.
 */
#include "meter.h"

#include <stdio.h>

float
meter_peak (const float *buf, size_t n)
{
  float peak = 0.0f;
  for (size_t i = 0; i < n; i++)
    peak = buf[i] > peak ? buf[i] : -buf[i] > peak ? -buf[i] : peak;
  return peak;
}

int
main (void)
{
  const float x[] = { 0.5f, -0.75f, 0.25f };
  printf ("peak %.2f\n", (double)meter_peak (x, 3));
  return 0;
}
