#!/usr/bin/env bash
# Several functions, one script: jbx runs only the one you name.
#
#   jbx gh:hunterdsp/examples/scripts/greet.sh hello Ada
#   # Hello, Ada!
#   jbx gh:hunterdsp/examples/scripts/greet.sh goodbye Ada
#   # Goodbye, Ada!
#   jbx -l gh:hunterdsp/examples/scripts/greet.sh
#   # lists hello, goodbye and shout, running nothing
#
# jbx sources the file (which only defines functions, so nothing happens),
# then calls the FUNCTION you named with the rest of the arguments.

# hello [NAME] -- greet NAME, or the world.
hello() { echo "Hello, ${1:-World}!"; }

# goodbye [NAME] -- see NAME off.
goodbye() { echo "Goodbye, ${1:-World}!"; }

# shout WORDS... -- say it louder.
shout() { printf '%s!\n' "$*" | tr '[:lower:]' '[:upper:]'; }
