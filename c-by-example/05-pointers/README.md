# 05 · Pointers

```bash
cd c-by-example/05-pointers
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined pointers.c -o pointers
./pointers
```

```
after swap: x = 2.0, y = 1.0
doubled: 1.00 -2.00 4.00 0.60
halved:  0.25 -0.50 1.00 0.15
peak 4.00 at index 2
empty buffer: no peak (NULL)
sig[1] lives 4 bytes after sig[0]
```

Every variable lives somewhere in memory, at an *address*. A **pointer** is
a variable that holds an address. Two operators are all there is:

| | |
|---|---|
| `&x` | the address of `x` |
| `*p` | the thing `p` points at: read it, or write it |

That is the whole idea. Every buffer, every object in Part III and every
library API in Part V is built on it.

## What it shows

- **`swap` changes the caller's variables.** Lesson 03 couldn't, because
  arguments are copies. A copy of an *address* still leads to the original,
  so `*a = *b` writes where the caller's `x` lives.
- **A buffer is a pointer and a length.** `gain_in_place(float *buf, size_t n, …)`
  is the shape of nearly every DSP function you will ever write. `buf[i]` is
  shorthand for `*(buf + i)`.
- **The types state the contract.** In `gain(const float *in, float *out, …)`,
  the `const` promises the input comes back untouched, and the compiler
  enforces it. `restrict` promises `in` and `out` don't overlap, which frees
  the compiler to optimise (lesson 26).
- **Pointer arithmetic counts elements, not bytes.** `p + 1` is the next
  `float`, 4 bytes on. Subtracting two pointers into the same array gives
  the distance in elements, which is how `find_peak`'s result becomes an
  index.
- **`NULL` means "points at nothing".** `find_peak` returns it for an empty
  buffer, and the caller checks before using the result.

## Pitfalls

- **Following a NULL pointer.** [`broken-null.c`](broken-null.c) forgets the
  check. Address 0 is never valid, so the program dies, and the sanitizer
  names the line:

  ```
  broken-null.c:25:26: runtime error: load of null pointer of type 'const float'
  ```

  Without the sanitizers it is a plain `Segmentation fault`, with no line
  number at all.

  If a function can return `NULL`, every caller checks. If it never returns
  `NULL`, say so in its documentation, so nobody wastes a check.
- **Writing through `const`.** [`broken-const.c`](broken-const.c) "tidies up"
  its input. The build stops (gcc: "assignment of read-only location";
  clang: "read-only variable is not assignable"). Use
  `const` on every pointer a function only reads: it's documentation the
  compiler checks.
- **Pointers into dead memory.** A pointer stays valid only as long as the
  thing it points at. Lesson 09 returns a pointer to a local variable, which
  is gone the moment the function returns.
- **Uninitialised pointers.** `float *p; *p = 1;` writes to a random
  address. Initialise every pointer, to `NULL` if to nothing else.

## Exercises

Try each one before opening its answer. Every answer's commands run from
the repo root, like the lesson's, and its code is in
[`exercises/`](exercises/), one folder per exercise.

1. Write `void min_max(const float *buf, size_t n, float *lo, float *hi)`
   that hands back two results through pointers.

   <details><summary>Output and answer</summary>

   ```bash
   cd c-by-example/05-pointers/exercises/ex1-min-max
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined min_max.c -o min_max
   ./min_max
   ```

   ```
   min -1.00, max 2.00
   ```

   The caller passes `&lo` and `&hi`, and [`min_max`](exercises/ex1-min-max/min_max.c)
   writes through them, as `swap` did. The types tell the story:
   `const float *buf` is read, `float *lo` and `float *hi` are written.
   It needs `n` to be at least 1, since an empty buffer has no minimum;
   say so where it is declared, as the code does.

   </details>

2. Rewrite `gain_in_place` to walk a pointer (`for (float *p = buf; …)`)
   instead of an index. Check the output is identical.

   <details><summary>Output and answer</summary>

   ```bash
   cd c-by-example/05-pointers/exercises/ex2-pointer-walk
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined walk.c -o walk
   ./walk
   ```

   ```
   doubled: 1.00 -2.00 4.00 0.60
   identical to the index version: yes
   ```

   The loop is `for (float *p = buf; p < buf + n; p++) *p *= g;`.
   `buf + n` points one past the last sample: C lets you form that
   address and compare against it, but never read through it. An
   optimising compiler turns both versions into the same machine code, so
   pick whichever reads better.

   </details>

3. Print `sizeof(float *)` and `sizeof(char *)`. Why are they the same?

   <details><summary>Output and answer</summary>

   ```bash
   cd c-by-example/05-pointers/exercises/ex3-pointer-size
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined sizes.c -o sizes
   ./sizes
   ```

   ```
   sizeof(float *)  = 8
   sizeof(char *)   = 8
   sizeof(double *) = 8
   but sizeof(float) = 4 and sizeof(char) = 1
   ```

   A pointer holds an address, and an address is the same size whatever
   lives there: 8 bytes on a 64-bit machine. The pointed-to type does not
   change the pointer; it tells the compiler how many bytes to read
   through it, and how far `p + 1` steps.

   </details>
