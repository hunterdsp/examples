/* sig-wav -- turn raw float samples from stdin into a .wav file.
 *
 *   sig-wav FILE.wav
 *
 * 16-bit, mono, 48000 samples per second: a file any audio player can
 * play. A WAV file starts with a 44-byte header that holds the length of
 * the data, which isn't known until stdin ends, so the header is written
 * with zeros first, and filled in at the end by seeking back to it. A
 * file can seek; a pipe can't, which is why this tool writes a file.
 */
#include <stdint.h>
#include <stdio.h>

enum
{
  RATE  = 48000,
  BLOCK = 1024
};

/* WAV numbers are little-endian, whatever this machine is: write them a
   byte at a time, lowest first, instead of fwrite'ing an int. */
static void
put16 (FILE *f, uint32_t x)
{
  putc ((int)(x & 0xffu), f);
  putc ((int)((x >> 8) & 0xffu), f);
}

static void
put32 (FILE *f, uint32_t x)
{
  put16 (f, x & 0xffffu);
  put16 (f, x >> 16);
}

static void
header (FILE *f, uint32_t data_bytes)
{
  fputs ("RIFF", f);
  put32 (f, 36u + data_bytes); /* the size of everything after this */
  fputs ("WAVEfmt ", f);
  put32 (f, 16u);       /* the size of this "fmt " part */
  put16 (f, 1u);        /* 1: plain PCM integers */
  put16 (f, 1u);        /* one channel */
  put32 (f, RATE);      /* samples per second */
  put32 (f, RATE * 2u); /* bytes per second */
  put16 (f, 2u);        /* bytes per sample, all channels */
  put16 (f, 16u);       /* bits per sample */
  fputs ("data", f);
  put32 (f, data_bytes);
}

int
main (int argc, char *argv[])
{
  if (argc != 2)
    {
      fprintf (stderr, "usage: sig-wav FILE.wav\n");
      return 2;
    }
  FILE *f = fopen (argv[1], "wb");
  if (f == NULL)
    {
      perror (argv[1]);
      return 1;
    }
  header (f, 0); /* the sizes come later */

  float    buf[BLOCK];
  size_t   n;
  uint32_t samples = 0;
  while ((n = fread (buf, sizeof buf[0], BLOCK, stdin)) > 0)
    for (size_t i = 0; i < n; i++, samples++)
      {
        /* Scale to 16 bits, saturating, and round (lesson 27). */
        float   x = buf[i] * 32767.0f;
        int32_t s = x >= 32767.0f    ? 32767
                    : x <= -32768.0f ? -32768
                    : x >= 0.0f      ? (int32_t)(x + 0.5f)
                                     : (int32_t)(x - 0.5f);
        put16 (f, (uint32_t)s & 0xffffu); /* two's complement bits */
      }

  if (fseek (f, 0, SEEK_SET) != 0)
    {
      perror (argv[1]);
      return 1;
    }
  header (f, samples * 2u);
  if (ferror (stdin) || fclose (f) != 0)
    {
      perror ("sig-wav");
      return 1;
    }
  return 0;
}
