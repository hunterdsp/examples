/* fft.h -- the fast Fourier transform, radix 2.
 *
 * The discrete Fourier transform (DFT) of n samples is n complex numbers,
 * one per frequency: bin k says how much of the frequency k cycles per n
 * samples is in the signal, and at what angle (lesson 23's measurement,
 * for every k at once). Done directly, that is n * n multiplies. The FFT
 * gets the same answer in about n log2 n, for n a power of two.
 *
 * Setup and use are separate, as lesson 13 asks: fft_plan_create
 * allocates and computes everything that depends only on n, once;
 * fft_forward then allocates nothing.
 */
#ifndef FFT_H
#define FFT_H

#include <complex.h>
#include <stddef.h>

typedef struct fft_plan fft_plan;

/* A plan for n-point transforms. NULL if n is not a power of two (2, 4,
   8, ...) or memory is short. Free it with fft_plan_destroy. */
fft_plan *fft_plan_create (size_t n);

void fft_plan_destroy (fft_plan *p);

/* The plan's n. */
size_t fft_size (const fft_plan *p);

/* Transform x, which holds fft_size (p) samples, in place: afterwards
   x[k] is bin k. */
void fft_forward (const fft_plan *p, double complex *x);

#endif /* FFT_H */
