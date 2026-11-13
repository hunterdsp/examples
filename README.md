# examples

[![c-by-example](https://github.com/hunterdsp/examples/actions/workflows/c-by-example.yml/badge.svg)](https://github.com/hunterdsp/examples/actions/workflows/c-by-example.yml)
[![lint](https://github.com/hunterdsp/examples/actions/workflows/lint.yml/badge.svg)](https://github.com/hunterdsp/examples/actions/workflows/lint.yml)
[![License: MIT](https://img.shields.io/github/license/hunterdsp/examples?logo=opensourceinitiative&logoColor=white)](LICENSE)
[![C99](https://img.shields.io/badge/C-99-informational?logo=c&logoColor=white)](https://en.cppreference.com/w/c/99)
[![Python 3.9+](https://img.shields.io/badge/Python-3.9%2B-informational?logo=python&logoColor=white)](https://www.python.org/)
[![shellcheck](https://img.shields.io/badge/shellcheck-enabled-brightgreen)](https://www.shellcheck.net/)
[![shfmt](https://img.shields.io/badge/shfmt-conformant-blue)](https://github.com/mvdan/sh#shfmt)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit)](.pre-commit-config.yaml)
[![jbx](https://img.shields.io/badge/-jbx-1a3050?labelColor=0d1b2a&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjE0IDQ0IDE0MCAxMTIiPjxwYXRoIGQ9Ik0zMCA2MiBMODYgMTAwIEwzMCAxMzgiIGZpbGw9Im5vbmUiIHN0cm9rZT0iI2Y1OWUwYiIgc3Ryb2tlLXdpZHRoPSIxOCIgc3Ryb2tlLWxpbmVjYXA9InNxdWFyZSIgc3Ryb2tlLWxpbmVqb2luPSJtaXRlciIvPjxyZWN0IHg9IjEwNCIgeT0iMTI5IiB3aWR0aD0iNDAiIGhlaWdodD0iMTgiIGZpbGw9IiNmZmZmZmYiLz48L3N2Zz4K)](https://just-buildit.github.io/just-bashit/just-runit/#getting-just-runit)

Examples are a great way to learn new tools and languages.

That's the whole point of this space: one repo dedicated to various examples.

## Contents

| Example                      | What it is                                                     |
| -----------------------------|--------------------------------------------------------------- |
| [`scripts/`][scr]            | Shell scripts to demo [`jbx`][jbx] run-from-web features       |
| [`python-c-extension/`][pcx] | C library + automatic Python binding using [`just-makeit`][jm] |
| [`c-by-example/`][cbe]       | Learning C by Example: from an empty file to a C library       |

## Contributing

Install the pre-commit hooks once per clone, and every commit runs the checks
for the files it touches:

```bash
make hooks    # needs uv: https://docs.astral.sh/uv/
```

`make lint` runs every check, `make format` fixes the formatting ones, and
`make test` builds and checks every c-by-example lesson. CI runs the same
targets.

<!-- Define your link aliases anywhere at the bottom of the file -->
[jbx]: https://just-buildit.github.io/just-bashit/just-runit/
[jm]: https://just-buildit.github.io/just-makeit/
[pcx]: python-c-extension/
[cbe]: c-by-example/
[scr]: scripts/
