/* sig-gain -- multiply raw float samples by a number, stdin to stdout.
 *
 *   sig-gain FACTOR
 *
 * A tool in the pipe needs no library at all: read floats, change them,
 * write them, and say what went wrong on stderr.
 */
#include <stdio.h>
#include <stdlib.h>

enum
{
  BLOCK = 1024
};

int
main (int argc, char *argv[])
{
  char *end;
  if (argc != 2)
    {
      fprintf (stderr, "usage: sig-gain FACTOR\n");
      return 2;
    }
  float gain = strtof (argv[1], &end);
  if (end == argv[1] || *end != '\0')
    {
      fprintf (stderr, "sig-gain: %s: not a number\n", argv[1]);
      return 2;
    }

  float  buf[BLOCK];
  size_t n;
  while ((n = fread (buf, sizeof buf[0], BLOCK, stdin)) > 0)
    {
      for (size_t i = 0; i < n; i++)
        buf[i] *= gain;
      if (fwrite (buf, sizeof buf[0], n, stdout) != n)
        {
          perror ("sig-gain: write");
          return 1;
        }
    }
  if (ferror (stdin))
    {
      perror ("sig-gain: read");
      return 1;
    }
  return 0;
}
