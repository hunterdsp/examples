# 03 · Functions

```bash
cd c-by-example/03-functions
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined table.c sine.c -o table -lm
./table
```

```
  inside:  x = 42
  outside: n = 21
1 cycle :  0.0000  0.7071  1.0000  0.7071  0.0000 -0.7071 -1.0000 -0.7071
2 cycles:  0.0000  1.0000  0.0000 -1.0000 -0.0000  1.0000  0.0000 -1.0000
tables made: 2
```

A sine wave is the atom of signal processing: every signal is a sum of them
(lesson 24 proves it). This lesson builds a table of one, and in doing so
splits a program into two files, the way every C library is built.

## A header is a promise

| File | Holds |
|---|---|
| [`sine.h`](sine.h) | **Declarations**: the name, arguments and return type of each function. No code. |
| [`sine.c`](sine.c) | **Definitions**: the code. |
| [`table.c`](table.c) | A caller. It includes `sine.h`, so the compiler checks every call. |

The compiler builds `sine.c` and `table.c` separately, and the linker joins
them (lesson 00). The header is how the two agree on what a function looks
like without either seeing the other's code.

## What it shows

- **Pass by value.** `try_to_double(n)` doubles its own *copy*: 42 inside,
  still 21 outside. A C function never changes the caller's variables
  through its arguments, unless you hand it an address (lesson 05).
- **Arrays are different.** `sine_table(one, 8, 1.0)` fills the caller's
  array. An array argument is passed as the address of its first element,
  so the function writes straight into it.
- **`static` makes a name private.** `made` in `sine.c` is invisible to
  every other file; only `sine_tables_made()` can read it. This is the first
  step toward the objects of Part III.
- **`const` is a promise not to write.** `print_table` takes
  `const float table[]`, and the compiler holds it to that.

## Two periods, twice the frequency

The `2 cycles` row fits two sine periods into the same 8 samples, so it
repeats every 4: twice the frequency. Push `cycles` up to 4 and every
sample lands on a zero crossing: the table is all zeros, and the wave has
vanished. Push it past 4 and 5 cycles gives the same samples as 3 cycles,
upside down: the samples can no longer tell which frequency they came from.
That limit, half the sample rate, is the **Nyquist frequency**, and lesson
20 comes back to it.

## Why `-0.0000`?

sin(2π) should be 0, but 2π has no exact binary form (lesson 02), so the
result is −2.4×10⁻¹⁶, and `%.4f` keeps its sign. Floating-point results are
*nearly* right, never exactly. Plan for it.

## Pitfalls

- **Declaring a function differently in two files.** If `table.c` declared
  `sine_table` itself, with the wrong arguments, both files would compile
  and link, and the program would quietly pass garbage. Declare a function
  in exactly one header and include it everywhere, *including* in its own
  `.c` file, so the compiler checks the definition against the promise.
- **`M_PI` is not standard C.** It's POSIX; under `-std=c99` it is not
  defined. `sine.c` defines its own constant.

## Exercises

1. Add `void sine_scale(float *table, size_t n, float gain)` to `sine.h` and
   `sine.c`, and use it to halve a table.
2. Remove `static` from `made` and declare `extern int made;` in `table.c`.
   It works. Why is that worse?
3. Generate 3.5 cycles over 8 samples and print it. Where does the
   discontinuity land, and what would it sound like on repeat?
