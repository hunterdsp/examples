#!/bin/sh
# check-hashes.sh -- scripts/README.md's table of hashes must be current.
#
# Readers pin a script with `jbx -k sha256:<hash>` using the hash that
# README lists, so a stale row makes jbx refuse the very script it names.
# Fail unless every scripts/*.sh has exactly one row, and every row's hash
# is the sha256 of that script as it is in this checkout.
#
# Usage: .github/scripts/check-hashes.sh     (from the repo root)

fail=0
readme=scripts/README.md
for path in scripts/*.sh; do
	f=${path#scripts/}
	want=$(sha256sum <"$path" | cut -c1-64)
	# A hash-table row is the script, then a 64-hex-digit hash, and nothing
	# else: the "What each one shows" table lists the same names with prose.
	row="^\\| \`$f\` +\\| \`[0-9a-f]{64}\` \\|\$"
	rows=$(grep -cE "$row" "$readme")
	# The backticks are Markdown's, matched literally: nothing to expand.
	# shellcheck disable=SC2016
	got=$(grep -E "$row" "$readme" | sed -E 's/.*\| `([0-9a-f]{64})` \|$/\1/')
	if [ "$rows" -ne 1 ]; then
		echo "FAIL $f: $rows rows in $readme's hash table, want 1"
		fail=1
	elif [ "$got" != "$want" ]; then
		echo "FAIL $f: $readme lists $got"
		echo "     but the script hashes to $want"
		fail=1
	else
		echo "ok   $f"
	fi
done
# A row for a script that no longer exists would send readers to a 404.
# shellcheck disable=SC2016  # Markdown's backticks, matched literally
listed=$(sed -nE 's/^\| `([^`]+\.sh)` +\| `[0-9a-f]{64}` \|$/\1/p' "$readme")
while read -r f; do
	[ -n "$f" ] && [ ! -e "scripts/$f" ] && {
		echo "FAIL $f: listed in $readme, not in scripts/"
		fail=1
	}
done <<EOF
$listed
EOF
exit $fail
