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
# Every example is built the way lesson 01 tells the reader to build, and
# stricter: C99, -pedantic, every warning an error, and AddressSanitizer +
# UBSan, with -fno-sanitize-recover=all so that UBSan stops the program at
# its first report instead of carrying on, as it does by default.
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

ok() {
	pass=$((pass + 1))
	printf '  ok    %s\n' "$1"
}
bad() {
	fail=$((fail + 1))
	printf '  FAIL  %s: %s\n' "$1" "$2"
	[ -s "$work/out" ] && sed 's/^/        | /' "$work/out" | head -20
}

# The scratch copy the README blocks run in: every file git tracks, plus
# new ones not yet committed, but nothing the reader's own builds left
# behind. A `cc` on PATH stands in for the compiler under test, and adds
# the flags above that a reader's command lacks: -pedantic -Werror, and
# -fno-sanitize-recover=all. That is how exercise answers, which only
# README blocks build, get the strict build too. None of them changes what
# a correct program prints: the first two act only at compile time, and
# the last only once UBSan has found a bug, which fails the block anyway.
# So a block that passes here prints, under the reader's own command,
# exactly the output shown.
here=$(pwd)
# A git hook in a linked worktree runs with GIT_DIR set, and ls-files then
# lists paths from the repo root, not from here (as in the Makefile).
unset GIT_DIR
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
real_cc=$(command -v "$cc_test") || {
	echo "no compiler: $cc_test" >&2
	exit 2
}
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

	# The README is itself a test. Its ```bash blocks run, in order, in
	# one shell, inside a scratch copy of the repo. A reader works in the
	# lesson's folder, so each block runs there, except in an answer that
	# opens `In \`exercises/exN-name/\`:`, alone on its line, whose blocks
	# run in that folder up to its `</details>` (one that does not exist
	# fails), and one whose first line is `cd c-by-example/<lesson>`, which
	# runs from the repo root (lesson 00 teaches that `cd`). No other `cd`
	# is allowed: the README says where a command runs, and the reader
	# goes there. A bash block immediately followed (blank lines only) by
	# another fenced block is a command and its output, and the output
	# must match byte for byte. A bash block with no output block must
	# exit 0.
	# `<!-- no-run -->` skips the next bash block: kept for setup run once
	# (00), and commands that fail on purpose, whose message differs by
	# compiler (a broken-*.c checks those).
	rm -rf "$work/rg" && mkdir "$work/rg"
	awk -v dir="$work/rg" -v home="cd c-by-example/$lesson" '
		/^[ ]*<!-- no-run -->[ ]*$/ { norun = 1; next }
		# A fence is three or more backticks (mdformat uses more when
		# the block itself holds three), and only a fence at least as
		# long closes it, as in Markdown.
		!inblk && /^[ ]*```/ {
			ind = match($0, /[^ ]/) - 1
			match($0, /`+/); fence = substr($0, RSTART, RLENGTH)
			tag = substr($0, RSTART + RLENGTH)
			close_re = "^[ ]*" fence "`*[ ]*$"
			inblk = 1; n_lines = 0; ncd = 0; opened = NR
			if (tag == "bash") { kind = "bash"; where = indir }
			else if (pending && !gap) { kind = "out" }
			else { kind = "other" }
			if (kind != "out") pending = 0
			next
		}
		inblk && $0 ~ close_re {
			inblk = 0
			if (kind == "bash") {
				if (badcd) print NR, badcd >> (dir "/badcd")
				badcd = ""
				if (!norun) {
					k++
					printf "%s", body > (dir "/" k ".sh"); close(dir "/" k ".sh")
					from = first == home ? "/" : where == "" ? "." : where
					print k, NR, from >> (dir "/list")
					pending = 1; gap = 0
				} else { pending = 0 }
				norun = 0
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
			if (line ~ /^cd([ ;]|$)/ && kind == "bash" &&
			    !(n_lines == 1 && line == home) && line != "cd examples")
				badcd = line
			body = body line "\n"
			next
		}
		/^[ ]*In `[^`]+`:[ ]*$/ {
			indir = $0; sub(/^[ ]*In `/, "", indir); sub(/`:[ ]*$/, "", indir)
			gap = 1; next
		}
		# A marker with more on its line is not a marker: its blocks
		# would run in the lesson folder, and fail far from the cause.
		/^[ ]*In `exercises\// { print NR >> (dir "/badin") }
		/^[ ]*<\/details>/ { indir = "" }
		/[^ \t]/ { gap = 1 }
		# A fence that never closes swallows the rest of the README.
		END { if (inblk) print opened >> (dir "/badfence") }
	' "$lesson/README.md"
	lessondir=$work/root/c-by-example/$lesson
	if [ -s "$work/rg/badcd" ]; then
		while read -r line cmd; do
			: >"$work/out"
			bad "README" "block at line $line has \`$cmd\`: say where it runs instead"
		done <"$work/rg/badcd"
	fi
	if [ -s "$work/rg/badin" ]; then
		while read -r line; do
			: >"$work/out"
			bad "README" "line $line: put \`In \`exercises/…\`:\` alone on its line"
		done <"$work/rg/badin"
	fi
	if [ -s "$work/rg/badfence" ]; then
		: >"$work/out"
		bad "README" "the fence at line $(cat "$work/rg/badfence") never closes"
	fi
	if [ ! -s "$work/rg/list" ]; then
		: >"$work/out"
		bad "README" "no runnable block (absent output is not a pass)"
	else
		checked=0
		while read -r k line from; do
			case $from in
			/) cd "$work/root" ;;
			*) cd "$lessondir/$from" 2>/dev/null ;;
			esac || {
				: >"$work/out"
				bad "README" "block at line $line runs in $from, which is not there"
				continue
			}
			# shellcheck disable=SC1090  # the block is generated
			. "$work/rg/$k.sh" >"$work/rg/$k.got" 2>&1
			status=$?
			if [ -e "$work/rg/$k.exp" ]; then
				checked=$((checked + 1))
				# Both sides, so a backtick in a listed file still matches
				# while make 3.81's `meter' matches make 4's 'meter'.
				tr '`' "'" <"$work/rg/$k.exp" >"$work/rg/$k.want"
				if tr '`' "'" <"$work/rg/$k.got" | cmp -s - "$work/rg/$k.want"; then
					ok "README block at line $line"
				else
					tr '`' "'" <"$work/rg/$k.got" |
						diff "$work/rg/$k.want" - >"$work/out"
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
				# `; exit $?` keeps the subshell from exec'ing the program,
				# so the subshell reports an assert's SIGABRT ("Aborted"),
				# into the log, instead of the outer shell, on the terminal.
				(
					cd "$work" && "./$base"
					exit $?
				) >>"$work/out" 2>&1 &&
					{
						bad "$base" "built and ran cleanly; it should fail"
						continue
					}
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
