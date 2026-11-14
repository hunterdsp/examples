/* sig.c -- the implementation behind sig.h. */
#include "sig.h"

#include <math.h>
#include <stdlib.h>

/* Strings are built from the numbers, so the two can't disagree. */
#define SIG_STR_(x) #x
#define SIG_STR(x) SIG_STR_ (x)

const char *
sig_version (void)
{
  return SIG_STR (SIG_VERSION_MAJOR) "." SIG_STR (
      SIG_VERSION_MINOR) "." SIG_STR (SIG_VERSION_PATCH);
}

const char *
sig_strerror (sig_status s)
{
  switch (s)
    {
    case SIG_OK:
      return "success";
    case SIG_EINVAL:
      return "argument out of range";
    case SIG_ENOMEM:
      return "out of memory";
    }
  return "unknown status"; /* a value no sig_ function returns */
}

struct sig_lowpass
{
  float a; /* smoothing coefficient, from the cutoff */
  float y; /* the last output */
};

sig_status
sig_lowpass_create (double cutoff_hz, double rate_hz, sig_lowpass **out)
{
  *out = NULL;
  if (!(rate_hz > 0.0 && cutoff_hz > 0.0 && cutoff_hz < rate_hz / 2.0))
    return SIG_EINVAL;
  sig_lowpass *f = malloc (sizeof *f);
  if (f == NULL)
    return SIG_ENOMEM;
  /* The coefficient that puts the filter's corner at cutoff_hz. The
     caller thinks in Hz; converting is the library's job, done once. */
  const double pi = 3.14159265358979323846;
  f->a            = (float)(1.0 - exp (-2.0 * pi * cutoff_hz / rate_hz));
  f->y            = 0.0f;
  *out            = f;
  return SIG_OK;
}

void
sig_lowpass_destroy (sig_lowpass *f)
{
  free (f);
}

void
sig_lowpass_process (sig_lowpass *f, const float *in, float *out, size_t n)
{
  const float a = f->a;
  float       y = f->y;
  for (size_t i = 0; i < n; i++)
    {
      y += a * (in[i] - y); /* in[i] is read before out[i] is written, */
      out[i] = y;           /* so in == out is safe */
    }
  f->y = y;
}

void
sig_lowpass_reset (sig_lowpass *f)
{
  f->y = 0.0f;
}
