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

1. Write `void min_max(const float *buf, size_t n, float *lo, float *hi)`
   that hands back two results through pointers.
2. Rewrite `gain_in_place` to walk a pointer (`for (float *p = buf; …)`)
   instead of an index. Check the output is identical.
3. Print `sizeof(float *)` and `sizeof(char *)`. Why are they the same?
