# C quick reference

The tables the lessons keep coming back to, in one place. Every example value
was printed by a real run, with gcc and clang on 64-bit Linux. Where macOS or
Windows differ, the table says so.

## `printf` conversions

`printf ("%d\n", x)`: each `%` conversion takes the next argument, and its
letter must match the argument's type. `-Wall` checks this for you.

| conversion | prints                             | argument type     | example          | output            |
| ---------- | ---------------------------------- | ----------------- | ---------------- | ----------------- |
| `%d`, `%i` | signed decimal                     | `int`             | `-42`            | `-42`             |
| `%u`       | unsigned decimal                   | `unsigned`        | `42u`            | `42`              |
| `%x`, `%X` | hexadecimal, lower or upper case   | `unsigned`        | `255u`           | `ff`, `FF`        |
| `%o`       | octal                              | `unsigned`        | `8u`             | `10`              |
| `%f`       | fixed point, 6 decimals by default | `double`, `float` | `3.14159`        | `3.141590`        |
| `%e`       | scientific notation                | `double`, `float` | `1234.5`         | `1.234500e+03`    |
| `%g`       | `%f` or `%e`, whichever is shorter | `double`, `float` | `0.0001`, `1e-5` | `0.0001`, `1e-05` |
| `%a`       | hexadecimal floating point, exact  | `double`, `float` | `1.0`            | `0x1p+0`          |
| `%c`       | one character                      | `int`             | `'A'`            | `A`               |
| `%s`       | a string, up to its `'\0'`         | `char *`          | `"tone"`         | `tone`            |
| `%p`       | an address                         | `void *`          | `(void *)&x`     | differs every run |
| `%%`       | a literal `%`                      | none              |                  | `%`               |

A `float` argument is passed to `printf` as a `double`, so `%f` prints both.
(`scanf` is different: `%f` reads a `float`, `%lf` a `double`.)

### Length modifiers

Put between the `%` and the letter, they say how big the integer argument is.

| modifier | with `d`/`i`    | with `u`/`x`/`o`     | example               |
| -------- | --------------- | -------------------- | --------------------- |
| `hh`     | `signed char`   | `unsigned char`      | `%hhd`                |
| `h`      | `short`         | `unsigned short`     | `%hd`                 |
| `l`      | `long`          | `unsigned long`      | `%ld`                 |
| `ll`     | `long long`     | `unsigned long long` | `%lld`                |
| `z`      | signed `size_t` | `size_t`             | `%zu`: lessons 04, 06 |
| `t`      | `ptrdiff_t`     | unsigned `ptrdiff_t` | `%td`: lesson 05      |
| `j`      | `intmax_t`      | `uintmax_t`          | `%jd`                 |

### Fixed-width types: `<inttypes.h>`

`int32_t` is an `int` on one machine and a `long` on another, so no single
letter fits it everywhere. `<inttypes.h>` defines the right one as a string,
which C pastes next to the rest of the format:

| type       | signed decimal | unsigned decimal | hex      | example                     | output |
| ---------- | -------------- | ---------------- | -------- | --------------------------- | ------ |
| `int32_t`  | `PRId32`       |                  |          | `printf ("%" PRId32, x)`    | `-5`   |
| `uint32_t` |                | `PRIu32`         | `PRIx32` | `printf ("%" PRIx32, 255u)` | `ff`   |
| `uint64_t` |                | `PRIu64`         | `PRIx64` | `printf ("%" PRIu64, n)`    | `5`    |

The same pattern covers 8, 16 and 64 bits: `PRId8`, `PRIu16`, `PRIx64`, …

### Flags, width and precision

The full form is `%[flags][width][.precision]conversion`.

| format  | meaning                                 | argument      | output     |
| ------- | --------------------------------------- | ------------- | ---------- |
| `%5d`   | width 5, padded with spaces on the left | `42`          | `   42`    |
| `%-5d`  | width 5, left-aligned                   | `42`          | `42   `    |
| `%05d`  | width 5, padded with zeros              | `42`          | `00042`    |
| `%+d`   | always show the sign                    | `42`          | `+42`      |
| `% d`   | a space where a `+` would go            | `42`          | ` 42`      |
| `%#x`   | with the `0x` prefix                    | `255u`        | `0xff`     |
| `%08x`  | 8 hex digits, zero-padded               | `0x3f800000u` | `3f800000` |
| `%.2f`  | 2 decimals                              | `3.14159`     | `3.14`     |
| `%8.3f` | width 8, 3 decimals                     | `3.14159`     | `   3.142` |
| `%.3s`  | at most 3 characters of the string      | `"tone"`      | `ton`      |
| `%*d`   | width taken from an `int` argument      | `5, 42`       | `   42`    |
| `%.*f`  | precision taken from an `int` argument  | `2, 3.14159`  | `3.14`     |

`%.0f` rounds halves to even, the IEEE default: `0.5` prints `0`, `1.5` and
`2.5` both print `2`.

### Printing every digit

| to print a… | exactly enough to read it back | example                            |
| ----------- | ------------------------------ | ---------------------------------- |
| `float`     | `%.9g`                         | `0.1f` prints `0.100000001`        |
| `double`    | `%.17g`                        | `0.1` prints `0.10000000000000001` |

Neither is a bug: `0.1` has no exact binary form (lesson 02).

## Data types

### Integers

Sizes as printed by `sizeof` on 64-bit Linux and macOS. C only promises
minimums, so where the size matters, use the fixed-width types below.

| type                 | size (bytes)   | range                                    | literal |
| -------------------- | -------------- | ---------------------------------------- | ------- |
| `char`               | 1              | −128…127 or 0…255 (see below)            | `'A'`   |
| `signed char`        | 1              | −128…127                                 |         |
| `unsigned char`      | 1              | 0…255                                    |         |
| `short`              | 2              | −32,768…32,767                           |         |
| `int`                | 4              | −2,147,483,648…2,147,483,647 (`INT_MAX`) | `42`    |
| `unsigned`           | 4              | 0…4,294,967,295                          | `42u`   |
| `long`               | 8 (Windows: 4) | ±9.2 × 10¹⁸ (`LONG_MAX`)                 | `42L`   |
| `long long`          | 8              | ±9.2 × 10¹⁸                              | `42LL`  |
| `unsigned long long` | 8              | 0…1.8 × 10¹⁹                             | `42ULL` |

Whether plain `char` is signed is up to the platform: it is on x86 and on
Apple's ARM, and is not on Linux on ARM. Use `signed char` or `unsigned char`
(or `uint8_t`) for small numbers, and plain `char` only for text.

### Fixed-width integers: `<stdint.h>`

| type                  | size | range                     | lesson's use                      |
| --------------------- | ---- | ------------------------- | --------------------------------- |
| `int8_t`, `uint8_t`   | 1    | −128…127, 0…255           | bytes; an index that wraps at 256 |
| `int16_t`, `uint16_t` | 2    | −32,768…32,767, 0…65,535  | 16-bit audio samples              |
| `int32_t`, `uint32_t` | 4    | ±2.1 × 10⁹, 0…4.3 × 10⁹   | accumulators, phase               |
| `int64_t`, `uint64_t` | 8    | ±9.2 × 10¹⁸, 0…1.8 × 10¹⁹ | long sums, sample counters        |

### Floating point: `<float.h>`

| type          | size                                           | significant digits  | largest   | smallest normal | epsilon (`1 + ε ≠ 1`) | literal |
| ------------- | ---------------------------------------------- | ------------------- | --------- | --------------- | --------------------- | ------- |
| `float`       | 4                                              | 6 (`FLT_DIG`)       | 3.40e+38  | 1.18e-38        | 1.19e-07              | `0.5f`  |
| `double`      | 8                                              | 15 (`DBL_DIG`)      | 1.80e+308 | 2.23e-308       | 2.22e-16              | `0.5`   |
| `long double` | 16 on x86-64 Linux; 8 on Apple ARM and Windows | at least `double`'s |           |                 |                       | `0.5L`  |

A literal with no suffix is a `double`: `0.5f * x` stays in `float`, `0.5 * x`
does not.

### Sizes and addresses

| type                    | header       | what it holds                                    | `printf`  |
| ----------------------- | ------------ | ------------------------------------------------ | --------- |
| `size_t`                | `<stddef.h>` | a size or a count; what `sizeof` gives (8 bytes) | `%zu`     |
| `ptrdiff_t`             | `<stddef.h>` | the distance between two pointers, in elements   | `%td`     |
| `T *`                   |              | the address of a `T` (8 bytes on 64-bit)         | `%p`      |
| `intptr_t`, `uintptr_t` | `<stdint.h>` | an address as an integer                         | `PRIxPTR` |

## Escape sequences

| sequence | character                     | sequence | character                             |
| -------- | ----------------------------- | -------- | ------------------------------------- |
| `\n`     | newline                       | `\\`     | backslash                             |
| `\t`     | tab                           | `\'`     | single quote                          |
| `\r`     | carriage return               | `\"`     | double quote                          |
| `\0`     | the zero byte ending a string | `\x41`   | the byte 0x41 (`'A'`)                 |
| `\a`     | bell                          | `\101`   | the byte with octal value 101 (`'A'`) |

## Operator precedence

Highest first. Operators in one row group as the last column says. When in
doubt, add parentheses: they cost nothing, and the next reader will thank you.

| operators                                                 | groups        |
| --------------------------------------------------------- | ------------- |
| `()` `[]` `->` `.` `x++` `x--`                            | left to right |
| `++x` `--x` `+x` `-x` `!` `~` `(type)` `*p` `&x` `sizeof` | right to left |
| `*` `/` `%`                                               | left to right |
| `+` `-`                                                   | left to right |
| `<<` `>>`                                                 | left to right |
| `<` `<=` `>` `>=`                                         | left to right |
| `==` `!=`                                                 | left to right |
| `&`                                                       | left to right |
| `^`                                                       | left to right |
| `\|`                                                      | left to right |
| `&&`                                                      | left to right |
| `\|\|`                                                    | left to right |
| `?:`                                                      | right to left |
| `=` `+=` `-=` `*=` `/=` `%=` `<<=` `>>=` `&=` `^=` `\|=`  | right to left |
| `,`                                                       | left to right |

The classic trap: `&`, `^` and `|` bind *looser* than `==`, so `x & MASK == 0`
means `x & (MASK == 0)`. Write `(x & MASK) == 0`.

## Headers used in this course

| header         | for                                                         |
| -------------- | ----------------------------------------------------------- |
| `<stdio.h>`    | `printf`, `fprintf`, `snprintf`, `fopen`, `fread`, `fwrite` |
| `<stdlib.h>`   | `malloc`, `calloc`, `realloc`, `free`, `strtod`, `exit`     |
| `<string.h>`   | `strlen`, `memcpy`, `memset`, `memcmp`                      |
| `<math.h>`     | `sin`, `sqrt`, `log10`, `fabsf`: link with `-lm`            |
| `<stdint.h>`   | `int16_t`, `uint32_t`, … and their limits                   |
| `<inttypes.h>` | the `PRId32` family for `printf`                            |
| `<stddef.h>`   | `size_t`, `ptrdiff_t`, `NULL`, `offsetof`                   |
| `<limits.h>`   | `INT_MAX`, `LONG_MAX`, `CHAR_BIT`, …                        |
| `<float.h>`    | `FLT_EPSILON`, `DBL_MAX`, `FLT_DIG`, …                      |
| `<assert.h>`   | `assert`, removed by `-DNDEBUG`                             |

## Library functions

Every library function the lessons call, linked to its definition: the C
standard library's on [cppreference.com](https://en.cppreference.com/c), and
the few from POSIX (marked so) in the
[POSIX standard](https://pubs.opengroup.org/onlinepubs/9799919799/). Each
lesson's README ends with the ones its own code uses, taken from this table by
`make format`.

| function                                                                                                       | header        | what it does                                                            |
| -------------------------------------------------------------------------------------------------------------- | ------------- | ----------------------------------------------------------------------- |
| [`UINT32_C`](https://en.cppreference.com/c/types/integer)                                                      | `<stdint.h>`  | a constant of at least `uint32_t`'s range: `UINT32_C (1)` (a macro)     |
| [`assert`](https://en.cppreference.com/c/error/assert)                                                         | `<assert.h>`  | stop the program if a condition is false; removed by `-DNDEBUG`         |
| [`cabs`](https://en.cppreference.com/c/numeric/complex/cabs)                                                   | `<complex.h>` | size (magnitude) of a `double complex`                                  |
| [`cabsf`](https://en.cppreference.com/c/numeric/complex/cabs)                                                  | `<complex.h>` | size of a `float complex`                                               |
| [`calloc`](https://en.cppreference.com/c/memory/calloc)                                                        | `<stdlib.h>`  | allocate memory for n elements, all bytes zero                          |
| [`carg`](https://en.cppreference.com/c/numeric/complex/carg)                                                   | `<complex.h>` | angle of a complex number, in radians                                   |
| [`cexp`](https://en.cppreference.com/c/numeric/complex/cexp)                                                   | `<complex.h>` | *e* raised to a complex power: `cexp (I * t)` is a phasor               |
| [`cimag`](https://en.cppreference.com/c/numeric/complex/cimag)                                                 | `<complex.h>` | imaginary part of a `double complex`                                    |
| [`cimagf`](https://en.cppreference.com/c/numeric/complex/cimag)                                                | `<complex.h>` | imaginary part of a `float complex`                                     |
| [`clock`](https://en.cppreference.com/c/chrono/clock)                                                          | `<time.h>`    | processor time used by the program, in `CLOCKS_PER_SEC` units           |
| [`clock_gettime`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/clock_gettime.html)               | `<time.h>`    | read a clock, such as `CLOCK_MONOTONIC` (POSIX)                         |
| [`conj`](https://en.cppreference.com/c/numeric/complex/conj)                                                   | `<complex.h>` | complex conjugate: the imaginary part negated                           |
| [`cos`](https://en.cppreference.com/c/numeric/math/cos)                                                        | `<math.h>`    | cosine of an angle in radians                                           |
| [`creal`](https://en.cppreference.com/c/numeric/complex/creal)                                                 | `<complex.h>` | real part of a `double complex`                                         |
| [`crealf`](https://en.cppreference.com/c/numeric/complex/creal)                                                | `<complex.h>` | real part of a `float complex`                                          |
| [`exp`](https://en.cppreference.com/c/numeric/math/exp)                                                        | `<math.h>`    | *e* raised to the power x                                               |
| [`fabs`](https://en.cppreference.com/c/numeric/math/fabs)                                                      | `<math.h>`    | absolute value of a `double`                                            |
| [`fabsf`](https://en.cppreference.com/c/numeric/math/fabs)                                                     | `<math.h>`    | absolute value of a `float`                                             |
| [`fclose`](https://en.cppreference.com/c/io/fclose)                                                            | `<stdio.h>`   | finish writing a file, and close it                                     |
| [`floor`](https://en.cppreference.com/c/numeric/math/floor)                                                    | `<math.h>`    | largest whole number not above x                                        |
| [`fmax`](https://en.cppreference.com/c/numeric/math/fmax)                                                      | `<math.h>`    | the larger of two numbers (and not a NaN, if one is)                    |
| [`fopen`](https://en.cppreference.com/c/io/fopen)                                                              | `<stdio.h>`   | open a file                                                             |
| [`fpclassify`](https://en.cppreference.com/c/numeric/math/fpclassify)                                          | `<math.h>`    | what kind of number: zero, subnormal, normal, infinite or NaN (a macro) |
| [`fprintf`](https://en.cppreference.com/c/io/fprintf)                                                          | `<stdio.h>`   | `printf` to a stream, such as `stderr`                                  |
| [`fread`](https://en.cppreference.com/c/io/fread)                                                              | `<stdio.h>`   | read raw bytes from a file                                              |
| [`free`](https://en.cppreference.com/c/memory/free)                                                            | `<stdlib.h>`  | give back memory from `malloc`, `calloc` or `realloc`                   |
| [`fwrite`](https://en.cppreference.com/c/io/fwrite)                                                            | `<stdio.h>`   | write raw bytes to a file                                               |
| [`ldexp`](https://en.cppreference.com/c/numeric/math/ldexp)                                                    | `<math.h>`    | x times 2 to the power e                                                |
| [`log10`](https://en.cppreference.com/c/numeric/math/log10)                                                    | `<math.h>`    | base-10 logarithm of a `double`                                         |
| [`log10f`](https://en.cppreference.com/c/numeric/math/log10)                                                   | `<math.h>`    | base-10 logarithm of a `float`                                          |
| [`log2`](https://en.cppreference.com/c/numeric/math/log2)                                                      | `<math.h>`    | base-2 logarithm                                                        |
| [`lround`](https://en.cppreference.com/c/numeric/math/round)                                                   | `<math.h>`    | round to the nearest whole number, halves away from 0, as a `long`      |
| [`malloc`](https://en.cppreference.com/c/memory/malloc)                                                        | `<stdlib.h>`  | allocate memory, contents unset                                         |
| [`memcmp`](https://en.cppreference.com/c/string/byte/memcmp)                                                   | `<string.h>`  | compare two blocks of bytes                                             |
| [`memcpy`](https://en.cppreference.com/c/string/byte/memcpy)                                                   | `<string.h>`  | copy bytes between blocks that don't overlap                            |
| [`memset`](https://en.cppreference.com/c/string/byte/memset)                                                   | `<string.h>`  | fill a block of bytes with one value                                    |
| [`offsetof`](https://en.cppreference.com/c/types/offsetof)                                                     | `<stddef.h>`  | a member's distance from the start of its struct (a macro)              |
| [`perror`](https://en.cppreference.com/c/io/perror)                                                            | `<stdio.h>`   | print a message, and why the last call failed                           |
| [`pow`](https://en.cppreference.com/c/numeric/math/pow)                                                        | `<math.h>`    | x raised to the power y                                                 |
| [`printf`](https://en.cppreference.com/c/io/fprintf)                                                           | `<stdio.h>`   | formatted output to `stdout`                                            |
| [`pthread_cond_signal`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_signal.html)   | `<pthread.h>` | wake one thread sleeping on a condition variable (POSIX)                |
| [`pthread_cond_wait`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_wait.html)       | `<pthread.h>` | release a mutex and sleep until signalled (POSIX)                       |
| [`pthread_create`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_create.html)             | `<pthread.h>` | start a thread running a function (POSIX)                               |
| [`pthread_join`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_join.html)                 | `<pthread.h>` | wait for a thread to finish (POSIX)                                     |
| [`pthread_mutex_lock`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_mutex_lock.html)     | `<pthread.h>` | take a mutex, waiting while another thread holds it (POSIX)             |
| [`pthread_mutex_unlock`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_mutex_unlock.html) | `<pthread.h>` | release a mutex (POSIX)                                                 |
| [`puts`](https://en.cppreference.com/c/io/puts)                                                                | `<stdio.h>`   | write a string and a newline to `stdout`                                |
| [`qsort`](https://en.cppreference.com/c/algorithm/qsort)                                                       | `<stdlib.h>`  | sort an array, given a function that compares two elements              |
| [`realloc`](https://en.cppreference.com/c/memory/realloc)                                                      | `<stdlib.h>`  | resize allocated memory, perhaps moving it                              |
| [`remove`](https://en.cppreference.com/c/io/remove)                                                            | `<stdio.h>`   | delete a file                                                           |
| [`round`](https://en.cppreference.com/c/numeric/math/round)                                                    | `<math.h>`    | round to the nearest whole number, halves away from 0                   |
| [`sin`](https://en.cppreference.com/c/numeric/math/sin)                                                        | `<math.h>`    | sine of an angle in radians                                             |
| [`snprintf`](https://en.cppreference.com/c/io/fprintf)                                                         | `<stdio.h>`   | `printf` into a buffer, never past its size                             |
| [`sqrt`](https://en.cppreference.com/c/numeric/math/sqrt)                                                      | `<math.h>`    | square root                                                             |
| [`strcmp`](https://en.cppreference.com/c/string/byte/strcmp)                                                   | `<string.h>`  | compare two strings                                                     |
| [`strcpy`](https://en.cppreference.com/c/string/byte/strcpy)                                                   | `<string.h>`  | copy a string, with no check of the room                                |
| [`strlen`](https://en.cppreference.com/c/string/byte/strlen)                                                   | `<string.h>`  | a string's length, not counting its `'\0'`                              |
| [`strtod`](https://en.cppreference.com/c/string/byte/strtof)                                                   | `<stdlib.h>`  | read a `double` from a string                                           |
| [`strtol`](https://en.cppreference.com/c/string/byte/strtol)                                                   | `<stdlib.h>`  | read a `long` from a string                                             |
| [`strtoul`](https://en.cppreference.com/c/string/byte/strtoul)                                                 | `<stdlib.h>`  | read an `unsigned long` from a string                                   |

## References

Where to read more. The links in the tables above are to cppreference, because
it documents what the C *standard* promises, which is what every compiler and
library in this course must do. A real library does more, and documents it
itself.

| reference                                                                                                   | what it is                                                                                                                 |
| ----------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| [cppreference: C](https://en.cppreference.com/c)                                                            | the standard library, function by function, with the C version that added each                                             |
| [ISO C99, draft N1256](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf)                          | the C99 standard itself (the last free draft, with corrections): the final word on what is defined                         |
| [The GNU C Library manual](https://www.gnu.org/software/libc/manual/)                                       | glibc, the C library on most Linux systems: the standard functions with longer explanations, plus GNU and POSIX extensions |
| [musl](https://musl.libc.org/)                                                                              | a small, strict C library for Linux (Alpine uses it): a good test that code relies on the standard, not on glibc           |
| [FreeBSD manual pages](https://man.freebsd.org/cgi/man.cgi)                                                 | the BSD C library, which macOS's is descended from: closest to `man 3 printf` on a Mac                                     |
| [Microsoft C runtime](https://learn.microsoft.com/en-us/cpp/c-runtime-library/c-run-time-library-reference) | the C library on Windows, with MSVC                                                                                        |
| [POSIX (2024)](https://pubs.opengroup.org/onlinepubs/9799919799/)                                           | the Unix standard: functions beyond C, such as `open`, `read` and threads, shared by Linux and macOS                       |

On Linux and macOS, `man 3 printf` shows the manual for the library you are
actually using.
