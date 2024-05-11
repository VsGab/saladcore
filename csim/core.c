#include "defs.h"
#include "core_state.h"
#include "core_helpers.h"

void exec_mem_op(byte op) {
  trace(DECODE_TRC, MEM_INSN , op);

  switch(op) {
    case ILOADB:  rset(0, byte_reg_load(0)); break;
    case ILOADW:  rset(0, word_reg_load(0)); break;
    case MEMCPYB: byte_store(rget(1), byte_load(rget(0))); break;
    case MEMCPYW: word_store(rget(1), word_load(rget(0))); break;
    case STOREB:  byte_reg_store(1, 0); ds_pop(); break;
    case STOREW:  word_reg_store(1, 0); ds_pop(); break;
    case ASTOREB: byte_reg_store(0, 1); ds_pop(); break;
    case ASTOREW: word_reg_store(0, 1); ds_pop(); break;
    default: unimplemented();
  }
}

void exec_cond_op(byte op) {
  trace(DECODE_TRC, COND_INSN , op);

  switch(op) {
    case CZERO: take_jmp = take_jmp && (!rget(0)); break;
    case CNZERO: take_jmp = take_jmp && !!rget(0); break;
    case CLESS: take_jmp = take_jmp && (rget(1) < rget(0)); break;
    case CGEQ: take_jmp = take_jmp && (rget(1) >= rget(0)); break;
    case CEQ: take_jmp = take_jmp && (rget(1) == rget(0)); break;
    case CNEQ: take_jmp = take_jmp && (rget(1) != rget(0)); break;
    default: unimplemented();
  }
  trace(COND_TRC, op, take_jmp);
  ds_pop();
}

void exec_alu_op(byte op) {
  wd t = rget(0);
  wd n = rget(1);
  wd _t = n;
  trace(DECODE_TRC, ALU_INSN , op);

  switch (op) {
    case DROP: break;
    case ADD: _t += t;  break;
    case SUB: _t -= t;  break;
    case AND: _t &= t;  break;
    case OR:  _t |= t;  break;
    case XOR: _t ^= t;  break;
    case MUL: _t *= t;  break;
    case DIV: _t /= t;  break;
    case SHR: _t >>= t; break;
    case SHL: _t <<= t; break;
    default: unimplemented();
  }
  ds_pop();
  rset(0, _t);
}

void exec_frame_op(byte var, byte op) {
  reg x = (var == NOS) ? 1 : fsz - var - 1;
  trace(DECODE_TRC, REG_INSN , op);
  trace(REGOP_TRC, var, x);

  switch (op) {
    case RPICK: ds_push(rget(x)); break;
    case RSWAP: swap(0,x);        break;
    case RMOV:  rset(x, rget(0));
                ds_pop();         break;
    default: unimplemented();
  }
}

void exec_una_nlr_op(byte op) {
  trace(DECODE_TRC, TOS_INSN , op);

  switch (op) {
    case DUP: ds_push(rget(0)); break;
    case NOP: break;
    case HALT: halted = true; break;
    case NEG: rset(0, -rget(0)); break;
    case INC: rset(0, rget(0)+1); break;
    case DEC: rset(0, rget(0)-1); break;
    case INT: soft_irq(rget(0)); break;
    case SWAPP: swap(1,2); break;
    case FNE: take_jmp = take_jmp && (fsz != 0); break;
    case FSET: set_frame_size(rget(0)); break;
    case FKEEP: frame_keep_shift(rget(0)); break;
    case RET: next_pc = rspop();
      trace(RET_TRC, fsz, dsz);
      break;
    case CALL:
      do_call(rget(0));
      break;
    case LOOP:
      if (take_jmp) next_pc = pc - (rget(0) + 1);
      take_jmp = true;
      ds_pop();
      break;
    case JMP:
      if (take_jmp) next_pc = pc + (rget(0) + 1);
      take_jmp = true;
      ds_pop();
      break;
    case LJMP:
      next_pc = rget(0);
      ds_pop();
      break;
    default: unimplemented();
  }
}

void exec_lit(byte lit7) {
  if (lit_dirty) {
    // not first lit
    rset(0, (rget(0) << 7) | lit7);
  } else {
    // first lit
    ds_push(lit7);
    lit_dirty = true;
  }
  trace(DECODE_TRC, LIT_INSN , lit7);
}

void saladcore_init(byte* user_mem, wd user_mem_size,
               wd initial_dsp, wd initial_rsp, wd ivt_base,
               iowritehandler oniowrite, ioreadhandler onioread,
               tracehandler ontrace) {
  // interface
  memory = user_mem;
  memory_size = user_mem_size;
  iowrite = oniowrite;
  ioread = onioread;
  tracefunc = ontrace;

  // core state
  dsz = 0;
  fsz = 0;
  watchdog = WATCHDOG;
  rb_init();
  lit_dirty = false;
  take_jmp = true;
  halted = true;
  dsp = initial_dsp;
  rsp = initial_rsp;
  ivt = ivt_base;
  pc = word_load(0);
}

void saladcore_execute()
{
  while(!halted) {
    byte insn = memory[pc];
    next_pc = pc+1;
    trace(INSN_TRC, insn, 0);

    if (!(insn & OP_BIT)) {
      exec_lit(insn & LIT_MASK);
      goto next_insn;
    }

    lit_dirty = false;

    if (!(insn & TOS_OP_BIT)) {
      wd op = (insn) & REG_OP_MASK;
      reg var = (insn >> REG_X_SHIFT) & REG_X_MASK;
      exec_frame_op(var, op);
      goto next_insn;
    }

    byte op_class = (insn >> 4) & 3;
    switch(op_class) {
      case ALU_OP:
        exec_alu_op(insn & TOS_OP_ARG_MASK);
        break;
      case UNLR_OP:
        exec_una_nlr_op(insn & TOS_OP_ARG_MASK);
        break;
      case MEM_COND_OP:
        if (insn & MEM_OP_BIT)
          exec_mem_op(insn & MEM_COND_OP_MASK);
        else
          exec_cond_op(insn & MEM_COND_OP_MASK);
        break;
      default:  unimplemented();
    }

  next_insn:
    trace(AFTER_TRC, next_pc, 0);
    check(pc < memory_size, ERR_PC);
    check(pc > 0, ERR_PC);
    check_watchdog();
    pc = next_pc;
  }
}
