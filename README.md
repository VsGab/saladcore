# saladcore - A hybrid stack machine

### Main characteristics
- fixed size 8-bit instructions
- variable precision literals via concatenation
- 16-level deep stack overflowing to memory
- direct stack access with PICK/MOV instructions (frame indexed)
- spare encoding space for custom / emulated instructions

***

# Implementation
Saladcore is a work-in-progress ISA. Currently a [C simulator](./csim) is available which can also be compiled to WASM. This was used to experiment with a custom Forth-like language, [saladforth](./fth), making use of the frame instructions.

A Verilog implementation of a minimal instruction set is planned, and the current instructions are expected to change, to accomodate both the minimal hardware and the more fully-featured simulator, while not requiring major saladforth changes.

A demo including basic emulated hardware (a serial link and a 1-bit 128x128 display) is available at [https://vsgab.github.io/demo/salad/](https://vsgab.github.io/demo/salad/)

# Usage
The WASM demo uses clang directly for compilation, so the Makefile defaults to it. \
Useful targets:
- `make dev` runs ROM tester binary with *fth/dev.sf* as input source ; this loads the *rom.bin* file built previously, runs the reset code in the reset word `^^` and then interprets the source received over serial0 line by line
- `make web` creates a directory *web* in the output directory with the demo ; this needs to be served over HTTP
- `make serve` starts a python http.server serving the *web* directory locally

## Future
- Minimal Verilog implementation &rarr; FPGA
- Expand demo to fully featured fantasy console
- Compile C to saladforth

# References
[1] [Microcore](https://microcore.org/Introduction/index.html) \
[2] [ZPU](https://en.wikipedia.org/wiki/ZPU_(processor)) \
[3] [UXN](https://wiki.xxiivv.com/site/uxn.html) \
[4] ["Tom Thumb" font](https://robey.lag.net/2010/01/23/tiny-monospace-font.html)