"""libfuncs.py -- every library function a lesson calls, linked in its README.

Each lesson's README ends with a "Library functions" table: one row for
every function its code (programs, broken examples and exercises) calls
from the C standard library, linked to its definition. The rows come from
ONE table, the "Library functions" section of REFERENCE.md, which says
each function's link, header and use. So a link is written once, and a
lesson's table is derived, never typed.

    python libfuncs.py           check: exit 1 if any README's table is
                                 missing or stale, or a lesson calls a
                                 function REFERENCE.md doesn't list
    python libfuncs.py --write   rewrite each README's table

So does the README of sig/, the library Part V builds, for the library's
own code; its table links no further into the course, because Doxygen
reads that README as the front page of the library's documentation, and
fails on a link it can't follow.

`make lint` runs the check, `make format` the write (then mdformat, which
re-wraps the prose and pads the table; the check compares words and cells,
so neither matters).

A call is NAME followed by `(` in a .c or .h file, after comments, strings
and character constants are removed. It is the library's unless the
lesson defines or declares NAME itself: a GNU-style definition (the name
at the start of a line), a declaration in one of its headers, a #define,
or a function pointer, `(*name)` or a `*_fn` variable. Member calls,
`p->ops->process (`, are never the library's.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent
SIG = ROOT / "sig"
BEGIN = "<!-- libfuncs: made by `make format` from REFERENCE.md -->"
END = "<!-- /libfuncs -->"
NOT_CALLS = {"if", "for", "while", "switch", "return", "sizeof", "defined"}
TYPES = re.compile(
    r"^(unsigned|signed|int|float|double|char|long|short|void|size_t|const)$"
)


def strip(src):
    """Remove comments, strings and character constants from C source."""
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", " ", src)
    # A GNU attribute looks like a call: __attribute__ ((visibility (...))).
    src = re.sub(r"__attribute__\s*\(\(.*?\)\)", " ", src)
    src = re.sub(r'"(\\.|[^"\\\n])*"', '""', src)
    return re.sub(r"'(\\.|[^'\\\n])*'", "''", src)


def sources(folder):
    """Every .c and .h file under folder, stripped (see strip)."""
    return {
        p: strip(p.read_text())
        for p in sorted(folder.rglob("*"))
        if p.suffix in (".c", ".h")
    }


def defines(texts, headers=True):
    """The names these files define or declare themselves: a GNU-style
    definition (the name at the start of a line), a #define with
    arguments, a function pointer, or a declaration in a header (only
    if headers: a header with code in it, such as sig's tests/check.h,
    would have its calls taken for declarations)."""
    own = set()
    for path, text in texts.items():
        own |= set(re.findall(r"^(\w+) \(", text, re.M))
        own |= set(re.findall(r"#\s*define\s+(\w+)\(", text))
        own |= set(re.findall(r"\(\s*\*\s*(\w+)\s*\)", text))
        own |= set(re.findall(r"\b\w+_fn\s+(\w+)", text))
        if headers and path.suffix == ".h":
            own |= set(re.findall(r"\b(\w+)\s*\([^;{]*\)\s*;", text))
    return own


def calls(lesson):
    """The names a lesson calls but does not define: the library's.
    Whatever sig/ defines, which Part V's lessons call (its functions,
    and its tests' CHECK), is the course's own, documented in sig."""
    texts = sources(lesson)
    own = {"main"} | defines(texts)
    if lesson != SIG:
        own |= defines(sources(SIG), headers=False)
        own |= defines(sources(SIG / "include"))
    found = set()
    for text in texts.values():
        for m in re.finditer(r"(?<![\w.>])(\w+)\s*\(", text):
            name = m.group(1)
            if name in NOT_CALLS or name in own or TYPES.match(name):
                continue
            found.add(name)
    return found


def cells(row):
    """A markdown table row's cells, stripped of padding."""
    return [c.strip() for c in row.strip().strip("|").split("|")]


def reference():
    """REFERENCE.md's Library functions table: {name: (cells of its row)}."""
    text = (ROOT / "REFERENCE.md").read_text()
    section = text.split("\n## Library functions\n", 1)[1].split("\n## ", 1)[0]
    rows = {}
    for line in section.splitlines():
        m = re.match(r"\| \[`(\w+)`\]\(https://\S+\)", line)
        if m:
            rows[m.group(1)] = cells(line)
    return rows


def table(names, ref, library=False):
    """The README section for these names, before mdformat pads it."""
    intro = [
        "Every library function this lesson's code calls, linked to its",
        "definition. The [quick reference](../REFERENCE.md) lists them all.",
    ]
    if library:
        intro = [
            "Every C library function sig's code calls, linked to its",
            "definition.",
        ]
    lines = [
        "## Library functions",
        "",
        BEGIN,
        "",
        *intro,
        "",
        "| function | header | what it does |",
        "| --- | --- | --- |",
    ]
    lines += ["| " + " | ".join(ref[n]) + " |" for n in sorted(names)]
    return "\n".join(lines + ["", END]) + "\n"


def normal(block):
    """A README section as mdformat-proof content: its prose as one string
    of words (mdformat re-wraps lines) and its table rows as lists of
    cells (mdformat pads them). Separator rows carry nothing."""
    prose, rows = [], []
    for line in block.splitlines():
        if not line.startswith("|"):
            prose += line.split()
        elif not re.match(r"\|[\s|:-]+\|$", line):
            rows.append(cells(line))
    return " ".join(prose), rows


def main(write):
    ref = reference()
    errors = []
    for lesson in [*sorted(ROOT.glob("[0-9][0-9]-*/")), SIG]:
        readme = lesson / "README.md"
        names = calls(lesson)
        unknown = sorted(names - ref.keys())
        for name in unknown:
            errors.append(
                f"{lesson.name}: calls `{name}`, which REFERENCE.md's"
                " Library functions table does not list"
            )
        if unknown:
            continue
        if not readme.exists():
            errors.append(f"{lesson.name}: has no README.md")
            continue
        text = readme.read_text()
        want = table(names, ref, lesson == SIG) if names else ""
        start = text.find("## Library functions\n")
        have = text[start:] if start >= 0 else ""
        stale = normal(have) != normal(want)
        if not stale:
            continue  # already right: leave mdformat's padding alone
        if write:
            body = text[:start].rstrip("\n") if start >= 0 else text.rstrip("\n")
            readme.write_text(body + ("\n\n" + want if want else "\n"))
        else:
            errors.append(
                f"{lesson.name}/README.md: its Library functions table is"
                " missing or stale (make format rewrites it)"
            )
    for e in errors:
        print(e, file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main("--write" in sys.argv[1:]))
