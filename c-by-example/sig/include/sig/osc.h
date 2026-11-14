/**
 * @file osc.h
 * @brief A sine oscillator whose phase never drifts.
 *
 * Lesson 22's oscillator: a 32-bit phase accumulator, so the phase wraps
 * at the end of every cycle by itself, and a 1024-entry sine table read
 * with linear interpolation. Integer adds are exact, so the phase gathers
 * no rounding error, however long it runs. The frequency is the one asked
 * for, rounded to a multiple of rate_hz / 2^32 (1.1e-5 Hz at 48 kHz),
 * and each sample is within 1e-5 of the sine of its phase. Against an
 * ideal sine at exactly the frequency asked for, that rounding adds an
 * error that grows with time: up to 3.3e-5 after one second at 48 kHz.
 */
#ifndef SIG_OSC_H
#define SIG_OSC_H

#include <sig/common.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** A sine oscillator. Opaque: made by sig_osc_create(). */
  typedef struct sig_osc sig_osc;

  /**
   * Make an oscillator.
   *
   * @param freq_hz The frequency to play. Needs 0 < freq_hz < rate_hz / 2.
   * @param rate_hz The sample rate, in samples per second.
   * @param[out] out On SIG_OK, the new oscillator, owned by the caller;
   *                 on any error, NULL.
   * @return SIG_OK, SIG_EINVAL or SIG_ENOMEM.
   */
  SIG_API sig_status sig_osc_create (double freq_hz, double rate_hz,
                                     sig_osc **out);

  /**
   * Free an oscillator.
   *
   * @param o The oscillator, or NULL, which does nothing.
   */
  SIG_API void sig_osc_destroy (sig_osc *o);

  /**
   * Write the next n samples of a sine of amplitude 1, starting at phase 0
   * on the first call and carrying on where the last call stopped.
   *
   * @param o The oscillator.
   * @param[out] out Where the n samples go.
   * @param n How many samples.
   */
  SIG_API void sig_osc_process (sig_osc *o, float *out, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* SIG_OSC_H */
