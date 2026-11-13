/* files.c -- strings are bytes, and so are files.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       files.c -o files && ./files
 *
 * Writes a few float samples to a binary file, reads them back, and looks
 * at the raw bytes one float is made of.
 */
#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  /* A C string is an array of char ending in a zero byte ('\0'). The
     compiler adds that byte to a string literal for you. */
  char tone[] = "tone";
  printf ("\"%s\": strlen %zu, sizeof %zu (the extra byte is the '\\0')\n",
          tone, strlen (tone), sizeof tone);

  /* A name from the command line, like any real file name: its length is
     only known when the program runs. */
  const char *name = argc > 1 ? argv[1] : tone;

  /* Build a file name safely. snprintf never writes more than the size it
     is given, and returns how long the full result WOULD have been, so
     truncation can be detected. */
  char path[32];
  int  need = snprintf (path, sizeof path, "%s-%d.f32", name, 440);
  printf ("path \"%s\" (%d chars, fits: %s)\n", path, need,
          need < (int)sizeof path ? "yes" : "no");

  char tiny[12];
  need = snprintf (tiny, sizeof tiny, "%s-%d.f32", name, 440);
  printf ("tiny \"%s\" (needed %d, truncated: %s)\n", tiny, need,
          need >= (int)sizeof tiny ? "yes" : "no");

  /* Write raw samples: no text, just the bytes of each float. "wb" opens
     for writing in binary mode. Every call that can fail is checked. */
  float out[4] = { 0.0f, 0.5f, 1.0f, -1.0f };
  FILE *f      = fopen (path, "wb");
  if (f == NULL)
    {
      perror (path); /* prints the reason, e.g. "Permission denied" */
      return 1;
    }
  size_t wrote = fwrite (out, sizeof out[0], 4, f);
  if (fclose (f) != 0 || wrote != 4)
    {
      fprintf (stderr, "%s: write failed\n", path);
      return 1;
    }

  /* Read them back. fread returns how many it got, which can be fewer. */
  float in[4];
  f = fopen (path, "rb");
  if (f == NULL)
    {
      perror (path);
      return 1;
    }
  size_t got = fread (in, sizeof in[0], 4, f);
  fclose (f);
  remove (path);
  printf ("read %zu samples:", got);
  for (size_t i = 0; i < got; i++)
    printf (" %.2f", in[i]);
  printf ("\n");
  printf ("round trip exact: %s\n",
          memcmp (in, out, sizeof out) == 0 ? "yes" : "no");

  /* What is 1.0f, as bytes? Any object can be viewed as unsigned char. */
  const unsigned char *bytes = (const unsigned char *)&out[2];
  printf ("1.0f in memory:");
  for (size_t i = 0; i < sizeof out[2]; i++)
    printf (" %02x", bytes[i]);
  /* The lowest-addressed byte holds the LEAST significant part: this
     machine is little-endian, like every x86 and Apple Silicon Mac. */
  printf ("  (%s-endian)\n", bytes[0] == 0x00 ? "little" : "big");
  return 0;
}
