from typing import final

import numpy as np
import numpy.typing as npt
from numpy.typing import NDArray

@final
class Gain:
    """Gain component.

    Parameters
    ----------
    k : float, default 2.0
        k state variable.

    Examples
    --------
    Create with defaults:

    >>> from hello import Gain
    >>> obj = Gain(k=2.0)
    >>> obj.get_k()
    2.0

    Reset restores defaults:

    >>> obj.set_k(0.0)
    >>> obj.reset()
    >>> obj.get_k()
    2.0

    """

    def __init__(self, k: float = 2.0) -> None: ...
    def reset(self) -> None:
        """Reset state to post-create defaults."""

    def step(self, x: complex, /) -> complex:
        """Process one input sample.

        Parameters
        ----------
        x : complex
            Input sample.

        Returns
        -------
        complex
            Output sample.
        """

    def steps(
        self,
        x: npt.NDArray[np.complex64],
        out: npt.NDArray[np.complex64] | None = None,
    ) -> NDArray[np.complex64]:
        """Process a samples array. Returns ndarray, or fills out= if supplied.

        Parameters
        ----------
        x : npt.NDArray[np.complex64]
            Input sample.

        Returns
        -------
        NDArray[np.complex64]
            Output sample.
        """

    def get_k(self) -> float:
        """Return current k."""

    def set_k(self, value: float, /) -> None:
        """Set k."""

    def destroy(self) -> None:
        """Release the underlying C resources immediately.

        Ordinarily unnecessary: the resources are freed when the object is
        garbage-collected. Call this to release them at a definite point
        instead, or use the object as a context manager, which calls it on
        exit.

        Idempotent: calling it again on an already-released object does
        nothing. Every other method raises ``RuntimeError`` once it has run.
        """

    def __enter__(self) -> "Gain":
        """Enter a context manager, returning this object.

        Lets a Gain be used in a `with` statement so its C resources are
        released deterministically on exit rather than at collection time.

        Returns
        -------
        Gain
            This same object, not a copy.
        """
    def __exit__(
        self,
        exc_type: object | None = ...,
        exc: object | None = ...,
        tb: object | None = ...,
        /,
    ) -> None:
        """Exit a context manager, releasing the Gain.

        Equivalent to calling `destroy()`. Returns ``None``, so an exception
        raised inside the `with` body propagates normally; this never
        suppresses one.

        Parameters
        ----------
        exc_type : object | None
            Exception class, or None. Ignored.
        exc : object | None
            Exception instance, or None. Ignored.
        tb : object | None
            Traceback object, or None. Ignored.
        """
