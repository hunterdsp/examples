/* fir.h -- a library that didn't prefix its names. */
#ifndef FIR_H
#define FIR_H

#include <stddef.h>

typedef struct
{
  float taps[4];
} filter;

void process (filter *f, float *buf, size_t n);

#endif /* FIR_H */
