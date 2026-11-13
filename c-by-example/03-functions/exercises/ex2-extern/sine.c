/* sine.c -- the IMPLEMENTATION of sine.h. */
#include "sine.h"

#include <math.h>

/* M_PI is POSIX, not C99, so -std=c99 does not provide it: define our own. */
static const double TWO_PI = 6.283185307179586;

/* `static` at file scope means PRIVATE: visible in this file only. Nothing
   outside can reach `made` except through sine_tables_made(). */
int made = 0; /* exercise 2: no longer static, so any file can reach it */

void
sine_table (float out[], size_t n, double cycles)
{
  for (size_t i = 0; i < n; i++)
    out[i] = (float)sin (TWO_PI * cycles * (double)i / (double)n);
  made++;
}

int
sine_tables_made (void)
{
  return made;
}
