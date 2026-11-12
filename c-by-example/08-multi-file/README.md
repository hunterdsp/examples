# 08 · The preprocessor and multi-file builds

Run this lesson's commands in `c-by-example/08-multi-file/`.

```bash
make && ./meter
```

```
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c meter.c
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c dsp.c
cc -fsanitize=address,undefined -o meter meter.o dsp.o -lm
loud : rms 1.000, peak 1.000 =   0.00 dBFS
quiet: rms 0.500, peak 0.500 =  -6.02 dBFS
BAD_SQUARE(1 + 2) = 5, DSP_SQUARE(1 + 2) = 9
```

A level meter in three files: a header that promises (`dsp.h`), the code
that keeps the promise (`dsp.c`), and a program that uses it (`meter.c`).
Half amplitude reads −6 dB, as lesson 02 said it would.

## Translation units

The compiler never sees your *project*. It sees one `.c` file at a time,
after the preprocessor has pasted in its headers: a **translation unit**.
`meter.c` and `dsp.c` are compiled separately into `meter.o` and `dsp.o`,
and only the linker sees both. Two consequences:

- **Headers are how translation units agree.** Each `.c` that uses `dsp_rms`
  includes `dsp.h`, so each compiles its calls against the same declaration.
- **Change one file, recompile one file.** In a library of hundreds of
  files, that is the difference between a one-second and a ten-minute build.

## `make`

The [`Makefile`](Makefile) says what each file is built *from*:

```make
meter: meter.o dsp.o          # the program needs both objects
meter.o: meter.c dsp.h        # each object needs its source and the header
dsp.o: dsp.c dsp.h
```

`make` compares timestamps and runs only the recipes whose inputs changed.
Edit `dsp.c` and only `dsp.o` and `meter` rebuild; touch `dsp.h` and both
objects do. Recipe lines must start with a **tab**, not spaces.

## The preprocessor

- **Include guards.** `#ifndef DSP_H / #define DSP_H / … / #endif` makes a
  header's body appear once per translation unit, however often it's
  included. `meter.c` includes `dsp.h` twice to prove it. In a real project,
  headers include other headers, and without guards the same declaration
  would arrive twice.
- **Macros are text substitution**, done before compiling. That makes them
  fast and dangerous. `BAD_SQUARE(1 + 2)` expands to `1 + 2 * 1 + 2`, which
  is 5. `DSP_SQUARE` wraps every use of the argument, and the whole result,
  in parentheses: `((1 + 2) * (1 + 2))`, which is 9.

## Pitfalls

- **Linker errors.** [`broken-link.c`](broken-link.c) declares `dsp_rms` and
  calls it, but nothing in the build defines it. The compiler is satisfied
  (the declaration told it the shape); the linker is not:

  ```
  undefined reference to `dsp_rms'                     (Linux, GNU ld)
  Undefined symbols for architecture arm64: "_dsp_rms"  (macOS)
  ```

  Read it as "a `.c` file or a library is missing from the link line". Here,
  `dsp.c`.
- **Macro arguments evaluated twice.** `DSP_SQUARE(i++)` expands to
  `((i++) * (i++))`, which increments `i` twice and is undefined behaviour.
  Prefer a `static inline` function when you can: it has none of these
  problems, and costs nothing (Part III uses them).
- **Defining things in headers.** A header *declares*. A function or
  variable *defined* in a header is defined again in every `.c` that
  includes it, and the linker reports a duplicate. (`static inline`
  functions are the exception.)
- **Spaces instead of a tab in a Makefile**: `missing separator. Stop.`

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says
which folder to run it in.

1. Run `make`, then `make` again. Then `touch dsp.c` and `make`. Which
   commands ran each time?

   <details><summary>Output and answer</summary>

   This one uses the lesson's own Makefile. `make clean` first, so the
   first `make` starts from nothing:

   ```bash
   make clean
   make
   make
   sleep 1       # some makes (macOS's) only see whole seconds
   touch dsp.c
   make
   ```

   ```
   rm -f meter meter.o dsp.o
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c meter.c
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c dsp.c
   cc -fsanitize=address,undefined -o meter meter.o dsp.o -lm
   make: 'meter' is up to date.
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined -c dsp.c
   cc -fsanitize=address,undefined -o meter meter.o dsp.o -lm
   ```

   The first `make` compiles both files and links. The second does
   nothing: every target is newer than what it is built from. After
   `touch dsp.c`, which only updates the file's timestamp, `make`
   recompiles `dsp.c` alone and relinks; `meter.o` is still up to date.
   That is the whole of `make`: compare timestamps, rebuild what is out
   of date, in the right order. (The `sleep 1` is for older versions of
   `make`, such as the one macOS ships, which compare timestamps to the
   whole second: touched in the same second it was built, `dsp.c` would
   look no newer than `dsp.o`. Typing by hand, you will never be that
   quick.)

   </details>

2. Add `float dsp_crest_db(const float *buf, size_t n)` (peak over rms, in
   dB) to `dsp.h` and `dsp.c`. A sine wave's crest factor is 3.01 dB; what
   is a square wave's?

   <details><summary>Output and answer</summary>

   In `exercises/ex2-crest/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined crest.c dsp.c -o crest -lm
   ./crest
   ```

   ```
   sine  : crest 3.01 dB
   square: crest 0.00 dB
   ```

   0 dB: a square wave sits at its peak all the time, so its peak and rms
   are the same. A sine spends most of its time below its peak, so its
   rms is lower by √2, which is 3.01 dB. [`dsp_crest_db`](exercises/ex2-crest/dsp.c)
   is one line, built from the three functions already there. Crest
   factor says how much headroom a signal needs above its average level.

   </details>

3. Rewrite `DSP_SQUARE` as a `static inline float dsp_square(float x)` in
   `dsp.h`, and check the output is unchanged.

   <details><summary>Output and answer</summary>

   In `exercises/ex3-inline/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined meter.c dsp.c -o meter -lm
   ./meter
   ```

   ```
   loud : rms 1.000, peak 1.000 =   0.00 dBFS
   quiet: rms 0.500, peak 0.500 =  -6.02 dBFS
   BAD_SQUARE(1 + 2) = 5, dsp_square(1 + 2) = 9
   ```

   The same numbers, and `dsp_square(1 + 2)` is 9 with no parentheses
   needed: a function's argument is a value, evaluated once, not text
   pasted in. It is `static inline` *in the header*, so each `.c` that
   includes it gets its own copy, which the compiler folds into the
   caller, as fast as the macro, and the linker sees no duplicates. One
   thing did change: the macro squared a `double` in `dsp_rms`, and
   `dsp_square` takes a `float`. The output is the same here, but a type
   is a decision a macro never made for you.

   </details>
