#!/bin/sh
# check-hashes.sh -- the README's table of script hashes must be current.
#
# Readers pin a script with `jbx -k sha256:<hash>` using the hash the
# README lists, so a stale row makes jbx refuse the very script it names.
# Fail unless every scripts/*.sh has exactly one row, and every row's hash
# is the sha256 of that script as it is in this checkout.
#
# Usage: .github/scripts/check-hashes.sh     (from the repo root)

fail=0
for f in scripts/*.sh; do
	want=$(sha256sum <"$f" | cut -c1-64)
	# A hash-table row is the script, then a 64-hex-digit hash, and nothing
	# else: the "Scripts" table below lists the same names with prose.
	row="^\\| \`$f\` +\\| \`[0-9a-f]{64}\` \\|\$"
	rows=$(grep -cE "$row" README.md)
	got=$(grep -E "$row" README.md | sed -E 's/.*\| `([0-9a-f]{64})` \|$/\1/')
	if [ "$rows" -ne 1 ]; then
		echo "FAIL $f: $rows rows in README.md's hash table, want 1"
		fail=1
	elif [ "$got" != "$want" ]; then
		echo "FAIL $f: README.md lists $got"
		echo "     but the script hashes to $want"
		fail=1
	else
		echo "ok   $f"
	fi
done
# A row for a script that no longer exists would send readers to a 404.
for f in $(sed -nE 's/^\| `(scripts\/[^`]+\.sh)` +\| `[0-9a-f]{64}` \|$/\1/p' README.md); do
	[ -e "$f" ] || { echo "FAIL $f: listed in README.md, not in scripts/"; fail=1; }
done
exit $fail
