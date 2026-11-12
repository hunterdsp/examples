# Learning C by Example -- build and check every lesson.
#
#   make test             every lesson, with the default compiler (cc)
#   make test CC=clang    the same, with clang
#   make test LESSONS=05-pointers

LESSONS ?= $(sort $(wildcard [0-9][0-9]-*))

.PHONY: test help

test: ## Build, run and check every example in LESSONS
	@CC="$(CC)" ./check.sh $(LESSONS)

help: ## Show this message
	@grep -E '^[a-z-]+:.*## ' $(MAKEFILE_LIST) | \
	  sed 's/:.*## /\t/' | expand -t 12
