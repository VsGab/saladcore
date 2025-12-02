#include "defs.h"
#include "opcodes.h"

// STATE
static long watchdog;
static bool halted;
static wd xreg, yreg;
static int pc, next_pc;

#define CHECK(cond, msg) {if (!cond) iowrite(msg,sizeof(msg)); }

int exec_op_insn(sf_ctx *ctx, byte op) {
  switch (op)
  {
    case i_add: yreg += xreg; break;
    case i_sub: yreg -= xreg; break;
    case i_and: yreg &= xreg; break;
    case i_or:  yreg |= xreg; break;
    case i_inc: yreg++;       break;
    case i_dec: yreg--;       break;
    case i_shl: yreg <<= 1;   break;
    case i_shr: yreg >>= 1;   break;
    case i_zero: xreg = 0;    break;
    case i_swap: SWAP(xreg, yreg); break;
    case i_neg: yreg = -yreg; break;
    case i_xor: yreg ^= xreg; break;
    case i_less: yreg = yreg < xreg; break;
    case i_mul: yreg *= xreg; break;
    case i_div: yreg /= xreg; break;

    case i_xpu: DPUSH(xreg); xreg=0; break;
    case i_ypu: DPUSH(yreg); break;
    case i_lds: xreg = DSGET(xreg); break;
    // unused
    case i_drop: DPOP();     break;
    case i_ldg: yreg = ADSGET(xreg);          break;
    case i_stg: ADSSET(xreg, yreg);           break;
    case i_ld:  xreg = xreg << 8 | MEM(yreg); break;
    case i_st:  MEMSET(yreg, xreg & 0xff); xreg >>= 8; break;

    case i_jmp:  next_pc = pc + xreg + 1; break;
    case i_loop: if (yreg) next_pc = pc - xreg;  break;
    case i_cjmp: if (yreg) next_pc = pc + xreg + 1; break;
    case i_sig: return xreg;
    case i_call: RPUSH(next_pc);
                next_pc = xreg;
                xreg = yreg = 0;
                break;
    case i_ret: if (RSP==RS_SZ) halted=true; else next_pc = RPOP();
               xreg = yreg = 0; break;
    case i_stop: halted = true;   break;
    default: error("invalid-op");
  }
  return 0;
}

int exec_insn(sf_ctx *ctx, byte ins) {
  next_pc = pc + 1;
  if (ins & LIT_MASK) {
      // non-lit
      byte val = ins & LSB5_MASK;
      switch((ins & MSB2_MASK) >> 5) {
          case OP_INS: return exec_op_insn(ctx, val);
          case XRG_INS: xreg = DSGET(val);
              break;
          case YRS_INS: DSSET(val, yreg);
              break;
          case YRG_INS: yreg = DSGET(val);
              break;
      }
  } else {
      xreg = (xreg << 7) | (ins & 127);
  }
  return 0;
}

int saladcore_resume(sf_ctx *ctx, int upper_limit) {
  while(!halted) {
    next_pc = pc+1;
    assert(pc < upper_limit);
    byte insn = MEM(pc);
    int ret = exec_insn(ctx, insn);
    pc = next_pc;
    if (ret) return ret;
    assert(watchdog-- > 0);
  }
  return 0;
}

int saladcore_execute(sf_ctx *ctx, int initial_pc, int upper_limit)
{
  pc = initial_pc;
  halted = false;
  watchdog = 10000;
  return saladcore_resume(ctx, upper_limit);
}

