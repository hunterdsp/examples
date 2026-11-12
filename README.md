# examples

Example scripts that don't belong to any one repo. Each one runs straight
from GitHub with [`jbx`](https://just-buildit.github.io/just-bashit/just-runit/),
so there's nothing to clone:

```bash
. <(curl -sSL https://just-buildit.github.io/get-jb.sh)   # jbx, in this shell
jbx gh:hunterdsp/examples/hello-world.sh
# Hello World!
```

| Script           | What it shows                    |
| ---------------- | -------------------------------- |
| `hello-world.sh` | The smallest thing jbx can run   |
