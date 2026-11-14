# 35 · Epilogue: the same library, from Python

Run this lesson's commands in `c-by-example/35-python/`.

C's last advantage is that every other language can call it. Python, Rust, Go,
Julia, Java and JavaScript all have a way to load a shared library and call its
functions, because C's calling convention, how arguments are passed and results
returned, is the one every system defines, and a C function's name in the
library is simply its name, where C++ encodes its argument types into it. That
is why so much of the world's numerical code, NumPy's included, is C
underneath, and a C library written once serves them all.

Python's simplest way in is `ctypes`, in its standard library: no compiler, no
build step, just the shared library of lesson 34. `sig`'s Python wrapper:

```bash
cat ../sig/python/sig.py
```

```python
"""sig.py -- the sig C library, from Python, through ctypes.

ctypes is in Python's standard library: it loads a shared library and
calls its C functions, given each one's argument and result types. Those
types are the C header, written again in Python, and nothing checks that
the two agree, so they sit together at the top of this file, one line
per function, in the order of the headers.

The library is the shared build of sig (lesson 34):

    cmake -S .. -B build -DBUILD_SHARED_LIBS=ON
    cmake --build build

and SIG_LIBRARY names it: the file, or the folder it is in, where this
module looks for the name this system gives it (libsig.so on Linux,
libsig.dylib on macOS).

    >>> import sig
    >>> osc = sig.Osc(750.0, 48000.0)
    >>> [round(x, 4) for x in osc.process(4)]
    [0.0, 0.098, 0.1951, 0.2903]
"""

import ctypes
import os
import sys


def _path():
    """The shared library SIG_LIBRARY names, or names the folder of."""
    path = os.environ["SIG_LIBRARY"]
    if os.path.isdir(path):
        name = "libsig.dylib" if sys.platform == "darwin" else "libsig.so"
        path = os.path.join(path, name)
    return path


_lib = ctypes.CDLL(_path())

_status = ctypes.c_int  # sig_status: an enum, which is an int
_size = ctypes.c_size_t
_float_p = ctypes.POINTER(ctypes.c_float)
_handle = ctypes.c_void_p  # every sig object is opaque: just an address


def _declare(name, result, *args):
    """Give C function `name` its result and argument types."""
    fn = getattr(_lib, name)
    fn.restype = result
    fn.argtypes = list(args)


_out = ctypes.POINTER(_handle)
# common.h
_declare("sig_strerror", ctypes.c_char_p, _status)
_declare("sig_version", ctypes.c_char_p)
# osc.h
_declare("sig_osc_create", _status, ctypes.c_double, ctypes.c_double, _out)
_declare("sig_osc_destroy", None, _handle)
_declare("sig_osc_process", None, _handle, _float_p, _size)
# biquad.h: sig_biquad_type is an enum, SIG_LOWPASS = 0, SIG_HIGHPASS = 1
_declare(
    "sig_biquad_create",
    _status,
    ctypes.c_int,
    ctypes.c_double,
    ctypes.c_double,
    ctypes.c_double,
    _out,
)
_declare("sig_biquad_destroy", None, _handle)
_declare("sig_biquad_process", None, _handle, _float_p, _float_p, _size)
_declare("sig_biquad_reset", None, _handle)
# fft.h
_declare("sig_fft_create", _status, _size, _out)
_declare("sig_fft_destroy", None, _handle)
_declare("sig_fft_size", _size, _handle)
_declare("sig_fft_amplitude", None, _handle, _float_p, _float_p)


class SigError(Exception):
    """A sig function returned something other than SIG_OK."""


def version():
    """The linked library's version, as "MAJOR.MINOR.PATCH"."""
    return _lib.sig_version().decode()


def _create(name, *args):
    """Call a sig_..._create function; return the new object's address,
    or raise SigError with sig_strerror's description."""
    handle = _handle()
    status = getattr(_lib, name)(*args, ctypes.byref(handle))
    if status != 0:
        raise SigError(_lib.sig_strerror(status).decode())
    return handle


def _floats(samples):
    """A C array of floats holding samples (any sequence of numbers)."""
    return (ctypes.c_float * len(samples))(*samples)


class _Object:
    """Owns one C object, and destroys it exactly once: on close(), at
    the end of a `with` block, or when Python frees the wrapper."""

    _destroy = ""

    def close(self):
        if getattr(self, "_h", None):
            getattr(_lib, self._destroy)(self._h)
            self._h = None

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def __del__(self):
        self.close()


class Osc(_Object):
    """A sine oscillator: sig_osc."""

    _destroy = "sig_osc_destroy"

    def __init__(self, freq_hz, rate_hz):
        self._h = _create("sig_osc_create", freq_hz, rate_hz)

    def process(self, n):
        """The next n samples, as a list of floats."""
        out = (ctypes.c_float * n)()
        _lib.sig_osc_process(self._h, out, n)
        return list(out)


class Biquad(_Object):
    """A low-pass or high-pass filter: sig_biquad."""

    _destroy = "sig_biquad_destroy"
    _types = {"lowpass": 0, "highpass": 1}

    def __init__(self, kind, f0_hz, q, rate_hz):
        if kind not in self._types:
            raise ValueError(f"kind must be 'lowpass' or 'highpass': {kind!r}")
        self._h = _create(
            "sig_biquad_create", self._types[kind], f0_hz, q, rate_hz
        )

    def process(self, samples):
        """The samples, filtered, as a new list."""
        buf = _floats(samples)
        _lib.sig_biquad_process(self._h, buf, buf, len(buf))
        return list(buf)

    def reset(self):
        _lib.sig_biquad_reset(self._h)


class FFT(_Object):
    """The amplitude spectrum of blocks of n samples: sig_fft."""

    _destroy = "sig_fft_destroy"

    def __init__(self, n):
        self._h = _create("sig_fft_create", n)
        self.n = _lib.sig_fft_size(self._h)

    def amplitude(self, samples):
        """The amplitudes of bins 0 to n / 2, as a list."""
        if len(samples) != self.n:
            raise ValueError(f"need {self.n} samples, got {len(samples)}")
        amp = (ctypes.c_float * (self.n // 2 + 1))()
        _lib.sig_fft_amplitude(self._h, _floats(samples), amp)
        return list(amp)
```

## Using it

A shared, optimised build (lesson 34), without the sanitizers, for a reason the
pitfalls explain:

```bash
cmake -S ../sig -B build -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Lesson 34's `analyse.c`, in Python:

```bash
cat spectrum.py
```

```python
"""spectrum.py -- lesson 34's analyse.c, in Python, with the same library.

Two tones, a low-pass filter, and the amplitude spectrum of the last 4096
samples: the same calls to the same compiled code, so the same numbers.
"""

import sig

RATE = 48000
N = 4096

a = sig.Osc(750.0, RATE)
b = sig.Osc(6000.0, RATE)
x = [0.5 * p + 0.5 * q for p, q in zip(a.process(RATE), b.process(RATE))]

with sig.Biquad("lowpass", 1500.0, 0.7071, RATE) as lp:
    x = lp.process(x)
with sig.FFT(N) as fft:
    amp = fft.amplitude(x[-N:])

print(f"sig {sig.version()}, from Python")
for k, v in enumerate(amp):
    if v > 0.01:
        print(f"{k * RATE / N:8.2f} Hz  {v:.3f}")
```

`SIG_LIBRARY` tells the wrapper where the library is, and `PYTHONPATH` tells
Python where the wrapper is:

```bash
SIG_LIBRARY=build PYTHONPATH=../sig/python python3 spectrum.py
```

```
sig 1.0.0, from Python
  750.00 Hz  0.485
 6000.00 Hz  0.028
```

The numbers of lesson 33's pipe and lesson 34's program, because it is the same
compiled code doing the work.

## What it shows

- **The header, written again.** For each function, `_declare` tells `ctypes`
  its result type (`restype`) and argument types (`argtypes`), which `ctypes`
  can't read from a C header. With them, `ctypes` converts Python's numbers to
  C's `double`s and `size_t`s, and refuses arguments of the wrong kind.
- **Opaque objects stay opaque.** A `sig_osc *` is just an address to Python, a
  `c_void_p`, handed back to the functions that know what is there (lesson 14).
  Python can't reach inside, and doesn't need to.
- **Out-parameters.** `ctypes.byref (handle)` passes the address of a
  `c_void_p`, which is how `sig_osc_create`'s `sig_osc **out` gets filled in.
- **Statuses become exceptions.** `_create` turns any status but `SIG_OK` into
  a `SigError`, with `sig_strerror`'s words (exercise 2): Python's way of
  saying what C says with a return value.
- **Ownership becomes `with`.** Each wrapper owns one C object and destroys it
  exactly once: on `close()`, at the end of a `with` block, or when Python
  frees the wrapper. C's rule from lesson 10, one owner and one free, holds
  across the language boundary.

## Pitfalls

- **Nothing checks the declarations.** `argtypes` and `restype` are the header
  copied by hand, and if they disagree with the real one, `ctypes` passes the
  wrong bytes, with no error. Without a `restype`, `ctypes` assumes a C
  function returns an `int` (exercise 1). Keep them in one place, in the
  header's order, and test every function through them.

- **Sanitized libraries.** A library built with AddressSanitizer needs the
  sanitizer's own library loaded before anything else in the process, and
  Python is already running when it loads `sig`, so a `SIG_SANITIZE` build
  won't load. Test the C under the sanitizers (lesson 31), and give Python a
  plain build.

- **Crossing the boundary costs.** Every call converts its arguments, and a
  Python list of floats is copied into a C array and back. Call with large
  blocks, as `spectrum.py` does, not once per sample. Libraries that care about
  speed pass NumPy arrays, whose numbers are already a C array, without
  copying.

- **A crash is a crash.** C called from Python can still write out of bounds,
  and Python can't catch it: the whole interpreter stops. The wrapper is the
  place to check what C would trust, which is why `FFT.amplitude` checks the
  block's length before calling.

## Exercises

Try each one before opening its answer. All three use the lesson's own folder,
and its build.

1. Call `sig_version` through `ctypes` without declaring its `restype`. What
   comes back?

   <details><summary>Output and answer</summary>

   ```bash
   SIG_LIBRARY=build PYTHONPATH=../sig/python python3 -c "import ctypes, sig; lib = ctypes.CDLL(sig._path()); print(type(lib.sig_version()))"
   ```

   ```
   <class 'int'>
   ```

   An `int`: `ctypes` assumed the function returns one, and gave back the low
   32 bits of the string's 64-bit address, as a number. No error, no warning,
   and reading a string from that number would crash. A missing or wrong
   declaration is the most common `ctypes` bug, which is why `sig.py` declares
   every function once, at the top, from the header.

   </details>

2. Ask the wrapper for an oscillator at 30000 Hz, and a `bandpass` filter.

   <details><summary>Output and answer</summary>

   ```bash
   SIG_LIBRARY=build PYTHONPATH=../sig/python python3 -c "
   import sig
   try:
       sig.Osc(30000.0, 48000.0)
   except sig.SigError as e:
       print('SigError:', e)
   try:
       sig.Biquad('bandpass', 1000.0, 0.7071, 48000.0)
   except ValueError as e:
       print('ValueError:', e)
   "
   ```

   ```
   SigError: argument out of range
   ValueError: kind must be 'lowpass' or 'highpass': 'bandpass'
   ```

   The first is the C library's own check, `SIG_EINVAL`, turned into a Python
   exception with `sig_strerror`'s words. The second never reaches C: the
   wrapper turns a name into `sig_biquad_type`'s number, and refuses a name it
   doesn't know, because C would trust whatever number it was given.

   </details>

3. `sig.py`'s docstring shows an example with its output. Check that it is
   still true.

   <details><summary>Output and answer</summary>

   ```bash
   SIG_LIBRARY=build python3 -m doctest -v ../sig/python/sig.py | tail -1
   ```

   ```
   Test passed.
   ```

   `doctest` runs every `>>>` line in a module's docstrings and compares what
   it prints with the line below: lesson 32's runnable examples, built into
   Python. The wrapper's documentation is tested by the same command that reads
   it.

   </details>

## Where next

That is the course: from an empty `main` to a library with a build, tests,
documentation, tools, an install, and a second language. The ideas carry over
to any C library you will meet, and the best next step is to read some.
[KissFFT](https://github.com/mborgerding/kissfft) is an FFT small enough to
read in an afternoon. [FFTW](https://www.fftw.org/) is the FFT many others are
measured against, with its plans (lesson 24) taken much further.
[liquid-dsp](https://github.com/jgaeddert/liquid-dsp) is a signal-processing
library for software radio, built of `create`/`execute`/`destroy` objects much
like `sig`'s. The [C reference](../REFERENCE.md) collects every function and
flag the course used, with links to the standard.
