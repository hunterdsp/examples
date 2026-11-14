/* osc.c -- a sine oscillator with a 32-bit phase accumulator.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       osc.c -o osc -lm && ./osc
 *
 * The oscillator's PHASE says how far through the current cycle it is.
 * Here a whole cycle is the whole range of a uint32_t: 0 is the start,
 * 2^32 would be the start of the next, and since unsigned arithmetic
 * wraps at 2^32 (C defines it to), the phase wraps exactly where a cycle
 * ends, by itself, every time. Each sample adds the same INCREMENT;
 * the top 10 bits pick an entry of a 1024-entry sine table, and the low
 * 22 bits say how far it is to the next one.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>

enum
{
  TABLE_BITS = 10,
  TABLE      = 1 << TABLE_BITS, /* 1024 */
  FRAC_BITS  = 32 - TABLE_BITS  /* 22 */
};

static const double TWO_PI = 6.283185307179586;
static const double RATE   = 48000.0;
static const double CYCLE  = 4294967296.0; /* 2^32: one whole cycle */

static float table[TABLE + 1]; /* [TABLE] repeats [0], for k + 1 */

typedef struct
{
  uint32_t phase; /* where in the cycle, 0 .. 2^32 - 1 */
  uint32_t inc;   /* how far one sample moves it */
} osc_t;

static osc_t
osc_make (double freq_hz)
{
  osc_t o = { 0, (uint32_t)(freq_hz / RATE * CYCLE + 0.5) };
  return o;
}

static float
osc_next (osc_t *o)
{
  uint32_t k    = o->phase >> FRAC_BITS; /* top 10 bits: the entry */
  float    frac = (float)(o->phase & ((UINT32_C (1) << FRAC_BITS) - 1u))
                  / (float)(UINT32_C (1) << FRAC_BITS);
  o->phase += o->inc; /* wraps at 2^32: no if, no fmod */
  return table[k] + frac * (table[k + 1] - table[k]);
}

int
main (void)
{
  for (size_t k = 0; k <= TABLE; k++)
    table[k] = (float)sin (TWO_PI * (double)k / (double)TABLE);

  osc_t o = osc_make (440.0);
  printf ("440 Hz: increment %u of 2^32 per sample\n", (unsigned)o.inc);
  printf ("frequency played: %.4f Hz (steps of %.2e Hz)\n",
          (double)o.inc * RATE / CYCLE, RATE / CYCLE);

  /* Compare with sin() of a phase kept in double, over one second. */
  double worst = 0.0;
  for (size_t i = 0; i < (size_t)RATE; i++)
    {
      double t = (double)o.inc / CYCLE * (double)i; /* cycles so far */
      double d = fabs ((double)osc_next (&o) - sin (TWO_PI * t));
      worst    = d > worst ? d : worst;
    }
  printf ("one second: largest difference from sin() below 1e-5: %s\n",
          worst < 1e-5 ? "yes" : "no");
  printf ("after one second, the phase is at %.6f of a cycle\n",
          (double)o.phase / CYCLE);

  /* The tempting alternative: time in seconds, as a float. */
  const float when[] = { 1.0f, 60.0f, 600.0f, 3600.0f };
  for (size_t i = 0; i < sizeof when / sizeof when[0]; i++)
    {
      float t    = when[i];
      float next = t + (float)(1.0 / RATE); /* one sample later */
      printf ("float time at %5.0f s: next sample %s\n", (double)t,
              next == t ? "is the SAME time" : "is later");
    }
  return 0;
}
