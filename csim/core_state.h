#pragma once
#include "defs.h"
#include "ring.h" // has static rb (regs ringbuffer)

static wd pc; // program counter
static wd next_pc; // program counter
static wd dsp; // data stack pointer -> mem
static wd rsp; // return stack pointer -> mem
static wd ivt; // interrupt vector table base
static bool lit_dirty;
static bool take_jmp; // default true
static bool halted;  // default true

static wd dsz; // stack size
static byte fsz; // frame size - auto inc/dec