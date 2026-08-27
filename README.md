# Custom ISA Assembler & CPU Simulator

An end-to-end C toolchain for a custom 32-register instruction set architecture. The project translates assembly programs into executable binaries and runs them on a software CPU simulator.

### Features

* Two-pass assembler with label resolution and pseudo-instruction expansion
* ~30 instruction types covering arithmetic, branching, memory access, stack operations, and I/O
* 32-bit instruction encoding with 32 64-bit registers
* 512 KiB simulated memory
* Fetch/decode/execute CPU loop with function-pointer instruction dispatch
* Integer and floating-point execution

### Example

The repository includes a Fibonacci program written in the custom assembly language that exercises input, branching, arithmetic, and output.
