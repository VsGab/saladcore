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
