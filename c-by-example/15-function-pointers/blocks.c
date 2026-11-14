/* blocks.c -- a callback, and the void * that carries its state.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       blocks.c -o blocks && ./blocks
 *
 * for_each_block cuts a signal into blocks and calls a function on each:
 * a CALLBACK. Most callbacks need somewhere to keep what they find, and
 * for_each_block can't know what that is, so it passes along a void *
 * it was given, untouched. The caller knows what it points at; the
 * callback turns it back into that.
 */
#include <stdio.h>

/* The callback's type: a block, its length, and the caller's pointer. */
typedef void (*block_fn) (const float *blk, size_t n, void *user);

/* Call fn on each block of size samples (the last may be shorter),
   handing it user every time. */
static void
for_each_block (const float *buf, size_t n, size_t size, block_fn fn,
                void *user)
{
  for (size_t start = 0; start < n; start += size)
    {
      size_t len = n - start < size ? n - start : size;
      fn (buf + start, len, user);
    }
}

/* One callback: the peak of each block, appended to a log. */
typedef struct
{
  float  peak[8];
  size_t count;
} peak_log;

static void
log_peak (const float *blk, size_t n, void *user)
{
  peak_log *log  = user; /* we passed a peak_log *, so that is what it is */
  float     peak = 0.0f;
  for (size_t i = 0; i < n; i++)
    {
      float a = blk[i] < 0.0f ? -blk[i] : blk[i];
      if (a > peak)
        peak = a;
    }
  if (log->count < sizeof log->peak / sizeof log->peak[0])
    log->peak[log->count++] = peak;
}

/* Another: count samples beyond a limit. Different state, same type. */
typedef struct
{
  float  limit;
  size_t over;
} clip_count;

static void
count_clips (const float *blk, size_t n, void *user)
{
  clip_count *c = user;
  for (size_t i = 0; i < n; i++)
    if (blk[i] > c->limit || blk[i] < -c->limit)
      c->over++;
}

enum
{
  N = 20
};

int
main (void)
{
  /* A sawtooth: 0.0 to 0.6 in steps of 0.1, then twice as loud. */
  float buf[N];
  for (size_t i = 0; i < N; i++)
    buf[i] = (float)(i % (size_t)7) * (i < (size_t)7 ? 0.1f : 0.2f);

  peak_log log = { { 0.0f }, 0 };
  for_each_block (buf, N, 8, log_peak, &log);
  printf ("peaks of %zu blocks:", log.count);
  for (size_t b = 0; b < log.count; b++)
    printf (" %.1f", (double)log.peak[b]);
  printf ("\n");

  clip_count clips = { 1.0f, 0 };
  for_each_block (buf, N, 8, count_clips, &clips);
  printf ("samples beyond %.1f: %zu\n", (double)clips.limit, clips.over);
  return 0;
}
