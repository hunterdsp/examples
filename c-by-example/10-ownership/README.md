# 10 · Ownership and lifecycle

Run this lesson's commands in `c-by-example/10-ownership/`.

Lesson 09's rule was "every `malloc` gets exactly one `free`". In a program of
three functions that is easy to keep. In a library of three hundred, with
buffers handed from one part to another, it is one of the hardest things in C,
and the language does not help: a pointer does not say whether you are meant to
free what it points at.

So the code says it instead. Every block of heap memory has one **owner**, the
part of the program responsible for freeing it, and every function says, in its
name or its comment, what it does to ownership:

| a function that…                       | example          | afterwards                                      |
| -------------------------------------- | ---------------- | ----------------------------------------------- |
| **creates**: allocates and hands over  | `signal_create`  | the caller owns the result                      |
| **destroys**: takes back and frees     | `signal_destroy` | the memory is gone: don't use the pointer again |
| **borrows**: uses, and leaves it alone | `signal_push`    | the caller still owns it                        |
| **releases**: hands its memory onward  | `signal_release` | the caller owns the part                        |

Here is a buffer that grows as samples arrive, with a function for each row:

```bash
cat signal.c
```

```c
/* signal.c -- a growing buffer of samples, and who owns its memory.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       signal.c -o signal && ./signal
 *
 * A signal_t owns two blocks of heap memory: the struct itself and the
 * samples it points at. One function makes them (signal_create), one
 * function gives them back (signal_destroy), and every signal_create is
 * matched by exactly one signal_destroy. That pairing is what OWNERSHIP
 * means in C: the language does not track it, so the code says it.
 */
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
  float *data; /* owned: freed by signal_destroy */
  size_t len;  /* samples in use */
  size_t cap;  /* samples allocated; len <= cap */
} signal_t;

/* Make an empty signal with room for cap samples (cap > 0). The caller
   owns the result and must pass it to signal_destroy. Returns NULL if
   there is not enough memory, having freed anything it did allocate. */
static signal_t *
signal_create (size_t cap)
{
  signal_t *s = malloc (sizeof *s);
  if (s == NULL)
    return NULL;
  s->data = malloc (cap * sizeof *s->data);
  if (s->data == NULL)
    {
      free (s); /* the second allocation failed: undo the first */
      return NULL;
    }
  s->len = 0;
  s->cap = cap;
  return s;
}

/* Free everything s owns, then s itself. Like free, it accepts NULL, so
   cleanup code never needs an `if`. */
static void
signal_destroy (signal_t *s)
{
  if (s == NULL)
    return;
  free (s->data);
  free (s);
}

/* Append one sample, doubling the capacity when it is full. Returns 0, or
   -1 if there is no memory to grow, in which case s is unchanged. */
static int
signal_push (signal_t *s, float x)
{
  if (s->len == s->cap)
    {
      size_t new_cap = s->cap * (size_t)2;
      /* realloc into a TEMPORARY: on failure it returns NULL and leaves
         the old block alone, which s->data must still point at. */
      float *grown = realloc (s->data, new_cap * sizeof *grown);
      if (grown == NULL)
        return -1;
      s->data = grown; /* the old address may now be invalid */
      s->cap  = new_cap;
    }
  s->data[s->len] = x;
  s->len++;
  return 0;
}

/* Hand the samples to the caller and destroy the rest. Ownership MOVES:
   afterwards the caller must free the returned block, and s is gone. */
static float *
signal_release (signal_t *s, size_t *len)
{
  float *data = s->data;
  *len        = s->len;
  free (s); /* the struct only: its samples now belong to the caller */
  return data;
}

int
main (void)
{
  signal_t *s = signal_create (4);
  if (s == NULL)
    return 1;

  printf ("capacity %zu", s->cap);
  for (int i = 0; i < 1000; i++)
    {
      size_t cap = s->cap;
      if (signal_push (s, (float)i) != 0)
        {
          signal_destroy (s); /* every exit path gives back what it owns */
          return 1;
        }
      if (s->cap != cap)
        printf (" -> %zu", s->cap);
    }
  printf ("\npushed %zu samples, capacity %zu\n", s->len, s->cap);

  size_t n;
  float *samples = signal_release (s, &n); /* s must not be used again */
  double sum     = 0.0;
  for (size_t i = 0; i < n; i++)
    sum += (double)samples[i];
  printf ("released %zu samples (sum %.0f); main owns them now\n", n, sum);
  free (samples);
  return 0;
}
```

Build and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined signal.c -o signal
./signal
```

```
capacity 4 -> 8 -> 16 -> 32 -> 64 -> 128 -> 256 -> 512 -> 1024
pushed 1000 samples, capacity 1024
released 1000 samples (sum 499500); main owns them now
```

## What it shows

- **Create and destroy come in pairs.** `signal_create` makes two allocations,
  the struct and its samples, and `signal_destroy` frees both, in the reverse
  order. The caller never calls `free` on a `signal_t` itself: only the type's
  own code knows what is inside it.
- **A failed create cleans up after itself.** If the second `malloc` fails,
  `signal_create` frees the first before returning `NULL`. The caller sees all
  or nothing, never half a signal.
- **Destroy accepts `NULL`**, as `free` does. Cleanup code can then destroy
  everything it might have made, without checking which it did.
- **Growing by doubling.** When the buffer is full, `signal_push` asks
  `realloc` for twice the room. `realloc` keeps the samples, but may move them
  to a new address, so anything that kept the old one is now pointing at freed
  memory. Doubling means 1000 pushes cost 8 moves, not 1000 (exercise 1).
- **Ownership can move.** `signal_release` hands the samples to `main` and
  frees only the struct. After it, `s` is gone, and `main` frees the samples
  with plain `free`. The comment says so, because nothing else will.

## Pitfalls

- **Two names, one block, two frees.**
  [`broken-double-free.c`](broken-double-free.c) copies a pointer and frees
  both copies. `copy = buf` copied the *address*, not the samples, so the block
  is given back twice, which corrupts the allocator's own records. gcc sees
  this one while compiling (`pointer 'copy' used after 'free'`); where the
  compiler can't see it, AddressSanitizer stops the program at the second
  `free`:

  ```
  ERROR: AddressSanitizer: attempting double-free on 0x7486707e0040 in thread T0:
  ```

  One block, one owner. Everyone else *borrows*, and does not free.

- **`p = realloc(p, n)` leaks on failure.** If `realloc` fails it returns
  `NULL` and leaves the old block alone, but assigning straight back to `p` has
  just overwritten the only copy of its address. Always `realloc` into a
  temporary, as `signal_push` does.

- **Using a pointer after `realloc`.** `float *first = &s->data[0];`, then a
  push that grows the buffer: `first` may now point at freed memory. Re-read
  `s->data` after anything that can grow it.

- **Every exit path.** A function that returns early has to give back what it
  owns first, which is why `main` destroys `s` before its `return 1`. Miss one
  path and that path leaks; lesson 11's LeakSanitizer finds it.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Grow by 16 samples at a time instead of doubling. How many times does the
   buffer grow for 1000 samples, and how many samples might `realloc` have to
   copy?

   <details><summary>Output and answer</summary>

   In `exercises/ex1-grow-by-16/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined grow.c -o grow
   ./grow
   ```

   ```
   doubling: 1000 samples,  8 growths, capacity 1024, up to  1020 samples copied
   +16     : 1000 samples, 63 growths, capacity 1012, up to 31500 samples copied
   ```

   Each growth may move the whole buffer, copying every sample so far. Growing
   by 16, that is 63 moves and up to 31,500 samples copied to store 1000, and
   it gets worse with the square of the length. Doubling copies up to 1020: on
   average each sample is copied about once, however long the signal grows. The
   price is wasted room, up to half the capacity. That trade is why doubling
   (or 1.5×) is the usual choice.

   </details>

2. Write `signal_t *signal_clone(const signal_t *s)`. Why would
   `signal_t copy = *s;` not do?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-clone/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined clone.c -o clone
   ./clone
   ```

   ```
   original: 0.0 1.0 2.0 3.0
   clone   : 9.0 1.0 2.0 3.0
   separate blocks: yes
   ```

   `*s` copies the struct, and the struct holds the *address* of the samples,
   so the two would share one block: a change to one shows in the other, and
   destroying both frees it twice. That is a *shallow* copy.
   [`signal_clone`](exercises/ex2-clone/clone.c) is a *deep* copy: it creates a
   signal of its own and copies the samples into it. Two creates, so two
   destroys.

   </details>

3. Write `int signal_push_n(signal_t *s, const float *x, size_t n)` that
   appends a whole block and grows at most once.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-push-n/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined push_n.c -o push_n
   ./push_n
   ```

   ```
   pushed 1000 samples in one call: capacity 4 -> 1024, one realloc
   last sample 999
   ```

   Knowing the count up front, [`signal_push_n`](exercises/ex3-push-n/push_n.c)
   doubles the capacity until the block fits, calls `realloc` once, and copies
   the block in with one `memcpy`. Audio arrives in blocks, so this is the push
   real code calls. It borrows `x`: the caller still owns the block, and may
   reuse it as soon as the call returns.

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                          |
| ------------------------------------------------------------ | ------------ | ----------------------------------------------------- |
| [`free`](https://en.cppreference.com/c/memory/free)          | `<stdlib.h>` | give back memory from `malloc`, `calloc` or `realloc` |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)      | `<stdlib.h>` | allocate memory, contents unset                       |
| [`memcpy`](https://en.cppreference.com/c/string/byte/memcpy) | `<string.h>` | copy bytes between blocks that don't overlap          |
| [`printf`](https://en.cppreference.com/c/io/fprintf)         | `<stdio.h>`  | formatted output to `stdout`                          |
| [`realloc`](https://en.cppreference.com/c/memory/realloc)    | `<stdlib.h>` | resize allocated memory, perhaps moving it            |

<!-- /libfuncs -->
