# 28 · Concurrency

Run this lesson's commands in `c-by-example/28-concurrency/`.

A laptop has several processor cores, and a program that runs on one of them
leaves the rest idle. A **thread** is another path of execution through the
same program, sharing all of its memory, which the operating system can run on
another core at the same time. Threads come from POSIX, not from C99:
`<pthread.h>`, with `-pthread` when building. (C11 added `<threads.h>`, with
the same ideas under other names.)

Sharing all memory is both the point and the danger. The safest, and usually
fastest, use of threads is work that splits into pieces that share nothing at
all. A multichannel recording is exactly that: each channel can be filtered on
its own, by its own thread, and no thread ever touches another's samples.

```bash
cat threads.c
```

```c
/* threads.c -- four channels, filtered on four threads.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread \
 *       threads.c -o threads -lm && ./threads
 *   cc -std=c99 -Wall -Wextra -O2 -pthread threads.c -o threads -lm \
 *       && ./threads time
 *
 * A THREAD is a second (third, ...) path of execution through the same
 * program, sharing all its memory, which the operating system can run on
 * another processor core at the same time. Threads come from POSIX, not
 * from C99: <pthread.h>, and -pthread when building.
 *
 * The easy case, and the best one, is work that splits into pieces that
 * share nothing: here, the channels of a recording, each filtered on its
 * own, each by its own thread. No locks are needed, because no thread
 * ever touches another's data.
 */
#define _POSIX_C_SOURCE 200809L /* clock_gettime: POSIX, beyond C99 */

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
  CHANNELS = 4,
  N        = 1 << 22 /* samples per channel: 87 s at 48 kHz */
};

/* One channel's job: everything a thread needs, in one struct. */
typedef struct
{
  float *x; /* the samples, filtered in place */
  size_t n;
  float  a;     /* one-pole low-pass coefficient (lesson 14) */
  double total; /* the result: the sum of the output */
} job;

/* A thread's starting function: takes and returns a void * (lesson 15).
   This one is handed its job. */
static void *
filter_channel (void *arg)
{
  job  *j = arg;
  float y = 0.0f;
  for (size_t i = 0; i < j->n; i++)
    {
      y += j->a * (j->x[i] - y);
      j->x[i] = y;
    }
  double total = 0.0;
  for (size_t i = 0; i < j->n; i++)
    total += (double)j->x[i];
  j->total = total; /* written only by this thread, read after the join */
  return NULL;
}

static void
fill (float *x, size_t n, size_t channel)
{
  for (size_t i = 0; i < n; i++)
    x[i] = (float)sin (0.001 * (double)(channel + 1) * (double)i);
}

/* Wall-clock seconds. clock() would add up the CPU time of every thread,
   and so hide exactly the speed-up being measured. */
static double
now (void)
{
  struct timespec t;
  clock_gettime (CLOCK_MONOTONIC, &t);
  return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

int
main (int argc, char **argv)
{
  float *data = malloc ((size_t)CHANNELS * N * sizeof *data);
  if (data == NULL)
    return 1;
  job jobs[CHANNELS];
  for (size_t c = 0; c < CHANNELS; c++)
    jobs[c] = (job){ data + c * N, N, 0.01f, 0.0 };

  /* One thread after another, on this one. */
  for (size_t c = 0; c < CHANNELS; c++)
    fill (jobs[c].x, N, c);
  double start = now ();
  double one[CHANNELS];
  for (size_t c = 0; c < CHANNELS; c++)
    {
      filter_channel (&jobs[c]);
      one[c] = jobs[c].total;
    }
  double t_one = now () - start;

  /* All four at once, one thread each. */
  for (size_t c = 0; c < CHANNELS; c++)
    fill (jobs[c].x, N, c);
  start = now ();
  pthread_t tid[CHANNELS];
  for (size_t c = 0; c < CHANNELS; c++)
    if (pthread_create (&tid[c], NULL, filter_channel, &jobs[c]) != 0)
      return 1;
  for (size_t c = 0; c < CHANNELS; c++)
    pthread_join (tid[c], NULL); /* wait for each to finish */
  double t_four = now () - start;

  int same = 1;
  for (size_t c = 0; c < CHANNELS; c++)
    same = same && jobs[c].total == one[c];
  printf ("%d channels: threaded results identical: %s\n", CHANNELS,
          same ? "yes" : "no");
  if (argc > 1 && strcmp (argv[1], "time") == 0)
    printf ("one thread:   %5.1f ms\nfour threads: %5.1f ms\n", t_one * 1e3,
            t_four * 1e3);
  free (data);
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread threads.c -o threads -lm
./threads
```

```
4 channels: threaded results identical: yes
```

Timed:

```bash
cc -std=c99 -Wall -Wextra -O2 -pthread threads.c -o threads -lm
```

<!-- no-run -->

```bash
./threads time
```

On a machine with 12 cores:

```
4 channels: threaded results identical: yes
one thread:    42.0 ms
four threads:  17.1 ms
```

## What it shows

- **Start, then join.** `pthread_create` starts a thread running a function
  that takes and returns a `void *` (lesson 15), and hands it one argument:
  here, its `job`. `pthread_join` waits for that thread to finish. Everything a
  thread wrote is safe to read after its join, and not before.
- **Share nothing.** Each thread gets its own `job`, with its own samples and
  its own `total`. No two threads write the same memory, so no locks are
  needed, and the results are identical to the single-threaded ones, bit for
  bit.
- **2.5 times, not 4.** Starting threads costs time, and four cores reading 64
  MB of samples compete for the same memory (lesson 25). A speed-up below the
  number of threads is normal; measure, rather than assume.
- **Wall-clock time.** `clock()` adds up the CPU time of every thread, which
  would hide exactly the speed-up being measured, so `now()` uses POSIX's
  `clock_gettime` with `CLOCK_MONOTONIC`, a clock that only moves forward. The
  `#define _POSIX_C_SOURCE` at the top asks `<time.h>` for POSIX functions that
  strict `-std=c99` leaves out.
- **A compound literal.** `(job){ data + c * N, N, 0.01f, 0.0 }` is a C99
  *compound literal*: a struct value written in place, with its type in
  parentheses, like a cast.

## Pitfalls

- **Freeing what a thread still uses.**
  [`broken-free-before-join.c`](broken-free-before-join.c) frees a buffer
  before joining the thread that reads it. Whether the thread had finished is a
  matter of timing, so this bug can pass many runs; here a *mutex*, a lock that
  only one thread can hold at a time (exercise 1), makes the bad timing
  certain, and AddressSanitizer catches the read:

  ```
  ERROR: AddressSanitizer: heap-use-after-free on address 0x6cf059de0010 at pc 0x5c2cb20dd431 bp 0x6cd0577fed30 sp 0x6cd0577fed20
  ```

  Ownership (lesson 10) gains a time dimension: a thread borrows what it is
  handed until it is joined.

- **Data races.** Two threads touching the same memory, at least one of them
  writing, with nothing ordering them, is a *data race*, and undefined
  behaviour (exercise 1). ThreadSanitizer, `-fsanitize=thread`, finds them. It
  can't be combined with AddressSanitizer, so it is a separate build.

- **False sharing.** Threads writing *different* variables that share a cache
  line slow each other down badly (exercise 2).

- **The real-time thread.** A thread that must meet an audio deadline, as in
  lesson 13, must never wait for a lock that another thread might hold for
  long. Real-time audio code passes data between threads with lock-free queues,
  built on C11's `<stdatomic.h>`, which is beyond C99. Exercise 3's locked
  queue is the right tool everywhere else.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Four threads each add 1 to a shared counter a million times: once with a
   mutex around `count++`, once without.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-race/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread counter.c -o counter
   ./counter
   ```

   ```
   expected: 4000000
   locked:   4000000
   ```

   Without the mutex, every run gives a different count:

   <!-- no-run -->

   ```bash
   ./counter race
   ./counter race
   ```

   ```
   expected: 4000000
   locked:   4000000
   racing:   2575335
   expected: 4000000
   locked:   4000000
   racing:   1663321
   ```

   `count++` is three steps: read, add, write. Two threads can both read 41 and
   both write 42, and one count is lost; here, from a third to over half of
   them. Built with `-O2`, the racing version happened to print 4000000: the
   compiler, allowed to assume no other thread touches `count`, added the whole
   million at once. A race can pass every test and still be wrong.
   ThreadSanitizer reports it whatever the timing (trimmed):

   <!-- no-run -->

   ```bash
   cc -std=c99 -g -O1 -fsanitize=thread -pthread counter.c -o counter
   ./counter race
   ```

   ```
   WARNING: ThreadSanitizer: data race (pid=134438)
     Read of size 8 at 0x555555558068 by thread T6:
       #0 count_racing …/28-concurrency/exercises/ex1-race/counter.c:41
     Previous write of size 8 at 0x555555558068 by thread T5:
       #0 count_racing …/28-concurrency/exercises/ex1-race/counter.c:41
     Location is global 'count' of size 8 at 0x555555558068 (counter+0x4068)
   ```

   </details>

2. Four threads each count into their *own* counter, so there is no race. Put
   the four counters side by side in an array, then 64 bytes apart.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-false-sharing/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread sharing.c -o sharing
   ./sharing
   ```

   ```
   packed total 200000000, padded total 200000000
   ```

   Timed:

   ```bash
   cc -std=c99 -Wall -Wextra -O2 -pthread sharing.c -o sharing
   ```

   <!-- no-run -->

   ```bash
   ./sharing time
   ```

   ```
   packed total 200000000, padded total 200000000
   packed:  377.5 ms
   padded:   15.3 ms
   ```

   Over 20 times slower when packed, for the same correct answer. A core must
   own a cache line to write to it, and the four packed counters share one
   line, so every write takes the line away from the other three cores, and it
   travels back and forth constantly. Each `padded_counter` fills a line of its
   own, so no line is shared. Per-thread data that is written often deserves
   its own line.

   </details>

3. One thread produces 10,000 numbers and another consumes them, through lesson
   12's ring buffer of 8 slots. Make it safe, and let each thread sleep while
   it can't proceed.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-queue/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -pthread queue.c -o queue
   ./queue
   ```

   ```
   received 10000 items, in order: yes, sum 50005000
   ```

   [`queue.c`](exercises/ex3-queue/queue.c) guards the ring with a mutex, and
   adds two *condition variables*: `pthread_cond_wait` releases the mutex and
   sleeps until another thread signals, then takes the mutex back before
   returning. The consumer waits on `not_empty`, the producer on `not_full`.
   The waits sit in `while` loops, not `if`s, because a thread may wake up
   before its condition holds, and must check again. ThreadSanitizer finds no
   race in it.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                                                                       | header        | what it does                                                |
| -------------------------------------------------------------------------------------------------------------- | ------------- | ----------------------------------------------------------- |
| [`calloc`](https://en.cppreference.com/c/memory/calloc)                                                        | `<stdlib.h>`  | allocate memory for n elements, all bytes zero              |
| [`clock_gettime`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/clock_gettime.html)               | `<time.h>`    | read a clock, such as `CLOCK_MONOTONIC` (POSIX)             |
| [`free`](https://en.cppreference.com/c/memory/free)                                                            | `<stdlib.h>`  | give back memory from `malloc`, `calloc` or `realloc`       |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)                                                        | `<stdlib.h>`  | allocate memory, contents unset                             |
| [`printf`](https://en.cppreference.com/c/io/fprintf)                                                           | `<stdio.h>`   | formatted output to `stdout`                                |
| [`pthread_cond_signal`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_signal.html)   | `<pthread.h>` | wake one thread sleeping on a condition variable (POSIX)    |
| [`pthread_cond_wait`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_wait.html)       | `<pthread.h>` | release a mutex and sleep until signalled (POSIX)           |
| [`pthread_create`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_create.html)             | `<pthread.h>` | start a thread running a function (POSIX)                   |
| [`pthread_join`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_join.html)                 | `<pthread.h>` | wait for a thread to finish (POSIX)                         |
| [`pthread_mutex_lock`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_mutex_lock.html)     | `<pthread.h>` | take a mutex, waiting while another thread holds it (POSIX) |
| [`pthread_mutex_unlock`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_mutex_unlock.html) | `<pthread.h>` | release a mutex (POSIX)                                     |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)                                                        | `<math.h>`    | sine of an angle in radians                                 |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp)                                                   | `<string.h>`  | compare two strings                                         |

<!-- /libfuncs -->
