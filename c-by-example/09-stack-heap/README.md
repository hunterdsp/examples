# 09 · Stack vs heap

Run this lesson's commands in `c-by-example/09-stack-heap/`.

Every array so far had its size written into the program: `float in[14]`,
`char path[32]`. Real signals don't work that way. How long is the file? How
many seconds did the user ask for? The answer arrives while the program runs,
and the memory has to come from somewhere that can be asked for it then: the
**heap**.

There are two places a C program's data can live:

|          | the **stack**                | the **heap**                      |
| -------- | ---------------------------- | --------------------------------- |
| what     | local variables              | memory from `malloc` and `calloc` |
| size     | fixed when compiled          | chosen while the program runs     |
| lifetime | until its function returns   | until you `free` it               |
| capacity | small: megabytes, often 8 MB | most of the machine's memory      |
| cost     | free: moving one register    | a function call, and a bookkeeper |

Here is the program. `tone_new` makes a tone as long as the command line asks,
and hands it back:

```bash
cat heap.c
```

```c
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
```

Build and run it, with no argument (half a second):

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined heap.c -o heap -lm
./heap
```

```
a local float[64]: 256 bytes, fixed when compiled
0.50 s at 48000 Hz: 24000 samples, 96000 bytes on the heap
rms 0.7071, as a sine's should be (1 / sqrt 2)
calloc'd buffer: 0 nonzero samples
```

## What it shows

- **`malloc(n * sizeof *buf)`** asks for `n` floats' worth of bytes and returns
  the address of the first, or `NULL` if there isn't that much to give.
  `sizeof *buf` is "the size of what `buf` points at", so it stays right if
  `buf` ever changes type.
- **Heap memory outlives the function that asked for it.** `tone_new` returns
  the address, and the samples are still there in `main`. A local array would
  not be (Pitfalls).
- **Every `malloc` gets exactly one `free`.** `tone_new` allocates, `main`
  frees: the comment above `tone_new` says whose job that is. Lesson 10 makes
  that a rule.
- **`calloc(n, size)` zeroes; `malloc` does not.** Memory from `malloc` holds
  whatever the last user left there. `calloc` also checks that `n * size` does
  not overflow, which `malloc(n * size)` cannot.
- **Check for `NULL`.** On a desktop `malloc` rarely fails, but on a small
  device, or for an absurd size, it does, and writing through `NULL` crashes
  (lesson 05).

## Pitfalls

- **Returning the address of a local.**
  [`broken-return-local.c`](broken-return-local.c) returns a local array. Its
  memory belongs to the function's *stack frame*, which the next call reuses,
  so the caller would hold the address of someone else's data. The compiler
  sees it and `-Werror` stops the build (gcc: "function returns address of
  local variable"; clang: "address of stack memory associated with local
  variable 'buf' returned"). This is lesson 05's dead memory: return heap
  memory, or have the caller pass a buffer in.

- **Big arrays on the stack.** The stack is small, often 8 MB on Linux and
  macOS, and less in a thread. A local `float buf[4000000]` (16 MB) overflows
  it, and the program dies before its first line runs. Lesson 06's `tone.c`
  made its one-second buffer `static` for this reason; the heap is the other
  way out.

- **Forgetting to `free`.** The memory stays taken until the program exits: a
  *leak*. Once is harmless; once per audio block, a long-running program grows
  until the machine runs out. Lesson 11 shows the tool that finds them.

- **Variable-length arrays.** C99 allows `float buf[n]` with `n` known only at
  run time. It is on the stack, so a large `n` overflows it, and nothing
  reports the failure the way `malloc`'s `NULL` does. Prefer `malloc` for any
  size you did not choose yourself.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Run `./heap 2`, then `./heap 0`. What changes, and why does the second one
   refuse?

   <details><summary>Output and answer</summary>

   This one needs no new code, only the lesson's program:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined heap.c -o heap -lm
   ./heap 2
   ```

   ```
   a local float[64]: 256 bytes, fixed when compiled
   2.00 s at 48000 Hz: 96000 samples, 384000 bytes on the heap
   rms 0.7071, as a sine's should be (1 / sqrt 2)
   calloc'd buffer: 0 nonzero samples
   ```

   ```bash
   ./heap 0
   ```

   ```
   usage: ./heap SECONDS (more than 0)
   ```

   Four times the samples, four times the bytes, from the same program: the
   heap gave each run what it asked for. The local `float[64]` is 256 bytes
   either way, because its size is part of the program. `./heap 0` is turned
   away before anything is allocated: a length of 0 has no rms, and a negative
   one would turn into an enormous `size_t`. Check a size where it comes in.

   </details>

2. Write `float *signal_copy(const float *in, size_t n)` that returns a copy on
   the heap. Change the copy and show the original is untouched.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-copy/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined copy.c -o copy
   ./copy
   ```

   ```
   original: 0.0 0.5 1.0 0.5
   copy x2:  0.0 1.0 2.0 1.0
   ```

   [`signal_copy`](exercises/ex2-copy/copy.c) is `malloc` then `memcpy`, which
   copies *bytes*: `n * sizeof *out` of them, not `n`. Lesson 04 said arrays
   don't assign; this is how you copy one. The copy belongs to the caller, so
   `main` frees it, and the comment on `signal_copy` says so.

   </details>

3. Allocate 4 channels of 8 samples in *one* `malloc`, fill channel `ch` with a
   ramp of slope `ch + 1`, and print one row per channel.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-channels/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined channels.c -o channels
   ./channels
   ```

   ```
   ch 0:    0    1    2    3    4    5    6    7
   ch 1:    0    2    4    6    8   10   12   14
   ch 2:    0    3    6    9   12   15   18   21
   ch 3:    0    4    8   12   16   20   24   28
   4 x 8 samples in one block of 128 bytes
   ```

   Sample `i` of channel `ch` lives at `ch * len + i`, and `&buf[ch * len]` is
   where a channel starts: one row, `len` long, which any function from lesson
   05 can take as a buffer and a length. One allocation means one `free` and
   one `NULL` check, and the rows sit next to each other in memory, which
   lesson 25 shows is what the cache wants. Try `./channels 2 5`: both sizes
   come from the command line.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                       | header       | what it does                                          |
| -------------------------------------------------------------- | ------------ | ----------------------------------------------------- |
| [`calloc`](https://en.cppreference.com/c/memory/calloc)        | `<stdlib.h>` | allocate memory for n elements, all bytes zero        |
| [`fprintf`](https://en.cppreference.com/c/io/fprintf)          | `<stdio.h>`  | `printf` to a stream, such as `stderr`                |
| [`free`](https://en.cppreference.com/c/memory/free)            | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc` |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)        | `<stdlib.h>` | allocate memory, contents unset                       |
| [`memcpy`](https://en.cppreference.com/c/string/byte/memcpy)   | `<string.h>` | copy bytes between blocks that don't overlap          |
| [`printf`](https://en.cppreference.com/c/io/fprintf)           | `<stdio.h>`  | formatted output to `stdout`                          |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)        | `<math.h>`   | sine of an angle in radians                           |
| [`sqrt`](https://en.cppreference.com/c/numeric/math/sqrt)      | `<math.h>`   | square root                                           |
| [`strtod`](https://en.cppreference.com/c/string/byte/strtof)   | `<stdlib.h>` | read a `double` from a string                         |
| [`strtoul`](https://en.cppreference.com/c/string/byte/strtoul) | `<stdlib.h>` | read an `unsigned long` from a string                 |

<!-- /libfuncs -->
