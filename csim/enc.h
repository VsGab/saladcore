#pragma once
#include "defs.h"

#define dup() assert(false) // use dup_
#define div() assert(false) // use div_
#define or() assert(false) // use or_
#define and() assert(false) // use and_
#define xor() assert(false) // use xor_
#define REG_OP(op,reg) ;
#define ARG_OP(op, arg) ;
#define NOARG_OP(op) ;

static inline byte lb(wd val) {return val & LIT_MASK;}
static inline byte hb(wd val) {return (val >> 7) & LIT_MASK;}

inline static byte lit(wd val)
{
  ARG_OP("ins", val);
  return val & LIT_MASK;
}

inline static byte reg_op(wd op, reg x)
{
  byte insn = 0;
  insn |= OP_BIT;
  insn |= ((byte)x & REG_X_MASK) << REG_X_SHIFT;
  insn |= (op & REG_OP_MASK);
  return insn;
}

inline static byte nullary_tos_op(wd op)
{
  byte insn = 0;
  insn |= OP_BIT;
  insn |= TOS_OP_BIT;
  insn |= (UNLR_OP & TOS_OP_TYPE_MASK) << TOS_OP_TYPE_SHIFT;
  insn |= op & TOS_OP_ARG_MASK;
  return insn;
}

inline static byte alu_tos_op(wd op)
{
  byte insn = 0;
  insn |= OP_BIT;
  insn |= TOS_OP_BIT;
  insn |= (ALU_OP & TOS_OP_TYPE_MASK) << TOS_OP_TYPE_SHIFT;
  insn |= op & TOS_OP_ARG_MASK;
  return insn;
}

inline static byte cond(wd op)
{
  byte insn = 0;
  insn |= OP_BIT;
  insn |= TOS_OP_BIT;
  insn |= (MEM_COND_OP & TOS_OP_TYPE_MASK) << TOS_OP_TYPE_SHIFT;
  insn |= op & MEM_COND_OP_MASK;
  return insn;
}

inline static byte memop(wd op)
{
  byte insn = 0;
  insn |= OP_BIT;
  insn |= TOS_OP_BIT;
  insn |= (MEM_COND_OP & TOS_OP_TYPE_MASK) << TOS_OP_TYPE_SHIFT;
  insn |= MEM_OP_BIT;
  insn |= op & MEM_COND_OP_MASK;
  return insn;
}



// ----  Reg ops  ----

inline static byte rpick(reg x)
{
  REG_OP("f>", x);
  return reg_op(RPICK, x);
}

inline static byte rswap(reg x)
{
  REG_OP(">f<", x);
  return reg_op(RSWAP, x);
}

inline static byte rmov(reg x)
{
  REG_OP(">f", x);
  return reg_op(RMOV, x);
}


// ---- TOS ALU ops ----

inline static byte drop()
{
  NOARG_OP(__func__);
  return alu_tos_op(DROP);
}

inline static byte add()
{
  NOARG_OP("+");
  return alu_tos_op(ADD);
}

inline static byte sub()
{
  NOARG_OP("-");
  return alu_tos_op(SUB);
}

inline static byte and_()
{
  NOARG_OP("and");
  return alu_tos_op(AND);
}

inline static byte or_()
{
  NOARG_OP("or");
  return alu_tos_op(OR);
}

inline static byte shr()
{
  NOARG_OP(__func__);
  return alu_tos_op(SHR);
}

inline static byte shl()
{
  NOARG_OP(__func__);
  return alu_tos_op(SHL);
}

inline static byte xor_()
{
  NOARG_OP("xor");
  return alu_tos_op(XOR);
}

inline static byte mul()
{
  NOARG_OP(__func__);
  return alu_tos_op(MUL);
}

inline static byte div_()
{
  NOARG_OP('div');
  return alu_tos_op(DIV);
}

// ---- Nullary ops ----

inline static byte dup_()
{
  NOARG_OP("dup");
  return nullary_tos_op(DUP);
}

inline static byte nop()
{
  NOARG_OP(",");
  return nullary_tos_op(NOP);
}

inline static byte neg()
{
  NOARG_OP(__func__);
  return nullary_tos_op(NEG);
}

inline static byte inc()
{
  NOARG_OP("1+");
  return nullary_tos_op(INC);
}

inline static byte dec()
{
  NOARG_OP("1-");
  return nullary_tos_op(DEC);
}

inline static byte swapp()
{
  NOARG_OP("swap");
  return nullary_tos_op(SWAPP);
}

inline static byte fnempty()
{
  NOARG_OP("fne");
  return nullary_tos_op(FNE);
}

inline static byte fset()
{
  NOARG_OP("f!");
  return nullary_tos_op(FSET);
}

inline static byte fkeep()
{
  NOARG_OP("f-");
  return nullary_tos_op(FKEEP);
}

inline static byte ret()
{
  NOARG_OP(__func__);
  return nullary_tos_op(RET);
}

inline static byte call()
{
  NOARG_OP(__func__);
  return nullary_tos_op(CALL);
}

inline static byte jmp()
{
  NOARG_OP(__func__);
  return nullary_tos_op(JMP);
}

inline static byte loop()
{
  NOARG_OP(__func__);
  return nullary_tos_op(LOOP);
}

inline static byte ljmp()
{
  NOARG_OP(__func__);
  return nullary_tos_op(LJMP);
}

inline static byte doint()
{
  NOARG_OP("int");
  return nullary_tos_op(INT);
}

inline static byte halt()
{
  NOARG_OP(__func__);
  return nullary_tos_op(HALT);
}

// Memory ops




inline static byte iloadb()
{
  NOARG_OP("lb");
  return memop(ILOADB);
}

inline static byte iloadw()
{
  NOARG_OP("lw");
  return memop(ILOADW);
}

inline static byte mcpb()
{
  NOARG_OP(__func__);
  return memop(MEMCPYB);
}

inline static byte mcpw()
{
  NOARG_OP(__func__);
  return memop(MEMCPYW);
}

inline static byte snb()
{
  NOARG_OP(__func__);
  return memop(ASTOREB);
}

inline static byte snw()
{
  NOARG_OP(__func__);
  return memop(ASTOREW);
}

inline static byte stb()
{
  NOARG_OP(__func__);
  return memop(STOREB);
}

inline static byte stw()
{
  NOARG_OP(__func__);
  return memop(STOREW);
}


// pseudo-instructions
inline static byte nip()
{
  NOARG_OP(__func__);
  return rmov(NOS);
}

inline static byte swap()
{
  NOARG_OP(__func__);
  return rswap(NOS);
}


inline static byte over()
{
  NOARG_OP(__func__);
  return rpick(NOS);
}
