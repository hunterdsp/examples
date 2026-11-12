#!/usr/bin/env bash
# Hello World, the jbx way: nothing to clone, nothing to install.
#
#   . <(curl -sSL https://just-buildit.github.io/get-jb.sh)   # jbx
#   jbx gh:hunterdsp/examples/hello-world.sh
#   # Hello World!
#
# jbx fetches this file, runs it and discards it. Running it locally works
# the same way: bash hello-world.sh
echo "Hello World!"
