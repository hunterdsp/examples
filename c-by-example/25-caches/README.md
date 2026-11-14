# 25 · Caches and memory layout

Run this lesson's commands in `c-by-example/25-caches/`.

A modern processor can do several arithmetic operations in the time it takes
light to cross a desk, while fetching a number from main memory takes around a
hundred times as long. What keeps it busy is the **cache**: small, fast memory
on the chip holding copies of what was used recently. A program that mostly
finds its data in the cache runs at the processor's speed. One that doesn't
spends most of its time waiting.

Two facts about caches decide almost everything:

- Memory moves in **cache lines**, 64 bytes on most machines (128 on Apple's
  M-series chips): 16 `float`s at once. Use all 16, and the fetch was cheap per
  number. Use one, and it cost as much as all 16.
- The processor **fetches ahead** when it can see a pattern: walking straight
  through memory, the next lines are already arriving before they are asked
  for.

So the same arithmetic can be fast or slow depending only on the *order* it
touches memory in, and on how the data is laid out. This program does the same
work two ways, twice:

```bash
cat cache.c
```

```c
/* cache.c -- the same work, in two memory orders.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       cache.c -o cache && ./cache
 *   cc -std=c99 -Wall -Wextra -O2 cache.c -o cache && ./cache time
 *
 * Memory is far slower than the processor, so the processor keeps copies
 * of recently used memory in CACHES, small and fast, and fetches memory
 * a whole CACHE LINE at a time (64 bytes on most machines: 16 floats).
 * Code that uses every byte of each line it fetches runs at cache speed.
 * Code that uses one float per line fetches sixteen times the memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
  ROWS   = 2048,
  COLS   = 2048, /* 2048 x 2048 floats: 16 MB, far bigger than any cache */
  VOICES = 100000
};

/* A synthesizer voice with everything about it in one struct: an ARRAY
   OF STRUCTS. The loop below only needs phase and inc. */
typedef struct
{
  float phase, inc; /* used every sample */
  float gain, pan;  /* used now and then */
  char  name[48];   /* used by the user interface */
} voice;            /* 64 bytes: one cache line per voice */

/* The same voices as a STRUCT OF ARRAYS: each field in its own array. */
typedef struct
{
  float *phase, *inc, *gain, *pan;
  char (*name)[48];
} voices;

static double
seconds_since (clock_t start)
{
  return (double)(clock () - start) / (double)CLOCKS_PER_SEC;
}

int
main (int argc, char **argv)
{
  int timing = argc > 1 && strcmp (argv[1], "time") == 0;

  /* 1. One 2-D array, summed along its rows and down its columns. C
        stores a 2-D array row after row, so a[r][c] and a[r][c + 1] are
        neighbours in memory, and a[r][c] and a[r + 1][c] are 8 KB apart. */
  float (*a)[COLS] = malloc (sizeof (float[ROWS][COLS]));
  if (a == NULL)
    return 1;
  for (size_t r = 0; r < ROWS; r++)
    for (size_t c = 0; c < COLS; c++)
      a[r][c] = (float)((r + c) % (size_t)4); /* small whole numbers: exact */

  clock_t start  = clock ();
  double  by_row = 0.0;
  for (size_t r = 0; r < ROWS; r++)
    for (size_t c = 0; c < COLS; c++)
      by_row += (double)a[r][c];
  double t_row = seconds_since (start);

  start         = clock ();
  double by_col = 0.0;
  for (size_t c = 0; c < COLS; c++)
    for (size_t r = 0; r < ROWS; r++)
      by_col += (double)a[r][c];
  double t_col = seconds_since (start);
  free (a);
  printf ("sum by rows %.0f, by columns %.0f\n", by_row, by_col);

  /* 2. Advancing every voice's phase: array of structs, struct of
        arrays. */
  voice *aos = calloc (VOICES, sizeof *aos);
  voices soa = {
    calloc (VOICES, sizeof (float)),   calloc (VOICES, sizeof (float)),
    calloc (VOICES, sizeof (float)),   calloc (VOICES, sizeof (float)),
    calloc (VOICES, sizeof *soa.name),
  };
  if (aos == NULL || !soa.phase || !soa.inc || !soa.gain || !soa.pan
      || !soa.name)
    return 1;
  for (size_t v = 0; v < VOICES; v++)
    aos[v].inc = soa.inc[v] = 0.001f * (float)(v % (size_t)100);

  start = clock ();
  for (int step = 0; step < 100; step++)
    for (size_t v = 0; v < VOICES; v++)
      aos[v].phase += aos[v].inc;
  double t_aos = seconds_since (start);

  start = clock ();
  for (int step = 0; step < 100; step++)
    for (size_t v = 0; v < VOICES; v++)
      soa.phase[v] += soa.inc[v];
  double t_soa = seconds_since (start);

  int same = 1;
  for (size_t v = 0; v < VOICES; v++)
    same = same && aos[v].phase == soa.phase[v];
  printf ("%zu-byte voices: both layouts agree: %s\n", sizeof (voice),
          same ? "yes" : "no");

  if (timing)
    {
      printf ("sum by rows:     %6.1f ms\n", t_row * 1e3);
      printf ("sum by columns:  %6.1f ms\n", t_col * 1e3);
      printf ("array of structs: %5.1f ms\n", t_aos * 1e3);
      printf ("struct of arrays: %5.1f ms\n", t_soa * 1e3);
    }
  free (aos);
  free (soa.phase);
  free (soa.inc);
  free (soa.gain);
  free (soa.pan);
  free (soa.name);
  return 0;
}
```

Checked the course's way:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined cache.c -o cache
./cache
```

```
sum by rows 6291456, by columns 6291456
64-byte voices: both layouts agree: yes
```

And timed:

```bash
cc -std=c99 -Wall -Wextra -O2 cache.c -o cache
```

<!-- no-run -->

```bash
./cache time
```

On the machine from lesson 19:

```
sum by rows 6291456, by columns 6291456
64-byte voices: both layouts agree: yes
sum by rows:        2.3 ms
sum by columns:    32.9 ms
array of structs:   9.1 ms
struct of arrays:   0.9 ms
```

## What it shows

- **Rows, not columns.** C stores a 2-D array one row after another, so
  `a[r][c]` and `a[r][c + 1]` are neighbours, and `a[r + 1][c]` is a whole row,
  8 KB, further on. Summing along rows uses every float of every line fetched;
  summing down columns uses one float per line and moves on. Same additions,
  same answer, 14 times slower.
- **A pointer to a row.** `float (*a)[COLS]` is a pointer to an array of `COLS`
  floats: one row. With it, `a[r][c]` works on a 2-D array that came from
  `malloc`, which is 16 MB, much too big for the stack.
- **Lay data out for the loop that runs most.** Each `voice` is 64 bytes, but
  the per-sample loop needs only 8 of them, `phase` and `inc`. As an *array of
  structs*, every voice costs a whole cache line of traffic for those 8 bytes.
  As a *struct of arrays*, the phases sit side by side, and so do the
  increments: every byte fetched is used. That is 10 times faster here, and
  part of the gain is lesson 26's: side-by-side floats are also what the
  processor's vector instructions need.
- **Same answer, different speed.** Both pairs agree exactly. Layout is a
  performance decision that the results can't reveal; only measuring can.

## Pitfalls

- **Guessing the cache.** 64-byte lines are common, not universal, and cache
  sizes vary from machine to machine. Exercise 2's best tile size is different
  on different processors. Measure on the machines you ship to.

- **Pointer-chasing structures.** Linked lists and trees put the next element's
  address inside the current one, so the processor can't fetch ahead (exercise
  3). An array is almost always the faster container, even where a list looks
  like the natural fit.

- **Two threads, one line.** Two threads writing different variables that share
  a cache line fight over it, each write taking the line away from the other
  core: *false sharing* (lesson 28).

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Sum every `k`-th `int` of a 64 MB array, for `k` = 1, 2, 4, … 64. Reading
   every 16th int reads a sixteenth of them; is it 16 times quicker?

   <details><summary>Output and answer</summary>

   In `exercises/ex1-stride/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined stride.c -o stride
   ./stride
   ```

   ```
   stride    ints read  bytes apart
        1     16777216            4
        2      8388608            8
        4      4194304           16
        8      2097152           32
       16      1048576           64
       32       524288          128
       64       262144          256
   ```

   Timed:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 stride.c -o stride
   ```

   <!-- no-run -->

   ```bash
   ./stride time
   ```

   ```
   stride    ints read  bytes apart  ms total  ns per int
        1     16777216            4       4.7        0.28
        2      8388608            8       3.4        0.40
        4      4194304           16       2.0        0.48
        8      2097152           32       1.6        0.77
       16      1048576           64       1.3        1.26
       32       524288          128       1.7        3.25
       64       262144          256       1.1        4.25
   ```

   No. From stride 4 to stride 64, the program reads 16 times fewer ints and
   takes about the same time. Up to stride 16, every 64-byte line is still
   fetched, whole, and beyond that the processor fetches neighbouring lines
   ahead anyway. The cost of memory is counted in lines, not in the numbers you
   use from them.

   </details>

2. [`transpose.c`](exercises/ex2-transpose/transpose.c) swaps the rows and
   columns of a 2048 × 2048 matrix. Whichever loop is outside, one of the two
   arrays is walked down its columns. Try working in square tiles of 8, 16, 32
   and 64.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-transpose/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined transpose.c -o transpose
   ./transpose
   ```

   ```
   tile  1: same as plain: yes
   tile  8: same as plain: yes
   tile 16: same as plain: yes
   tile 32: same as plain: yes
   tile 64: same as plain: yes
   ```

   Timed:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 transpose.c -o transpose
   ```

   <!-- no-run -->

   ```bash
   ./transpose time
   ```

   ```
   tile  1: same as plain: yes,  29.7 ms
   tile  8: same as plain: yes,  13.2 ms
   tile 16: same as plain: yes,  11.3 ms
   tile 32: same as plain: yes,  23.5 ms
   tile 64: same as plain: yes,  24.8 ms
   ```

   With 16 × 16 tiles it is about 2.6 times as fast: a 16-row strip of the
   output is 16 lines, few enough to stay in the cache while all 16 floats of
   each are written. Tiles of 32 and 64 do much less well on this machine,
   since their strips no longer fit in the fastest cache. (2048 being a power
   of two makes it worse: the lines of one column all land in the same few
   places in the cache, and evict each other.) The right tile is found by
   measuring, on each machine, which is what tuned libraries do when they are
   installed.

   </details>

3. Sum 2 million numbers from an array, and the same numbers from a linked list
   whose nodes are linked in a shuffled order.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-linked-list/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined list.c -o list
   ./list
   ```

   ```
   2097152 numbers: array sum 2199022206976, list sum 2199022206976
   ```

   Timed:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 list.c -o list
   ```

   <!-- no-run -->

   ```bash
   ./list time
   ```

   ```
   2097152 numbers: array sum 2199022206976, list sum 2199022206976
   array:    0.5 ms
   list:   276.1 ms
   ```

   Over 500 times slower, for the same additions. Each node's `next` holds the
   only copy of the following address, so the processor can't start fetching
   node `i + 1` until node `i` has arrived, and in shuffled order every node is
   a fresh trip to main memory: about 130 ns each, one after another. The
   array's addresses are known in advance, so its fetches overlap and the
   prefetcher runs ahead. [`list.c`](exercises/ex3-linked-list/list.c) shuffles
   with its own small generator so that every run, on every machine, builds the
   same list.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                                  |
| ------------------------------------------------------------ | ------------ | ------------------------------------------------------------- |
| [`calloc`](https://en.cppreference.com/c/memory/calloc)      | `<stdlib.h>` | allocate memory for n elements, all bytes zero                |
| [`clock`](https://en.cppreference.com/c/chrono/clock)        | `<time.h>`   | processor time used by the program, in `CLOCKS_PER_SEC` units |
| [`free`](https://en.cppreference.com/c/memory/free)          | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc`         |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)      | `<stdlib.h>` | allocate memory, contents unset                               |
| [`memcmp`](https://en.cppreference.com/c/string/byte/memcmp) | `<string.h>` | compare two blocks of bytes                                   |
| [`memset`](https://en.cppreference.com/c/string/byte/memset) | `<string.h>` | fill a block of bytes with one value                          |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                                  |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp) | `<string.h>` | compare two strings                                           |

<!-- /libfuncs -->
