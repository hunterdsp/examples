# examples

Examples that don't belong to any one repo. Scripts live in `scripts/`;
each runs straight from GitHub with [`jbx`](https://just-buildit.github.io/just-bashit/just-runit/),
no need to clone.

## Get `jbx`

A single curl and it's available immediately:

```bash
. <(curl -sSL https://just-buildit.github.io/get-jb.sh)   # jbx, in this shell
jbx -h
```

Run a script directly from a repo - no download, no clone:

```bash
jbx gh:hunterdsp/examples/scripts/hello-world.sh
# Hello World!
```

### Pin it

`-k` verifies the script before anything runs, and a mismatch is refused.
Pin the commit too (`@<sha>`): a hash against `main` breaks on the next edit,
while this pair always means exactly these bytes.

```bash
jbx -k sha256:5c847edda073c53e57fa6ece0a3d3e2c36551c37fc097d230806b43f9d499ab4 \
    gh:hunterdsp/examples/scripts/hello-world.sh@50b94ecae1a1298160104edf89e67856001818e0
# Hello World!
```

To pin a script yourself, hash it at the commit you reviewed:

```bash
c=50b94ecae1a1298160104edf89e67856001818e0
curl -sSL https://raw.githubusercontent.com/hunterdsp/examples/$c/scripts/hello-world.sh |
    sha256sum
```

## One script, many functions

Name a function after the script and only that one runs, with the rest of
the arguments:

```bash
jbx gh:hunterdsp/examples/scripts/greet.sh hello Ada
# Hello, Ada!
jbx gh:hunterdsp/examples/scripts/greet.sh shout see you soon
# SEE YOU SOON!
```

Not sure what's in a script? `-l` lists its functions and runs nothing:

```bash
jbx -l gh:hunterdsp/examples/scripts/greet.sh
```

## Short names

Register this repo as a namespace once, and every script in it gets a short
name:

```bash
just-runit ns add examples gh:hunterdsp/examples/scripts
jbx examples:hello
# Hello World!
jbx examples:greet goodbye Ada
# Goodbye, Ada!
```

The short names come from [`scripts/aliases.toml`](scripts/aliases.toml).
Add `@<sha>` to the base (`gh:hunterdsp/examples/scripts@<sha>`) to pin
every script in the namespace to that commit.

## Scripts

| Script                   | What it shows                                     |
| ------------------------ | ------------------------------------------------- |
| `scripts/hello-world.sh` | The smallest thing jbx can run                    |
| `scripts/greet.sh`       | Several functions; jbx runs only the one you name |
| `scripts/aliases.toml`   | The short names behind `jbx examples:NAME`        |
