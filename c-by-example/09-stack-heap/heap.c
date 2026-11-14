/* heap.c -- memory whose size is only known when the program runs.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       heap.c -o heap -lm && ./heap
 *
 * A local array lives on the STACK: its size is fixed when the program is
 * compiled, and it is gone when its function returns. Memory from malloc
 * lives on the HEAP: any size, decided while the program runs, and it
 * stays until free() hands it back.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h> /* malloc, calloc, free, strtod */

enum
{
  RATE = 48000
};

/* Returns n samples of a sine at freq Hz, in heap memory that the CALLER
   must free, or NULL if there is not enough memory. A local array could
   not be returned: it would be gone the moment tone_new returned (see
   broken-return-local.c). */
static float *
tone_new (size_t n, double freq)
{
  const double two_pi = 6.283185307179586;
  float       *buf    = malloc (n * sizeof *buf); /* n floats' worth */
  if (buf == NULL)
    return NULL;
  for (size_t i = 0; i < n; i++)
    buf[i] = (float)sin (two_pi * freq * (double)i / (double)RATE);
  return buf;
}

int
main (int argc, char *argv[])
{
  /* How long a tone? The command line says, so nobody knows until the
     program runs. */
  double seconds = argc > 1 ? strtod (argv[1], NULL) : 0.5;
  if (!(seconds > 0.0))
    {
      fprintf (stderr, "usage: ./heap SECONDS (more than 0)\n");
      return 2;
    }

  /* On the stack: the size is part of the program, fixed when compiling,
     and sizeof can tell it. */
  float window[64];
  printf ("a local float[64]: %zu bytes, fixed when compiled\n",
          sizeof window);

  /* On the heap: as many bytes as this run needs. */
  size_t n   = (size_t)(seconds * (double)RATE);
  float *buf = tone_new (n, 440.0);
  if (buf == NULL)
    {
      fprintf (stderr, "out of memory for %zu samples\n", n);
      return 1;
    }
  printf ("%.2f s at %d Hz: %zu samples, %zu bytes on the heap\n", seconds,
          RATE, n, n * sizeof *buf);

  double sum = 0.0;
  for (size_t i = 0; i < n; i++)
    sum += (double)buf[i] * (double)buf[i];
  printf ("rms %.4f, as a sine's should be (1 / sqrt 2)\n",
          sqrt (sum / (double)n));
  free (buf); /* every malloc gets exactly one free */

  /* calloc: n elements of a given size, every byte zero. malloc's memory
     holds whatever was there before. */
  float *zeros = calloc (n, sizeof *zeros);
  if (zeros == NULL)
    return 1;
  size_t nonzero = 0;
  for (size_t i = 0; i < n; i++)
    if (zeros[i] != 0.0f)
      nonzero++;
  printf ("calloc'd buffer: %zu nonzero samples\n", nonzero);
  free (zeros);
  return 0;
}
