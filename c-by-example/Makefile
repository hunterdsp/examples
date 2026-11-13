# Learning C by Example -- build and check every lesson.
#
#   make test             every lesson, with the default compiler (cc)
#   make test CC=clang    the same, with clang
#   make test LESSONS=05-pointers
#   make lint             check every README is formatted (mdformat)
#   make format           format them

LESSONS ?= $(sort $(wildcard [0-9][0-9]-*))

.PHONY: test lint format help

# Every README in the course, formatted by mdformat (settings in
# .mdformat.toml). `make lint` fails if formatting would change any of
# them; `make format` changes them.
MD_FILES := README.md $(wildcard [0-9][0-9]-*/README.md)
MDFORMAT := uv run --frozen --only-group lint mdformat

test: ## Build, run and check every example in LESSONS
	@CC="$(CC)" ./check.sh $(LESSONS)

lint: ## Fail if any README is not as mdformat would write it
	@$(MDFORMAT) --check $(MD_FILES)

format: ## Format every README with mdformat
	@$(MDFORMAT) $(MD_FILES)

help: ## Show this message
	@grep -E '^[a-z-]+:.*## ' $(MAKEFILE_LIST) | \
	  sed 's/:.*## /\t/' | expand -t 12
