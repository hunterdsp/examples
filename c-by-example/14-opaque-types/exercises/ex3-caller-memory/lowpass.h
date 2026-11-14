/* lowpass.h -- the opaque low-pass filter, in memory the caller provides.
 *
 * lowpass_create calls malloc, which real-time code (lesson 13) must not
 * do except at setup, and maybe not at all. Here the caller asks how
 * many bytes a filter needs and hands them over: the struct stays
 * hidden, but where it lives is the caller's choice.
 */
#ifndef LOWPASS_H
#define LOWPASS_H

#include <stddef.h>

typedef struct lowpass lowpass_t;

/* Bytes one filter needs: a multiple of 16, so filters can sit side by
   side in one malloc'd block and each one is still aligned. */
size_t lowpass_size (void);

/* Set up a filter in mem, which must hold lowpass_size() bytes, aligned
   as malloc aligns. Returns mem as a lowpass_t *, or NULL if a is out of
   range (0 < a <= 1). The caller owns mem; there is no destroy. */
lowpass_t *lowpass_init (void *mem, float a);

void lowpass_process (lowpass_t *f, float *buf, size_t n);

#endif /* LOWPASS_H */
