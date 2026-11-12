# jm:generated test_gain.py
# jm regenerates this file on apply while the line above is here.
# To make it yours, delete that line: jm then never writes it again.
import unittest

import numpy as np

from hello import Gain

# ---------------------------------------------------------------------------
# pytest compatibility shim — tests run under both pytest and unittest discover
# ---------------------------------------------------------------------------
try:
    import pytest as _pytest

    _approx = _pytest.approx
    _raises = _pytest.raises


except ImportError:
    import contextlib

    class _Approx:
        def __init__(self, expected, rel=1e-6):
            self._exp = expected
            self._tol = rel * (abs(expected) if expected else 1e-12)

        def __eq__(self, other):
            import cmath

            return cmath.isclose(
                complex(other), complex(self._exp), rel_tol=1e-6, abs_tol=1e-12
            )

        def __repr__(self):
            return f"approx({self._exp!r})"

    @contextlib.contextmanager
    def _raises(exc_type, match=None):
        import re

        try:
            yield
        except exc_type as e:
            if match and not re.search(match, str(e)):
                raise AssertionError(
                    f"Exception message {str(e)!r} did not match {match!r}"
                ) from e
        else:
            raise AssertionError(f"{exc_type.__name__} was not raised")

    _approx = _Approx


# ---------------------------------------------------------------------------


class TestGain(unittest.TestCase):
    def test_create(self):
        obj = Gain(k=2.0)
        self.assertIsNotNone(obj)

    def test_step_runs(self):
        obj = Gain(k=2.0)
        y = obj.step(1.0 + 0.0j)
        assert isinstance(y, complex)

    def test_steps_shape_dtype(self):
        obj = Gain(k=2.0)
        x = np.ones(64, dtype=np.complex64)
        y = obj.steps(x)
        self.assertEqual(y.shape, (64,))
        self.assertEqual(y.dtype, np.complex64)

    def test_steps_out_param(self):
        x = np.ones(64, dtype=np.complex64)
        buf = np.zeros(64, dtype=np.complex64)
        obj1 = Gain(k=2.0)
        ret = obj1.steps(x, buf)
        self.assertIs(ret, buf)
        obj2 = Gain(k=2.0)
        np.testing.assert_array_equal(ret, obj2.steps(x))

    def test_getter_setter(self):
        obj = Gain(k=2.0)
        assert obj.get_k() == _approx(2.0)
        obj.set_k(2.0)
        assert obj.get_k() == _approx(2.0)

    def test_reset(self):
        obj = Gain(k=2.0)
        obj.set_k(2.0)
        obj.reset()
        assert obj.get_k() == _approx(2.0)

    def test_context_manager(self):
        with Gain(k=2.0) as obj:
            y = obj.step(1.0 + 0.0j)
        assert isinstance(y, complex)

    def test_destroy(self):
        obj = Gain(k=2.0)
        obj.destroy()
        with _raises(RuntimeError, match="destroyed"):
            obj.step(1.0 + 0.0j)
