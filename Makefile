CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -O2
LDLIBS ?= -lm

.PHONY: all clean test

all: tinker-asm tinker-sim

tinker-asm: tinker-asm.c
	$(CC) $(CFLAGS) -o $@ $<

tinker-sim: tinker-sim.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

test: all
	sh tests/run-tests.sh

clean:
	$(RM) tinker-asm tinker-sim tinker-asm.exe tinker-sim.exe *.o *.tko temp.tk
