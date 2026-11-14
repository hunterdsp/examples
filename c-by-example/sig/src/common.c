/* common.c -- status codes, the version, and shared checks. */
#include <sig/common.h>

#include "internal.h"

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

int
sig__freq_ok (double freq_hz, double rate_hz)
{
  return rate_hz > 0.0 && freq_hz > 0.0 && freq_hz < rate_hz / 2.0;
}
