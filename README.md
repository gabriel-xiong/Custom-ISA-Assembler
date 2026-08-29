# Tinker VM

Tinker VM is a compact assembler and virtual-machine simulator for a custom 32-bit instruction set. It turns human-readable Tinker assembly (`.tk`) into executable object files (`.tko`), then runs them on a simulated 64-bit register machine.

The project explores the boundary between language tooling and computer architecture: parsing source code, resolving labels, encoding instructions, laying out code and data, loading binaries, and executing them through a fetch-decode-execute loop.

## Highlights

- Two-pass assembler with symbolic labels and separate `.code` and `.data` sections
- 32 general-purpose 64-bit registers and 512 KiB of simulated memory
- Integer and floating-point arithmetic, bitwise operations, branches, calls, and memory access
- Macro-style instructions for common operations such as `clr`, `push`, `pop`, `in`, `out`, and `halt`
- Portable C99 implementation with no dependencies beyond the C standard library and `libm`
- Automated end-to-end tests running under GitHub Actions

## Build

You need a C99-compatible compiler. Build both tools with Make:

```sh
make
```

Alternatively, run the build script:

```sh
./build.sh
```

On Windows without a POSIX shell, compile directly:

```powershell
gcc -std=c99 -Wall -Wextra -Wpedantic -O2 -o tinker-asm.exe tinker-asm.c
gcc -std=c99 -Wall -Wextra -Wpedantic -O2 -o tinker-sim.exe tinker-sim.c -lm
```

## Run the example

Assemble the included Fibonacci program:

```sh
./tinker-asm examples/fibonacci.tk fibonacci.tko
```

Then launch it in the simulator:

```sh
./tinker-sim fibonacci.tko
```

Enter a sequence index when prompted. The program prints the corresponding Fibonacci value.

On Windows, use `tinker-asm.exe` and `tinker-sim.exe` in the commands above.

For example, entering `10` prints:

```text
21
```

## Test

Run the end-to-end smoke tests after building:

```sh
make test
```

The suite assembles all three example programs, checks Fibonacci, binary-search, and matrix-multiplication results, and verifies that invalid CLI usage fails cleanly. The same strict build and test flow runs automatically for pushes and pull requests through GitHub Actions.

## Assembly overview

Tinker source files use `.code` and `.data` directives. Instructions inside a section are indented, while labels begin at the start of a line:

```asm
.code
	clr r1
	addi r1, 8
	ld r2, :done
	br r2
:done
	halt
```

The assembler writes a compact binary containing a header followed by the encoded code and data segments. The simulator loads those segments into virtual memory and executes instructions beginning at the code entry address.

## Repository layout

```text
.
├── examples/
│   ├── binary-search.tk
│   ├── fibonacci.tk
│   └── matrix-multiplication.tk
├── tests/
│   └── run-tests.sh   # End-to-end smoke tests
├── tinker-asm.c       # Parser, label resolver, and instruction encoder
├── tinker-sim.c       # Binary loader and virtual-machine execution engine
├── Makefile           # Make-based build
└── build.sh           # Portable shell build
```

## Possible extensions

- Expand coverage with instruction-level unit tests
- Improve diagnostics with source line and column information
- Add an optional execution trace or interactive debugger
- Define a versioned, platform-independent object-file format
