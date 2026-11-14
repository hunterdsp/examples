/* shape.c -- lesson 15's waveshapers, with a fourth: a rectifier.
 *
 * Adding it took one function and one row of the table. Nothing that
 * loops over the table changed, because it counts the rows itself.
 *
 * A waveshaper maps each sample through a curve: it is how a guitar
 * pedal distorts. Every curve here has the same signature, float in and
 * float out, so a pointer to any of them has the same type, shaper_fn,
 * and one loop can apply whichever it is handed.
 */
#include <stdio.h>

/* The type of a pointer to a function taking a float and returning one.
   Read it inside out: shaper_fn is a pointer (*) to a function (float)
   returning float. */
typedef float (*shaper_fn) (float x);

/* Hard clip: anything past +-1 is cut flat. */
static float
hard (float x)
{
  if (x > 1.0f)
    return 1.0f;
  if (x < -1.0f)
    return -1.0f;
  return x;
}

/* Soft clip: a cubic that bends smoothly into +-1 instead of cutting. */
static float
soft (float x)
{
  if (x > 1.0f)
    return 1.0f;
  if (x < -1.0f)
    return -1.0f;
  return 1.5f * x - 0.5f * x * x * x;
}

/* Fold: past +-1 the signal is reflected back, like a mirror. */
static float
fold (float x)
{
  if (x > 1.0f)
    return 2.0f - x;
  if (x < -1.0f)
    return -2.0f - x;
  return x;
}

/* Half-wave rectifier: negative samples become 0, as a single diode
   would make them. */
static float
rectify (float x)
{
  return x < 0.0f ? 0.0f : x;
}

/* Apply f to every sample of buf, in place. apply doesn't know or care
   which curve f is: it calls whatever function the pointer points at. */
static void
apply (float *buf, size_t n, shaper_fn f)
{
  for (size_t i = 0; i < n; i++)
    buf[i] = f (buf[i]);
}

/* A table of names and functions: adding a shaper is adding a row. A
   function's name, used without (), is its address. */
static const struct
{
  const char *name;
  shaper_fn   fn;
} shapers[] = {
  { "hard", hard },
  { "soft", soft },
  { "fold", fold },
  { "rect", rectify },
};

enum
{
  N = 9
};

int
main (void)
{
  float x[N];
  for (int i = 0; i < N; i++)
    x[i] = (float)(i - 4) * 0.5f; /* -2.0, -1.5, ..., 2.0 */

  printf ("in  ");
  for (int i = 0; i < N; i++)
    printf (" %5.2f", (double)x[i]);
  printf ("\n");

  size_t count = sizeof shapers / sizeof shapers[0];
  for (size_t k = 0; k < count; k++)
    {
      float buf[N];
      for (int i = 0; i < N; i++)
        buf[i] = x[i];
      apply (buf, N, shapers[k].fn);
      printf ("%s", shapers[k].name);
      for (int i = 0; i < N; i++)
        printf (" %5.2f", (double)buf[i]);
      printf ("\n");
    }

  shaper_fn f = soft; /* a variable like any other: assign, compare, call */
  printf ("f == soft: %s; f(0.5) = %.4f\n", f == soft ? "yes" : "no",
          (double)f (0.5f));
  return 0;
}
