/* feedback.c (exercise 2) -- the same biquad with stronger feedback.
 *
 * a1 and a2 feed the filter's past outputs back in. With a1 = -1.8,
 * a2 = 0.9 each output leans hard on the last two, so the response rings
 * for a long time before it settles. Push a2 past 1 and every ring is
 * bigger than the one before: the filter is UNSTABLE, and its output
 * grows without limit. Lesson 21 says which coefficients are safe.
 */
#include <stdio.h>

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

static void
step_response (const char *label, struct biquad_coeffs c, int steps)
{
  biquad_t f = { .c = c }; /* z1 and z2 start at 0 */
  printf ("%s\n", label);
  for (int i = 0; i < steps; i++)
    printf ("%9.2f%s", biquad_step (&f, 1.0f), i % 8 == 7 ? "\n" : "");
}

int
main (void)
{
  struct biquad_coeffs ringing
      = { .b0 = 0.2f, .b1 = 0.4f, .b2 = 0.2f, .a1 = -1.8f, .a2 = 0.9f };
  struct biquad_coeffs unstable = ringing;
  unstable.a2 = 1.1f;

  step_response ("a1 = -1.8, a2 = 0.9:", ringing, 40);
  step_response ("a1 = -1.8, a2 = 1.1:", unstable, 40);
  return 0;
}
