/* dsp.h -- level measurements on a buffer of samples.
 *
 * The include guard below means the body is seen once per translation
 * unit, however many times it is #included (meter.c does it twice, on
 * purpose, to show this).
 */
#ifndef DSP_H
#define DSP_H

#include <stddef.h>

/* A function-like macro, written defensively: every use of the argument
   and the whole expansion are in parentheses (see meter.c for why). */
#define DSP_SQUARE(x) ((x) * (x))

/* Root mean square: the "average loudness" of a buffer. n must be > 0. */
float dsp_rms (const float *buf, size_t n);

/* Largest absolute sample value. n must be > 0. */
float dsp_peak (const float *buf, size_t n);

/* An amplitude in decibels relative to full scale (1.0 = 0 dBFS). */
float dsp_dbfs (float amplitude);

/* Crest factor: peak over rms, in dB. How spiky a signal is: 0 dB for a
   square wave, whose every sample is at the peak. n must be > 0. */
float dsp_crest_db (const float *buf, size_t n);

#endif /* DSP_H */
