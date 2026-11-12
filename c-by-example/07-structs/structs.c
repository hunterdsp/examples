/* structs.c -- grouping values that belong together.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       structs.c -o structs && ./structs
 *
 * A biquad is the workhorse IIR filter of lesson 21: five coefficients
 * and two numbers of memory. Passing seven loose floats to every call
 * invites mistakes; a struct keeps them together, named.
 */
#include <stddef.h> /* offsetof */
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

/* By VALUE: a copy arrives, so the caller's struct cannot change. */
static float
dc_gain (struct biquad_coeffs c)
{
  return (c.b0 + c.b1 + c.b2) / (1.0f + c.a1 + c.a2);
}

struct padded
{
  char tag;     /* 1 byte, then 7 bytes of padding so... */
  double value; /* ...this lands on a multiple of 8 */
  char flag;    /* 1 byte, then 7 more so an ARRAY of these stays aligned */
};

struct packed
{
  double value; /* largest first: no gaps */
  char tag;
  char flag;
};

int
main (void)
{
  /* Designated initializers: name each field. Anything not named is 0, so
     z1 and z2 start cleared. This one is a gentle low-pass filter. */
  biquad_t lp = {
    .c = { .b0 = 0.2f, .b1 = 0.4f, .b2 = 0.2f, .a1 = -0.4f, .a2 = 0.2f },
  };
  printf ("DC gain %.3f, state starts at z1=%.1f z2=%.1f\n", dc_gain (lp.c),
          lp.z1, lp.z2);

  printf ("step response:");
  for (int i = 0; i < 8; i++)
    printf (" %.3f", biquad_step (&lp, 1.0f));
  printf ("\n");

  /* Assignment copies the whole struct (unlike an array, lesson 04). */
  biquad_t copy = lp;
  copy.z1 = 0.0f;
  printf ("copy is independent: lp.z1 = %.3f, copy.z1 = %.1f\n", lp.z1,
          copy.z1);

  printf ("sizeof(struct padded) = %zu, sizeof(struct packed) = %zu\n",
          sizeof (struct padded), sizeof (struct packed));
  printf ("offsetof(struct padded, value) = %zu\n",
          offsetof (struct padded, value));
  return 0;
}
