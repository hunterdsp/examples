# 06 · Strings and files

Run this lesson's commands in `c-by-example/06-strings-files/`.

C has no string type. A string is an array of `char` that ends at the first
zero byte, `'\0'`. A file is a sequence of bytes, and so is a `float`. This
lesson moves samples between the three.

Here is the program:

```bash
cat files.c
```

```c
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

  /* main's own arguments are strings too. argv is an array of them, argc
     long: argv[0] is the program's name, argv[1] the first word typed
     after it, and so on. Each string is itself an array of char, so argv
     is an array of arrays, which is why it is written char *argv[]: an
     array ([]) of strings (char *). A name from the command line, like
     any real file name, has a length known only when the program runs. */
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
     for writing in binary mode. Every call that can fail is checked.

     fopen returns a FILE *: a pointer to a FILE, the struct <stdio.h>
     uses to track an open file (where it is up to, its buffer, whether
     an error happened). Its members are the library's business: the
     program only passes the pointer to fwrite, fread and fclose. */
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
```

Build and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined files.c -o files
./files
```

```
"tone": strlen 4, sizeof 5 (the extra byte is the '\0')
path "tone-440.f32" (12 chars, fits: yes)
tiny "tone-440.f3" (needed 12, truncated: yes)
read 4 samples: 0.00 0.50 1.00 -1.00
round trip exact: yes
1.0f in memory: 00 00 80 3f  (little-endian)
```

## Strings

- **The terminator counts.** `"tone"` is 4 characters and 5 bytes: `strlen`
  counts up to the `'\0'`, `sizeof` counts the whole array. Every buffer you
  build a string in needs that extra byte.
- **`argv` is an array of strings.** `main (int argc, char *argv[])` receives
  the command line as `argc` strings: `argv[0]` is the program's name,
  `argv[1]` the first word after it. Each string is an array of `char`, so
  `argv` is an array of arrays, and the declaration says so: `[]`, an array, of
  `char *`, strings. `./files sine` makes `argv[1]` the string `"sine"`.
- **`snprintf` is the safe way to build one.** It never writes past the size it
  is given, always terminates, and returns the length the *whole* result
  needed. If that is ≥ the buffer size, the result was cut short, and you can
  tell. The `tiny` line shows it happen: `tone-440.f32` is 12 characters, and
  `tiny` is 12 bytes, so it holds 11 characters and the `'\0'`. The `2` is what
  was cut, leaving `tone-440.f3`.

## Files

- **`FILE *` is an open file.** `fopen` returns a pointer to a `FILE`, a struct
  defined by `<stdio.h>` that keeps track of the file: how far into it the
  program is, a buffer of bytes not yet written, and whether an error happened.
  You never look inside it; you hand the pointer to every other file function,
  and `fclose` frees it. `stdout`, `stderr` and `stdin` are `FILE *`s too,
  opened for you, which is why `fprintf (stderr, …)` works.
- **`fopen` / `fwrite` / `fread` / `fclose`**, with `"wb"` and `"rb"` for
  binary. Raw samples are written as their bytes, with no text conversion: 4
  bytes per `float`, read back bit-for-bit (`round trip exact: yes`). This is
  the format the tools in lesson 33 pipe between each other.
- **Every call that can fail is checked.** `fopen` returns `NULL` (and `perror`
  prints why), `fwrite` and `fread` return how many items they managed, and
  `fclose` can fail too, since it flushes the last bytes. A disk-full error
  goes unnoticed if nobody looks.

## Bytes

`1.0f in memory: 00 00 80 3f`. Any object can be viewed as bytes through an
`unsigned char *`. The float 1.0 is `0x3f800000`, and this machine stores the
*least* significant byte first: it is **little-endian**, like every x86 PC and
Apple Silicon Mac. A file of raw floats written on one is readable on the
other. Some network protocols and older hardware are big-endian, and a file
from those needs its bytes swapped.

## Pitfalls

- **`strcpy` into a buffer too small.** [`broken-strcpy.c`](broken-strcpy.c)
  copies the program's own path into 8 bytes. `strcpy` stops only at the
  `'\0'`, however far away it is, so it writes past the end of the array: the
  buffer overflow behind decades of security holes. Two layers can catch it:
  AddressSanitizer (`stack-buffer-overflow`), and, on distributions that build
  with `_FORTIFY_SOURCE` such as Ubuntu, the C library itself
  (`*** buffer overflow detected ***`). Don't rely on either in production: use
  `snprintf`, which is told the size.
- **Forgetting the `'\0'`.** A `char[4]` holding `"tone"` has no room for it,
  and every string function will read past the end looking for one.
- **Text mode vs binary mode.** On Windows, `"w"` translates `\n` to `\r\n`,
  corrupting binary data. Always open sample files with `"wb"` and `"rb"`, even
  on systems where it makes no difference.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Run `./files sine`, then `./files a-much-longer-name`. Which lines change,
   and why?

   <details><summary>Output and answer</summary>

   This one needs no new code, only the lesson's program:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined files.c -o files
   ./files sine
   ```

   ```
   "tone": strlen 4, sizeof 5 (the extra byte is the '\0')
   path "sine-440.f32" (12 chars, fits: yes)
   tiny "sine-440.f3" (needed 12, truncated: yes)
   read 4 samples: 0.00 0.50 1.00 -1.00
   round trip exact: yes
   1.0f in memory: 00 00 80 3f  (little-endian)
   ```

   ```bash
   ./files a-much-longer-name
   ```

   ```
   "tone": strlen 4, sizeof 5 (the extra byte is the '\0')
   path "a-much-longer-name-440.f32" (26 chars, fits: yes)
   tiny "a-much-long" (needed 26, truncated: yes)
   read 4 samples: 0.00 0.50 1.00 -1.00
   round trip exact: yes
   1.0f in memory: 00 00 80 3f  (little-endian)
   ```

   Only `path` and `tiny` change, since only they are built from the name.
   `path` has room for 31 characters and the `'\0'`, so both names fit. `tiny`
   has room for 11: `snprintf` stops there, still terminates the string, and
   its return value (12, then 26) says how much more it needed. The rest of the
   output never touches the name.

   </details>

2. Write 48000 samples of a 440 Hz sine (lesson 03) to `tone.f32`. If you have
   `sox`: `play -t f32 -r 48000 -c 1 tone.f32`.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-tone/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined tone.c -o tone -lm
   ./tone
   ```

   ```
   wrote 48000 samples, 192000 bytes, to tone.f32
   ```

   One second of audio: 48000 samples of 4 bytes each. Sample `i` is
   `sin(2π · 440 · i / 48000)`, the same formula as lesson 03's table with time
   in place of the table index. The file is just the floats' bytes, with no
   header, which is why `play` has to be told the format, rate and channel
   count. Audacity opens it with *File → Import → Raw Data*.

   </details>

3. Write `uint32_t swap32(uint32_t x)` that reverses the byte order, and check
   it turns `0x3f800000` into `0x0000803f`.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-swap32/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined swap32.c -o swap32
   ./swap32
   ```

   ```
   swap32(0x3f800000) = 0x0000803f
   swap32(swap32(0x3f800000)) = 0x3f800000
   ```

   [`swap32`](exercises/ex3-swap32/swap32.c) shifts each byte to its mirror
   position and masks off the rest. Swapping twice gives back the original,
   which makes a quick test of any byte swap. Use `uint32_t`, not `int`:
   shifting a negative signed value is a trap of its own.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                  |
| ------------------------------------------------------------ | ------------ | --------------------------------------------- |
| [`fclose`](https://en.cppreference.com/c/io/fclose)          | `<stdio.h>`  | finish writing a file, and close it           |
| [`fopen`](https://en.cppreference.com/c/io/fopen)            | `<stdio.h>`  | open a file                                   |
| [`fprintf`](https://en.cppreference.com/c/io/fprintf)        | `<stdio.h>`  | `printf` to a stream, such as `stderr`        |
| [`fread`](https://en.cppreference.com/c/io/fread)            | `<stdio.h>`  | read raw bytes from a file                    |
| [`fwrite`](https://en.cppreference.com/c/io/fwrite)          | `<stdio.h>`  | write raw bytes to a file                     |
| [`memcmp`](https://en.cppreference.com/c/string/byte/memcmp) | `<string.h>` | compare two blocks of bytes                   |
| [`perror`](https://en.cppreference.com/c/io/perror)          | `<stdio.h>`  | print a message, and why the last call failed |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                  |
| [`remove`](https://en.cppreference.com/c/io/remove)          | `<stdio.h>`  | delete a file                                 |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)      | `<math.h>`   | sine of an angle in radians                   |
| [`snprintf`](https://en.cppreference.com/c/io/fprintf)       | `<stdio.h>`  | `printf` into a buffer, never past its size   |
| [`strcpy`](https://en.cppreference.com/c/string/byte/strcpy) | `<string.h>` | copy a string, with no check of the room      |
| [`strlen`](https://en.cppreference.com/c/string/byte/strlen) | `<string.h>` | a string's length, not counting its `'\0'`    |

<!-- /libfuncs -->
