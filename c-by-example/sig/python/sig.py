"""sig.py -- the sig C library, from Python, through ctypes.

ctypes is in Python's standard library: it loads a shared library and
calls its C functions, given each one's argument and result types. Those
types are the C header, written again in Python, and nothing checks that
the two agree, so they sit together at the top of this file, one line
per function, in the order of the headers.

The library is the shared build of sig (lesson 34):

    cmake -S .. -B build -DBUILD_SHARED_LIBS=ON
    cmake --build build

and SIG_LIBRARY names it, e.g. build/libsig.so (Linux) or
build/libsig.dylib (macOS).

    >>> import sig
    >>> osc = sig.Osc(750.0, 48000.0)
    >>> [round(x, 4) for x in osc.process(4)]
    [0.0, 0.098, 0.1951, 0.2903]
"""

import ctypes
import os

_lib = ctypes.CDLL(os.environ["SIG_LIBRARY"])

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
