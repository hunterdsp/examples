/* fir.h -- a finite impulse response (FIR) filter.
 *
 * Each output is a weighted sum of the last n inputs:
 *
 *   y[i] = h[0] x[i] + h[1] x[i-1] + ... + h[n-1] x[i-(n-1)]
 *
 * The weights h are the filter's TAPS; choosing them chooses what the
 * filter does. The filter keeps the last n inputs between calls, so a
 * signal may be processed in blocks of any size.
 */
#ifndef FIR_H
#define FIR_H

#include <stddef.h>

typedef struct fir fir_t;

/* A filter with a copy of the n taps (n > 0). The caller owns the
   result; free it with fir_destroy. NULL if memory is short. */
fir_t *fir_create (const float *taps, size_t n);

/* Free f. Accepts NULL. */
void fir_destroy (fir_t *f);

/* Filter n samples from in into out. in and out may be the same buffer. */
void fir_process (fir_t *f, const float *in, float *out, size_t n);

/* Forget past inputs: the next output is as if the input before it were
   all zeros. */
void fir_reset (fir_t *f);

#endif /* FIR_H */
