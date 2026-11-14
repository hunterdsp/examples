# 12 · Ring buffers

Run this lesson's commands in `c-by-example/12-ring-buffers/`.

Audio arrives in blocks while the program is busy with the last one. A filter
needs the last N inputs. A delay effect plays back what came in half a second
ago. Each needs a queue of samples that never ends, in memory that must not
grow. Lesson 10's growing buffer would grow forever.

A **ring buffer** is a fixed array used as that queue. Writes go in at the
*head* and reads come out at the *tail*, and when either reaches the end of the
array it carries on from the start. Nothing is ever moved, and nothing is ever
allocated.

Here is one, used twice: as a queue, and as a delay line:

```bash
cat ring.c
```

```c
/* ring.c -- a ring buffer: a fixed array used as a queue that never ends.
 *
 *   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined \
 *       ring.c -o ring && ./ring
 *
 * Samples go in at one end (push) and come out at the other (pop), first
 * in, first out. Two counters say where: `head` counts every sample ever
 * pushed, `tail` every sample ever popped. They only ever go up; the
 * array index is the count modulo the capacity, so the writes go round
 * and round the same CAP slots. No sample is ever moved, and no memory
 * is ever allocated.
 */
#include <stddef.h>
#include <stdio.h>

/* A power of two, so `count % CAP` is `count & MASK`: one AND, no
   division (exercise 1). */
enum
{
  CAP  = 8,
  MASK = CAP - 1 /* 0b111: keeps the low 3 bits, 0..7 */
};

typedef struct
{
  float  buf[CAP];
  size_t head; /* samples pushed, ever: the next write is buf[head & MASK] */
  size_t tail; /* samples popped, ever: the next read is buf[tail & MASK] */
} ring_t;

/* How many samples are waiting. Right even after head wraps past
   SIZE_MAX, because unsigned subtraction is modulo 2^N (lesson 02). */
static size_t
ring_count (const ring_t *r)
{
  return r->head - r->tail;
}

/* Add x at the head. Returns 0, or -1 if the ring is full. */
static int
ring_push (ring_t *r, float x)
{
  if (ring_count (r) == (size_t)CAP)
    return -1;
  r->buf[r->head & (size_t)MASK] = x;
  r->head++;
  return 0;
}

/* Take the oldest sample into *x. Returns 0, or -1 if the ring is empty. */
static int
ring_pop (ring_t *r, float *x)
{
  if (ring_count (r) == (size_t)0)
    return -1;
  *x = r->buf[r->tail & (size_t)MASK];
  r->tail++;
  return 0;
}

int
main (void)
{
  ring_t r = { { 0 }, 0, 0 };

  /* Push 10 samples into 8 slots: the last two are refused. */
  int refused = 0;
  for (int i = 1; i <= 10; i++)
    if (ring_push (&r, (float)i) != 0)
      refused++;
  printf ("pushed 1..10 into %d slots: %zu waiting, %d refused (full)\n", CAP,
          ring_count (&r), refused);

  float x;
  printf ("pop:");
  while (ring_pop (&r, &x) == 0)
    printf (" %.0f", x);
  printf ("\n");

  /* A delay line: prime the ring with 3 zeros, then for each input push
     it and pop the oldest. What comes out is the input, 3 samples late. */
  for (int i = 0; i < 3; i++)
    ring_push (&r, 0.0f);
  printf ("in     :");
  for (int i = 1; i <= 10; i++)
    printf (" %2d", i);
  printf ("\ndelay 3:");
  for (int i = 1; i <= 10; i++)
    {
      ring_push (&r, (float)i);
      ring_pop (&r, &x);
      printf (" %2.0f", x);
    }
  printf ("\n");

  /* The counters only went up; the index went round. The 3 samples
     still waiting ARE the delay. */
  printf ("head %zu, tail %zu: %zu waiting, at slots %zu to %zu\n", r.head,
          r.tail, ring_count (&r), r.tail & (size_t)MASK,
          (r.head - (size_t)1) & (size_t)MASK);
  return 0;
}
```

Build and run it:

```bash
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined ring.c -o ring
./ring
```

```
pushed 1..10 into 8 slots: 8 waiting, 2 refused (full)
pop: 1 2 3 4 5 6 7 8
in     :  1  2  3  4  5  6  7  8  9 10
delay 3:  0  0  0  1  2  3  4  5  6  7
head 21, tail 18: 3 waiting, at slots 2 to 4
```

## What it shows

- **Two counters that only go up.** `head` counts every push and `tail` every
  pop, so `head - tail` is how many are waiting, and full and empty are
  different numbers (8 and 0). The common alternative, two indexes that wrap at
  8, cannot tell full from empty without a flag or a wasted slot.
- **The index is the count, masked.** With 8 slots, `head & 7` keeps the low 3
  bits: 0, 1, …, 7, 0, 1, … That is `head % 8`, as one AND instead of a
  division, and it works because 8 is a power of two.
- **The counters can overflow, on purpose.** After 2⁶⁴ pushes `head` wraps to
  0, but unsigned subtraction is modulo 2⁶⁴ (lesson 02), so `head - tail` stays
  right. And since 8 divides 2⁶⁴, the masked index goes on round without a
  jump.
- **A delay line is a ring, primed.** Fill it with 3 zeros, then push each
  input and pop the oldest: everything comes out 3 samples late. Slots 2 to 4
  at the end hold 8, 9 and 10, still waiting their turn. Lesson 20's FIR filter
  keeps its inputs this way.
- **Full and empty are answers, not errors.** `ring_push` refuses when full,
  and the caller decides: drop the sample, or wait. Audio code usually drops,
  and counts what it dropped.

## Pitfalls

- **Forgetting to wrap.** [`broken-ring-overflow.c`](broken-ring-overflow.c)
  indexes with the raw count. The 9th push writes `buf[8]`, past the array,
  onto the struct's next member. UBSan knows the array's length:

  ```
  broken-ring-overflow.c:26:3: runtime error: index 8 out of bounds for type 'float[8]'
  ```

  ASan would not catch this one: the bad write lands inside the struct, which
  is all allocated memory. It is the sanitizer for *types*, not the one for
  memory, that knows `buf` has only 8 elements.

- **A mask on a size that isn't a power of two.** `head & 5` is not `head % 6`;
  it skips slots 2 and 3 entirely, silently. Keep the size a power of two, or
  use `%` (exercise 1).

- **Signed counters.** An `int` head overflows after 2³¹ pushes, which is
  undefined behaviour, and at 48 kHz takes only 12 hours. Counters that may
  wrap are unsigned, always.

- **Two threads, one ring.** One thread pushing while another pops is the
  classic use, and it needs atomic operations to be correct, which C99 does not
  have. Lesson 28 adds them.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Make the ring 6 slots, with `%` in place of the mask. Does it still work?
   What does the power of two buy?

   <details><summary>Output and answer</summary>

   In `exercises/ex1-cap-6/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined cap6.c -o cap6
   ./cap6
   ```

   ```
   pushed 1..10 into 6 slots: 6 waiting, 4 refused (full)
   pop: 1 2 3 4 5 6
   in     :  1  2  3  4  5  6  7  8  9 10
   delay 3:  0  0  0  1  2  3  4  5  6  7
   head 19, tail 16: 3 waiting, at slots 4 to 0
   ```

   It works: 6 slots, and the delay line goes round them (slots 4, 5, 0). Two
   things are lost. `%` costs more than the AND, on every push and pop: for a
   constant 6 the compiler swaps the division for a multiply and shifts, and
   for a size chosen at run time it is a true division, tens of cycles. And the
   counter's wrap no longer lines up: 2⁶⁴ is not a multiple of 6, so when
   `head` wraps to 0, the index jumps from slot 3 to slot 0 and the order
   breaks. Never in this program's life, but a ring in a device that runs for
   years could get there. With a power of two neither problem exists.

   </details>

2. Keep a history of the last 256 samples with a `uint8_t` index and no mask at
   all. Write 300 samples, then print the last 4.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-uint8/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined history.c -o history
   ./history
   ```

   ```
   wrote 300 samples into 256 slots; next write at slot 44
   the last 4, newest first: 299 298 297 296
   slot before 0: 255
   ```

   A `uint8_t` holds 0 to 255, and storing a result into it keeps it modulo
   256: lesson 02's "free modulus". So `w + 1` stored back goes from 255 to 0,
   and `w - k` reaches back past slot 0 to 255, with no `if` and no mask. 300
   writes went round once and on to slot 44. It works for exactly 256 slots, no
   other size, which is why it is a classic for byte-sized histories and lookup
   tables.

   </details>

3. Rewrite lesson 04's moving average with a ring of the last 4 inputs and a
   running sum. Is the output the same?

   <details><summary>Output and answer</summary>

   In `exercises/ex3-running-average/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined average.c -o average
   ./average
   ```

   ```
   out: 0.00 0.00 0.00 0.25 0.50 0.75 1.00 1.00 1.00 0.75 0.50 0.25 0.00 0.00
   ```

   The same as lesson 04's `out`. Each step reads the oldest sample from the
   slot the newest is about to take, so the sum loses exactly what leaves the
   window. Two additions per sample, whatever the window, and no look-back loop
   with its edge guard: the ring starts full of zeros, so the edge is just
   silence. Every block-based filter in Part IV keeps its history like this.

   </details>
