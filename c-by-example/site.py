"""site.py -- the course's website, made from its READMEs.

The READMEs are written for GitHub, where they are read and where
check.sh tests them, and they stay that way. The website is derived from
them, never edited: this script writes build/site/ and zensical builds
it, as `make docs` does.

    python site.py            write build/site/: docs/ (the pages) and
                              mkdocs.yml (mkdocs.base.yml plus the
                              sidebar); exit 1 on a link that leads
                              nowhere

What changes on the way, and why:

- Links to code and folders go to GitHub. A page can link to
  `pointers.c` or `exercises/`; the site holds no copy of either, and a
  folder has no page. So a relative link to anything that is not a
  README or REFERENCE.md becomes a link to the file or folder on GitHub,
  at the commit being built (SITE_REF, or HEAD), where it is shown with
  its syntax highlighted. Every relative link must lead somewhere: one
  that names a missing file fails the build.
- Lists are re-indented: Python-Markdown wants what belongs to a list
  item 4 columns in, where mdformat writes 3 (`1.`) or 2 (`-`).
- `<details>` answers become collapsed `??? example` blocks, which render
  their Markdown inside a list item, as raw HTML there does not.
- GitHub's `> [!TIP]` boxes become admonitions, which are the site's.
- The back/next lines (nav.py) are dropped: the site's own footer and
  sidebar do that job.
- The sidebar is README.md's Contents, grouped by Part (nav.parts()),
  the same list the back/next lines come from.
"""

import json
import os
import pathlib
import re
import shutil
import subprocess
import sys

import nav

ROOT = pathlib.Path(__file__).resolve().parent
REPO = ROOT.parent
OUT = ROOT / "build" / "site"
GITHUB = "https://github.com/hunterdsp/examples"
PAGES = ["README.md", "REFERENCE.md", "sig/README.md"]
NAV_BLOCK = re.compile(
    re.escape(nav.BEGIN) + r".*?" + re.escape(nav.END) + r"\n*", re.S
)
# The end of a link, `](target)`, which also finds the outer link of a
# badge, [![alt](image)](target), whose text holds a `]`.
LINK = re.compile(r"\]\(([^)\s]+)\)")
FENCE = re.compile(r"^\s*(`{3,}|~{3,})")
ALERT = re.compile(r"^(\s*)> \[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\]\s*$")
ITEM_LINE = re.compile(r"^(\s*)(\d+\.|[-*+]) ")
DETAILS = re.compile(r"^(\s*)<details><summary>(.*)</summary>\s*$")
KIND = {
    "NOTE": "note",
    "TIP": "tip",
    "IMPORTANT": "info",
    "WARNING": "warning",
    "CAUTION": "danger",
}


def ref():
    """The commit the site's GitHub links point at."""
    if os.environ.get("SITE_REF"):
        return os.environ["SITE_REF"]
    return subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()


def pages():
    """Every README the site is made of, relative to ROOT."""
    lessons = [f"{folder}/README.md" for folder, _ in nav.lessons()]
    return PAGES + lessons


def relink(target, page, commit, errors):
    """target, a link in page, as the site needs it."""
    if re.match(r"^[a-z][a-z0-9+.-]*:", target) or target.startswith("#"):
        return target  # a URL, or an anchor on the same page
    path, _, anchor = target.partition("#")
    resolved = (ROOT / page).parent.joinpath(path).resolve()
    if not resolved.exists():
        errors.append(f"{page}: link to {target}, which does not exist")
        return target
    if resolved.is_dir() and (resolved / "README.md").exists():
        resolved = resolved / "README.md"
    rel = resolved.relative_to(REPO).as_posix()
    if resolved.is_file() and rel.startswith("c-by-example/"):
        inside = rel[len("c-by-example/") :]
        if inside in pages():
            return target  # another page: zensical links it
    kind = "tree" if resolved.is_dir() else "blob"
    url = f"{GITHUB}/{kind}/{commit}/{rel}"
    return url + (f"#{anchor}" if anchor else "")


def closes(line, fence):
    """Whether line closes a fence opened with `fence` (``` or ~~~)."""
    m = FENCE.match(line)
    g = m.group(1) if m else ""
    return g[:1] == fence[:1] and len(g) >= len(fence) and not line.strip()[len(g) :]


def reindent(lines):
    """Lists as Python-Markdown reads them: what belongs to a list item
    indented 4 columns past the item, where mdformat (and GitHub) use the
    marker's width, 3 for `1.` and 2 for `-`. Each item's content column
    is moved to its new place, fenced code inside it too."""
    out, stack, fence, at_fence = [], [], None, []

    def remap(line, contexts):
        ind = len(line) - len(line.lstrip(" "))
        for src, dst in reversed(contexts):
            if ind >= src:
                return " " * dst + line[src:]
        return line

    for line in lines:
        if fence:
            out.append(remap(line, at_fence) if line.strip() else line)
            if closes(line, fence):
                fence = None
            continue
        if not line.strip():
            out.append(line)
            continue
        ind = len(line) - len(line.lstrip(" "))
        item = ITEM_LINE.match(line)
        while stack and ind < stack[-1][0]:
            stack.pop()
        if item:
            # A sibling item replaces the previous one at its level.
            while stack and stack[-1][2] >= ind:
                stack.pop()
            new = remap(line, [(s, d) for s, d, _ in stack])
            new_ind = len(new) - len(new.lstrip(" "))
            width = len(item.group(2)) + 1
            stack.append((ind + width, new_ind + 4, ind))
            out.append(new)
            continue
        new = remap(line, [(s, d) for s, d, _ in stack])
        m = FENCE.match(line)
        if m:
            fence = m.group(1)
            at_fence = [(s, d) for s, d, _ in stack]
        out.append(new)
    return out


def fold(lines):
    """Each <details><summary>T</summary> ... </details> as a collapsed
    block, `??? example "T"`, which works inside a list item; raw HTML
    there does not render the Markdown inside it."""
    out, depth = [], None
    for line in lines:
        if depth is None:
            m = DETAILS.match(line)
            if m:
                depth = m.group(1)
                out.append(f'{depth}??? example {json.dumps(m.group(2))}')
                continue
            out.append(line)
            continue
        if line.strip() == "</details>" and line.startswith(depth + "<"):
            depth = None
            continue
        if line.strip():
            out.append(depth + "    " + line[len(depth) :])
        else:
            out.append(line)
    return out


def convert(text, page, commit, errors):
    """One README's text, as the site's page."""
    text = NAV_BLOCK.sub("", text)
    out, fence, alert = [], None, None
    for line in text.split("\n"):
        if fence:
            if closes(line, fence):
                fence = None
            out.append(line)
            continue
        m = FENCE.match(line)
        if m:
            fence = m.group(1)
            out.append(line)
            continue
        if alert is not None:
            if line.startswith(alert + ">"):
                body = line[len(alert) + 1 :]
                out.append(alert + "    " + body.removeprefix(" "))
                continue
            alert = None
        a = ALERT.match(line)
        if a:
            alert = a.group(1)
            out.append(f"{alert}!!! {KIND[a.group(2)]}")
            continue
        line = LINK.sub(
            lambda l: "](" + relink(l.group(1), page, commit, errors) + ")",
            line,
        )
        out.append(line)
    return "\n".join(fold(reindent(out)))


def sidebar():
    """The nav: section of mkdocs.yml, from README.md's Contents."""
    q = json.dumps  # a JSON string is a YAML string, quotes and all
    lines = ["nav:", f"  - {q('Learning C by Example')}: README.md"]
    for part, items in nav.parts():
        lines.append(f"  - {q(part)}:")
        for folder, title in items:
            lines.append(f"      - {q(title)}: {folder}/README.md")
        if part.startswith("Part V "):
            lines.append(f"      - {q('The sig library')}: sig/README.md")
    lines.append(f"  - {q('Quick reference')}: REFERENCE.md")
    return "\n".join(lines) + "\n"


def main():
    commit = ref()
    errors = []
    if OUT.exists():
        shutil.rmtree(OUT)
    for page in pages():
        text = (ROOT / page).read_text()
        dest = OUT / "docs" / page
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(convert(text, page, commit, errors))
    base = (ROOT / "mkdocs.base.yml").read_text()
    (OUT / "mkdocs.yml").write_text(base + "\n" + sidebar())
    for e in errors:
        print(e, file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
