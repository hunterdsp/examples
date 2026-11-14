/**
 * @file biquad.h
 * @brief Low-pass and high-pass biquad filters.
 *
 * Lesson 21's second-order IIR filter, designed with the formulas of
 * Robert Bristow-Johnson's "Audio EQ Cookbook" and run in transposed
 * direct form II.
 */
#ifndef SIG_BIQUAD_H
#define SIG_BIQUAD_H

#include <sig/common.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** Which frequencies a biquad passes. */
  typedef enum
  {
    SIG_LOWPASS, /**< Below the corner frequency. */
    SIG_HIGHPASS /**< Above the corner frequency. */
  } sig_biquad_type;

  /** A biquad filter. Opaque: made by sig_biquad_create(). */
  typedef struct sig_biquad sig_biquad;

  /**
   * Make a filter.
   *
   * @param type SIG_LOWPASS or SIG_HIGHPASS.
   * @param f0_hz The corner frequency. Needs 0 < f0_hz < rate_hz / 2.
   * @param q The shape of the corner, > 0. 0.7071 gives the flattest
   *          passband (a Butterworth filter); higher values peak just
   *          before the corner.
   * @param rate_hz The sample rate, in samples per second.
   * @param[out] out On SIG_OK, the new filter, owned by the caller; on any
   *                 error, NULL.
   * @return SIG_OK, SIG_EINVAL or SIG_ENOMEM.
   */
  SIG_API sig_status sig_biquad_create (sig_biquad_type type, double f0_hz,
                                        double q, double rate_hz,
                                        sig_biquad **out);

  /**
   * Free a filter.
   *
   * @param f The filter, or NULL, which does nothing.
   */
  SIG_API void sig_biquad_destroy (sig_biquad *f);

  /**
   * Filter n samples. in and out may be the same buffer (in place), but
   * must not otherwise overlap.
   *
   * @param f The filter.
   * @param in The n samples to filter.
   * @param[out] out Where the n filtered samples go.
   * @param n How many samples.
   */
  SIG_API void sig_biquad_process (sig_biquad *f, const float *in, float *out,
                                   size_t n);

  /**
   * Forget the past, as if just created.
   *
   * @param f The filter.
   */
  SIG_API void sig_biquad_reset (sig_biquad *f);

#ifdef __cplusplus
}
#endif

#endif /* SIG_BIQUAD_H */
