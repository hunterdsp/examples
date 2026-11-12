/* tone.c (exercise 2) -- one second of a 440 Hz sine, as raw floats.
 *
 * 48000 samples, each sin(2 pi f t) at t = i / 48000, written as the 4
 * bytes of a float: the format lesson 06 reads back, and that sox and
 * Audacity import as "32-bit float, little-endian, mono".
 */
#include <math.h>
#include <stdio.h>

enum { RATE = 48000 };

int
main (void)
{
  const double two_pi = 6.283185307179586; /* M_PI is not C99: lesson 03 */
  static float samples[RATE]; /* static: too big to want on the stack */

  for (int i = 0; i < RATE; i++)
    samples[i] = (float)sin (two_pi * 440.0 * i / RATE);

  FILE *f = fopen ("tone.f32", "wb");
  if (f == NULL)
    {
      perror ("tone.f32");
      return 1;
    }
  size_t wrote = fwrite (samples, sizeof samples[0], RATE, f);
  if (fclose (f) != 0 || wrote != RATE)
    {
      fprintf (stderr, "tone.f32: write failed\n");
      return 1;
    }
  printf ("wrote %zu samples, %zu bytes, to tone.f32\n", wrote,
          wrote * sizeof samples[0]);
  return 0;
}
