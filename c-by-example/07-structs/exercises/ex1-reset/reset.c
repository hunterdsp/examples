/* reset.c (exercise 1) -- clear a filter's memory, keep what it is.
 *
 * The coefficients are the filter's configuration; z1 and z2 are its
 * memory of the samples before. Resetting clears the memory only, so the
 * same filter can start again on a new signal, as if just made.
 */
#include <stdio.h>
#include <string.h> /* memcmp */

/* The coefficients: what the filter IS. */
struct biquad_coeffs
{
  float b0, b1, b2; /* feed-forward: how much of each input */
  float a1, a2;     /* feedback: how much of each past output */
};

/* The state: what the filter REMEMBERS between samples. */
struct biquad
{
  struct biquad_coeffs c;
  float z1, z2;
};

/* A typedef gives a struct type a one-word name. */
typedef struct biquad biquad_t;

/* By POINTER: the function updates the caller's state, and no copy of the
   struct is made. This is how every object in Part III is passed. */
static float
biquad_step (biquad_t *f, float x)
{
  /* Transposed direct form II; lesson 21 derives it. */
  float y = f->c.b0 * x + f->z1; /* p->m is (*p).m */
  f->z1 = f->c.b1 * x - f->c.a1 * y + f->z2;
  f->z2 = f->c.b2 * x - f->c.a2 * y;
  return y;
}

/* Clear the state; the coefficients are untouched. */
static void
biquad_reset (biquad_t *f)
{
  f->z1 = 0.0f;
  f->z2 = 0.0f;
}

int
main (void)
{
  biquad_t lp = {
    .c = { .b0 = 0.2f, .b1 = 0.4f, .b2 = 0.2f, .a1 = -0.4f, .a2 = 0.2f },
  };
  float first[8], second[8];

  for (int i = 0; i < 8; i++)
    first[i] = biquad_step (&lp, 1.0f);
  biquad_reset (&lp);
  for (int i = 0; i < 8; i++)
    second[i] = biquad_step (&lp, 1.0f);

  printf ("first :");
  for (int i = 0; i < 8; i++)
    printf (" %.3f", first[i]);
  printf ("\nsecond:");
  for (int i = 0; i < 8; i++)
    printf (" %.3f", second[i]);
  printf ("\nidentical: %s\n",
          memcmp (first, second, sizeof first) == 0 ? "yes" : "no");
  return 0;
}
