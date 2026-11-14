# Learning C by Example -- build and check every lesson.
#
#   make test             every lesson, with the default compiler (cc)
#   make test CC=clang    the same, with clang
#   make test LESSONS=05-pointers
#   make lint             check every README (mdformat) and C file
#                         (clang-format) is formatted, and every README
#                         links the library functions its lesson calls
#   make format           format them, and rewrite those links

LESSONS ?= $(sort $(wildcard [0-9][0-9]-*))

.PHONY: test lint lint-mdformat lint-clang-format lint-libfuncs format help

# Every README in the course, formatted by mdformat (settings in
# .mdformat.toml). `make lint` fails if formatting would change any of
# them; `make format` changes them.
MD_FILES := README.md REFERENCE.md sig/README.md \
            $(wildcard [0-9][0-9]-*/README.md)
MDFORMAT := uv run --frozen --only-group lint mdformat

# Every C file, formatted by clang-format in ~/doppler's style: the same
# .clang-format, copied, and the same pinned version (pyproject.toml).
C_FILES := $(shell git ls-files -co --exclude-standard '*.c' '*.h')
CLANG_FORMAT := uv run --frozen --only-group lint clang-format --style=file

# Each README ends with a table of the library functions its lesson's code
# calls, made from REFERENCE.md's one table of links (libfuncs.py).
LIBFUNCS := uv run --frozen --only-group lint python libfuncs.py

test: ## Build, run and check every example in LESSONS
	@CC="$(CC)" ./check.sh $(LESSONS)

lint: lint-mdformat lint-clang-format lint-libfuncs ## Every README and C file is formatted, and links its library functions

lint-mdformat: ## Fail if any README is not as mdformat would write it
	@$(MDFORMAT) --check $(MD_FILES)

lint-clang-format: ## Fail if any C file is not as clang-format would write it
	@$(CLANG_FORMAT) --dry-run --Werror $(C_FILES)

lint-libfuncs: ## Fail if a README's library function table is missing or stale
	@$(LIBFUNCS)

format: ## Format every README (mdformat) and C file (clang-format)
	@$(LIBFUNCS) --write
	@$(MDFORMAT) $(MD_FILES)
	@$(CLANG_FORMAT) -i $(C_FILES)

help: ## Show this message
	@grep -E '^[a-z-]+:.*## ' $(MAKEFILE_LIST) | \
	  sed 's/:.*## /\t/' | expand -t 12
