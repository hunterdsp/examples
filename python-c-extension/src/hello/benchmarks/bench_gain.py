# jm:generated bench_gain.py
# jm regenerates this file on apply while the line above is here.
# To make it yours, delete that line: jm then never writes it again.
"""Benchmark for Gain.

Run standalone:  python src/hello/benchmarks/bench_gain.py
This file defines no pytest test, so neither `jm bench` nor `make bench`
runs it: it is a timeit script, run standalone.
"""

import time

import numpy as np

from hello import Gain

REPS = 1_000
BLOCK_1K = 1_024
BLOCK_64K = 65_536


def _bench(label: str, fn, *args, reps: int = REPS) -> float:
    for _ in range(max(1, reps // 10)):  # warmup
        fn(*args)
    t0 = time.perf_counter()
    for _ in range(reps):
        fn(*args)
    return (time.perf_counter() - t0) / reps


def main() -> None:
    obj = Gain(k=2.0)
    print("gain")
    dt = _bench("step", obj.step, 1.0 + 0.0j)
    print(f"  {'step':<22} {dt * 1e9:9.1f} ns/call")

    x1k = np.ones(BLOCK_1K, dtype=np.complex64)
    dt = _bench("steps 1k", obj.steps, x1k, reps=max(1, REPS // 10))
    msa = BLOCK_1K / dt / 1e6
    print(f"  {'steps 1k':<22} {dt * 1e6:9.3f} µs  ({msa:.1f} MSa/s)")
    x64k = np.ones(BLOCK_64K, dtype=np.complex64)
    dt = _bench("steps 64k", obj.steps, x64k, reps=max(1, REPS // 100))
    msa = BLOCK_64K / dt / 1e6
    print(f"  {'steps 64k':<22} {dt * 1e3:9.3f} ms  ({msa:.1f} MSa/s)")


if __name__ == "__main__":
    main()
