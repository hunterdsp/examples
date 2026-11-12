# examples

Examples that don't belong to any one repo. Scripts live in `scripts/`;
each runs straight from GitHub with [`jbx`](https://just-buildit.github.io/just-bashit/just-runit/),
so there's nothing to clone:

```bash
. <(curl -sSL https://just-buildit.github.io/get-jb.sh)   # jbx, in this shell
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

| Script                   | What it shows                  |
| ------------------------ | ------------------------------ |
| `scripts/hello-world.sh` | The smallest thing jbx can run |
