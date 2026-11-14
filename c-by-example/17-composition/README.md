# 17 · Composition and "inheritance"

Run this lesson's commands in `c-by-example/17-composition/`.

Lesson 16's interface lets one loop run many kinds of processor. This lesson
builds new kinds out of old ones, the two ways object-oriented languages do:

- **Composition**: a new kind that *has* processors inside it. A chain is a
  processor whose job is to run other processors.
- **Inheritance**: a new kind that *is* an old kind, with more. A ramp is a
  gain whose level glides to a new value instead of jumping: the way lesson
  13's gain stage changed level without a click.

C has keywords for neither. Both are structs, embedded the right way round.

The interface is lesson 16's, with `reset` optional (lesson 16's exercise 3)
and three new kinds declared at the end:

```bash
cat processor.h
```

```c
/* processor.h -- lesson 16's interface, with reset optional (lesson 16,
 * exercise 3), and three kinds more.
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
  void (*reset) (processor_t *self); /* may be NULL: nothing to reset */
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
  if (p->ops->reset != NULL)
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

/* The kinds this lesson has. Each returns a new processor, owned by the
   caller and freed with processor_destroy, or NULL. */
processor_t *gain_create (float g);
processor_t *lowpass_create (float a);

/* A gain that glides from 0 to g over `samples` samples: a fade-in. */
processor_t *ramp_create (float g, size_t samples);

/* Glide p, which must be a ramp, to g over `samples` samples. Returns 0,
   or -1 (and does nothing) if p is some other kind. */
int ramp_set (processor_t *p, float g, size_t samples);

/* A processor made of n others, run in order. The chain takes ownership
   of the stages: destroying it destroys them. If it can't be created, it
   destroys them anyway, so the caller never has to. */
processor_t *chain_create (processor_t *const *stages, size_t n);

#endif /* PROCESSOR_H */
```

## Inheritance: a ramp is a gain

To be built on, a gain must show its inside to the kinds that extend it. So
`gain_t` moves out of `gain.c` into a header of its own, which ordinary callers
never include:

```bash
cat gain.h
```

```c
/* gain.h -- the inside of a gain, for kinds that EXTEND it.
 *
 * Callers that only use a gain need processor.h and nothing else. A kind
 * that builds on a gain (ramp.c) needs more: the struct's layout, to put
 * a gain_t first in its own struct, and the gain's functions, to reuse
 * them. That is what this header gives, and the price is that gain_t is
 * no longer private: change it, and every kind built on it recompiles.
 */
#ifndef GAIN_H
#define GAIN_H

#include "processor.h"

typedef struct
{
  processor_t base; /* first: a gain_t * is a processor_t * */
  float       g;
} gain_t;

/* Set up a gain in memory the caller has, with the given table: a kind
   built on gain passes its own. */
void gain_init (gain_t *gain, const processor_ops *ops, float g);

/* The gain's operations, for a derived kind to reuse or call. */
void gain_process (processor_t *self, float *buf, size_t n);
void gain_destroy (processor_t *self);

#endif /* GAIN_H */
```

```bash
cat gain.c
```

```c
/* gain.c -- a processor that scales every sample, open to extension. */
#include "gain.h"

#include <stdlib.h>

void
gain_process (processor_t *self, float *buf, size_t n)
{
  gain_t *gain = (gain_t *)self;
  for (size_t i = 0; i < n; i++)
    buf[i] *= gain->g;
}

void
gain_destroy (processor_t *self)
{
  free (self);
}

/* No .reset: a gain has nothing to forget (lesson 16, exercise 3). */
static const processor_ops gain_ops = {
  .name    = "gain",
  .process = gain_process,
  .destroy = gain_destroy,
};

void
gain_init (gain_t *gain, const processor_ops *ops, float g)
{
  gain->base.ops = ops;
  gain->g        = g;
}

processor_t *
gain_create (float g)
{
  gain_t *gain = malloc (sizeof *gain);
  if (gain == NULL)
    return NULL;
  gain_init (gain, &gain_ops, g);
  return &gain->base;
}
```

A ramp puts a whole `gain_t` first, the way a gain puts a `processor_t` first:

```bash
cat ramp.c
```

```c
/* ramp.c -- a gain that glides: a kind DERIVED from gain.
 *
 * A ramp_t starts with a whole gain_t, which starts with a processor_t,
 * so one address is all three at once. A ramp IS a gain: anything that
 * works on a gain_t works on a ramp. It adds a target and a step, and
 * of the gain's operations it keeps one (destroy), replaces one
 * (process) and adds one (reset).
 */
#include "gain.h"

#include <stdlib.h>

typedef struct
{
  gain_t gain;   /* first: the PARENT, so a ramp_t * is a gain_t * */
  float  target; /* where g is gliding to */
  float  step;   /* how far g moves per sample */
  size_t left;   /* samples of glide still to go */
} ramp_t;

/* Overrides gain_process: glide g sample by sample while a ramp is
   running, then hand the rest of the block to the parent's code. */
static void
ramp_process (processor_t *self, float *buf, size_t n)
{
  ramp_t *r = (ramp_t *)self;
  size_t  i = 0;
  for (; i < n && r->left > 0; i++)
    {
      r->left--;
      r->gain.g = r->left > 0 ? r->gain.g + r->step : r->target;
      buf[i] *= r->gain.g;
    }
  gain_process (self, buf + i, n - i); /* the parent does the steady part */
}

/* Added: a gain has nothing to reset, but a ramp jumps to its target. */
static void
ramp_reset (processor_t *self)
{
  ramp_t *r = (ramp_t *)self;
  r->gain.g = r->target;
  r->left   = 0;
}

static const processor_ops ramp_ops = {
  .name    = "ramp",
  .process = ramp_process, /* overridden */
  .reset   = ramp_reset,   /* added */
  .destroy = gain_destroy, /* inherited: a ramp owns nothing more */
};

int
ramp_set (processor_t *p, float g, size_t samples)
{
  if (p->ops != &ramp_ops) /* the one check that makes the cast safe */
    return -1;
  ramp_t *r = (ramp_t *)p;
  r->target = g;
  r->left   = samples;
  r->step   = samples > 0 ? (g - r->gain.g) / (float)samples : 0.0f;
  if (samples == 0)
    r->gain.g = g;
  return 0;
}

processor_t *
ramp_create (float g, size_t samples)
{
  ramp_t *r = malloc (sizeof *r);
  if (r == NULL)
    return NULL;
  gain_init (&r->gain, &ramp_ops, 0.0f); /* the parent's setup first */
  ramp_set (&r->gain.base, g, samples);
  return &r->gain.base;
}
```

## Composition: a chain has stages

A chain holds its stages through the interface only, so it doesn't care what
they are, and owns them:

```bash
cat chain.c
```

```c
/* chain.c -- a processor made of processors: COMPOSITION.
 *
 * A chain HAS stages, where a ramp IS a gain. It holds them only through
 * the interface, so a stage may be any kind, another chain included, and
 * the chain never needs to know which. It owns them: its destroy
 * destroys each, then itself.
 */
#include "processor.h"

#include <stdlib.h>

typedef struct
{
  processor_t   base;
  processor_t **stages; /* owned, and so is each stage */
  size_t        n;
} chain_t;

static void
chain_process (processor_t *self, float *buf, size_t n)
{
  chain_t *c = (chain_t *)self;
  for (size_t s = 0; s < c->n; s++)
    processor_process (c->stages[s], buf, n); /* whatever kind it is */
}

static void
chain_reset (processor_t *self)
{
  chain_t *c = (chain_t *)self;
  for (size_t s = 0; s < c->n; s++)
    processor_reset (c->stages[s]);
}

static void
chain_destroy (processor_t *self)
{
  chain_t *c = (chain_t *)self;
  for (size_t s = 0; s < c->n; s++)
    processor_destroy (c->stages[s]);
  free (c->stages);
  free (c);
}

static const processor_ops chain_ops = {
  .name    = "chain",
  .process = chain_process,
  .reset   = chain_reset,
  .destroy = chain_destroy,
};

processor_t *
chain_create (processor_t *const *stages, size_t n)
{
  chain_t *c = malloc (sizeof *c);
  if (c != NULL)
    c->stages = malloc (n * sizeof *c->stages);
  int ok = c != NULL && c->stages != NULL;
  for (size_t s = 0; s < n; s++)
    ok = ok && stages[s] != NULL;
  if (!ok) /* all or nothing: destroy every stage we were given */
    {
      for (size_t s = 0; s < n; s++)
        processor_destroy (stages[s]);
      if (c != NULL)
        free (c->stages);
      free (c);
      return NULL;
    }
  c->base.ops = &chain_ops;
  for (size_t s = 0; s < n; s++)
    c->stages[s] = stages[s];
  c->n = n;
  return &c->base;
}
```

## Putting them together

`lowpass.c` is lesson 16's, unchanged. Here is an effect made of a chain inside
a chain, with a ramp at the front:

```bash
cat effects.c
```

```c
/* effects.c -- a chain inside a chain, and a ramp that is a gain.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       effects.c gain.c ramp.c lowpass.c chain.c -o effects && ./effects
 *
 * The whole effect is ONE processor_t *: a chain whose first stage is
 * itself a chain. main processes it, resets it and destroys it with the
 * same three calls as any other processor, and destroying it destroys
 * everything inside.
 */
#include "gain.h"
#include "processor.h"

#include <stdio.h>

enum
{
  N = 8
};

static void
show (const char *label, const float *buf)
{
  printf ("%-9s", label);
  for (size_t i = 0; i < N; i++)
    printf (" %.4f", (double)buf[i]);
  printf ("\n");
}

int
main (void)
{
  /* A fade-in over 4 samples, then a low-pass: the "voice". */
  processor_t *ramp            = ramp_create (1.0f, 4);
  processor_t *voice_stages[2] = { ramp, lowpass_create (0.5f) };
  processor_t *voice           = chain_create (voice_stages, 2);
  /* The voice, then a master gain: the whole effect. */
  processor_t *effect_stages[2] = { voice, gain_create (2.0f) };
  processor_t *effect           = chain_create (effect_stages, 2);
  if (effect == NULL)
    return 1; /* chain_create already destroyed every stage */

  float buf[N] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
  show ("in", buf);
  processor_process (effect, buf, N);
  show ("effect", buf);

  /* A ramp IS a gain: the same memory, read through the parent's type. */
  printf ("the ramp, as a gain: g = %.2f\n", (double)((gain_t *)ramp)->g);

  /* A method only ramps have checks the kind before using it. */
  printf ("ramp_set on the ramp:  %d\n", ramp_set (ramp, 0.5f, 2));
  printf ("ramp_set on the chain: %d\n", ramp_set (effect, 0.5f, 2));

  processor_destroy (effect); /* and with it, all five processors */
  return 0;
}
```

Five `.c` files, one program:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined effects.c gain.c ramp.c lowpass.c chain.c -o effects
./effects
```

```
in        1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000
effect    0.2500 0.6250 1.0625 1.5312 1.7656 1.8828 1.9414 1.9707
the ramp, as a gain: g = 1.00
ramp_set on the ramp:  0
ramp_set on the chain: -1
```

The ramp fades the step in over four samples, the low-pass smooths it, and the
master gain doubles it, so the output climbs toward 2.0 without a jump.

## What it shows

- **Inheritance is a struct inside a struct, first.** A `ramp_t` begins with a
  `gain_t`, which begins with a `processor_t`, so one address is all three at
  once. That is why `(gain_t *)ramp` reads the ramp's level: a ramp *is* a
  gain, and any code written for a gain works on it.
- **Keep, replace, add.** The ramp's table reuses the gain's `destroy`
  unchanged, replaces `process` with its own, and adds a `reset` the gain never
  had. Its `process` still calls the parent's `gain_process` for the part of
  the block where the level is steady, as a method in another language calls
  `super`.
- **Extending a type costs it its privacy.** A ramp needs `gain_t`'s layout, so
  `gain.h` publishes it. Every kind built on a gain now depends on that layout,
  and changing it means recompiling them all: the opposite of lesson 14.
- **Composition needs nothing but the interface.** `chain.c` includes only
  `processor.h`. A stage can be any kind, a chain included: `effect` is a chain
  whose first stage is a chain. Adding a kind tomorrow won't change `chain.c`.
- **Ownership flows down the tree.** `chain_create` takes ownership of its
  stages, so destroying `effect` destroys the voice, which destroys the ramp
  and the low-pass, then the master gain: five processors, freed by one call.
  And if creating a chain fails, it destroys the stages it was given, so the
  `NULL` checks happen once, at the end, not after every create.
- **Check the kind before using what only one kind has.** `ramp_set` works only
  on ramps, but is handed a `processor_t *`. Comparing its table with
  `&ramp_ops` tells it whether the cast to `ramp_t *` is safe, which is why
  `ramp_set` on the chain is refused with −1 instead of scribbling over it.

## Pitfalls

- **Treating a parent as a child.**
  [`broken-wrong-kind.c`](broken-wrong-kind.c) casts a plain gain to a ramp and
  sets its target. A ramp is a gain with more after it, and a gain has nothing
  after it: the write lands past the end of its block.

  ```
  broken-wrong-kind.c:49:25: runtime error: store to address 0x74bc597e0020 with insufficient space for an object of type 'float'
  ```

  Child to parent is always safe; parent to child only after a check, like
  `ramp_set`'s.

- **Deep hierarchies.** A ramp is a gain; a "stereo ramp" could be a ramp; and
  three levels down, a change at the top recompiles and can break everything
  below it. Composition doesn't have that problem: prefer it, and inherit only
  to reuse a parent's code, as the ramp reuses `gain_process`.

- **Ownership that isn't written down.** `chain_create` *takes* its stages.
  Destroy one of them yourself afterwards, and the chain frees it a second
  time. The comment in `processor.h` is the only thing that says so: read it,
  and write it for every function that takes or gives away ownership.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. A chain runs its stages one after another. Write a `mix` that runs two
   processors side by side on the same input and adds their outputs, and blend
   half the dry signal with a smoothed copy.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-mix/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined blend.c mix.c gain.c lowpass.c -o blend
   ./blend
   ```

   ```
   mix: 0.6000 0.6900 0.7710 0.8439 0.9095 0.9686 1.0217 1.0695
   ```

   The dry half contributes 0.5 throughout; the low-pass climbs from 0.1. Both
   processors work in place, so [`mix.c`](exercises/ex1-mix/mix.c) gives the
   second a copy of the input in a scratch buffer. The buffer is part of the
   `mix_t`, allocated once by `mix_create` (lesson 13), and a block longer than
   it is processed in pieces that fit. Like a chain, a mix owns what it holds,
   and like a chain it is just another processor: a mix can go in a chain, and
   a chain in a mix.

   </details>

2. Move the `gain_t` from the start of `ramp_t` to its end. What breaks, and
   how does the ramp get from the gain back to itself?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-container-of/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined effects.c gain.c ramp.c lowpass.c chain.c -o effects
   ./effects
   ```

   ```
   in        1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000 1.0000
   effect    0.2500 0.6250 1.0625 1.5312 1.7656 1.8828 1.9414 1.9707
   the ramp, as a gain: g = 1.00
   ramp_set on the ramp:  0
   ramp_set on the chain: -1
   ```

   The same output, after two fixes in
   [`ramp.c`](exercises/ex2-container-of/ramp.c). The `processor_t *` a ramp
   hands out now points into the middle of the ramp, so every `(ramp_t *)self`
   is wrong. `container_of` subtracts the gain's distance from the start of the
   struct, which `offsetof` (from `<stddef.h>`) knows:

   ```c
   #define container_of(ptr, type, member) \
     ((type *)(void *)((char *)(ptr) - offsetof (type, member)))
   ```

   And the inherited `gain_destroy` frees the address it is given, which is no
   longer where `malloc`'s block starts:

   ```
   ERROR: AddressSanitizer: attempting free on address which was not malloc()-ed: 0x737a503e0050 in thread T0
   ```

   So the ramp needs a `destroy` of its own. With `container_of`, a struct can
   embed a base anywhere, even several, which is how the Linux kernel builds
   its objects. Putting it first is still the simplest choice, and where it
   fits, the cast is enough.

   </details>

3. The ramp can only ever fade in a gain. Write a `fade` that fades in the
   output of *any* processor.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-fade/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined fades.c fade.c gain.c lowpass.c chain.c -o fades
   ./fades
   ```

   ```
   fade(gain):     0.1250 0.2500 0.3750 0.5000 0.5000 0.5000 0.5000 0.5000
   fade(chain):    0.0625 0.1875 0.3281 0.4688 0.4844 0.4922 0.4961 0.4980
   ```

   A [`fade`](exercises/ex3-fade/fade.c) *has* a processor rather than *being*
   one: it runs whatever it holds, then scales the output from 1/4 up to 1 over
   the first four samples. It includes only `processor.h` and works on a gain,
   a chain, or another fade. That is composition's advantage, and why it is the
   usual first choice: the ramp reuses a gain's code, but the fade works with
   kinds that haven't been written yet.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every function this lesson's code calls from the C standard library, linked to
its definition. The [quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                               |
| ------------------------------------------------------------ | ------------ | ---------------------------------------------------------- |
| [`free`](https://en.cppreference.com/c/memory/free)          | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc`      |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)      | `<stdlib.h>` | allocate memory, contents unset                            |
| [`memcpy`](https://en.cppreference.com/c/string/byte/memcpy) | `<string.h>` | copy bytes between blocks that don't overlap               |
| [`offsetof`](https://en.cppreference.com/c/types/offsetof)   | `<stddef.h>` | a member's distance from the start of its struct (a macro) |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                               |

<!-- /libfuncs -->
