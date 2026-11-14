/**
 * @file fft.h
 * @brief The amplitude spectrum of a block of samples.
 *
 * Lesson 24's radix-2 FFT, behind an interface that takes real samples
 * and gives back the amplitude of each frequency: no complex numbers
 * cross the API, so any language that can pass an array of floats can
 * call it.
 */
#ifndef SIG_FFT_H
#define SIG_FFT_H

#include <sig/common.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** An FFT of one size. Opaque: made by sig_fft_create(). */
  typedef struct sig_fft sig_fft;

  /**
   * Make an FFT for blocks of n samples. Everything that depends only on
   * n is computed here, once.
   *
   * @param n The block size: a power of two, at least 2.
   * @param[out] out On SIG_OK, the new FFT, owned by the caller; on any
   *                 error, NULL.
   * @return SIG_OK, SIG_EINVAL or SIG_ENOMEM.
   */
  SIG_API sig_status sig_fft_create (size_t n, sig_fft **out);

  /**
   * Free an FFT.
   *
   * @param p The FFT, or NULL, which does nothing.
   */
  SIG_API void sig_fft_destroy (sig_fft *p);

  /**
   * The block size the FFT was made for.
   *
   * @param p The FFT.
   * @return Its n.
   */
  SIG_API size_t sig_fft_size (const sig_fft *p);

  /**
   * The amplitude of each frequency in a block of samples. Bin k is the
   * frequency k * rate / n; a sine of amplitude A that lands exactly on a
   * bin shows up there as A. The FFT keeps its working space inside p, so
   * one sig_fft must not be used by two threads at once.
   *
   * @param p The FFT.
   * @param in The n samples.
   * @param[out] amp Where the n / 2 + 1 amplitudes go, for bins 0 (the
   *                 average) to n / 2 (half the sample rate).
   */
  SIG_API void sig_fft_amplitude (sig_fft *p, const float *in, float *amp);

#ifdef __cplusplus
}
#endif

#endif /* SIG_FFT_H */
