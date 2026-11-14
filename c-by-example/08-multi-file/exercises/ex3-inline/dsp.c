/* dsp.c -- the implementation of dsp.h. */
#include "dsp.h"

#include <math.h>

float
dsp_rms (const float *buf, size_t n)
{
  double sum = 0.0; /* accumulate in double: lesson 02 */
  for (size_t i = 0; i < n; i++)
    sum += (double)dsp_square (buf[i]);
  return (float)sqrt (sum / (double)n);
}

float
dsp_peak (const float *buf, size_t n)
{
  float peak = 0.0f;
  for (size_t i = 0; i < n; i++)
    {
      float a = fabsf (buf[i]);
      if (a > peak)
        peak = a;
    }
  return peak;
}

float
dsp_dbfs (float amplitude)
{
  return 20.0f * log10f (amplitude);
}
