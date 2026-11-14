/* lowpass.c -- the filter, in memory it is given. */
#include "lowpass.h"

struct lowpass
{
  float a;
  float y;
};

size_t
lowpass_size (void)
{
  /* Round up to the next multiple of 16 (lesson 13's arena_alloc). */
  return (sizeof (struct lowpass) + (size_t)15) & ~(size_t)15;
}

lowpass_t *
lowpass_init (void *mem, float a)
{
  if (!(a > 0.0f && a <= 1.0f))
    return NULL;
  lowpass_t *f = mem;
  f->a         = a;
  f->y         = 0.0f;
  return f;
}

void
lowpass_process (lowpass_t *f, float *buf, size_t n)
{
  const float a = f->a;
  float       y = f->y;
  for (size_t i = 0; i < n; i++)
    {
      y += a * (buf[i] - y);
      buf[i] = y;
    }
  f->y = y;
}
