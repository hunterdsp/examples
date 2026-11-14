# 16 · Interfaces (vtables)

Run this lesson's commands in `c-by-example/16-interfaces/`.

Lesson 14 hid one kind of object behind a header. Lesson 15 chose behaviour at
run time with function pointers. Together they give an **interface**: many
kinds of object, each doing the same jobs in its own way, all used through one
type.

An audio effects chain is the classic case. A gain, a low-pass filter and a
delay work very differently, but each one *processes a block*, can be *reset*,
and must be *destroyed*. Code that runs the chain wants to do those three
things to each stage without caring which kind it is.

The trick is a table of function pointers, one per kind, called a **vtable** (a
"virtual function table", the name C++ uses for the same thing). Every object
starts with a pointer to its kind's table:

```bash
cat processor.h
```

```c
/* processor.h -- an INTERFACE: anything that processes a block of audio.
 *
 * A processor is a pointer to its operations, nothing more:
 *
 *   struct processor { const processor_ops *ops; };
 *
 * Each kind of processor (a gain, a low-pass, ...) puts a processor_t as
 * the FIRST member of its own struct and fills in one processor_ops: its
 * VTABLE, the table of functions that do the work for that kind. Code
 * that holds a processor_t * calls through the table, and never needs to
 * know which kind it has.
 */
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stddef.h>

typedef struct processor     processor_t;
typedef struct processor_ops processor_ops;

/* What every processor can do. Each function takes the processor itself
   first, as `self`: the data the function works on. */
struct processor_ops
{
  const char *name;
  void (*process) (processor_t *self, float *buf, size_t n);
  void (*reset) (processor_t *self);
  void (*destroy) (processor_t *self);
};

struct processor
{
  const processor_ops *ops; /* one shared table per kind */
};

/* Calls through the table, so callers needn't spell out p->ops->... */
static inline void
processor_process (processor_t *p, float *buf, size_t n)
{
  p->ops->process (p, buf, n);
}

static inline void
processor_reset (processor_t *p)
{
  p->ops->reset (p);
}

static inline void
processor_destroy (processor_t *p)
{
  if (p != NULL)
    p->ops->destroy (p);
}

static inline const char *
processor_name (const processor_t *p)
{
  return p->ops->name;
}

/* The two kinds this lesson has. Each returns a new processor, owned by
   the caller and freed with processor_destroy, or NULL. */
processor_t *gain_create (float g);
processor_t *lowpass_create (float a);

#endif /* PROCESSOR_H */
```

Each kind puts a `processor_t` *first* in its own struct, and fills in one
table. A gain:

```bash
cat gain.c
```

```c
/* gain.c -- a processor that scales every sample. */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t base; /* FIRST, so a gain_t * is also a processor_t * */
  float       g;
} gain_t;

static void
gain_process (processor_t *self, float *buf, size_t n)
{
  gain_t *gain = (gain_t *)self; /* back from the interface to the kind */
  for (size_t i = 0; i < n; i++)
    buf[i] *= gain->g;
}

static void
gain_reset (processor_t *self)
{
  (void)self; /* a gain remembers nothing, so there is nothing to reset */
}

static void
gain_destroy (processor_t *self)
{
  free (self); /* the address of base is the address of the whole gain_t */
}

/* One table for every gain there will ever be: static, const, shared. */
static const processor_ops gain_ops = {
  .name    = "gain",
  .process = gain_process,
  .reset   = gain_reset,
  .destroy = gain_destroy,
};

processor_t *
gain_create (float g)
{
  gain_t *gain = malloc (sizeof *gain);
  if (gain == NULL)
    return NULL;
  gain->base.ops = &gain_ops;
  gain->g        = g;
  return &gain->base;
}
```

The low-pass from lesson 14, the same way:

```bash
cat lowpass.c
```

```c
/* lowpass.c -- lesson 14's one-pole low-pass, as a processor. */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t base; /* first, as in every kind */
  float       a;    /* smoothing coefficient, 0 < a <= 1 */
  float       y;    /* the last output */
} lowpass_t;

static void
lowpass_process (processor_t *self, float *buf, size_t n)
{
  lowpass_t  *f = (lowpass_t *)self;
  const float a = f->a;
  float       y = f->y;
  for (size_t i = 0; i < n; i++)
    {
      y += a * (buf[i] - y);
      buf[i] = y;
    }
  f->y = y;
}

static void
lowpass_reset (processor_t *self)
{
  ((lowpass_t *)self)->y = 0.0f;
}

static void
lowpass_destroy (processor_t *self)
{
  free (self);
}

static const processor_ops lowpass_ops = {
  .name    = "lowpass",
  .process = lowpass_process,
  .reset   = lowpass_reset,
  .destroy = lowpass_destroy,
};

processor_t *
lowpass_create (float a)
{
  if (!(a > 0.0f && a <= 1.0f))
    return NULL;
  lowpass_t *f = malloc (sizeof *f);
  if (f == NULL)
    return NULL;
  f->base.ops = &lowpass_ops;
  f->a        = a;
  f->y        = 0.0f;
  return &f->base;
}
```

And a chain that runs both, without knowing which is which:

```bash
cat chain.c
```

```c
/* chain.c -- different kinds of processor, one loop.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       chain.c gain.c lowpass.c -o chain && ./chain
 *
 * main holds an array of processor_t *, and treats every element the
 * same way: through the interface. Which function actually runs is
 * decided by each processor's own table, at run time.
 */
#include "processor.h"

#include <stdio.h>

enum
{
  STAGES = 2,
  N      = 8
};

static void
show (const char *label, const float *buf)
{
  printf ("%-8s", label);
  for (size_t i = 0; i < N; i++)
    printf (" %.4f", (double)buf[i]);
  printf ("\n");
}

/* Run a step through every stage in order, printing after each. */
static void
run (processor_t *const *chain)
{
  float buf[N];
  for (size_t i = 0; i < N; i++)
    buf[i] = 1.0f;
  show ("in", buf);
  for (size_t s = 0; s < STAGES; s++)
    {
      processor_process (chain[s], buf, N);
      show (processor_name (chain[s]), buf);
    }
}

int
main (void)
{
  processor_t *chain[STAGES] = { gain_create (0.5f), lowpass_create (0.5f) };
  for (size_t s = 0; s < STAGES; s++)
    if (chain[s] == NULL)
      {
        for (size_t k = 0; k < STAGES; k++)
          processor_destroy (chain[k]); /* accepts NULL */
        return 1;
      }

  run (chain);

  /* Without a reset, the low-pass would start this run where the last
     one ended, near 0.5. Reset every stage, whatever its kind: */
  for (size_t s = 0; s < STAGES; s++)
    processor_reset (chain[s]);
  printf ("reset, and again:\n");
  run (chain);

  for (size_t s = 0; s < STAGES; s++)
    processor_destroy (chain[s]);
  return 0;
}
```

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined chain.c gain.c lowpass.c -o chain
./chain
```

```
in       1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000
gain     0.5000 0.5000 0.5000 0.5000 0.5000 0.5000 0.5000 0.5000
lowpass  0.2500 0.3750 0.4375 0.4688 0.4844 0.4922 0.4961 0.4980
reset, and again:
in       1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000
gain     0.5000 0.5000 0.5000 0.5000 0.5000 0.5000 0.5000 0.5000
lowpass  0.2500 0.3750 0.4375 0.4688 0.4844 0.4922 0.4961 0.4980
```

The gain halves the step, and the low-pass smooths what is left toward 0.5.
After the reset, the second run matches the first.

## What it shows

- **One call, many behaviours.** `processor_process (chain[s], buf, N)`
  compiles to the same instructions for every stage: load the object's `ops`,
  load `process` from it, call. Which code that is was decided when the object
  was created, by the table its `create` put in it. This is what other
  languages call *dynamic dispatch*.
- **The base goes first.** C guarantees that a pointer to a struct, converted,
  points at its first member, and back again. So `&gain->base` and `gain` are
  the same address, and `gain_process` can turn the `processor_t *self` it is
  handed back into the `gain_t *` it really is. Put `base` anywhere but first,
  and that cast is wrong.
- **One table per kind.** `gain_ops` is `static const`: one copy, in read-only
  memory, shared by every gain there will ever be. Each object pays one pointer
  for it, however many operations the interface has (exercise 2).
- **`self` is the `void *user` of lesson 15**, made type-safe. The callback and
  its state travel together in one object, instead of separately.
- **Each kind is hidden in its `.c`.** `gain_t` and `lowpass_t` are defined in
  their own files; the header offers only `gain_create` and `lowpass_create`,
  which return the interface type. Adding a kind is a new `.c` and one line in
  the header. The chain doesn't change (exercise 1).

## Pitfalls

- **A hole in the table.** [`broken-missing-op.c`](broken-missing-op.c) fills
  in a vtable with designated initializers and leaves out `.reset`. A member an
  initializer doesn't name is zero, so `reset` is a `NULL` pointer, and calling
  it jumps to address 0. gcc's build crashes, and AddressSanitizer reports it:

  ```
  ERROR: AddressSanitizer: SEGV on unknown address 0x000000000000 (pc 0x000000000000 bp 0x7ffc68679430 sp 0x7ffc68679378 T0)
  ```

  clang's build is stopped one step earlier, by a check that the function
  called has the pointer's type (there is no function at address 0). Either
  way, nothing complained when the table was written: the hole is found when it
  is used. Fill in every slot, or make an operation optional on purpose
  (exercise 3).

- **Casting to the wrong kind.** `(gain_t *)self` is only right if `self`
  really is a gain. Each kind's functions are reached only through its own
  table, so they are only ever handed their own kind, as long as nothing else
  calls them directly. Keep them `static`, as here, and nothing can.

- **Freeing through the interface.** `processor_destroy` calls the kind's own
  `destroy`, because only the kind knows what it owns. A plain `free` of a
  `processor_t *` happens to work for a gain, which owns nothing else, and
  leaks for any kind that does.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Add a third kind, a delay that outputs each sample 3 samples late, and put
   it at the end of the chain. What changes in `chain.c`?

   <details><summary>Output and answer</summary>

   In `exercises/ex1-delay/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined chain.c gain.c lowpass.c delay.c -o chain
   ./chain
   ```

   ```
   in       1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000
   gain     0.5000 0.5000 0.5000 0.5000 0.5000 0.5000 0.5000 0.5000
   lowpass  0.2500 0.3750 0.4375 0.4688 0.4844 0.4922 0.4961 0.4980
   delay    0.0000 0.0000 0.0000 0.2500 0.3750 0.4375 0.4688 0.4844
   reset, and again:
   in       1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000
   gain     0.5000 0.5000 0.5000 0.5000 0.5000 0.5000 0.5000 0.5000
   lowpass  0.2500 0.3750 0.4375 0.4688 0.4844 0.4922 0.4961 0.4980
   delay    0.0000 0.0000 0.0000 0.2500 0.3750 0.4375 0.4688 0.4844
   ```

   [`delay.c`](exercises/ex1-delay/delay.c) is a new kind: a `delay_t` with
   `base` first, a three-sample ring (lesson 12) and its own `delay_ops`. The
   header gains one line, `delay_create`. In `chain.c`, only `STAGES` and the
   array's initializer change; `run`, the reset loop and the cleanup are
   untouched. Unlike the gain, the delay *has* something to forget: without its
   reset, the second run would start with the last three samples of the first.

   </details>

2. Why does every gain point at one shared `gain_ops`, instead of each `gain_t`
   holding its own `process`, `reset` and `destroy` pointers?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-shared-ops/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined tables.c gain.c lowpass.c -o tables
   ./tables
   ```

   ```
   two gains, one table:     yes
   gain and lowpass share:   no
   interface, per object:    8 bytes
   own pointers, per object: 32 bytes
   ```

   Memory, mostly. With a shared table, an object pays one pointer, 8 bytes,
   for all its operations, and the table itself exists once per kind.
   [`tables.c`](exercises/ex2-shared-ops/tables.c) compares a struct carrying
   its own name and three function pointers: 32 bytes in every object, and more
   with every operation the interface grows. The table is also `const`, in
   read-only memory, so a stray write can't change what a gain does; and
   comparing `ops` tells you two objects are the same kind.

   </details>

3. `gain_reset` does nothing. Make `reset` optional, so that a kind with
   nothing to forget can leave it out of its table, and the mute from
   `broken-missing-op.c` works.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-optional-reset/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined mute.c -o mute
   ./mute
   ```

   ```
   mute: 0.0 0.0 0.0 0.0
   reset: nothing to do, and no crash
   ```

   The fix is in [`processor.h`](exercises/ex3-optional-reset/processor.h), not
   in the mute: `processor_reset` checks for `NULL` before calling, and the
   comment on `reset` says that it may be `NULL`. Now leaving it out is a
   decision the interface allows, instead of a hole, and `gain.c` could drop
   `gain_reset` too. Decide, for each operation, whether it is required or
   optional, and write that down where the table is defined: `process` and
   `destroy` are still required, and are still called without a check.

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
