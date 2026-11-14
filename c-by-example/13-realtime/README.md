# 13 · Allocation for real time

Run this lesson's commands in `c-by-example/13-realtime/`.

An audio interface asks for 256 samples at 48 kHz: the program has 5.3 ms to
deliver them, every time, for as long as it runs. Late once, and the listener
hears a click. A program like that is **real time**: what matters is not that
it is fast on average, but that it is *never* slow.

`malloc` makes no such promise. It may search its free lists, wait for a lock
another thread holds, or ask the operating system for more memory, and any of
those can take far longer than 5.3 ms, once in a while, which is all it takes.
So real-time code follows one rule:

> Allocate at **setup**. Process with what setup gave you. Free at
> **teardown**.

Here is a gain stage that changes level without clicking, written that way. Its
memory comes from an **arena**: one `malloc`, carved into pieces. The program
counts every `malloc` it makes, to prove processing makes none:

```bash
cat realtime.c
```

```c
/* realtime.c -- allocate everything up front, then nothing per block.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       realtime.c -o realtime && ./realtime
 *
 * An audio callback has a deadline: 256 samples at 48 kHz must be ready
 * in 5.3 ms, every time, or the listener hears a click. malloc gives no
 * such promise: it may search, take a lock another thread holds, or ask
 * the operating system for memory. So a real-time program allocates at
 * SETUP, and its processing code only uses what it was given.
 *
 * Here setup is one malloc, carved into buffers by an ARENA, and the
 * program counts every malloc it makes to prove processing makes none.
 */
#include <stdio.h>
#include <stdlib.h>

static size_t n_mallocs; /* every malloc this program makes */

static void *
counted_malloc (size_t bytes)
{
  n_mallocs++;
  return malloc (bytes);
}

/* An arena: one block, handed out in pieces from the front, and freed
   all at once. Allocating is an addition and a comparison: always the
   same, short time. */
typedef struct
{
  unsigned char *base; /* the block: bytes, so offsets count bytes */
  size_t         size; /* its size */
  size_t         used; /* bytes handed out so far */
} arena_t;

/* Every piece starts at a multiple of 16 bytes, enough for any type in
   this course (malloc's own block is aligned for any type). */
enum
{
  ALIGN = 16
};

static int
arena_init (arena_t *a, size_t size)
{
  a->base = counted_malloc (size);
  a->size = size;
  a->used = 0;
  return a->base != NULL ? 0 : -1;
}

/* Returns `bytes` from the arena, or NULL if it has too few left. */
static void *
arena_alloc (arena_t *a, size_t bytes)
{
  /* Round the start up to the next multiple of ALIGN: add ALIGN - 1,
     then clear the low bits (ALIGN is a power of two, lesson 12). */
  size_t start
      = (a->used + ((size_t)ALIGN - (size_t)1)) & ~((size_t)ALIGN - (size_t)1);
  if (start > a->size || bytes > a->size - start)
    return NULL;
  a->used = start + bytes;
  return a->base + start;
}

/* Frees every piece at once. Each pointer arena_alloc gave is now dead. */
static void
arena_free (arena_t *a)
{
  free (a->base);
  a->base = NULL;
}

/* A gain stage that changes gain without a click: over one block it
   ramps from the old gain to the new. The ramp lives in a scratch buffer
   sized, at setup, for the largest block it will ever be given. */
typedef struct
{
  float  gain;
  size_t max_block;
  float *ramp; /* max_block floats, from the arena */
} gain_t;

static int
gain_init (gain_t *g, arena_t *a, size_t max_block)
{
  g->gain      = 1.0f;
  g->max_block = max_block;
  g->ramp      = arena_alloc (a, max_block * sizeof *g->ramp);
  return g->ramp != NULL ? 0 : -1;
}

/* The real-time part: no malloc, no free, no printf, no file, no lock.
   A block bigger than setup allowed is refused, not allocated for. */
static int
gain_process (gain_t *g, const float *in, float *out, size_t n, float target)
{
  if (n == 0 || n > g->max_block)
    return -1;
  float step = (target - g->gain) / (float)n;
  for (size_t i = 0; i < n; i++)
    g->ramp[i] = g->gain + step * ((float)i + 1.0f);
  for (size_t i = 0; i < n; i++)
    out[i] = in[i] * g->ramp[i];
  g->gain = target;
  return 0;
}

int
main (void)
{
  enum
  {
    MAX_BLOCK = 256
  };

  /* ---- setup: every allocation happens here ---- */
  arena_t arena;
  if (arena_init (&arena, 4096) != 0)
    return 1;
  gain_t g;
  float *in  = arena_alloc (&arena, (size_t)MAX_BLOCK * sizeof *in);
  float *out = arena_alloc (&arena, (size_t)MAX_BLOCK * sizeof *out);
  if (in == NULL || out == NULL
      || gain_init (&g, &arena, (size_t)MAX_BLOCK) != 0)
    {
      arena_free (&arena);
      return 1;
    }
  for (size_t i = 0; i < (size_t)MAX_BLOCK; i++)
    in[i] = 1.0f; /* a constant input shows the gain itself */
  printf ("setup: %zu malloc, 3 buffers carved from it (%zu of %zu bytes)\n",
          n_mallocs, arena.used, arena.size);

  /* ---- processing: blocks of 64, 128 and 256, gain 0.5 and 1 ---- */
  size_t before   = n_mallocs;
  size_t sizes[3] = { 64, 128, 256 };
  size_t n        = 0;
  float  from = 0.0f, to = 0.0f;
  for (int b = 0; b < 1000; b++)
    {
      n    = sizes[b % 3];
      from = g.gain;
      to   = b % 2 == 0 ? 0.5f : 1.0f;
      if (gain_process (&g, in, out, n, to) != 0)
        {
          arena_free (&arena);
          return 1;
        }
    }
  printf ("1000 blocks processed: %zu mallocs\n", n_mallocs - before);
  printf ("last block, %zu samples, ramps %.1f -> %.1f: out[0] %.4f, "
          "out[%zu] %.4f\n",
          n, from, to, out[0], n - (size_t)1, out[n - (size_t)1]);

  printf ("a block of 512: %s\n",
          gain_process (&g, in, out, (size_t)512, 1.0f) != 0
              ? "refused (max 256)"
              : "processed");
  arena_free (&arena); /* ---- teardown: one free for everything ---- */
  return 0;
}
```

Build and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined realtime.c -o realtime
./realtime
```

```
setup: 1 malloc, 3 buffers carved from it (3072 of 4096 bytes)
1000 blocks processed: 0 mallocs
last block, 64 samples, ramps 0.5 -> 1.0: out[0] 0.5078, out[63] 1.0000
a block of 512: refused (max 256)
```

## What it shows

- **Three phases, and allocation only in the first.** Setup makes one `malloc`,
  processing makes none in a thousand blocks, teardown makes one `free`. The
  counter, not a promise, is what shows it.
- **An arena hands out memory in constant time.** `arena_alloc` rounds `used`
  up and adds the size: an addition and a comparison, the same time on every
  call. It never frees a single piece; `arena_free` frees them all.
- **Sizes are decided at setup, and enforced after.** The gain stage is told
  the largest block it will ever see, and sizes its scratch buffer for it. A
  block of 512 is refused, not allocated for: a refusal is a bug to fix during
  development, while an allocation is a click waiting to happen.
- **Alignment.** Each piece starts at a multiple of 16 bytes, rounded up with
  lesson 12's power-of-two trick: add 15, clear the low 4 bits. A `float` read
  from a misaligned address is slow on some machines and a crash on others, and
  lesson 26's vector instructions want 16 or more.
- **The ramp is the point.** Jumping from gain 0.5 to 1.0 between two samples
  is a step, which is heard as a click. Spreading it over a block, from 0.5078
  up to exactly 1.0000, is not.

## Pitfalls

- **Freeing the arena under a live pointer.**
  [`broken-arena-dangling.c`](broken-arena-dangling.c) keeps a buffer from the
  arena, frees the arena, and writes. Every pointer from an arena dies with it,
  however far away it was kept:

  ```
  ERROR: AddressSanitizer: heap-use-after-free on address 0x75a8dade0100 at pc 0x5f25750dd4f5 bp 0x7ffdc9831f30 sp 0x7ffdc9831f20
  ```

- **An arena hides overflows from AddressSanitizer.** ASan guards each
  `malloc`ed block, but the pieces of an arena sit side by side *inside* one
  block, so writing past the end of one piece lands in the next, and ASan sees
  nothing wrong. Check sizes when carving, as `arena_alloc` does, and test with
  plain `malloc` too when in doubt.

- **It isn't only `malloc`.** `printf`, file access, and waiting on a lock all
  take unbounded time too. The processing function here does none of them:
  `main` prints, after the blocks are done.

- **Hidden allocations.** Some library calls allocate internally: the first
  `printf` sets up a buffer, for one. A function called from the processing
  path must be known not to allocate, not assumed.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Write the obvious version: `gain_process` `malloc`s a ramp the size of the
   block, and frees it at the end. Count the allocations.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-naive/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined naive.c -o naive
   ./naive
   ```

   ```
   1000 blocks processed: 1000 mallocs, and as many frees
   last block: out[0] 0.5078, out[63] 1.0000 (the same as realtime.c)
   ```

   The same output, and correct by any test that only checks output. But each
   of those 1000 `malloc`s is a chance to miss the deadline, and if one fails,
   the block is lost. [`naive.c`](exercises/ex1-naive/naive.c) is not wrong for
   a program without a deadline. Real-time code differs not in what it computes
   but in what it is allowed to call.

   </details>

2. Some scratch is needed only while one block is processed. Give the gain
   stage a *scratch* arena that it allocates from freely, and reset the arena
   after each block.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-frame-arena/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined frame.c -o frame
   ./frame
   ```

   ```
   1000 blocks processed: 0 mallocs
   scratch: peak 1024 of 1024 bytes, 0 after each reset
   last block: out[0] 0.5078, out[63] 1.0000
   ```

   `arena_reset` sets `used` to 0: one store frees everything the block
   allocated. Processing can now allocate as if with `malloc`, in constant time
   and with no `free` to forget, as long as nothing outlives the block. The
   arena is sized for the largest block, and the peak shows the 256-sample
   blocks used all of it. Games use the same trick for each video frame, which
   is where the name *frame arena* comes from.

   </details>

3. Run setup for a largest block of 1024 samples, with the same 4096-byte
   arena. Where does it fail, and what should happen then?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-setup-fails/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined setup.c -o setup
   ./setup
   ```

   ```
   max block  256: needs  3072 bytes, setup ok (3072 used)
   max block 1024: needs 12288 bytes, setup refused
   ```

   Three buffers of 1024 floats need 12288 bytes, three times what the arena
   has, so an `arena_alloc` returns `NULL` and
   [`setup`](exercises/ex3-setup-fails/setup.c) frees the arena and reports
   failure. That is the right place to fail: before any audio runs, where the
   program can say why and stop, or try a smaller block. Every failure moved
   from processing to setup is one that can no longer click. (No leak either:
   the refused setup freed its arena, and LeakSanitizer would have said
   otherwise.)

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                | header       | what it does                                          |
| ------------------------------------------------------- | ------------ | ----------------------------------------------------- |
| [`free`](https://en.cppreference.com/c/memory/free)     | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc` |
| [`malloc`](https://en.cppreference.com/c/memory/malloc) | `<stdlib.h>` | allocate memory, contents unset                       |
| [`printf`](https://en.cppreference.com/c/io/fprintf)    | `<stdio.h>`  | formatted output to `stdout`                          |

<!-- /libfuncs -->
