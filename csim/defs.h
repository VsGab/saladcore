#pragma once

// registers
#define NUM_REGS 16

// instruction bits
#define OP_BIT 1<<7
#define TOS_OP_BIT 1<<6

#define REG_OP_MASK ((wd)0x3)
#define REG_X_MASK ((wd)0xF)
#define REG_X_SHIFT 2

#define TOS_OP_TYPE_MASK ((wd)0x3)
#define TOS_OP_TYPE_SHIFT 4
#define TOS_OP_ARG_MASK ((wd)0xf)

#define MEM_COND_OP_MASK ((wd)0x7)
#define MEM_OP_BIT 1<<3

#define ALU_OP ((wd)0)
#define UNLR_OP ((wd)1) // unary and nullary ops
#define MEM_COND_OP ((wd)2) // memory and conditions, using MEM_OP_BIT
#define EMU_OP ((wd)3) // reserved


#define SMI_OP_HIGH_BIT 1<<3
#define SMI_OP_IMM_MASK ((wd)0x7)

#define LIT_MASK ((wd)0x7f)



// REG ops
#define RPICK 0
#define RSWAP 1
#define RMOV 2


// constants
#define NOS 15
#define FUNSET 127

// COND ops
#define CZERO 0 //  Jump if TOS is 0
#define CNZERO 1 // Jump if TOS not 0
#define CEQ 2  // Jump if NOS == TOS
#define CNEQ 3 // Jump if NOS != TOS
#define CGEQ 4 // Jump if NOS >= TOS
#define CLESS 5 // Jump if NOS < TOS


// TOS-NOS ALU ops - consuming TOS
#define DROP 0
#define ADD 1
#define SUB 2
#define AND 3
#define OR 4
#define SHR 5
#define SHL 6
#define XOR 7
// reserved
#define MUL 10
#define DIV 11
// 12-15 reserved

// TOS / nullary ops
// pushing to stack
#define DUP 0
// preserving stack size
#define NOP 1
#define NEG 2
#define INC 3
#define DEC 4
#define SWAPP 5
#define FNE 6 // frame not empty
#define FSET 7
#define FKEEP 8

// PC acting
#define RET 9
#define CALL 10
#define JMP 11 // relative jump forward
#define LOOP 12 // relative jump backwards
#define LJMP 13 // absolute jump - RS untouched
#define INT 14
#define HALT 15


// memory
#define ILOADB 0
#define ILOADW 1
#define MEMCPYB 2
#define MEMCPYW 3
#define ASTOREB 4 // store nos at tos
#define ASTOREW 5
#define STOREB 6  // store tos at
#define STOREW 7

typedef short int wd;
typedef char reg;
typedef unsigned char byte;

#ifndef bool
    #define bool byte
#endif

// I/O, mem layout
#define IO_DEV_SHIFT 3 // 8B each
#define IO_PORT_MASK 7
#define NUM_IO_DEVS 14 // mem 16-128
#define IO_MIN 16
#define IO_MAX 128

// interrupts
#define NUM_INT 128
#define IVT_ENTRY_SIZE 4
#define ISR_SHIFT 2 // 4B each, 3xlit + ljmp


///////////////////////////////
#define LIT_INSN 1
#define REG_INSN 2
#define ALU_INSN 3
#define TOS_INSN 4
#define MEM_INSN 5
#define COND_INSN 6
#define RES_INSN 7

#define HALT_TRC 0
#define INSN_TRC 1
#define DECODE_TRC 2
#define LOAD_TRC 3
#define HWINT_TRC 4
#define STORE_TRC 5
#define COND_TRC 7
#define CALL_TRC 8
#define RET_TRC 9
#define INT_TRC 10
#define REGOP_TRC 11
#define AFTER_TRC 12
#define ERROR_TRC 255

#define ERR_D_STACK 1
#define ERR_R_STACK 2
#define ERR_REG 3
#define ERR_FRAME 4
#define ERR_MEM 5
#define ERR_INT 6
#define ERR_PC 7
#define ERR_SW 8
#define ERR_WATCHDOG 9
#define ERR_UNIMPLEMENTED 10

// Util
#define true 1
#define false 0
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef void (*iowritehandler)(byte addr, wd val);
typedef wd (*ioreadhandler)(byte addr);
typedef void (*tracehandler)(int type, wd pc, wd arg1, wd arg2);


