# hello

A C library with a Python binding, exactly as
[just-makeit](https://just-buildit.github.io/just-makeit/) 0.128.0 generates
it with:

```bash
. <(curl -fsSL https://just-buildit.github.io/just-makeit/install.sh)   # jm
jm new hello python-c-extension --object gain --state k:double:2.0
```

Nothing is edited by hand except this description, which replaces the
generated `TODO`.

To build it and run its tests (C through CTest, Python through unittest), in
`python-c-extension/`:

```bash
jbx install-deps -g dev   # compiler, CMake: whatever bootstrap.toml lists
make test
```

`Gain(k=2.0).step(x)` returns `x` unchanged: the generated `step()` is a
stub, marked `TODO`, in `native/inc/hello/gain/gain_core.h`. That body is
the part you write; everything around it is generated.

## Requirements

- Python 3.9+
- CMake ≥ 3.16
- A C99 compiler — GCC or Clang on Linux and macOS. On
  Windows, **clang-cl** (Visual Studio Build Tools' C++
  workload plus LLVM), run from a Developer PowerShell; the
  `Makefile` selects it with Ninja. Not MSVC's own `cl.exe`:
  it has no C99 `float _Complex`. To use the `Makefile`,
  install GNU make (`winget install ezwinports.make`); or open
  the folder in Visual Studio, which configures through the
  generated `CMakePresets.json` -- after creating `.venv`,
  with pip or `uv sync`, as
  [Building on Windows](https://just-buildit.github.io/just-makeit/windows/#the-projects-venv)
  shows.
- NumPy (installed automatically by `make` if missing)

Install the system build dependencies `bootstrap.toml` lists with
[jbx](https://just-buildit.github.io/just-bashit/just-runit/#getting-just-runit)
(one `curl` installs it), which picks your OS's package manager:

```bash
jbx install-deps -g dev
```

## Quickstart

Build the extension in place, then install the package in editable mode:

```bash
make                     # compiles the extension into src/hello/
pip install -e .         # points Python at src/; compiles nothing
```

`make` builds against the project's `.venv` when it has one, otherwise the
first `python` on `PATH`; `PYTHON=/path/to/python` picks another. Re-run
`make` after any C change; Python edits take effect immediately.

## Development build

```bash
make                     # cmake configure + build
make test                # CTest + unittest
```

## Package

```bash
pip wheel . --no-deps -w dist     # wheel -> dist/
```

`pyproject.toml` names just-buildit as the PEP 517 build backend, which runs
`make just-build`, so any frontend builds the wheel. `just-makeit build`
does the same when just-buildit is installed in the environment
just-makeit runs from (`pip install just-makeit just-buildit`).

The wheel is then repaired through `uvx`, so `uv` must be on `PATH`. It runs
`delocate` on macOS, `delvewheel` on Windows and `auditwheel` on Linux, which
also needs `patchelf`: `bootstrap.toml` lists it in every Linux dev group, so
`jbx install-deps -g dev` above installs it. To skip the repair, set
`repair = false` under `[tool.just-buildit]` in `pyproject.toml`.
