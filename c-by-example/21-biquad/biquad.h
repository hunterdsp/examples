/* biquad.h -- the biquad: a second-order IIR filter.
 *
 * An infinite impulse response (IIR) filter feeds its own past outputs
 * back in. A biquad uses two past inputs and two past outputs:
 *
 *   y[i] = b0 x[i] + b1 x[i-1] + b2 x[i-2] - a1 y[i-1] - a2 y[i-2]
 *
 * Five coefficients do what an FIR filter needs dozens of taps for.
 * The struct is public, as in lesson 07: it is small, holds no pointers,
 * and a caller can keep one on the stack or in an array, with no malloc.
 */
#ifndef BIQUAD_H
#define BIQUAD_H

#include <stddef.h>

/* What the filter IS. */
typedef struct biquad_coeffs
{
  float b0, b1, b2; /* feed-forward: how much of each input */
  float a1, a2;     /* feedback: how much of each past output */
} biquad_coeffs;

/* What it REMEMBERS: transposed direct form II needs two numbers. */
typedef struct biquad
{
  biquad_coeffs c;
  float         z1, z2;
} biquad_t;

/* Coefficients for a low-pass or high-pass filter with its corner at
   f0_hz, for a signal sampled at rate_hz. q sets the shape of the corner:
   0.7071 is the flattest passband (a Butterworth filter); higher q gives
   a peak just before the corner. Needs 0 < f0_hz < rate_hz / 2, q > 0. */
biquad_coeffs biquad_lowpass (double f0_hz, double q, double rate_hz);
biquad_coeffs biquad_highpass (double f0_hz, double q, double rate_hz);

/* Filter n samples from in into out; in and out may be the same. */
void biquad_process (biquad_t *f, const float *in, float *out, size_t n);

#endif /* BIQUAD_H */
