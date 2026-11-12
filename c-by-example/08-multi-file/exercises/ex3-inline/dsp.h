/* dsp.h -- level measurements on a buffer of samples.
 *
 * The include guard below means the body is seen once per translation
 * unit, however many times it is #included (meter.c does it twice, on
 * purpose, to show this).
 */
#ifndef DSP_H
#define DSP_H

#include <stddef.h>

/* Exercise 3: a function instead of the macro. Its argument is evaluated
   once, has a type the compiler checks, and needs no parentheses. Being
   `static inline` and in the header, every file that includes it gets
   its own private copy, which the compiler folds into the caller: as fast
   as the macro, and no duplicate definitions for the linker. */
static inline float
dsp_square (float x)
{
  return x * x;
}

/* Root mean square: the "average loudness" of a buffer. n must be > 0. */
float dsp_rms (const float *buf, size_t n);

/* Largest absolute sample value. n must be > 0. */
float dsp_peak (const float *buf, size_t n);

/* An amplitude in decibels relative to full scale (1.0 = 0 dBFS). */
float dsp_dbfs (float amplitude);

#endif /* DSP_H */
