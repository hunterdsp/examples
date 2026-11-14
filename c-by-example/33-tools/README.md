# 33 · Bundled tools

Run this lesson's commands in `c-by-example/33-tools/`.

A library is used from C. Most libraries also ship a few small programs, so
that the library can be tried, scripted and debugged from a shell: libjpeg has
`cjpeg` and `djpeg`, `libpng` has `pngfix`, and FFmpeg's libraries have
`ffmpeg`. `sig` ships three, each doing one job, written to be joined by
*pipes*: the shell's `|` sends one program's standard output into the next
one's standard input.

| tool                                   | reads   | writes                                            |
| -------------------------------------- | ------- | ------------------------------------------------- |
| `sig-tone SECONDS FREQ_HZ...`          | nothing | sines of amplitude 0.5                            |
| `sig-filter lowpass\|highpass FREQ_HZ` | samples | the samples, filtered                             |
| `sig-fft`                              | samples | the frequencies in the last 4096 samples, as text |

The samples travel as **raw `float`s**, lesson 06's format: each one's four
bytes, exactly as they are in memory, one after another, at 48000 samples per
second. No header, no text, nothing to parse. The middle tool:

```bash
cat ../sig/tools/sig-filter.c
```

```c
/* sig-filter -- filter raw float samples from stdin to stdout.
 *
 *   sig-filter lowpass|highpass FREQ_HZ
 *
 * A Butterworth biquad (q = 0.7071) with its corner at FREQ_HZ, for
 * samples at 48000 per second, read and written as raw native floats.
 */
#include <sig/biquad.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
  RATE  = 48000,
  BLOCK = 1024
};

int
main (int argc, char *argv[])
{
  if (argc != 3
      || (strcmp (argv[1], "lowpass") != 0
          && strcmp (argv[1], "highpass") != 0))
    {
      fprintf (stderr, "usage: sig-filter lowpass|highpass FREQ_HZ\n");
      return 2;
    }
  sig_biquad_type type
      = strcmp (argv[1], "lowpass") == 0 ? SIG_LOWPASS : SIG_HIGHPASS;
  char  *end;
  double f0 = strtod (argv[2], &end);
  if (end == argv[2] || *end != '\0')
    {
      fprintf (stderr, "sig-filter: %s: not a number\n", argv[2]);
      return 2;
    }

  sig_biquad *f;
  sig_status  s = sig_biquad_create (type, f0, 0.7071, RATE, &f);
  if (s != SIG_OK)
    {
      fprintf (stderr, "sig-filter: %s Hz: %s\n", argv[2], sig_strerror (s));
      return 1;
    }

  float  buf[BLOCK];
  size_t n;
  int    status = 0;
  while ((n = fread (buf, sizeof buf[0], BLOCK, stdin)) > 0)
    {
      sig_biquad_process (f, buf, buf, n); /* in place */
      if (fwrite (buf, sizeof buf[0], n, stdout) != n)
        {
          perror ("sig-filter: write");
          status = 1;
          break;
        }
    }
  if (ferror (stdin))
    {
      perror ("sig-filter: read");
      status = 1;
    }
  sig_biquad_destroy (f);
  return status;
}
```

[`sig-tone.c`](../sig/tools/sig-tone.c) and
[`sig-fft.c`](../sig/tools/sig-fft.c) are built the same way. CMake builds all
three, linked with the library (lesson 30):

```bash
cmake -S ../sig -B build -DSIG_SANITIZE=ON
cmake --build build
```

## Pipes

Two tones, analysed:

```bash
build/sig-tone 1 750 6000 | build/sig-fft
```

```
  750.00 Hz  0.500
 6000.00 Hz  0.500
```

Through a low-pass at 1500 Hz first, which keeps 97% of 750 Hz and less than 6%
of 6000 Hz:

```bash
build/sig-tone 1 750 6000 | build/sig-filter lowpass 1500 | build/sig-fft
```

```
  750.00 Hz  0.485
 6000.00 Hz  0.028
```

And a high-pass, the other way round:

```bash
build/sig-tone 1 750 6000 | build/sig-filter highpass 1500 | build/sig-fft
```

```
  750.00 Hz  0.121
 6000.00 Hz  0.499
```

One second is 48000 samples of 4 bytes. `wc -c` counts bytes, and `tr -d ' '`
removes the spaces macOS's `wc` puts in front:

```bash
build/sig-tone 1 750 | wc -c | tr -d ' '
```

```
192000
```

## When something is wrong

A tool talks to two audiences. **Standard output** is data, for the next
program in the pipe; **standard error** is messages, for the person at the
keyboard, and goes to the terminal even when standard output is piped. The
**exit status** tells a script what happened: 0 for success, 1 for a failure,
and 2, by convention, for a command line it didn't understand:

```bash
build/sig-tone 1 30000
echo "exit status $?"
build/sig-filter bandpass 1000
echo "exit status $?"
```

```
sig-tone: 30000 Hz: argument out of range
exit status 1
usage: sig-filter lowpass|highpass FREQ_HZ
exit status 2
```

The message starts with the tool's name, so in a pipe of five tools it is clear
which one is complaining, and it comes from `sig_strerror`: the library's
words, not the tool's guess.

## What it shows

- **Thin tools, thick library.** `sig-filter.c` parses its arguments, moves
  blocks of samples and reports errors. Every line of signal processing is in
  the library, so the tools and every other program that uses `sig` share it,
  and the tests cover it (lesson 31).
- **Blocks.** `fread` asks for 1024 samples and returns how many it got: fewer
  at the end of the input, and 0 when it is over, which is what ends the loop.
  The filter keeps its state between calls, so blocks join with no seam (lesson
  14).
- **In place.** `sig_biquad_process (f, buf, buf, n)` filters in place, which
  the header promises is allowed (and lesson 31's exercise 3 tests).
- **Read errors too.** A `while` that stops at `fread` returning 0 can't tell
  the end of the input from a failure, so `ferror (stdin)` asks afterwards.
- **A test of all three.** The `pipeline` test in `CMakeLists.txt` runs the
  low-pass pipe above and checks its output, so the tools are tested with the
  library.

## Pitfalls

- **Raw floats are this machine's floats.** Every machine this course runs on
  stores a `float` in IEEE 754 single precision, with its lowest byte first
  (*little-endian*), so the bytes mean the same everywhere it runs. A file
  written on a big-endian machine would be nonsense here. A format meant to
  travel says which order it uses, and writes bytes in that order explicitly
  (exercise 2).

- **Windows translates newlines.** On Windows, `stdout` is opened in text mode:
  every byte 10 written becomes the two bytes 13 10, which corrupts binary
  data. A tool that writes raw samples there must switch `stdout` to binary
  first (with `_setmode`, from Microsoft's C library). Linux and macOS make no
  distinction.

- **The reader stops early.** In `sig-tone 3600 750 | head -c 8`, `head` exits
  after 8 bytes, and the next write to the pipe fails. By default the system
  then stops `sig-tone` with the signal `SIGPIPE`, quietly, which is what a
  pipe's writer usually wants. If the signal is ignored, as some programs that
  start others arrange, `fwrite` fails instead, and `sig-tone` reports the
  failure and exits with status 1.

## Exercises

Try each one before opening its answer. Its code is in
[`exercises/`](exercises/), one folder per exercise, and the answer says which
folder to run it in.

1. Write `sig-gain FACTOR`, which multiplies every sample by `FACTOR`, and put
   it between `sig-tone` and `sig-fft`.

   <details><summary>Output and answer</summary>

   In `exercises/ex1-gain/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined sig-gain.c -o sig-gain
   ../../build/sig-tone 1 750 | ./sig-gain 0.5 | ../../build/sig-fft
   ```

   ```
     750.00 Hz  0.250
   ```

   [`sig-gain.c`](exercises/ex1-gain/sig-gain.c) doesn't use `sig` at all. Raw
   floats on standard input and output are the whole interface, so a tool can
   be written in anything that can read and write bytes, by anyone, and join
   the pipe.

   </details>

2. Write `sig-wav FILE.wav`, which turns samples from standard input into a
   16-bit WAV file, so you can listen to them.

   <details><summary>Output and answer</summary>

   In `exercises/ex2-wav/`:

   ```bash
   cc -std=c99 -Wall -Wextra -g -fsanitize=address,undefined sig-wav.c -o sig-wav
   ../../build/sig-tone 1 750 6000 | ./sig-wav tone.wav
   ```

   A WAV file starts with the letters `RIFF`, and is 44 bytes of header and 2
   bytes for each of the 48000 samples:

   ```bash
   head -c 4 tone.wav
   echo
   wc -c < tone.wav | tr -d ' '
   ```

   ```
   RIFF
   96044
   ```

   And Python's own `wave` module, which knows nothing of `sig`, reads it as
   one channel of 2-byte samples at 48000 per second, 48000 of them:

   ```bash
   python3 -c "import wave; w = wave.open('tone.wav'); print(w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes())"
   ```

   ```
   1 2 48000 48000
   ```

   [`sig-wav.c`](exercises/ex2-wav/sig-wav.c) writes every number in the header
   a byte at a time, lowest byte first, because the WAV format says
   little-endian, whatever the machine. The header holds the length of the
   data, which isn't known until standard input ends, so it is written first
   with zeros and rewritten at the end, after `fseek` goes back to the start. A
   file can seek, and a pipe can't, so this tool writes to a file. The
   conversion to 16 bits rounds and saturates (lesson 27). Play `tone.wav` with
   any audio player.

   </details>

3. Make a fourth-order low-pass filter, which falls twice as steeply as
   `sig-filter`'s, without writing any C.

   <details><summary>Output and answer</summary>

   This one uses the lesson's own folder. Two biquads in a row (lesson 21's
   cascade) are one fourth-order filter, and in a pipe that is two
   `sig-filter`s:

   ```bash
   build/sig-tone 1 750 6000 | build/sig-filter lowpass 1500 | build/sig-filter lowpass 1500 | build/sig-fft
   ```

   ```
     750.00 Hz  0.471
   ```

   6000 Hz went from 0.028 to under 0.01, and dropped off the list; 750 Hz lost
   another 3%. Small tools that do one thing combine into things none of them
   does alone, without anyone writing a line of code for it. (Two Butterworth
   filters in a row aren't a fourth-order *Butterworth*, which would need two
   different `q`s, but they fall as steeply.)

   </details>

## Library functions

<!-- libfuncs: made by `make format` from REFERENCE.md -->

Every library function this lesson's code calls, linked to its definition. The
[quick reference](../REFERENCE.md) lists them all.

| function                                                     | header       | what it does                                      |
| ------------------------------------------------------------ | ------------ | ------------------------------------------------- |
| [`fclose`](https://en.cppreference.com/c/io/fclose)          | `<stdio.h>`  | finish writing a file, and close it               |
| [`ferror`](https://en.cppreference.com/c/io/ferror)          | `<stdio.h>`  | did a read or write on this stream fail?          |
| [`fopen`](https://en.cppreference.com/c/io/fopen)            | `<stdio.h>`  | open a file                                       |
| [`fprintf`](https://en.cppreference.com/c/io/fprintf)        | `<stdio.h>`  | `printf` to a stream, such as `stderr`            |
| [`fputs`](https://en.cppreference.com/c/io/fputs)            | `<stdio.h>`  | write a string to a stream, with no newline added |
| [`fread`](https://en.cppreference.com/c/io/fread)            | `<stdio.h>`  | read raw bytes from a file                        |
| [`fseek`](https://en.cppreference.com/c/io/fseek)            | `<stdio.h>`  | move to a position in a file                      |
| [`fwrite`](https://en.cppreference.com/c/io/fwrite)          | `<stdio.h>`  | write raw bytes to a file                         |
| [`perror`](https://en.cppreference.com/c/io/perror)          | `<stdio.h>`  | print a message, and why the last call failed     |
| [`putc`](https://en.cppreference.com/c/io/fputc)             | `<stdio.h>`  | write one byte (a character) to a stream          |
| [`strtof`](https://en.cppreference.com/c/string/byte/strtof) | `<stdlib.h>` | read a `float` from a string                      |

<!-- /libfuncs -->
