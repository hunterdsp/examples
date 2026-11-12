# 04 · Arrays and loops

```bash
cd c-by-example/04-arrays
cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined average.c -o average
./average
```

```
14 samples, window 4
 in : 0.00 0.00 0.00 1.00 1.00 1.00 1.00 1.00 1.00 0.00 0.00 0.00 0.00 0.00
 out: 0.00 0.00 0.00 0.25 0.50 0.75 1.00 1.00 1.00 0.75 0.50 0.25 0.00 0.00
```

A signal in a program is an array of samples. The **moving average** is the
simplest filter there is: each output is the mean of the last 4 inputs. Feed
it a sudden step and it answers with a ramp. It smooths, which is what a
*low-pass* filter does: slow changes pass, fast ones are blunted. Lesson 20's
FIR filter is this same loop with a weight on each tap.

## What it shows

- **An array is a block of same-typed values**, indexed from 0.
  `float in[] = { … }` counts the elements for you.
- **`sizeof` in the declaring scope knows the whole array.**
  `sizeof in / sizeof in[0]` is total bytes over bytes per element: the
  element count. This is the idiom for "how long is this array".
- **Passed to a function, the array decays to a pointer** to its first
  element, and its length is lost. That's why `moving_average` takes `n`.
  Every C function that takes a buffer takes its length too.
- **The inner loop guards its edge.** `i - k >= 0` stops the window looking
  before sample 0. Edges are where filters and indexes go wrong.

## Pitfalls

- **Off by one.** [`broken-off-by-one.c`](broken-off-by-one.c) loops
  `i <= n` over 8 elements, so it writes a 9th, onto whatever lives after
  the array. C does not check indexes. The sanitizers do, and stop it at the
  bad write (clang's AddressSanitizer here; gcc's UBSan reports the same
  write as "insufficient space for an object of type 'float'"):

  ```
  ERROR: AddressSanitizer: stack-buffer-overflow on address 0x71fe56c00040
  WRITE of size 4 at 0x71fe56c00040 thread T0
      #0 0x5c73315d0d1a in fill …/04-arrays/broken-off-by-one.c:13:12
  ```

  Without a sanitizer this usually *works*, corrupting a neighbouring
  variable instead. That is the worst kind of bug: the program carries on
  with a wrong answer. For N elements, loop `i < N`.
- **`sizeof` on an array parameter.** In [`broken-sizeof.c`](broken-sizeof.c),
  `float buf[8]` as a *parameter* means `float *buf`, so `sizeof buf` is 8,
  the size of a pointer, and the "length" comes out as 2. The compiler warns
  (`-Wsizeof-array-argument`). The `[8]` in a parameter is documentation,
  nothing more.
- **Arrays don't assign.** `out = in;` does not compile: copy element by
  element, or with `memcpy` (lesson 06).

## Exercises

Try each one before opening its answer. Every answer's commands run in
this lesson's folder, where the commands above left you, and end back
there. Its code is in [`exercises/`](exercises/), one folder per exercise.

1. Change `WINDOW` to 8. How long does the ramp take now? What is the
   trade-off as the window grows?

   <details><summary>Output and answer</summary>

   ```bash
   cd exercises/ex1-window-8
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined average.c -o average
   ./average
   cd ../..
   ```

   ```
   14 samples, window 8
    in :     0     0     0     1     1     1     1     1     1     0     0     0     0     0
    out: 0.000 0.000 0.000 0.250 0.400 0.500 0.571 0.625 0.750 0.750 0.750 0.625 0.500 0.375
   ```

   The ramp would take 8 samples, but the step is only 6 long, so the
   output never gets there: it peaks at 0.750, 6 ones out of 8. (The
   uneven start, 0.250 then 0.400, is the edge: the first outputs average
   fewer than 8 samples.) A wider window smooths more, but answers more
   slowly, and blunts short events until they barely show. Every filter
   trades smoothing against delay.

   </details>

2. Feed it alternating `1, -1, 1, -1, …`. What comes out, and why does that
   make the moving average a low-pass filter?

   <details><summary>Output and answer</summary>

   ```bash
   cd exercises/ex2-alternating
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined average.c -o average
   ./average
   cd ../..
   ```

   ```
   14 samples, window 4
    in :  1.0 -1.0  1.0 -1.0  1.0 -1.0  1.0 -1.0  1.0 -1.0  1.0 -1.0  1.0 -1.0
    out:  1.0  0.0  0.3  0.0  0.0  0.0  0.0  0.0  0.0  0.0  0.0  0.0  0.0  0.0
   ```

   Silence, once the window is full: any 4 neighbours hold two +1s and two
   −1s, which cancel. Alternating samples are the highest frequency a
   signal can have, and the filter removes it completely, while the step
   above, a low frequency, came through. Low frequencies pass, high ones
   are cut: a low-pass filter.

   </details>

3. Rewrite `moving_average` to keep a running sum (add the new sample,
   subtract the one leaving the window) instead of re-adding the window
   every time. How many additions per sample does each version need?

   <details><summary>Output and answer</summary>

   ```bash
   cd exercises/ex3-running-sum
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined average.c -o average
   ./average
   cd ../..
   ```

   ```
   running: 0.00 0.00 0.00 0.25 0.50 0.75 1.00 1.00 1.00 0.75 0.50 0.25 0.00 0.00
   same as re-adding the window: yes
   additions per sample: 4 re-adding, 2 running
   ```

   Re-adding costs `WINDOW` additions per sample, the running sum two,
   whatever the window: at a window of 1000, that is 500 times less work.
   One caution: with floating-point samples, what the running sum adds
   and later subtracts can differ in the last bit, so over millions of
   samples it drifts. Real code keeps the sum in `double`, or re-adds the
   window now and then. These samples are 0 and 1, which `float` holds
   exactly, so the two versions agree to the bit.

   </details>
