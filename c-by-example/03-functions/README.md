# 03 · Functions

Run this lesson's commands in `c-by-example/03-functions/`.

```bash
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

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says
which folder to run it in.

1. Add `void sine_scale(float *table, size_t n, float gain)` to `sine.h` and
   `sine.c`, and use it to halve a table.

   <details><summary>Output and answer</summary>

   The declaration goes in [`sine.h`](exercises/ex1-sine-scale/sine.h), the
   code in [`sine.c`](exercises/ex1-sine-scale/sine.c), and
   [`halve.c`](exercises/ex1-sine-scale/halve.c) calls it:

   In `exercises/ex1-sine-scale/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined halve.c sine.c -o halve -lm
   ./halve
   ```

   ```
   full:  0.0000  0.7071  1.0000  0.7071  0.0000 -0.7071 -1.0000 -0.7071
   half:  0.0000  0.3536  0.5000  0.3536  0.0000 -0.3536 -0.5000 -0.3536
   ```

   `sine_scale` changes the caller's samples, not a copy, because an array
   argument arrives as the address of its first element.

   </details>

2. Remove `static` from `made` and declare `extern int made;` in `table.c`.
   It works. Why is that worse?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-extern/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined table.c sine.c -o table -lm
   ./table
   ```

   ```
   tables made: 2
   tables made: -5 (according to sine_tables_made: -5)
   ```

   It works: `table.c` reads the count directly. But it can also *write*
   it, and so can every other file in the program, so the count is only
   as right as the least careful of them. Even `sine_tables_made()` now
   reports −5. With `static`, only `sine.c` can change `made`, and a
   wrong count can only be `sine.c`'s fault.

   </details>

3. Generate 3.5 cycles over 8 samples and print it. Where does the
   discontinuity land, and what would it sound like on repeat?

   <details><summary>Output and answer</summary>

   [`cycles.c`](exercises/ex3-half-cycle/cycles.c) builds against the
   lesson's own `sine.c`, two folders up (`-I ../..` tells the compiler to
   look there for `sine.h` too). It prints the table played twice
   (`looped`), under the same sine carried on for 16 samples (`wave`):

   In `exercises/ex3-half-cycle/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -I ../.. cycles.c ../../sine.c -o cycles -lm
   ./cycles
   ```

   ```
   wave  :  0.00  0.38 -0.71  0.92 -1.00  0.92 -0.71  0.38  0.00 -0.38  0.71 -0.92  1.00 -0.92  0.71 -0.38
   looped:  0.00  0.38 -0.71  0.92 -1.00  0.92 -0.71  0.38  0.00  0.38 -0.71  0.92 -1.00  0.92 -0.71  0.38
   ```

   They agree up to sample 8, where the second copy starts. There the wave
   is half-way through a cycle and heading down, but the loop starts the
   table again, heading up: from then on it is the wave upside down. That
   seam comes round every 8 samples, so instead of a pure tone, a loop of
   this table plays a harsh, buzzy one. Loop only tables that hold a whole
   number of cycles.

   </details>
