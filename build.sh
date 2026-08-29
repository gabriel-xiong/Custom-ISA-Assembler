#!/usr/bin/env sh
set -eu

cc="${CC:-cc}"

"$cc" -std=c99 -Wall -Wextra -Wpedantic -O2 -o tinker-asm tinker-asm.c
"$cc" -std=c99 -Wall -Wextra -Wpedantic -O2 -o tinker-sim tinker-sim.c -lm
