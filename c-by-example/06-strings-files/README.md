# 06 · Strings and files

```bash
cd c-by-example/06-strings-files
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

C has no string type. A string is an array of `char` that ends at the first
zero byte, `'\0'`. A file is a sequence of bytes, and so is a `float`. This
lesson moves samples between the three.

## Strings

- **The terminator counts.** `"tone"` is 4 characters and 5 bytes: `strlen`
  counts up to the `'\0'`, `sizeof` counts the whole array. Every buffer you
  build a string in needs that extra byte.
- **`snprintf` is the safe way to build one.** It never writes past the size
  it is given, always terminates, and returns the length the *whole* result
  needed. If that is ≥ the buffer size, the result was cut short, and you
  can tell. The `tiny` line shows it happen.

## Files

- **`fopen` / `fwrite` / `fread` / `fclose`**, with `"wb"` and `"rb"` for
  binary. Raw samples are written as their bytes, with no text conversion:
  4 bytes per `float`, read back bit-for-bit (`round trip exact: yes`). This
  is the format the tools in lesson 33 pipe between each other.
- **Every call that can fail is checked.** `fopen` returns `NULL` (and
  `perror` prints why), `fwrite` and `fread` return how many items they
  managed, and `fclose` can fail too, since it flushes the last bytes. A
  disk-full error goes unnoticed if nobody looks.

## Bytes

`1.0f in memory: 00 00 80 3f`. Any object can be viewed as bytes through an
`unsigned char *`. The float 1.0 is `0x3f800000`, and this machine stores the
*least* significant byte first: it is **little-endian**, like every x86 PC
and Apple Silicon Mac. A file of raw floats written on one is readable on the
other. Some network protocols and older hardware are big-endian, and a file
from those needs its bytes swapped.

## Pitfalls

- **`strcpy` into a buffer too small.** [`broken-strcpy.c`](broken-strcpy.c)
  copies the program's own path into 8 bytes. `strcpy` stops only at the
  `'\0'`, however far away it is, so it writes past the end of the array:
  the buffer overflow behind decades of security holes. Two layers can
  catch it: AddressSanitizer (`stack-buffer-overflow`), and, on distributions
  that build with `_FORTIFY_SOURCE` such as Ubuntu, the C library itself
  (`*** buffer overflow detected ***`). Don't rely on either in production:
  use `snprintf`, which is told the size.
- **Forgetting the `'\0'`.** A `char[4]` holding `"tone"` has no room for it,
  and every string function will read past the end looking for one.
- **Text mode vs binary mode.** On Windows, `"w"` translates `\n` to
  `\r\n`, corrupting binary data. Always open sample files with `"wb"` and
  `"rb"`, even on systems where it makes no difference.

## Exercises

Try each one before opening its answer. Every answer's commands run in
this lesson's folder, where the commands above left you, and end back
there. Its code is in [`exercises/`](exercises/), one folder per exercise.

1. Run `./files sine`, then `./files a-much-longer-name`. Which lines
   change, and why?

   <details><summary>Output and answer</summary>

   This one needs no new code, only the lesson's program:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined files.c -o files
   ./files sine
   ./files a-much-longer-name
   ```

   ```
   "tone": strlen 4, sizeof 5 (the extra byte is the '\0')
   path "sine-440.f32" (12 chars, fits: yes)
   tiny "sine-440.f3" (needed 12, truncated: yes)
   read 4 samples: 0.00 0.50 1.00 -1.00
   round trip exact: yes
   1.0f in memory: 00 00 80 3f  (little-endian)
   "tone": strlen 4, sizeof 5 (the extra byte is the '\0')
   path "a-much-longer-name-440.f32" (26 chars, fits: yes)
   tiny "a-much-long" (needed 26, truncated: yes)
   read 4 samples: 0.00 0.50 1.00 -1.00
   round trip exact: yes
   1.0f in memory: 00 00 80 3f  (little-endian)
   ```

   Only `path` and `tiny` change, since only they are built from the
   name. `path` has room for 31 characters and the `'\0'`, so both names
   fit. `tiny` has room for 11: `snprintf` stops there, still
   terminates the string, and its return value (12, then 26) says how
   much more it needed. The rest of the output never touches the name.

   </details>

2. Write 48000 samples of a 440 Hz sine (lesson 03) to `tone.f32`. If you
   have `sox`: `play -t f32 -r 48000 -c 1 tone.f32`.

   <details><summary>Output and answer</summary>

   ```bash
   cd exercises/ex2-tone
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined tone.c -o tone -lm
   ./tone
   cd ../..
   ```

   ```
   wrote 48000 samples, 192000 bytes, to tone.f32
   ```

   One second of audio: 48000 samples of 4 bytes each. Sample `i` is
   `sin(2π · 440 · i / 48000)`, the same formula as lesson 03's table with
   time in place of the table index. The file is just the floats' bytes,
   with no header, which is why `play` has to be told the format, rate
   and channel count. Audacity opens it with *File → Import → Raw Data*.

   </details>

3. Write `uint32_t swap32(uint32_t x)` that reverses the byte order, and
   check it turns `0x3f800000` into `0x0000803f`.

   <details><summary>Output and answer</summary>

   ```bash
   cd exercises/ex3-swap32
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined swap32.c -o swap32
   ./swap32
   cd ../..
   ```

   ```
   swap32(0x3f800000) = 0x0000803f
   swap32(swap32(0x3f800000)) = 0x3f800000
   ```

   [`swap32`](exercises/ex3-swap32/swap32.c) shifts each byte to its
   mirror position and masks off the rest. Swapping twice gives back the
   original, which makes a quick test of any byte swap. Use `uint32_t`,
   not `int`: shifting a negative signed value is a trap of its own.

   </details>
