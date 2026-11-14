/* pick.c -- choose a shaper by name, at run time.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       pick.c -o pick && ./pick soft 0.5
 *
 * The table maps names to functions, so turning a word typed on the
 * command line into a function to call is a search of the table. An
 * unknown name gives NULL, which must never be called.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef float (*shaper_fn) (float x);

static float
hard (float x)
{
  return x > 1.0f ? 1.0f : x < -1.0f ? -1.0f : x;
}

static float
soft (float x)
{
  if (x > 1.0f)
    return 1.0f;
  if (x < -1.0f)
    return -1.0f;
  return 1.5f * x - 0.5f * x * x * x;
}

static float
fold (float x)
{
  return x > 1.0f ? 2.0f - x : x < -1.0f ? -2.0f - x : x;
}

static const struct
{
  const char *name;
  shaper_fn   fn;
} shapers[] = {
  { "hard", hard },
  { "soft", soft },
  { "fold", fold },
};

enum
{
  COUNT = sizeof shapers / sizeof shapers[0]
};

/* The shaper called name, or NULL if there is none. */
static shaper_fn
find (const char *name)
{
  for (size_t k = 0; k < COUNT; k++)
    if (strcmp (shapers[k].name, name) == 0)
      return shapers[k].fn;
  return NULL;
}

int
main (int argc, char **argv)
{
  if (argc != 3)
    {
      fprintf (stderr, "usage: %s SHAPER VALUE\n", argv[0]);
      return 2;
    }
  shaper_fn f = find (argv[1]);
  if (f == NULL) /* calling a NULL function pointer crashes */
    {
      fprintf (stderr, "unknown shaper '%s'; try:", argv[1]);
      for (size_t k = 0; k < COUNT; k++)
        fprintf (stderr, " %s", shapers[k].name);
      fprintf (stderr, "\n");
      return 1;
    }
  float x = (float)strtod (argv[2], NULL);
  printf ("%s(%.2f) = %.4f\n", argv[1], (double)x, (double)f (x));
  return 0;
}
