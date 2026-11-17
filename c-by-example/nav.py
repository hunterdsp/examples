"""nav.py -- back, contents and next links at the top and bottom of lessons.

Each lesson's README opens and ends with one line:

    [← 04 Arrays and loops](...) · [Contents](...) · [06 Strings ... →](...)

The order and the names come from ONE list, the Contents section of
README.md, so a lesson added, renamed or moved there moves its neighbours'
links with it, and no link is typed by hand.

    python nav.py           check: exit 1 if any lesson's links are
                            missing or stale, or a lesson folder is not
                            in the Contents (or the Contents names one
                            that does not exist)
    python nav.py --write   rewrite every lesson's links

`make lint` runs the check, `make format` the write. Links name the
README.md itself, not its folder, so they work on GitHub and on a site
built from the same files.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent
BEGIN = "<!-- nav: made by `make format` from README.md's Contents -->"
END = "<!-- /nav -->"
ITEM = re.compile(r"^- \[(\d\d) ([^\]]+)\]\((\d\d-[a-z0-9-]+)/\)")


def lessons():
    """[(folder, "NN Title")] in the Contents' order."""
    text = (ROOT / "README.md").read_text()
    contents = text.split("\n## Contents\n", 1)[1].split("\n## ", 1)[0]
    found = []
    for line in contents.splitlines():
        m = ITEM.match(line)
        if m:
            found.append((m.group(3), f"{m.group(1)} {m.group(2)}"))
    return found


def block(order, i):
    """The nav block for lesson i of order."""
    parts = []
    if i > 0:
        folder, title = order[i - 1]
        parts.append(f"[← {title}](../{folder}/README.md)")
    parts.append("[Contents](../README.md#contents)")
    if i + 1 < len(order):
        folder, title = order[i + 1]
        parts.append(f"[{title} →](../{folder}/README.md)")
    return f"{BEGIN}\n\n{' · '.join(parts)}\n\n{END}"


SPAN = re.compile(re.escape(BEGIN) + r".*?" + re.escape(END), re.S)


def squash(s):
    """s with runs of whitespace as one space: mdformat may re-wrap."""
    return " ".join(s.split())


def placed(text, nav):
    """text with nav as its first and last block, whatever was there."""
    body = SPAN.sub("", text).strip("\n")
    return f"{nav}\n\n{body}\n\n{nav}\n"


def main(write):
    order = lessons()
    problems = []
    listed = {folder for folder, _ in order}
    on_disk = {p.parent.name for p in ROOT.glob("[0-9][0-9]-*/README.md")}
    for folder in sorted(on_disk - listed):
        problems.append(f"{folder}: not in README.md's Contents")
    for folder in sorted(listed - on_disk):
        problems.append(f"README.md's Contents names {folder}: no README")
    for i, (folder, _) in enumerate(order):
        path = ROOT / folder / "README.md"
        if not path.exists():
            continue
        text = path.read_text()
        want = placed(text, block(order, i))
        if write:
            if squash(text) != squash(want):
                path.write_text(want)
        elif squash(text) != squash(want):
            problems.append(f"{folder}/README.md: back/next links stale")
    for p in problems:
        print(p, file=sys.stderr)
    if any("stale" in p for p in problems) and not write:
        print("run `make format` to rewrite them", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main("--write" in sys.argv[1:]))
