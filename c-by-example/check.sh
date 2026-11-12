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
# Exercise answers live in exercises/exN-name/. Their commands and output
# are in the README, so the README check runs them ("The README is itself
# a test", below); a broken-*.c there is checked like one in the lesson.
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

# The scratch copy the README blocks run in: every file git tracks, plus
# new ones not yet committed, but nothing the reader's own builds left
# behind. A `cc` on PATH stands in for the compiler under test, with the
# flags above that a reader's command line lacks, so a README command
# is held to the same standard as the examples.
here=$(pwd)
# `make -s test` exports MAKEFLAGS=s, which would silence a README's own
# `make` and hide what it prints: run the blocks as a reader would.
unset MAKEFLAGS MFLAGS MAKELEVEL
# Nor a CC: a reader has none set, and an exported CC=clang would override
# make's built-in `cc`. The compiler under test reaches them through the
# stand-in below; the examples themselves are built with $real_cc.
cc_test=$CC
unset CC
mkdir -p "$work/root/c-by-example" "$work/bin"
git ls-files -co --exclude-standard . | while read -r f; do
	mkdir -p "$work/root/c-by-example/$(dirname "$f")"
	cp -p "$f" "$work/root/c-by-example/$f"
done
# Resolved to a path first: with CC=cc, a bare `cc` here would find this
# stand-in again and exec itself forever.
real_cc=$(command -v "$cc_test") || { echo "no compiler: $cc_test" >&2; exit 2; }
# clang (Apple's cc included) calls a flag it did not need, such as
# -pedantic when only assembling a .s, an error under -Werror.
quiet=""
"$real_cc" --version 2>/dev/null | grep -q clang && quiet=-Qunused-arguments
cat >"$work/bin/cc" <<EOF
#!/bin/sh
exec $real_cc $quiet -pedantic -Werror -fno-sanitize-recover=all "\$@"
EOF
chmod +x "$work/bin/cc"
PATH="$work/bin:$PATH"
export PATH

for lesson in "$@"; do
	lesson=${lesson%/}
	echo "$lesson"

	# The README's commands must run as written by a reader at the repo
	# root: the first one that touches the lesson's files (cc on a .c,
	# make, ./prog) comes after a `cd c-by-example/<lesson>` line.
	if awk -v want="cd c-by-example/$lesson" '
		/^```bash/ { inblk = 1; next }
		/^```/     { inblk = 0; next }
		inblk && $0 == want { cd = 1 }
		inblk && /^(cc .*\.c|make|\.\/)/ { exit cd ? 0 : 1 }
	' "$lesson/README.md"; then
		ok "README starts from the repo root"
	else
		bad "README" "a command runs before \`cd c-by-example/$lesson\`"
	fi

	# The README is itself a test. Every ```bash block from the first
	# `cd c-by-example/...` on runs, in order, in one shell, inside a
	# scratch copy of the repo; a block whose first line is such a `cd`
	# starts again from the repo root. A bash block immediately followed
	# (blank lines only) by another fenced block is a command and its
	# output, and the output must match byte for byte. A bash block with
	# no output block must exit 0. `<!-- no-run -->` skips the next bash
	# block: kept for commands that fail on purpose, whose message differs
	# by compiler (a broken-*.c checks those).
	rm -rf "$work/rg" && mkdir "$work/rg"
	awk -v dir="$work/rg" '
		function flush_pending() { pending = 0 }
		/^[ ]*<!-- no-run -->[ ]*$/ { norun = 1; next }
		!inblk && /^[ ]*```/ {
			ind = match($0, /[^ ]/) - 1
			tag = substr($0, ind + 4)
			inblk = 1; n_lines = 0
			if (tag == "bash") { kind = "bash" }
			else if (pending && !gap) { kind = "out" }
			else { kind = "other" }
			if (kind != "out") pending = 0
			next
		}
		inblk && /^[ ]*```[ ]*$/ {
			inblk = 0
			if (kind == "bash") {
				skip = norun; norun = 0
				if (!started && first !~ /^cd c-by-example\//) skip = 1
				if (!skip) {
					started = 1; k++
					printf "%s", body > (dir "/" k ".sh"); close(dir "/" k ".sh")
					print k, (first ~ /^cd c-by-example\// ? "root" : "here"), NR >> (dir "/list")
					pending = 1; gap = 0
				} else { pending = 0 }
			} else if (kind == "out") {
				printf "%s", body > (dir "/" k ".exp"); close(dir "/" k ".exp")
				pending = 0
			}
			body = ""; first = ""
			next
		}
		inblk {
			line = $0
			sub("^ {0," ind "}", "", line)
			if (n_lines++ == 0) first = line
			body = body line "\n"
			next
		}
		/[^ \t]/ { gap = 1 }
	' "$lesson/README.md"
	if [ ! -s "$work/rg/list" ]; then
		: >"$work/out"
		bad "README" "no runnable block (absent output is not a pass)"
	else
		checked=0
		# shellcheck disable=SC2034  # line is read for the messages
		while read -r k from line; do
			[ "$from" = root ] && { cd "$work/root" || exit 2; }
			# shellcheck disable=SC1090  # the block is generated
			. "$work/rg/$k.sh" >"$work/rg/$k.got" 2>&1
			status=$?
			if [ -e "$work/rg/$k.exp" ]; then
				checked=$((checked + 1))
				if tr '`' "'" <"$work/rg/$k.got" | cmp -s - "$work/rg/$k.exp"; then
					ok "README block at line $line"
				else
					tr '`' "'" <"$work/rg/$k.got" |
						diff "$work/rg/$k.exp" - >"$work/out"
					bad "README" "block at line $line: output differs"
				fi
			elif [ "$status" -ne 0 ]; then
				cp "$work/rg/$k.got" "$work/out"
				bad "README" "block at line $line exited $status"
			fi
		done <"$work/rg/list"
		cd "$here" || exit 2
		[ "$checked" -gt 0 ] || {
			: >"$work/out"
			bad "README" "no block has its output shown, so none is checked"
		}
	fi

	support=""
	for c in "$lesson"/*.c; do
		[ -e "$c" ] || continue
		name=${c%.c}
		case ${c##*/} in broken-*) continue ;; esac
		[ -e "$name.expected" ] || support="$support $c"
	done

	# Exercise answers that fail on purpose live in exercises/*/ too.
	for c in "$lesson"/*.c "$lesson"/exercises/*/broken-*.c; do
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
			if "$real_cc" $CFLAGS -o "$work/$base" "$c" $LDLIBS >"$work/out" 2>&1; then
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
			if ! "$real_cc" $CFLAGS -o "$work/$base" "$c" $support $LDLIBS \
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
		if make -s -C "$lesson" CC="$real_cc" >"$work/out" 2>&1 &&
			make -s -C "$lesson" clean >>"$work/out" 2>&1; then
			ok "make -C $lesson"
		else
			bad "make -C $lesson" "its own Makefile failed"
		fi
	fi
done

echo
echo "$pass passed, $fail failed ($cc_test)"
[ "$fail" -eq 0 ]
