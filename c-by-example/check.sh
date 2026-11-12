#!/bin/sh
# check.sh -- build and run every example in the lessons named, and fail on
# the first surprise. `make test` runs it over all of them.
#
# A lesson is a directory of .c files. Each one is one of three kinds:
#
#   NAME.c + NAME.expected   a program. Built with every support file of the
#                            lesson, run, and its stdout must equal
#                            NAME.expected byte for byte.
#   broken-NAME.c + .expect  a pitfall. Built ALONE, and it must fail: either
#                            the compiler refuses it (-Werror) or it runs and
#                            a sanitizer stops it. Either way the output must
#                            match the extended regex in broken-NAME.expect,
#                            so it fails for the reason the lesson says.
#   any other .c             a support file (no main), linked into every
#                            program in the lesson.
#
# A lesson with its own Makefile (08 teaches make) is also built with it.
#
# Every example is built the way lesson 01 tells the reader to build:
# C99, every warning an error, and AddressSanitizer + UBSan, which stop the
# program at the first bad memory access or undefined behaviour.
#
# Usage: ./check.sh 01-hello 02-numbers ...      (CC=clang ./check.sh ...)

CC=${CC:-cc}
CFLAGS="-std=c99 -pedantic -Wall -Wextra -Werror -g -O1"
CFLAGS="$CFLAGS -fsanitize=address,undefined -fno-sanitize-recover=all"
LDLIBS="-lm"

pass=0
fail=0
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

ok() { pass=$((pass + 1)); printf '  ok    %s\n' "$1"; }
bad() {
	fail=$((fail + 1))
	printf '  FAIL  %s: %s\n' "$1" "$2"
	[ -s "$work/out" ] && sed 's/^/        | /' "$work/out" | head -20
}

for lesson in "$@"; do
	lesson=${lesson%/}
	echo "$lesson"
	support=""
	for c in "$lesson"/*.c; do
		[ -e "$c" ] || continue
		name=${c%.c}
		case ${c##*/} in broken-*) continue ;; esac
		[ -e "$name.expected" ] || support="$support $c"
	done

	for c in "$lesson"/*.c; do
		[ -e "$c" ] || continue
		name=${c%.c}
		base=${name##*/}
		case $base in
		broken-*)
			if [ ! -e "$name.expect" ]; then
				bad "$base" "no $base.expect saying why it must fail"
				continue
			fi
			# shellcheck disable=SC2086  # flag lists are word-split
			if $CC $CFLAGS -o "$work/$base" "$c" $LDLIBS >"$work/out" 2>&1; then
				(cd "$work" && "./$base" >>"$work/out" 2>&1) &&
					{ bad "$base" "built and ran cleanly; it should fail"; continue; }
			fi
			if grep -Eq "$(cat "$name.expect")" "$work/out"; then
				ok "$base (fails as it should)"
			else
				bad "$base" "failed, but not with /$(cat "$name.expect")/"
			fi
			;;
		*)
			[ -e "$name.expected" ] || continue
			# shellcheck disable=SC2086
			if ! $CC $CFLAGS -o "$work/$base" "$c" $support $LDLIBS \
				>"$work/out" 2>&1; then
				bad "$base" "does not build"
				continue
			fi
			if ! (cd "$work" && "./$base" >"$work/out" 2>&1); then
				bad "$base" "exited non-zero"
				continue
			fi
			if cmp -s "$work/out" "$name.expected"; then
				ok "$base"
			else
				diff "$name.expected" "$work/out" >"$work/diff" 2>&1
				mv "$work/diff" "$work/out"
				bad "$base" "output differs from $base.expected"
			fi
			;;
		esac
	done

	if [ -e "$lesson/Makefile" ]; then
		if make -s -C "$lesson" CC="$CC" >"$work/out" 2>&1 &&
			make -s -C "$lesson" clean >>"$work/out" 2>&1; then
			ok "make -C $lesson"
		else
			bad "make -C $lesson" "its own Makefile failed"
		fi
	fi
done

echo
echo "$pass passed, $fail failed ($CC)"
[ "$fail" -eq 0 ]
