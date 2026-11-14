/* lowpass.h -- a one-pole low-pass filter, as an OPAQUE type.
 *
 * The header names the type but never says what is inside it:
 *
 *   typedef struct lowpass lowpass_t;
 *
 * declares `struct lowpass` without defining it. A file that includes
 * this header may hold a lowpass_t * and pass it around, but cannot
 * create one, copy one, or look inside one: only lowpass.c, which has
 * the definition, can. So the functions below are the ONLY way in, and
 * lowpass.c is free to change what the struct holds without breaking a
 * single caller.
 */
#ifndef LOWPASS_H
#define LOWPASS_H

#include <stddef.h>

typedef struct lowpass lowpass_t; /* declared here, defined in lowpass.c */

/* Make a filter with smoothing coefficient a, 0 < a <= 1: each output
   moves the fraction a of the way from the last output to the input.
   Small a smooths more. The caller owns the result and must pass it to
   lowpass_destroy. Returns NULL if a is out of range or memory is out. */
lowpass_t *lowpass_create (float a);

/* Free the filter. Accepts NULL. */
void lowpass_destroy (lowpass_t *f);

/* Filter n samples of buf in place. The filter remembers its last output,
   so a signal processed in blocks gives the same result as in one go. */
void lowpass_process (lowpass_t *f, float *buf, size_t n);

/* Forget the past: the next output starts again from 0. */
void lowpass_reset (lowpass_t *f);

#endif /* LOWPASS_H */
