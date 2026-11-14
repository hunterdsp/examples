/* cplx.c -- complex numbers without <complex.h>.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       cplx.c -o cplx -lm && ./cplx
 *
 * Microsoft's C compiler has never supported C99's complex types, and
 * C11 made them optional, so portable libraries often bring their own: a
 * struct of two numbers and a handful of functions. Here is lesson 23's
 * tone measurement both ways, to show they agree.
 */
#include <complex.h>
#include <math.h>
#include <stdio.h>

typedef struct
{
  double re, im;
} cplx;

/* (a + bi)(c + di) = (ac - bd) + (ad + bc)i, because i*i = -1. */
static cplx
cmul (cplx a, cplx b)
{
  cplx r = { a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re };
  return r;
}

static cplx
cadd (cplx a, cplx b)
{
  cplx r = { a.re + b.re, a.im + b.im };
  return r;
}

/* e^(i theta) = cos theta + i sin theta (Euler's formula). */
static cplx
cpolar (double theta)
{
  cplx r = { cos (theta), sin (theta) };
  return r;
}

static const double PI   = 3.141592653589793;
static const double RATE = 48000.0;

enum
{
  N = 4800
};

int
main (void)
{
  static double x[N];
  for (size_t i = 0; i < N; i++)
    x[i] = 0.5 * cos (2.0 * PI * 1000.0 * (double)i / RATE + PI / 6.0);

  cplx           s1 = { 0.0, 0.0 };
  double complex s2 = 0.0;
  for (size_t i = 0; i < N; i++)
    {
      double theta = -2.0 * PI * 1000.0 * (double)i / RATE;
      cplx   xi    = { x[i], 0.0 };
      s1           = cadd (s1, cmul (xi, cpolar (theta)));
      s2 += x[i] * cexp (I * theta);
    }
  double scale = 2.0 / (double)N;
  printf ("struct:      %.4f %+.4fi\n", s1.re * scale, s1.im * scale);
  printf ("<complex.h>: %.4f %+.4fi\n", creal (s2) * scale,
          cimag (s2) * scale);
  printf ("agree within 1e-12: %s\n",
          fabs (s1.re - creal (s2)) * scale < 1e-12
                  && fabs (s1.im - cimag (s2)) * scale < 1e-12
              ? "yes"
              : "no");
  return 0;
}
