/* naive.c (exercise 1) -- the gain stage, allocating its ramp per block.
 *
 * The obvious way to write gain_process: malloc a ramp the size of this
 * block, use it, free it. It gives the same output as realtime.c, and
 * makes a malloc and a free on every block, which is exactly what a
 * real-time callback must not do.
 */
#include <stdio.h>
#include <stdlib.h>

static size_t n_mallocs; /* every malloc this program makes */

static void *
counted_malloc (size_t bytes)
{
  n_mallocs++;
  return malloc (bytes);
}

typedef struct
{
  float gain;
} gain_t;

/* Allocates on every call: an unbounded wait, inside the deadline. */
static int
gain_process (gain_t *g, const float *in, float *out, size_t n, float target)
{
  float *ramp = counted_malloc (n * sizeof *ramp);
  if (ramp == NULL)
    return -1; /* and the block is lost: silence, or a click */
  float step = (target - g->gain) / (float)n;
  for (size_t i = 0; i < n; i++)
    ramp[i] = g->gain + step * ((float)i + 1.0f);
  for (size_t i = 0; i < n; i++)
    out[i] = in[i] * ramp[i];
  free (ramp);
  g->gain = target;
  return 0;
}

int
main (void)
{
  static float in[256], out[256];
  for (size_t i = 0; i < (size_t)256; i++)
    in[i] = 1.0f;
  gain_t g = { 1.0f };

  size_t sizes[3] = { 64, 128, 256 };
  size_t n        = 0;
  for (int b = 0; b < 1000; b++)
    {
      n = sizes[b % 3];
      if (gain_process (&g, in, out, n, b % 2 == 0 ? 0.5f : 1.0f) != 0)
        return 1;
    }
  printf ("1000 blocks processed: %zu mallocs, and as many frees\n",
          n_mallocs);
  printf ("last block: out[0] %.4f, out[%zu] %.4f (the same as realtime.c)\n",
          out[0], n - (size_t)1, out[n - (size_t)1]);
  return 0;
}
