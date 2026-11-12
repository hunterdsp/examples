# Makefile for the level meter: make's job is to rebuild only what changed.
#
#   make          build ./meter
#   make clean    remove what make built
#
# A rule is:  TARGET: PREREQUISITES  then a TAB-indented recipe. make runs
# the recipe when the target is missing or older than a prerequisite.

CC     ?= cc
CFLAGS  = -std=c99 -Wall -Wextra -g -fsanitize=address,undefined
LDFLAGS = -fsanitize=address,undefined
LDLIBS  = -lm

# Link: join the object files into the program.
meter: meter.o dsp.o
	$(CC) $(LDFLAGS) -o $@ meter.o dsp.o $(LDLIBS)

# Compile each .c into a .o. Both depend on dsp.h, so editing the header
# rebuilds both, and editing dsp.c rebuilds only dsp.o.
meter.o: meter.c dsp.h
	$(CC) $(CFLAGS) -c meter.c

dsp.o: dsp.c dsp.h
	$(CC) $(CFLAGS) -c dsp.c

.PHONY: clean
clean:
	rm -f meter meter.o dsp.o
