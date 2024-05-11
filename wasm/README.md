The demo uses the *serial0* device to send keypresses to the core as ASCII characters.
The *sr0rd* word is set as the interrupt handler and is responsible for character echoing, TIB buffering and calling the builtin *eval*.

The stub interrupt table looks as follows:
|IRQ#|Description|
|---|---|
|0  | Serial read
|1  | Keypad
|2  | 10Hz timer

The *isr* word is used to set execution tokens obtained by *&lt;tick&gt;* in the interrupt table.

The framebuffer is 128x128 1-bit, stored in column major order in memory at the 6K mark, as blocks of 4x2 (X/Y) pixels that fit a byte . It is redrawn to the canvas using hardwired RGBA values, once every 100ms. A timer interrupt is triggered with the same frequency.

The ["Tom Thumb"](https://robey.lag.net/2010/01/23/tiny-monospace-font.html) 3x5 font was converted and included in the main source file (*font* word). A character is comprised of 3 4x2 pixel blocks, arranged vertically.

> Note: the demo might be unstable still and error handling is quite primitive, requiring resets