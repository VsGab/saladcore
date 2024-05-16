# saladforth - a custom Forth for saladcore

Main characteristics:
- all words are immediate by default
- square bracket words enable/disable inlining mode
- in inlining mode numbers (their 8-bit LSB) are inserted at HERE instead of being left on the stack and word code is memcpy'd
- words have a no-inline flag which allows them to run immediately even in inline mode
- all-digits tokens are treated as numbers
- there are no special word characters

The interpreter was written in a saladcore-assembly-dsl in C ([base.c](./base.c)). The function code is under 400B, with the reset code and builtin word entries increasing it to little over 500B.

The source in [base.sf](./base.sf) builds from assembly words to cover:
- goto-like semantics with numeric labels
- hash table indexing of words
- strings
- drawing to framebuffer

|Variable|Description|
|---|---|
|ine      | Pointer to next-to-last char in buffer
|in*      | Pointer to next unparsed char
|code     | Pointer to the code start of current word
|last     | Pointer to byte after last word footer
|here     | Pointer to next free byte in dict
|INLINING | (C only) Flag enabling inlining of literals and code

|Builtin word|Description|
|---|---|
|**c:** | Writes word header at HERE
|**;**  | Writes word footer at HERE
| ins   | Writes 8-bit LSB of ToS at HERE
|**[**  | Enables inlining
|**]**     | Disables inlining
| word  | Scans next token
| find  | Softirq-indirected word-find
| ifind | Builtin word-find
| exe   | Effectively calls address in ToS
| eval  | Processes input buffer
|*cmps* | Reused string compare
|*wcode*| Reused word-code-range getter

### Internals
A word is stored in memory starting at *last* address as:
- N (<128) ascii chars comprising the name
- a name length (N) byte
- a type byte, currently only no-inline flag (6th)
- M (<2^14) bytes of word code
- 2 x 7-bit lit's containing code size (M), LSB first

Note: the size is encoded with two 7-bit lit's instead of a 16bit word, but they are loaded, not executed


### Examples
```
10 4 mul 2 +
```
- the interpreter scans the input buffer until the first whitespace, and the token `10` is read, it is all digits so treated as a number ; it is parsed and put on the stack
- the interpreter continues with another iteration, leaving `10` at the bottom of the stack
- the tokenizer skips the leading whitespace and scans for another token, this time `4` ; the parsed value is placed on the stack as well
- the token `mul` is reached, which is not all-digits so it must be a word
- the word list is traversed from end (`last` variable) to start (a fixed memory address), and word names are string-compared with `mul` until a match is found
- when it is found, the interpreter, in non-inlining mode, calls the `mul` word address ; `mul` has a two-byte body, `MUL RET`
- the stack is now clear of interpreter state, with only `10 4` on it ; `MUL` takes the two values and leaves only the result, `40` on the stack
- on the next interpreter iteration `2` is then placed on the stack, which is now `40 2`
- as with `mul`, `+` consumes the two values at the top of the stack and leaves the result, `42`


```
c: res-inl [ 1 0 , 42 ] ;
```
- `c:` defines a compile-time word named `res-inl`
- `[` switches on inlining
- `1` inserts a literal byte of value `1` in the word body
- `0` inserts a second literal byte ; on execution the two values would form `128 = 1<<7 + 0`
- `,` inserts a NOP instruction in the word body ; this is necessary to separate the first two `LIT`'s from the third, which represents a second value
- `42` inserts another literal
- `]` disables inlining
- `;` writes word footer, comprising of a `RET` and two bytes for code size, in this case `5 0` ; there are four instructions and the trailing `RET`

```
c: add5 [ 5 + ] ;
3 add5
```
- as previously, a word named `add5` is defined
- the word starts with a literal `5` ; if this were to be inlined, a `NOP` may need to preceed it
- `+` inserts an `ADD` instruction
- `;` completes the word definition; the word body is 3 bytes in total ; with the header and footer included it is 8 bytes in total
- `3` is pushed to the stack and `add5` is called, and the result, `8` is left on the stack

```
c: 4th-add [ 4 f! ] 0 f> [ nip 10 + ] 0 >f ;
9 8 7 6 5 4th-add
```
- a new word `4th-add` is defined
- the word body starts with a literal `4` and a frame-set instruction
- inlining must be switched off, as the next value, `0` is used at compile time by word `f>` to generate a `RPICK` instruction with offset `0`, which is only a single byte ; with frame size set to `4`, offset `0` will refer to the 4'th element on the stack, and `RPICK` will push it to the top of the stack
- switching inlining back on, three instructions are generated, a `NIP`, a literal `10` and an `ADD` ; on execution, the next on stack will be dropped, `10` will be added to the value pushed by `RPICK` and the result will be left on the stack
- finally, `>f` generates a `RMOV` instruction, with frame-offset `0` ; on execution this will update the value deeper in the stack with the top-of-stack value, which will be consumed ; although one value was dropped from the top, the offset remains the same as with `RPICK` due to frame-accounting
- after the word definition is completed, the interpretter places the four values on the stack and calls the just-defined word
- the frame is set to include `8 7 6 5` ; the function updates `8` to `18` and drops `5`

```
c: 1to5 #[ [ 1 ]
    0 # [ dup 1+ 5 <? ]
    0 #loop ]# ;
```
- this word uses the `#loop` word along with `#`, which are part of the relative offset calculator helper words, the third one being `#jmp` ; the difference between `#jmp` and `#loop`, as with `JMP` and `LOOP` is that `loop` jumps to a negative relative offset
- the three helper words use an additional piece of compiler state to track the numeric labels (`#` arguments) and jumps
- `#[` resets this state, and must be placed in a word before all uses of `#`, `#loop` and `#jmp` ; it does not insert any code in the word body
- the word starts with the literal `1`
- `0` is placed on the stack, and `#` uses it to store the current offset in the word body in an array, at index `0`
- next 4 more instructions are emmitted: `DUP INC LIT(5) COND(LESS)`
- on execution the `COND` will compare the value on the top of the stack with `5` and if not less, unset the `TAKE_JMP` flag, efectively terminating the loop
- next the `#loop` word will take the passed index `0` and lookup the offset of the numeric label stored previously ; it will then insert two instruction, a literal based on the difference in offsets and a `LOOP`
- finally, `]#` would normally patch `#jmp` relative jump offsets, but since only a `#loop` was used, it doesn't do anything


```
c: hi " hello world" str ;
hi puts
```
- the string word `"` (which must be followed by a space as all other words) scans for a matching `"` and inserts the string in the word body preceeded by a jump that skips it on execution ; it leaves on the stack a value pair of `END-OFFSET START-OFFSET`
- the `str` word inserts in the word body the literals (separated by `NOP`) needed to put the two values on the stack upon execution
- the `puts` word takes a pair of offsets and prints them to `serial0` device


### Other words
|Word|Interface|Description|
|---|---|---|
|**'**  | `( -- xt )`         | Scans word name and returns code address
|**`**  | `( -- )`            | Scans word name and inserts **lit**-lit-call (mind the nop)
|dch    | `( ascii y x -- )`  | Draws a character at block coords
|dnum   | `( y x num -- y x')`| Draws number at block coords
|dstr   | `( y x end str -- y x')`| Draws string at block coords
|dbla   | `( y x -- addr)`    | Returns memory address of pixel block
|dclr   | `( -- )`            | Clear framebuffer with 0
|pad    | `( -- addr)`        | Returns address of 1K user memory
|**"**  | `( -- end addr)`    | Creates string at HERE
|**.**  | `( val --)`         | Prints number
|min    | `( a b -- min)`     | Returns min of to values
|max    | `( a b -- max)`     | Returns max of to values
|isr    | `( xt idx --)`      | Sets addr `xt` as interrupt `idx` handler
|fill   | `( val end addr --)`| Fills `[addr end)` memory interval with `val`
