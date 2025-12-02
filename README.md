# saladcore - A hybrid stack machine with a minimal Forth-like language

### Main characteristics
- fixed size 8-bit instructions
- easy to implement two register machine
- top-of-stack relative stack access instructions
- variable precision literals via concatenation
- flexible word size, minor language changes required
- separate address spaces for stacks (word) and code/data (byte)

***

Note: for the initial prototype using frame pointer stack addressing and top-of-stack ops, check out [archive/mk1 branch](https://github.com/VsGab/saladcore/tree/archive/mk1).

# Goal
Have a simple to implement instruction set, in both hardware and software (e.g. JIT), and a below-C language for it with Forth-like metaprogramming.\
On the language side this was mostly achieved, with the resulting language supporting basic control structures (conditionals, while loop, goto), named locals, expressions and quoting. On the machine side, a Verilog implementation is still pending, but a word-level JIT for x64 was prototyped (coming soon).

**For a WASM-compiled web playground of the language, check out the latest [demo](https://vsgab.github.io/demo/salad2/).**

# Source structure
- [fth](/fth/) contains the saladforth - currently C - implementation
- [core](/core/) contains a basic ISA emulator
- [include](/include/) contains common headers, including the opcode listing
- [wasm](/wasm/) contains the web demo


# Inspiration
- [ZPU](https://en.wikipedia.org/wiki/ZPU_(processor))
- [UXN](https://wiki.xxiivv.com/site/uxn.html)
