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
