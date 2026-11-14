/* smooth.h -- another library that didn't, either. */
#ifndef SMOOTH_H
#define SMOOTH_H

#include <stddef.h>

typedef struct
{
  float a, y;
} filter;

void process (filter *f, float *buf, size_t n);

#endif /* SMOOTH_H */
